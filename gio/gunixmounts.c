/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#ifndef HAVE_SYSCTLBYNAME
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_POLL_H
#include <sys/poll.h>
#endif
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "gunixmounts.h"
#include "gfile.h"
#include "gfilemonitor.h"

/**
 * SECTION:gunixmounts
 * @short_description: Unix Mounts
 * 
 * Routines for managing mounted UNIX mount points and paths.
 *
 **/

struct _GUnixMount {
  char *mount_path;
  char *device_path;
  char *filesystem_type;
  gboolean is_read_only;
  gboolean is_system_internal;
};

struct _GUnixMountPoint {
  char *mount_path;
  char *device_path;
  char *filesystem_type;
  gboolean is_read_only;
  gboolean is_user_mountable;
  gboolean is_loopback;
};

enum {
  MOUNTS_CHANGED,
  MOUNTPOINTS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

struct _GUnixMountMonitor {
  GObject parent;

  GFileMonitor *fstab_monitor;
  GFileMonitor *mtab_monitor;
};

struct _GUnixMountMonitorClass {
  GObjectClass parent_class;
};
  
static GUnixMountMonitor *the_mount_monitor = NULL;

static GList *_g_get_unix_mounts (void);
static GList *_g_get_unix_mount_points (void);

G_DEFINE_TYPE (GUnixMountMonitor, g_unix_mount_monitor, G_TYPE_OBJECT);

#define MOUNT_POLL_INTERVAL 4000

#ifdef HAVE_SYS_MNTTAB_H
#define MNTOPT_RO	"ro"
#endif

#ifdef HAVE_MNTENT_H
#include <mntent.h>
#elif defined (HAVE_SYS_MNTTAB_H)
#include <sys/mnttab.h>
#endif

#ifdef HAVE_SYS_VFSTAB_H
#include <sys/vfstab.h>
#endif

#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
#include <sys/mntctl.h>
#include <sys/vfs.h>
#include <sys/vmount.h>
#include <fshelp.h>
#endif

#if defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)
#include <sys/ucred.h>
#include <sys/mount.h>
#include <fstab.h>
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

#ifndef HAVE_SETMNTENT
#define setmntent(f,m) fopen(f,m)
#endif
#ifndef HAVE_ENDMNTENT
#define endmntent(f) fclose(f)
#endif

static gboolean
is_in (const char *value, const char *set[])
{
  int i;
  for (i = 0; set[i] != NULL; i++)
    {
      if (strcmp (set[i], value) == 0)
	return TRUE;
    }
  return FALSE;
}

static gboolean
guess_system_internal (const char *mountpoint,
		       const char *fs,
		       const char *device)
{
  const char *ignore_fs[] = {
    "auto",
    "autofs",
    "devfs",
    "devpts",
    "kernfs",
    "linprocfs",
    "proc",
    "procfs",
    "ptyfs",
    "rootfs",
    "selinuxfs",
    "sysfs",
    "tmpfs",
    "usbfs",
    "nfsd",
    NULL
  };
  const char *ignore_devices[] = {
    "none",
    "sunrpc",
    "devpts",
    "nfsd",
    "/dev/loop",
    "/dev/vn",
    NULL
  };
  const char *ignore_mountpoints[] = {
    /* Includes all FHS 2.3 toplevel dirs */
    "/bin",
    "/boot",
    "/dev",
    "/etc",
    "/home",
    "/lib",
    "/lib64",
    "/media",
    "/mnt",
    "/opt",
    "/root",
    "/sbin",
    "/srv",
    "/tmp",
    "/usr",
    "/var",
    "/proc",
    "/sbin",
    "/net",
    NULL
  };
  
  if (is_in (fs, ignore_fs))
    return TRUE;
  
  if (is_in (device, ignore_devices))
    return TRUE;

  if (is_in (mountpoint, ignore_mountpoints))
    return TRUE;
  
  if (g_str_has_prefix (mountpoint, "/dev") ||
      g_str_has_prefix (mountpoint, "/proc") ||
      g_str_has_prefix (mountpoint, "/sys"))
    return TRUE;

  if (strstr (mountpoint, "/.gvfs") != NULL)
    return TRUE;
  
  return FALSE;
}

#ifdef HAVE_MNTENT_H

static char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
# ifdef __linux__
  return "/proc/mounts";
# else
  return _PATH_MOUNTED;
# endif
#else	
  return "/etc/mtab";
#endif
}

static char *
get_mtab_monitor_file (void)
{
#ifdef _PATH_MOUNTED
  return _PATH_MOUNTED;
#else	
  return "/etc/mtab";
#endif
}

G_LOCK_DEFINE_STATIC(getmntent);

static GList *
_g_get_unix_mounts ()
{
  struct mntent *mntent;
  FILE *file;
  char *read_file;
  GUnixMount *mount_entry;
  GHashTable *mounts_hash;
  GList *return_list;
  
  read_file = get_mtab_read_file ();

  file = setmntent (read_file, "r");
  if (file == NULL)
    return NULL;

  return_list = NULL;
  
  mounts_hash = g_hash_table_new (g_str_hash, g_str_equal);
  
  G_LOCK (getmntent);
  while ((mntent = getmntent (file)) != NULL)
    {
      /* ignore any mnt_fsname that is repeated and begins with a '/'
       *
       * We do this to avoid being fooled by --bind mounts, since
       * these have the same device as the location they bind to.
       * Its not an ideal solution to the problem, but its likely that
       * the most important mountpoint is first and the --bind ones after
       * that aren't as important. So it should work.
       *
       * The '/' is to handle procfs, tmpfs and other no device mounts.
       */
      if (mntent->mnt_fsname != NULL &&
	  mntent->mnt_fsname[0] == '/' &&
	  g_hash_table_lookup (mounts_hash, mntent->mnt_fsname))
	continue;
      
      mount_entry = g_new0 (GUnixMount, 1);
      mount_entry->mount_path = g_strdup (mntent->mnt_dir);
      mount_entry->device_path = g_strdup (mntent->mnt_fsname);
      mount_entry->filesystem_type = g_strdup (mntent->mnt_type);
      
#if defined (HAVE_HASMNTOPT)
      if (hasmntopt (mntent, MNTOPT_RO) != NULL)
	mount_entry->is_read_only = TRUE;
#endif
      
      mount_entry->is_system_internal =
	guess_system_internal (mount_entry->mount_path,
			       mount_entry->filesystem_type,
			       mount_entry->device_path);
      
      g_hash_table_insert (mounts_hash,
			   mount_entry->device_path,
			   mount_entry->device_path);
      
      return_list = g_list_prepend (return_list, mount_entry);
    }
  g_hash_table_destroy (mounts_hash);
  
  endmntent (file);

  G_UNLOCK (getmntent);
  
  return g_list_reverse (return_list);
}

#elif defined (HAVE_SYS_MNTTAB_H)

G_LOCK_DEFINE_STATIC(getmntent);

static char *
get_mtab_read_file (void)
{
#ifdef _PATH_MOUNTED
  return _PATH_MOUNTED;
#else	
  return "/etc/mnttab";
#endif
}

static char *
get_mtab_monitor_file (void)
{
  return get_mtab_read_file ();
}

static GList *
_g_get_unix_mounts (void)
{
  struct mnttab mntent;
  FILE *file;
  char *read_file;
  GUnixMount *mount_entry;
  GList *return_list;
  
  read_file = get_mtab_read_file ();
  
  file = setmntent (read_file, "r");
  if (file == NULL)
    return NULL;
  
  return_list = NULL;
  
  G_LOCK (getmntent);
  while (! getmntent (file, &mntent))
    {
      mount_entry = g_new0 (GUnixMount, 1);
      
      mount_entry->mount_path = g_strdup (mntent.mnt_mountp);
      mount_entry->device_path = g_strdup (mntent.mnt_special);
      mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);
      
#if defined (HAVE_HASMNTOPT)
      if (hasmntopt (&mntent, MNTOPT_RO) != NULL)
	mount_entry->is_read_only = TRUE;
#endif

      mount_entry->is_system_internal =
	guess_system_internal (mount_entry->mount_path,
			       mount_entry->filesystem_type,
			       mount_entry->device_path);
      
      return_list = g_list_prepend (return_list, mount_entry);
    }
  
  endmntent (file);
  
  G_UNLOCK (getmntent);
  
  return g_list_reverse (return_list);
}

#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

static char *
get_mtab_monitor_file (void)
{
  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
  struct vfs_ent *fs_info;
  struct vmount *vmount_info;
  int vmount_number;
  unsigned int vmount_size;
  int current;
  GList *return_list;
  
  if (mntctl (MCTL_QUERY, sizeof (vmount_size), &vmount_size) != 0)
    {
      g_warning ("Unable to know the number of mounted volumes\n");
      
      return NULL;
    }

  vmount_info = (struct vmount*)g_malloc (vmount_size);

  vmount_number = mntctl (MCTL_QUERY, vmount_size, vmount_info);
  
  if (vmount_info->vmt_revision != VMT_REVISION)
    g_warning ("Bad vmount structure revision number, want %d, got %d\n", VMT_REVISION, vmount_info->vmt_revision);

  if (vmount_number < 0)
    {
      g_warning ("Unable to recover mounted volumes information\n");
      
      g_free (vmount_info);
      return NULL;
    }
  
  return_list = NULL;
  while (vmount_number > 0)
    {
      mount_entry = g_new0 (GUnixMount, 1);
      
      mount_entry->device_path = g_strdup (vmt2dataptr (vmount_info, VMT_OBJECT));
      mount_entry->mount_path = g_strdup (vmt2dataptr (vmount_info, VMT_STUB));
      /* is_removable = (vmount_info->vmt_flags & MNT_REMOVABLE) ? 1 : 0; */
      mount_entry->is_read_only = (vmount_info->vmt_flags & MNT_READONLY) ? 1 : 0;

      fs_info = getvfsbytype (vmount_info->vmt_gfstype);
      
      if (fs_info == NULL)
	mount_entry->filesystem_type = g_strdup ("unknown");
      else
	mount_entry->filesystem_type = g_strdup (fs_info->vfsent_name);

      mount_entry->is_system_internal =
	guess_system_internal (mount_entry->mount_path,
			       mount_entry->filesystem_type,
			       mount_entry->device_path);
      
      return_list = g_list_prepend (return_list, mount_entry);
      
      vmount_info = (struct vmount *)( (char*)vmount_info 
				       + vmount_info->vmt_length);
      vmount_number--;
    }
  
  
  g_free (vmount_info);
  
  return g_list_reverse (return_list);
}

#elif defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

static char *
get_mtab_monitor_file (void)
{
  return NULL;
}

static GList *
_g_get_unix_mounts (void)
{
  struct statfs *mntent = NULL;
  int num_mounts, i;
  GUnixMount *mount_entry;
  GList *return_list;
  
  /* Pass MNT_NOWAIT to avoid blocking trying to update NFS mounts. */
  if ((num_mounts = getmntinfo (&mntent, MNT_NOWAIT)) == 0)
    return NULL;
  
  return_list = NULL;
  
  for (i = 0; i < num_mounts; i++)
    {
      mount_entry = g_new0 (GUnixMount, 1);
      
      mount_entry->mount_path = g_strdup (mntent[i].f_mntonname);
      mount_entry->device_path = g_strdup (mntent[i].f_mntfromname);
      mount_entry->filesystem_type = g_strdup (mntent[i].f_fstypename);
      if (mntent[i].f_flags & MNT_RDONLY)
	mount_entry->is_read_only = TRUE;

      mount_entry->is_system_internal =
	guess_system_internal (mount_entry->mount_path,
			       mount_entry->filesystem_type,
			       mount_entry->device_path);
      
      return_list = g_list_prepend (return_list, mount_entry);
    }
  
  return g_list_reverse (return_list);
}
#else
#error No _g_get_unix_mounts() implementation for system
#endif

/* _g_get_unix_mount_points():
 * read the fstab.
 * don't return swap and ignore mounts.
 */

static char *
get_fstab_file (void)
{
#if defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)
  /* AIX */
  return "/etc/filesystems";
#elif defined(_PATH_MNTTAB)
  return _PATH_MNTTAB;
#elif defined(VFSTAB)
  return VFSTAB;
#else
  return "/etc/fstab";
#endif
}

#ifdef HAVE_MNTENT_H
static GList *
_g_get_unix_mount_points (void)
{
  struct mntent *mntent;
  FILE *file;
  char *read_file;
  GUnixMountPoint *mount_entry;
  GList *return_list;
  
  read_file = get_fstab_file ();
  
  file = setmntent (read_file, "r");
  if (file == NULL)
    return NULL;

  return_list = NULL;
  
  G_LOCK (getmntent);
  while ((mntent = getmntent (file)) != NULL)
    {
      if ((strcmp (mntent->mnt_dir, "ignore") == 0) ||
	  (strcmp (mntent->mnt_dir, "swap") == 0))
	continue;
      
      mount_entry = g_new0 (GUnixMountPoint, 1);
      mount_entry->mount_path = g_strdup (mntent->mnt_dir);
      mount_entry->device_path = g_strdup (mntent->mnt_fsname);
      mount_entry->filesystem_type = g_strdup (mntent->mnt_type);
      
#ifdef HAVE_HASMNTOPT
      if (hasmntopt (mntent, MNTOPT_RO) != NULL)
	mount_entry->is_read_only = TRUE;
      
      if (hasmntopt (mntent, "loop") != NULL)
	mount_entry->is_loopback = TRUE;
      
#endif
      
      if ((mntent->mnt_type != NULL && strcmp ("supermount", mntent->mnt_type) == 0)
#ifdef HAVE_HASMNTOPT
	  || (hasmntopt (mntent, "user") != NULL
	      && hasmntopt (mntent, "user") != hasmntopt (mntent, "user_xattr"))
	  || hasmntopt (mntent, "pamconsole") != NULL
	  || hasmntopt (mntent, "users") != NULL
	  || hasmntopt (mntent, "owner") != NULL
#endif
	  )
	mount_entry->is_user_mountable = TRUE;
      
      return_list = g_list_prepend (return_list, mount_entry);
    }
  
  endmntent (file);
  G_UNLOCK (getmntent);
  
  return g_list_reverse (return_list);
}

#elif defined (HAVE_SYS_MNTTAB_H)

static GList *
_g_get_unix_mount_points (void)
{
  struct mnttab mntent;
  FILE *file;
  char *read_file;
  GUnixMountPoint *mount_entry;
  GList *return_list;
  
  read_file = get_fstab_file ();
  
  file = setmntent (read_file, "r");
  if (file == NULL)
    return NULL;

  return_list = NULL;
  
  G_LOCK (getmntent);
  while (! getmntent (file, &mntent))
    {
      if ((strcmp (mntent.mnt_mountp, "ignore") == 0) ||
	  (strcmp (mntent.mnt_mountp, "swap") == 0))
	continue;
      
      mount_entry = g_new0 (GUnixMountPoint, 1);
      
      mount_entry->mount_path = g_strdup (mntent.mnt_mountp);
      mount_entry->device_path = g_strdup (mntent.mnt_special);
      mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);
      
#ifdef HAVE_HASMNTOPT
      if (hasmntopt (&mntent, MNTOPT_RO) != NULL)
	mount_entry->is_read_only = TRUE;
      
      if (hasmntopt (&mntent, "lofs") != NULL)
	mount_entry->is_loopback = TRUE;
#endif
      
      if ((mntent.mnt_fstype != NULL)
#ifdef HAVE_HASMNTOPT
	  || (hasmntopt (&mntent, "user") != NULL
	      && hasmntopt (&mntent, "user") != hasmntopt (&mntent, "user_xattr"))
	  || hasmntopt (&mntent, "pamconsole") != NULL
	  || hasmntopt (&mntent, "users") != NULL
	  || hasmntopt (&mntent, "owner") != NULL
#endif
	  )
	mount_entry->is_user_mountable = TRUE;
      
      
      return_list = g_list_prepend (return_list, mount_entry);
    }
  
  endmntent (file);
  G_UNLOCK (getmntent);
  
  return g_list_reverse (return_list);
}
#elif defined(HAVE_SYS_MNTCTL_H) && defined(HAVE_SYS_VMOUNT_H) && defined(HAVE_SYS_VFS_H)

/* functions to parse /etc/filesystems on aix */

/* read character, ignoring comments (begin with '*', end with '\n' */
static int
aix_fs_getc (FILE *fd)
{
  int c;
  
  while ((c = getc (fd)) == '*')
    {
      while (((c = getc (fd)) != '\n') && (c != EOF))
	;
    }
}

/* eat all continuous spaces in a file */
static int
aix_fs_ignorespace (FILE *fd)
{
  int c;
  
  while ((c = aix_fs_getc (fd)) != EOF)
    {
      if (!g_ascii_isspace (c))
	{
	  ungetc (c,fd);
	  return c;
	}
    }
  
  return EOF;
}

/* read one word from file */
static int
aix_fs_getword (FILE *fd, char *word)
{
  int c;
  
  aix_fs_ignorespace (fd);

  while (((c = aix_fs_getc (fd)) != EOF) && !g_ascii_isspace (c))
    {
      if (c == '"')
	{
	  while (((c = aix_fs_getc (fd)) != EOF) && (c != '"'))
	    *word++ = c;
	  else
	    *word++ = c;
	}
    }
  *word = 0;
  
  return c;
}

typedef struct {
  char mnt_mount[PATH_MAX];
  char mnt_special[PATH_MAX];
  char mnt_fstype[16];
  char mnt_options[128];
} AixMountTableEntry;

/* read mount points properties */
static int
aix_fs_get (FILE *fd, AixMountTableEntry *prop)
{
  static char word[PATH_MAX] = { 0 };
  char value[PATH_MAX];
  
  /* read stanza */
  if (word[0] == 0)
    {
      if (aix_fs_getword (fd, word) == EOF)
	return EOF;
    }

  word[strlen(word) - 1] = 0;
  strcpy (prop->mnt_mount, word);
  
  /* read attributes and value */
  
  while (aix_fs_getword (fd, word) != EOF)
    {
      /* test if is attribute or new stanza */
      if (word[strlen(word) - 1] == ':')
	return 0;
      
      /* read "=" */
      aix_fs_getword (fd, value);
      
      /* read value */
      aix_fs_getword (fd, value);
      
      if (strcmp (word, "dev") == 0)
	strcpy (prop->mnt_special, value);
      else if (strcmp (word, "vfs") == 0)
	strcpy (prop->mnt_fstype, value);
      else if (strcmp (word, "options") == 0)
	strcpy(prop->mnt_options, value);
    }
  
  return 0;
}

static GList *
_g_get_unix_mount_points (void)
{
  struct mntent *mntent;
  FILE *file;
  char *read_file;
  GUnixMountPoint *mount_entry;
  AixMountTableEntry mntent;
  GList *return_list;
  
  read_file = get_fstab_file ();
  
  file = setmntent (read_file, "r");
  if (file == NULL)
    return NULL;
  
  return_list = NULL;
  
  while (!aix_fs_get (file, &mntent))
    {
      if (strcmp ("cdrfs", mntent.mnt_fstype) == 0)
	{
	  mount_entry = g_new0 (GUnixMountPoint, 1);
	  
	  
	  mount_entry->mount_path = g_strdup (mntent.mnt_mount);
	  mount_entry->device_path = g_strdup (mntent.mnt_special);
	  mount_entry->filesystem_type = g_strdup (mntent.mnt_fstype);
	  mount_entry->is_read_only = TRUE;
	  mount_entry->is_user_mountable = TRUE;
	  
	  return_list = g_list_prepend (return_list, mount_entry);
	}
    }
	
  endmntent (file);
  
  return g_list_reverse (return_list);
}

#elif defined(HAVE_GETMNTINFO) && defined(HAVE_FSTAB_H) && defined(HAVE_SYS_MOUNT_H)

static GList *
_g_get_unix_mount_points (void)
{
  struct fstab *fstab = NULL;
  GUnixMountPoint *mount_entry;
  GList *return_list;
#ifdef HAVE_SYS_SYSCTL_H
  int usermnt = 0;
  size_t len = sizeof(usermnt);
  struct stat sb;
#endif
  
  if (!setfsent ())
    return NULL;

  return_list = NULL;
  
#ifdef HAVE_SYS_SYSCTL_H
#if defined(HAVE_SYSCTLBYNAME)
  sysctlbyname ("vfs.usermount", &usermnt, &len, NULL, 0);
#elif defined(CTL_VFS) && defined(VFS_USERMOUNT)
  {
    int mib[2];
    
    mib[0] = CTL_VFS;
    mib[1] = VFS_USERMOUNT;
    sysctl (mib, 2, &usermnt, &len, NULL, 0);
  }
#elif defined(CTL_KERN) && defined(KERN_USERMOUNT)
  {
    int mib[2];
    
    mib[0] = CTL_KERN;
    mib[1] = KERN_USERMOUNT;
    sysctl (mib, 2, &usermnt, &len, NULL, 0);
  }
#endif
#endif
  
  while ((fstab = getfsent ()) != NULL)
    {
      if (strcmp (fstab->fs_vfstype, "swap") == 0)
	continue;
      
      mount_entry = g_new0 (GUnixMountPoint, 1);
      
      mount_entry->mount_path = g_strdup (fstab->fs_file);
      mount_entry->device_path = g_strdup (fstab->fs_spec);
      mount_entry->filesystem_type = g_strdup (fstab->fs_vfstype);
      
      if (strcmp (fstab->fs_type, "ro") == 0)
	mount_entry->is_read_only = TRUE;

#ifdef HAVE_SYS_SYSCTL_H
      if (usermnt != 0)
	{
	  uid_t uid = getuid ();
	  if (stat (fstab->fs_file, &sb) == 0)
	    {
	      if (uid == 0 || sb.st_uid == uid)
		mount_entry->is_user_mountable = TRUE;
	    }
	}
#endif

      return_list = g_list_prepend (return_list, mount_entry);
    }
  
  endfsent ();
  
  return g_list_reverse (return_list);
}
#else
#error No g_get_mount_table() implementation for system
#endif

static guint64
get_mounts_timestamp (void)
{
  const char *monitor_file;
  struct stat buf;

  monitor_file = get_mtab_monitor_file ();
  if (monitor_file)
    {
      if (stat (monitor_file, &buf) == 0)
	return (guint64)buf.st_mtime;
    }
  return 0;
}

static guint64
get_mount_points_timestamp (void)
{
  const char *monitor_file;
  struct stat buf;

  monitor_file = get_fstab_file ();
  if (monitor_file)
    {
      if (stat (monitor_file, &buf) == 0)
	return (guint64)buf.st_mtime;
    }
  return 0;
}

/**
 * g_get_unix_mounts:
 * @time_read: guint64 to contain a timestamp.
 * 
 * Gets a #GList of strings containing the unix mounts. If @time_read
 * is set, it will be filled with the mount timestamp, 
 * allowing for checking if the mounts have changed with 
 * g_unix_mounts_changed_since().
 * 
 * Returns: a #GList of the UNIX mounts. 
 **/
GList *
g_get_unix_mounts (guint64 *time_read)
{
  if (time_read)
    *time_read = get_mounts_timestamp ();

  return _g_get_unix_mounts ();
}

/**
 * g_get_unix_mount_at:
 * @mount_path: path for a possible unix mount.
 * @time_read: guint64 to contain a timestamp.
 * 
 * Gets a #GUnixMount for a given mount path. If @time_read
 * is set, it will be filled with a unix timestamp for checking
 * if the mounts have changed since with g_unix_mounts_changed_since().
 * 
 * Returns: a #GUnixMount. 
 **/
GUnixMount *
g_get_unix_mount_at (const char *mount_path,
		     guint64 *time_read)
{
  GList *mounts, *l;
  GUnixMount *mount_entry, *found;
  
  mounts = g_get_unix_mounts (time_read);

  found = NULL;
  for (l = mounts; l != NULL; l = l->next)
    {
      mount_entry = l->data;

      if (strcmp (mount_path, mount_entry->mount_path) == 0)
	found = mount_entry;
      else
	g_unix_mount_free (mount_entry);
      
    }
  g_list_free (mounts);

  return found;
}

/**
 * g_get_unix_mount_points:
 * @time_read: guint64 to contain a timestamp.
 * 
 * Gets a #GList of strings containing the unix mount points. 
 * If @time_read is set, it will be filled with the mount timestamp,
 * allowing for checking if the mounts have changed with 
 * g_unix_mounts_points_changed_since().
 * 
 * Returns a #GList of the UNIX mountpoints. 
 **/
GList *
g_get_unix_mount_points (guint64 *time_read)
{
  if (time_read)
    *time_read = get_mount_points_timestamp ();

  return _g_get_unix_mount_points ();
}

/**
 * g_unix_mounts_changed_since:
 * @time: guint64 to contain a timestamp.
 * 
 * Checks if the unix mounts have changed since a given unix time.
 * 
 * Returns %TRUE if the mounts have changed since @time. 
 **/
gboolean
g_unix_mounts_changed_since (guint64 time)
{
  return get_mounts_timestamp () != time;
}

/**
 * g_unix_mount_points_changed_since:
 * @time: guint64 to contain a timestamp.
 * 
 * Checks if the unix mount points have changed since a given unix time.
 * 
 * Returns %TRUE if the mount points have changed since @time. 
 **/
gboolean
g_unix_mount_points_changed_since (guint64 time)
{
  return get_mount_points_timestamp () != time;
}

static void
g_unix_mount_monitor_finalize (GObject *object)
{
  GUnixMountMonitor *monitor;
  
  monitor = G_UNIX_MOUNT_MONITOR (object);

  if (monitor->fstab_monitor)
    {
      g_file_monitor_cancel (monitor->fstab_monitor);
      g_object_unref (monitor->fstab_monitor);
    }
  
  if (monitor->mtab_monitor)
    {
      g_file_monitor_cancel (monitor->mtab_monitor);
      g_object_unref (monitor->mtab_monitor);
    }

  the_mount_monitor = NULL;
  
  if (G_OBJECT_CLASS (g_unix_mount_monitor_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_unix_mount_monitor_parent_class)->finalize) (object);
}


static void
g_unix_mount_monitor_class_init (GUnixMountMonitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_unix_mount_monitor_finalize;
  /**
   * GUnixMountMonitor::mounts-changed:
   * 
   * Emitted when the unix mounts have changed.
   **/ 
  signals[MOUNTS_CHANGED] =
    g_signal_new ("mounts_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  /**
   * GUnixMountMonitor::mountpoints-changed:
   * 
   * Emitted when the unix mount points have changed.
   **/
  signals[MOUNTPOINTS_CHANGED] =
    g_signal_new ("mountpoints_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_LAST,
		  0,
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
}

static void
fstab_file_changed (GFileMonitor* monitor,
		    GFile* file,
		    GFile* other_file,
		    GFileMonitorEvent event_type,
		    gpointer user_data)
{
  GUnixMountMonitor *mount_monitor;

  if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
      event_type != G_FILE_MONITOR_EVENT_CREATED &&
      event_type != G_FILE_MONITOR_EVENT_DELETED)
    return;

  mount_monitor = user_data;
  g_signal_emit (mount_monitor, signals[MOUNTPOINTS_CHANGED], 0);
}

static void
mtab_file_changed (GFileMonitor* monitor,
		   GFile* file,
		   GFile* other_file,
		   GFileMonitorEvent event_type,
		   gpointer user_data)
{
  GUnixMountMonitor *mount_monitor;

  if (event_type != G_FILE_MONITOR_EVENT_CHANGED &&
      event_type != G_FILE_MONITOR_EVENT_CREATED &&
      event_type != G_FILE_MONITOR_EVENT_DELETED)
    return;
  
  mount_monitor = user_data;
  g_signal_emit (mount_monitor, signals[MOUNTS_CHANGED], 0);
}

static void
g_unix_mount_monitor_init (GUnixMountMonitor *monitor)
{
  GFile *file;
    
  if (get_fstab_file () != NULL)
    {
      file = g_file_new_for_path (get_fstab_file ());
      monitor->fstab_monitor = g_file_monitor_file (file, 0, NULL);
      g_object_unref (file);
      
      g_signal_connect (monitor->fstab_monitor, "changed", (GCallback)fstab_file_changed, monitor);
    }
  
  if (get_mtab_monitor_file () != NULL)
    {
      file = g_file_new_for_path (get_mtab_monitor_file ());
      monitor->mtab_monitor = g_file_monitor_file (file, 0, NULL);
      g_object_unref (file);
      
      g_signal_connect (monitor->mtab_monitor, "changed", (GCallback)mtab_file_changed, monitor);
    }
}

/**
 * g_unix_mount_monitor_new:
 * 
 * Gets a new #GUnixMountMonitor.
 * 
 * Returns: a #GUnixMountMonitor. 
 **/
GUnixMountMonitor *
g_unix_mount_monitor_new (void)
{
  if (the_mount_monitor == NULL)
    {
      the_mount_monitor = g_object_new (G_TYPE_UNIX_MOUNT_MONITOR, NULL);
      return the_mount_monitor;
    }
  
  return g_object_ref (the_mount_monitor);
}

/**
 * g_unix_mount_free:
 * @mount_entry: a #GUnixMount.
 * 
 * Frees a unix mount.
 **/
void
g_unix_mount_free (GUnixMount *mount_entry)
{
  g_return_if_fail (mount_entry != NULL);

  g_free (mount_entry->mount_path);
  g_free (mount_entry->device_path);
  g_free (mount_entry->filesystem_type);
  g_free (mount_entry);
}

/**
 * g_unix_mount_point_free:
 * @mount_point: unix mount point to free.
 * 
 * Frees a unix mount point.
 **/
void
g_unix_mount_point_free (GUnixMountPoint *mount_point)
{
  g_return_if_fail (mount_point != NULL);

  g_free (mount_point->mount_path);
  g_free (mount_point->device_path);
  g_free (mount_point->filesystem_type);
  g_free (mount_point);
}

static int
strcmp_null (const char *str1,
	     const char *str2)
{
  if (str1 == str2)
    return 0;
  if (str1 == NULL && str2 != NULL) 
    return -1;
  if (str1 != NULL && str2 == NULL)
    return 1;
  return strcmp (str1, str2);
}

/**
 * g_unix_mount_compare:
 * @mount1: first #GUnixMount to compare.
 * @mount2: second #GUnixMount to compare.
 * 
 * Compares two unix mounts.
 * 
 * Returns 1, 0 or -1 if @mount1 is greater than, equal to,
 * or less than @mount2, respectively. 
 **/
gint
g_unix_mount_compare (GUnixMount      *mount1,
		      GUnixMount      *mount2)
{
  int res;

  g_return_val_if_fail (mount1 != NULL && mount2 != NULL, 0);
  
  res = strcmp_null (mount1->mount_path, mount2->mount_path);
  if (res != 0)
    return res;
	
  res = strcmp_null (mount1->device_path, mount2->device_path);
  if (res != 0)
    return res;
	
  res = strcmp_null (mount1->filesystem_type, mount2->filesystem_type);
  if (res != 0)
    return res;

  res =  mount1->is_read_only - mount2->is_read_only;
  if (res != 0)
    return res;
  
  return 0;
}

/**
 * g_unix_mount_get_mount_path:
 * @mount_entry: input #GUnixMount to get the mount path for.
 * 
 * Gets the mount path for a unix mount.
 * 
 * Returns: the mount path for @mount_entry.
 **/
const char *
g_unix_mount_get_mount_path (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->mount_path;
}

/**
 * g_unix_mount_get_device_path:
 * @mount_entry: a #GUnixMount.
 * 
 * Gets the device path for a unix mount.
 * 
 * Returns: a string containing the device path.
 **/
const char *
g_unix_mount_get_device_path (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->device_path;
}

/**
 * g_unix_mount_get_fs_type:
 * @mount_entry: a #GUnixMount.
 * 
 * Gets the filesystem type for the unix mount.
 * 
 * Returns: a string containing the file system type.
 **/
const char *
g_unix_mount_get_fs_type (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, NULL);

  return mount_entry->filesystem_type;
}

/**
 * g_unix_mount_is_readonly:
 * @mount_entry: a #GUnixMount.
 * 
 * Checks if a unix mount is mounted read only.
 * 
 * Returns: %TRUE if @mount_entry is read only.
 **/
gboolean
g_unix_mount_is_readonly (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, FALSE);

  return mount_entry->is_read_only;
}

/**
 * g_unix_mount_is_system_internal:
 * @mount_entry: a #GUnixMount.
 * 
 * Checks if a unix mount is a system path.
 * 
 * Returns: %TRUE if the unix mount is for a system path.
 **/
gboolean
g_unix_mount_is_system_internal (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, FALSE);

  return mount_entry->is_system_internal;
}

/**
 * g_unix_mount_point_compare:
 * @mount1: a #GUnixMount.
 * @mount2: a #GUnixMount.
 * 
 * Compares two unix mount points.
 * 
 * Returns 1, 0 or -1 if @mount1 is greater than, equal to,
 * or less than @mount2, respectively.
 **/
gint
g_unix_mount_point_compare (GUnixMountPoint *mount1,
			    GUnixMountPoint *mount2)
{
  int res;

  g_return_val_if_fail (mount1 != NULL && mount2 != NULL, 0);

  res = strcmp_null (mount1->mount_path, mount2->mount_path);
  if (res != 0) 
    return res;
	
  res = strcmp_null (mount1->device_path, mount2->device_path);
  if (res != 0) 
    return res;
	
  res = strcmp_null (mount1->filesystem_type, mount2->filesystem_type);
  if (res != 0) 
    return res;

  res =  mount1->is_read_only - mount2->is_read_only;
  if (res != 0) 
    return res;

  res = mount1->is_user_mountable - mount2->is_user_mountable;
  if (res != 0) 
    return res;

  res = mount1->is_loopback - mount2->is_loopback;
  if (res != 0)
    return res;
  
  return 0;
}

/**
 * g_unix_mount_point_get_mount_path:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Gets the mount path for a unix mount point.
 * 
 * Returns: a string containing the mount path.
 **/
const char *
g_unix_mount_point_get_mount_path (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->mount_path;
}

/**
 * g_unix_mount_point_get_device_path:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Gets the device path for a unix mount point.
 * 
 * Returns: a string containing the device path.
 **/
const char *
g_unix_mount_point_get_device_path (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->device_path;
}

/**
 * g_unix_mount_point_get_fs_type:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Gets the file system type for the mount point.
 * 
 * Returns: a string containing the file system type.
 **/
const char *
g_unix_mount_point_get_fs_type (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, NULL);

  return mount_point->filesystem_type;
}

/**
 * g_unix_mount_point_is_readonly:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Checks if a unix mount point is read only.
 * 
 * Returns: %TRUE if a mount point is read only.
 **/
gboolean
g_unix_mount_point_is_readonly (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_read_only;
}

/**
 * g_unix_mount_point_is_user_mountable:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Checks if a unix mount point is mountable by the user.
 * 
 * Returns: %TRUE if the mount point is user mountable.
 **/
gboolean
g_unix_mount_point_is_user_mountable (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_user_mountable;
}

/**
 * g_unix_mount_point_is_loopback:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Checks if a unix mount point is a loopback device.
 * 
 * Returns: %TRUE if the mount point is a loopback. %FALSE otherwise. 
 **/
gboolean
g_unix_mount_point_is_loopback (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, FALSE);

  return mount_point->is_loopback;
}

static GUnixMountType
guess_mount_type (const char *mount_path,
		  const char *device_path,
		  const char *filesystem_type)
{
  GUnixMountType type;
  char *basename;

  type = G_UNIX_MOUNT_TYPE_UNKNOWN;
  
  if ((strcmp (filesystem_type, "udf") == 0) ||
      (strcmp (filesystem_type, "iso9660") == 0) ||
      (strcmp (filesystem_type, "cd9660") == 0))
    type = G_UNIX_MOUNT_TYPE_CDROM;
  else if (strcmp (filesystem_type, "nfs") == 0)
    type = G_UNIX_MOUNT_TYPE_NFS;
  else if (g_str_has_prefix (device_path, "/vol/dev/diskette/") ||
	   g_str_has_prefix (device_path, "/dev/fd") ||
	   g_str_has_prefix (device_path, "/dev/floppy"))
    type = G_UNIX_MOUNT_TYPE_FLOPPY;
  else if (g_str_has_prefix (device_path, "/dev/cdrom") ||
	   g_str_has_prefix (device_path, "/dev/acd") ||
	   g_str_has_prefix (device_path, "/dev/cd"))
    type = G_UNIX_MOUNT_TYPE_CDROM;
  else if (g_str_has_prefix (device_path, "/vol/"))
    {
      const char *name = mount_path + strlen ("/");
      
      if (g_str_has_prefix (name, "cdrom"))
	type = G_UNIX_MOUNT_TYPE_CDROM;
      else if (g_str_has_prefix (name, "floppy") ||
	       g_str_has_prefix (device_path, "/vol/dev/diskette/")) 
	type = G_UNIX_MOUNT_TYPE_FLOPPY;
      else if (g_str_has_prefix (name, "rmdisk")) 
	type = G_UNIX_MOUNT_TYPE_ZIP;
      else if (g_str_has_prefix (name, "jaz"))
	type = G_UNIX_MOUNT_TYPE_JAZ;
      else if (g_str_has_prefix (name, "memstick"))
	type = G_UNIX_MOUNT_TYPE_MEMSTICK;
    }
  else
    {
      basename = g_path_get_basename (mount_path);
      
      if (g_str_has_prefix (basename, "cdrom") ||
	  g_str_has_prefix (basename, "cdwriter") ||
	  g_str_has_prefix (basename, "burn") ||
	  g_str_has_prefix (basename, "cdr") ||
	  g_str_has_prefix (basename, "cdrw") ||
	  g_str_has_prefix (basename, "dvdrom") ||
	  g_str_has_prefix (basename, "dvdram") ||
	  g_str_has_prefix (basename, "dvdr") ||
	  g_str_has_prefix (basename, "dvdrw") ||
	  g_str_has_prefix (basename, "cdrom_dvdrom") ||
	  g_str_has_prefix (basename, "cdrom_dvdram") ||
	  g_str_has_prefix (basename, "cdrom_dvdr") ||
	  g_str_has_prefix (basename, "cdrom_dvdrw") ||
	  g_str_has_prefix (basename, "cdr_dvdrom") ||
	  g_str_has_prefix (basename, "cdr_dvdram") ||
	  g_str_has_prefix (basename, "cdr_dvdr") ||
	  g_str_has_prefix (basename, "cdr_dvdrw") ||
	  g_str_has_prefix (basename, "cdrw_dvdrom") ||
	  g_str_has_prefix (basename, "cdrw_dvdram") ||
	  g_str_has_prefix (basename, "cdrw_dvdr") ||
	  g_str_has_prefix (basename, "cdrw_dvdrw"))
	type = G_UNIX_MOUNT_TYPE_CDROM;
      else if (g_str_has_prefix (basename, "floppy"))
	type = G_UNIX_MOUNT_TYPE_FLOPPY;
      else if (g_str_has_prefix (basename, "zip"))
	type = G_UNIX_MOUNT_TYPE_ZIP;
      else if (g_str_has_prefix (basename, "jaz"))
	type = G_UNIX_MOUNT_TYPE_JAZ;
      else if (g_str_has_prefix (basename, "camera"))
	type = G_UNIX_MOUNT_TYPE_CAMERA;
      else if (g_str_has_prefix (basename, "memstick") ||
	       g_str_has_prefix (basename, "memory_stick") ||
	       g_str_has_prefix (basename, "ram"))
	type = G_UNIX_MOUNT_TYPE_MEMSTICK;
      else if (g_str_has_prefix (basename, "compact_flash"))
	type = G_UNIX_MOUNT_TYPE_CF;
      else if (g_str_has_prefix (basename, "smart_media"))
	type = G_UNIX_MOUNT_TYPE_SM;
      else if (g_str_has_prefix (basename, "sd_mmc"))
	type = G_UNIX_MOUNT_TYPE_SDMMC;
      else if (g_str_has_prefix (basename, "ipod"))
	type = G_UNIX_MOUNT_TYPE_IPOD;
      
      g_free (basename);
    }
  
  if (type == G_UNIX_MOUNT_TYPE_UNKNOWN)
    type = G_UNIX_MOUNT_TYPE_HD;
  
  return type;
}

/**
 * g_unix_mount_guess_type:
 * @mount_entry: a #GUnixMount.
 * 
 * Guesses the type of a unix mount. If the mount type cannot be 
 * determined, returns %G_UNIX_MOUNT_TYPE_UNKNOWN.
 * 
 * Returns: a #GUnixMountType. 
 **/
GUnixMountType
g_unix_mount_guess_type (GUnixMount *mount_entry)
{
  g_return_val_if_fail (mount_entry != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->mount_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->device_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_entry->filesystem_type != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);

  return guess_mount_type (mount_entry->mount_path,
			   mount_entry->device_path,
			   mount_entry->filesystem_type);
}

/**
 * g_unix_mount_point_guess_type:
 * @mount_point: a #GUnixMountPoint.
 * 
 * Guesses the type of a unix mount point. If the mount type cannot be 
 * determined, returns %G_UNIX_MOUNT_TYPE_UNKNOWN.
 * 
 * Returns: a #GUnixMountType.
 **/
GUnixMountType
g_unix_mount_point_guess_type (GUnixMountPoint *mount_point)
{
  g_return_val_if_fail (mount_point != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->mount_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->device_path != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);
  g_return_val_if_fail (mount_point->filesystem_type != NULL, G_UNIX_MOUNT_TYPE_UNKNOWN);

  return guess_mount_type (mount_point->mount_path,
			   mount_point->device_path,
			   mount_point->filesystem_type);
}