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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#if HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#elif HAVE_SYS_MOUNT_H
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/mount.h>
#endif

#if defined(HAVE_STATFS) && defined(HAVE_STATVFS)
/* Some systems have both statfs and statvfs, pick the
   most "native" for these */
# if defined(sun) && defined(__SVR4)
   /* on solaris, statfs doesn't even have the
      f_bavail field */
#  define USE_STATVFS
# else
  /* at least on linux, statfs is the actual syscall */
#  define USE_STATFS
# endif

#elif defined(HAVE_STATFS)

# define USE_STATFS

#elif defined(HAVE_STATVFS)

# define USE_STATVFS

#endif

#include "glocalfile.h"
#include "glocalfileinfo.h"
#include "glocalfileenumerator.h"
#include "glocalfileinputstream.h"
#include "glocalfileoutputstream.h"
#include "glocaldirectorymonitor.h"
#include "glocalfilemonitor.h"
#include "gvolumeprivate.h"
#include <glib/gstdio.h>
#include "glibintl.h"

#include "gioalias.h"

static void g_local_file_file_iface_init (GFileIface       *iface);

static GFileAttributeInfoList *local_writable_attributes = NULL;
static GFileAttributeInfoList *local_writable_namespaces = NULL;

struct _GLocalFile
{
  GObject parent_instance;

  char *filename;
};

#define g_local_file_get_type _g_local_file_get_type
G_DEFINE_TYPE_WITH_CODE (GLocalFile, g_local_file, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_FILE,
						g_local_file_file_iface_init))

static char * find_mountpoint_for (const char *file, dev_t dev);

static void
g_local_file_finalize (GObject *object)
{
  GLocalFile *local;

  local = G_LOCAL_FILE (object);

  g_free (local->filename);
  
  if (G_OBJECT_CLASS (g_local_file_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_local_file_parent_class)->finalize) (object);
}

static void
g_local_file_class_init (GLocalFileClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GFileAttributeInfoList *list;

  gobject_class->finalize = g_local_file_finalize;

  /* Set up attribute lists */

  /* Writable attributes: */

  list = g_file_attribute_info_list_new ();

  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_UNIX_MODE,
				  G_FILE_ATTRIBUTE_TYPE_UINT32,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  
#ifdef HAVE_CHOWN
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_UNIX_UID,
				  G_FILE_ATTRIBUTE_TYPE_UINT32,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_UNIX_GID,
				  G_FILE_ATTRIBUTE_TYPE_UINT32,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
#endif
  
#ifdef HAVE_SYMLINK
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_STD_SYMLINK_TARGET,
				  G_FILE_ATTRIBUTE_TYPE_BYTE_STRING,
				  0);
#endif
  
#ifdef HAVE_UTIMES
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED,
				  G_FILE_ATTRIBUTE_TYPE_UINT64,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
				  G_FILE_ATTRIBUTE_TYPE_UINT32,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_TIME_ACCESS,
				  G_FILE_ATTRIBUTE_TYPE_UINT64,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  g_file_attribute_info_list_add (list,
				  G_FILE_ATTRIBUTE_TIME_ACCESS_USEC,
				  G_FILE_ATTRIBUTE_TYPE_UINT32,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
#endif

  local_writable_attributes = list;

  /* Writable namespaces: */
  
  list = g_file_attribute_info_list_new ();

#ifdef HAVE_XATTR
  g_file_attribute_info_list_add (list,
				  "xattr",
				  G_FILE_ATTRIBUTE_TYPE_STRING,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WITH_FILE |
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
  g_file_attribute_info_list_add (list,
				  "xattr_sys",
				  G_FILE_ATTRIBUTE_TYPE_STRING,
				  G_FILE_ATTRIBUTE_FLAGS_COPY_WHEN_MOVED);
#endif

  local_writable_namespaces = list;
}

static void
g_local_file_init (GLocalFile *local)
{
}


static char *
canonicalize_filename (const char *filename)
{
  char *canon, *start, *p, *q;
  char *cwd;
  
  if (!g_path_is_absolute (filename))
    {
      cwd = g_get_current_dir ();
      canon = g_build_filename (cwd, filename, NULL);
      g_free (cwd);
    }
  else
    canon = g_strdup (filename);

  start = (char *)g_path_skip_root (canon);

  p = start;
  while (*p != 0)
    {
      if (p[0] == '.' && (p[1] == 0 || G_IS_DIR_SEPARATOR (p[1])))
	{
	  memmove (p, p+1, strlen (p+1)+1);
	}
      else if (p[0] == '.' && p[1] == '.' && (p[2] == 0 || G_IS_DIR_SEPARATOR (p[2])))
	{
	  q = p + 2;
	  /* Skip previous separator */
	  p = p - 2;
	  if (p < start)
	    p = start;
	  while (p > start && !G_IS_DIR_SEPARATOR (*p))
	    p--;
	  if (G_IS_DIR_SEPARATOR (*p))
	    *p++ = G_DIR_SEPARATOR;
	  memmove (p, q, strlen (q)+1);
	}
      else
	{
	  /* Skip until next separator */
	  while (*p != 0 && !G_IS_DIR_SEPARATOR (*p))
	    p++;
	  
	  if (*p != 0)
	    {
	      /* Canonicalize one separator */
	      *p++ = G_DIR_SEPARATOR;
	    }
	}

      /* Remove additional separators */
      q = p;
      while (*q && G_IS_DIR_SEPARATOR (*q))
	q++;

      if (p != q)
	memmove (p, q, strlen (q)+1);
    }

  /* Remove trailing slashes */
  if (p > start && G_IS_DIR_SEPARATOR (*(p-1)))
    *(p-1) = 0;
  
  return canon;
}

/**
 * _g_local_file_new:
 * @filename: filename of the file to create.
 * 
 * Returns: new local #GFile.
 **/
GFile *
_g_local_file_new (const char *filename)
{
  GLocalFile *local;

  local = g_object_new (G_TYPE_LOCAL_FILE, NULL);
  local->filename = canonicalize_filename (filename);
  
  return G_FILE (local);
}

static gboolean
g_local_file_is_native (GFile *file)
{
  return TRUE;
}

static gboolean
g_local_file_has_uri_scheme (GFile      *file,
			     const char *uri_scheme)
{
  return g_ascii_strcasecmp (uri_scheme, "file") == 0;
}

static char *
g_local_file_get_uri_scheme (GFile *file)
{
  return g_strdup ("file");
}

static char *
g_local_file_get_basename (GFile *file)
{
  return g_path_get_basename (G_LOCAL_FILE (file)->filename);
}

static char *
g_local_file_get_path (GFile *file)
{
  return g_strdup (G_LOCAL_FILE (file)->filename);
}

static char *
g_local_file_get_uri (GFile *file)
{
  return g_filename_to_uri (G_LOCAL_FILE (file)->filename, NULL, NULL);
}

static gboolean
get_filename_charset (const gchar **filename_charset)
{
  const gchar **charsets;
  gboolean is_utf8;
  
  is_utf8 = g_get_filename_charsets (&charsets);

  if (filename_charset)
    *filename_charset = charsets[0];
  
  return is_utf8;
}

static gboolean
name_is_valid_for_display (const char *string,
			   gboolean    is_valid_utf8)
{
  char c;
  
  if (!is_valid_utf8 &&
      !g_utf8_validate (string, -1, NULL))
    return FALSE;

  while ((c = *string++) != 0)
    {
      if (g_ascii_iscntrl(c))
	return FALSE;
    }

  return TRUE;
}

static char *
g_local_file_get_parse_name (GFile *file)
{
  const char *filename;
  char *parse_name;
  const gchar *charset;
  char *utf8_filename;
  char *roundtripped_filename;
  gboolean free_utf8_filename;
  gboolean is_valid_utf8;

  filename = G_LOCAL_FILE (file)->filename;
  if (get_filename_charset (&charset))
    {
      utf8_filename = (char *)filename;
      free_utf8_filename = FALSE;
      is_valid_utf8 = FALSE; /* Can't guarantee this */
    }
  else
    {
      utf8_filename = g_convert (filename, -1, 
				 "UTF-8", charset, NULL, NULL, NULL);
      free_utf8_filename = TRUE;
      is_valid_utf8 = TRUE;

      if (utf8_filename != NULL)
	{
	  /* Make sure we can roundtrip: */
	  roundtripped_filename = g_convert (utf8_filename, -1,
					     charset, "UTF-8", NULL, NULL, NULL);
	  
	  if (roundtripped_filename == NULL ||
	      strcmp (utf8_filename, roundtripped_filename) != 0)
	    {
	      g_free (utf8_filename);
	      utf8_filename = NULL;
	    }
	}
    }


  if (utf8_filename != NULL &&
      name_is_valid_for_display (utf8_filename, is_valid_utf8))
    {
      if (free_utf8_filename)
	parse_name = utf8_filename;
      else
	parse_name = g_strdup (utf8_filename);
    }
  else
    {
      parse_name = g_filename_to_uri (filename, NULL, NULL);
      if (free_utf8_filename)
	g_free (utf8_filename);
    }
  
  return parse_name;
}

static GFile *
g_local_file_get_parent (GFile *file)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  const char *non_root;
  char *dirname;
  GFile *parent;

  /* Check for root */
  non_root = g_path_skip_root (local->filename);
  if (*non_root == 0)
    return NULL;

  dirname = g_path_get_dirname (local->filename);
  parent = _g_local_file_new (dirname);
  g_free (dirname);
  return parent;
}

static GFile *
g_local_file_dup (GFile *file)
{
  GLocalFile *local = G_LOCAL_FILE (file);

  return _g_local_file_new (local->filename);
}

static guint
g_local_file_hash (GFile *file)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  
  return g_str_hash (local->filename);
}

static gboolean
g_local_file_equal (GFile *file1,
		    GFile *file2)
{
  GLocalFile *local1 = G_LOCAL_FILE (file1);
  GLocalFile *local2 = G_LOCAL_FILE (file2);

  return g_str_equal (local1->filename, local2->filename);
}

static const char *
match_prefix (const char *path, 
              const char *prefix)
{
  int prefix_len;

  prefix_len = strlen (prefix);
  if (strncmp (path, prefix, prefix_len) != 0)
    return NULL;
  return path + prefix_len;
}

static gboolean
g_local_file_contains_file (GFile *parent,
			    GFile *descendant)
{
  GLocalFile *parent_local = G_LOCAL_FILE (parent);
  GLocalFile *descendant_local = G_LOCAL_FILE (descendant);
  const char *remainder;

  remainder = match_prefix (descendant_local->filename, parent_local->filename);
  if (remainder != NULL && G_IS_DIR_SEPARATOR (*remainder))
    return TRUE;
  return FALSE;
}

static char *
g_local_file_get_relative_path (GFile *parent,
				GFile *descendant)
{
  GLocalFile *parent_local = G_LOCAL_FILE (parent);
  GLocalFile *descendant_local = G_LOCAL_FILE (descendant);
  const char *remainder;

  remainder = match_prefix (descendant_local->filename, parent_local->filename);
  
  if (remainder != NULL && G_IS_DIR_SEPARATOR (*remainder))
    return g_strdup (remainder + 1);
  return NULL;
}

static GFile *
g_local_file_resolve_relative_path (GFile      *file,
				    const char *relative_path)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  char *filename;
  GFile *child;

  if (g_path_is_absolute (relative_path))
    return _g_local_file_new (relative_path);
  
  filename = g_build_filename (local->filename, relative_path, NULL);
  child = _g_local_file_new (filename);
  g_free (filename);
  
  return child;
}

static GFileEnumerator *
g_local_file_enumerate_children (GFile                *file,
				 const char           *attributes,
				 GFileQueryInfoFlags   flags,
				 GCancellable         *cancellable,
				 GError              **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  return _g_local_file_enumerator_new (local->filename,
				       attributes, flags,
				       cancellable, error);
}

static GFile *
g_local_file_get_child_for_display_name (GFile        *file,
					 const char   *display_name,
					 GError      **error)
{
  GFile *parent, *new_file;
  char *basename;

  basename = g_filename_from_utf8 (display_name, -1, NULL, NULL, NULL);
  if (basename == NULL)
    {
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_INVALID_FILENAME,
		   _("Invalid filename %s"), display_name);
      return NULL;
    }

  parent = g_file_get_parent (file);
  new_file = g_file_get_child (file, basename);
  g_object_unref (parent);
  g_free (basename);
  
  return new_file;
}

#ifdef USE_STATFS
static const char *
get_fs_type (long f_type)
{

  /* filesystem ids taken from linux manpage */
  switch (f_type) 
    {
    case 0xadf5:
      return "adfs";
    case 0xADFF:
      return "affs";
    case 0x42465331:
      return "befs";
    case 0x1BADFACE:
      return "bfs";
    case 0xFF534D42:
      return "cifs";
    case 0x73757245:
      return "coda";
    case 0x012FF7B7:
      return "coh";
    case 0x28cd3d45:
      return "cramfs";
    case 0x1373:
      return "devfs";
    case 0x00414A53:
      return "efs";
    case 0x137D:
      return "ext";
    case 0xEF51:
      return "ext2";
    case 0xEF53:
      return "ext3";
    case 0x4244:
      return "hfs";
    case 0xF995E849:
      return "hpfs";
    case 0x958458f6:
      return "hugetlbfs";
    case 0x9660:
      return "isofs";
    case 0x72b6:
      return "jffs2";
    case 0x3153464a:
      return "jfs";
    case 0x137F:
      return "minix";
    case 0x138F:
      return "minix2";
    case 0x2468:
      return "minix2";
    case 0x2478:
      return "minix22";
    case 0x4d44:
      return "msdos";
    case 0x564c:
      return "ncp";
    case 0x6969:
      return "nfs";
    case 0x5346544e:
      return "ntfs";
    case 0x9fa1:
      return "openprom";
    case 0x9fa0:
      return "proc";
    case 0x002f:
      return "qnx4";
    case 0x52654973:
      return "reiserfs";
    case 0x7275:
      return "romfs";
    case 0x517B:
      return "smb";
    case 0x012FF7B6:
      return "sysv2";
    case 0x012FF7B5:
      return "sysv4";
    case 0x01021994:
      return "tmpfs";
    case 0x15013346:
      return "udf";
    case 0x00011954:
      return "ufs";
    case 0x9fa2:
      return "usbdevice";
    case 0xa501FCF5:
      return "vxfs";
    case 0x012FF7B4:
      return "xenix";
    case 0x58465342:
      return "xfs";
    case 0x012FD16D:
      return "xiafs";
    default:
      return NULL;
    }
}
#endif

G_LOCK_DEFINE_STATIC(mount_info_hash);
static GHashTable *mount_info_hash = NULL;
guint64 mount_info_hash_cache_time = 0;

typedef enum {
  MOUNT_INFO_READONLY = 1<<0
} MountInfo;

static gboolean
device_equal (gconstpointer v1,
              gconstpointer v2)
{
  return *(dev_t *)v1 == * (dev_t *)v2;
}

static guint
device_hash (gconstpointer v)
{
  return (guint) *(dev_t *)v;
}

static void
get_mount_info (GFileInfo             *fs_info,
		const char            *path,
		GFileAttributeMatcher *matcher)
{
  struct stat buf;
  gboolean got_info;
  gpointer info_as_ptr;
  guint mount_info;
  char *mountpoint;
  dev_t *dev;
  GUnixMount *mount;
  guint64 cache_time;

  if (lstat (path, &buf) != 0)
    return;

  G_LOCK (mount_info_hash);

  if (mount_info_hash == NULL)
    mount_info_hash = g_hash_table_new_full (device_hash, device_equal,
					     g_free, NULL);


  if (g_unix_mounts_changed_since (mount_info_hash_cache_time))
    g_hash_table_remove_all (mount_info_hash);
  
  got_info = g_hash_table_lookup_extended (mount_info_hash,
					   &buf.st_dev,
					   NULL,
					   &info_as_ptr);
  
  G_UNLOCK (mount_info_hash);
  
  mount_info = GPOINTER_TO_UINT (info_as_ptr);
  
  if (!got_info)
    {
      mount_info = 0;

      mountpoint = find_mountpoint_for (path, buf.st_dev);
      if (mountpoint == NULL)
	mountpoint = "/";

      mount = g_get_unix_mount_at (mountpoint, &cache_time);
      if (mount)
	{
	  if (g_unix_mount_is_readonly (mount))
	    mount_info |= MOUNT_INFO_READONLY;
	  
	  g_unix_mount_free (mount);
	}

      dev = g_new0 (dev_t, 1);
      *dev = buf.st_dev;
      
      G_LOCK (mount_info_hash);
      mount_info_hash_cache_time = cache_time;
      g_hash_table_insert (mount_info_hash, dev, GUINT_TO_POINTER (mount_info));
      G_UNLOCK (mount_info_hash);
    }

  if (mount_info & MOUNT_INFO_READONLY)
    g_file_info_set_attribute_boolean (fs_info, G_FILE_ATTRIBUTE_FS_READONLY, TRUE);
}

static GFileInfo *
g_local_file_query_filesystem_info (GFile         *file,
				    const char    *attributes,
				    GCancellable  *cancellable,
				    GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  GFileInfo *info;
  int statfs_result;
  gboolean no_size;
  guint64 block_size;
#ifdef USE_STATFS
  struct statfs statfs_buffer;
  const char *fstype;
#elif defined(USE_STATVFS)
  struct statvfs statfs_buffer;
#endif
  GFileAttributeMatcher *attribute_matcher;
	
  no_size = FALSE;
  
#ifdef USE_STATFS
  
#if STATFS_ARGS == 2
  statfs_result = statfs (local->filename, &statfs_buffer);
#elif STATFS_ARGS == 4
  statfs_result = statfs (local->filename, &statfs_buffer,
			  sizeof (statfs_buffer), 0);
#endif
  block_size = statfs_buffer.f_bsize;
  
#if defined(__linux__)
  /* ncpfs does not know the amount of available and free space *
   * assuming ncpfs is linux specific, if you are on a non-linux platform
   * where ncpfs is available, please file a bug about it on bugzilla.gnome.org
   */
  if (statfs_buffer.f_bavail == 0 && statfs_buffer.f_bfree == 0 &&
      /* linux/ncp_fs.h: NCP_SUPER_MAGIC == 0x564c */
      statfs_buffer.f_type == 0x564c)
    no_size = TRUE;
#endif
  
#elif defined(USE_STATVFS)
  statfs_result = statvfs (local->filename, &statfs_buffer);
  block_size = statfs_buffer.f_frsize; 
#endif

  if (statfs_result == -1)
    {
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Error getting filesystem info: %s"),
		   g_strerror (errno));
      return NULL;
    }

  info = g_file_info_new ();

  attribute_matcher = g_file_attribute_matcher_new (attributes);
  
  if (g_file_attribute_matcher_matches (attribute_matcher,
					G_FILE_ATTRIBUTE_FS_FREE))
    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_FS_FREE, block_size * statfs_buffer.f_bavail);
  if (g_file_attribute_matcher_matches (attribute_matcher,
					G_FILE_ATTRIBUTE_FS_SIZE))
    g_file_info_set_attribute_uint64 (info, G_FILE_ATTRIBUTE_FS_SIZE, block_size * statfs_buffer.f_blocks);

#ifdef USE_STATFS
  fstype = get_fs_type (statfs_buffer.f_type);
  if (fstype &&
      g_file_attribute_matcher_matches (attribute_matcher,
					G_FILE_ATTRIBUTE_FS_TYPE))
    g_file_info_set_attribute_string (info, G_FILE_ATTRIBUTE_FS_TYPE, fstype);
#endif  

  if (g_file_attribute_matcher_matches (attribute_matcher,
					G_FILE_ATTRIBUTE_FS_READONLY))
    {
      get_mount_info (info, local->filename, attribute_matcher);
    }
  
  g_file_attribute_matcher_unref (attribute_matcher);
  
  return info;
}

static GVolume *
g_local_file_find_enclosing_volume (GFile         *file,
				    GCancellable  *cancellable,
				    GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  struct stat buf;
  char *mountpoint;
  GVolume *volume;

  if (lstat (local->filename, &buf) != 0)
    {
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_NOT_FOUND,
		   _("Containing volume does not exist"));
      return NULL;
    }

  mountpoint = find_mountpoint_for (local->filename, buf.st_dev);
  if (mountpoint == NULL)
    {
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_NOT_FOUND,
		   _("Containing volume does not exist"));
      return NULL;
    }

  volume = _g_volume_get_for_mount_path (mountpoint);
  g_free (mountpoint);
  if (volume)
    return volume;

  g_set_error (error, G_IO_ERROR,
	       G_IO_ERROR_NOT_FOUND,
	       _("Containing volume does not exist"));
  return NULL;
}

static GFile *
g_local_file_set_display_name (GFile         *file,
			       const char    *display_name,
			       GCancellable  *cancellable,
			       GError       **error)
{
  GLocalFile *local, *new_local;
  GFile *new_file, *parent;
  struct stat statbuf;
  int errsv;

  parent = g_file_get_parent (file);
  if (parent == NULL)
    {
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_FAILED,
		   _("Can't rename root directory"));
      return NULL;
    }
  
  new_file = g_file_get_child_for_display_name  (parent, display_name, error);
  g_object_unref (parent);
  
  if (new_file == NULL)
    return NULL;
  
  local = G_LOCAL_FILE (file);
  new_local = G_LOCAL_FILE (new_file);

  if (!(g_lstat (new_local->filename, &statbuf) == -1 &&
	errno == ENOENT))
    {
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_EXISTS,
		   _("Can't rename file, filename already exist"));
      return NULL;
    }

  if (rename (local->filename, new_local->filename) == -1)
    {
      errsv = errno;

      if (errsv == EINVAL)
	/* We can't get a rename file into itself error herer,
	   so this must be an invalid filename, on e.g. FAT */
	g_set_error (error, G_IO_ERROR,
		     G_IO_ERROR_INVALID_FILENAME,
		     _("Invalid filename"));
      else
	g_set_error (error, G_IO_ERROR,
		     g_io_error_from_errno (errsv),
		     _("Error renaming file: %s"),
		     g_strerror (errsv));
      g_object_unref (new_file);
      return NULL;
    }
  
  return new_file;
}

static GFileInfo *
g_local_file_query_info (GFile                *file,
			 const char           *attributes,
			 GFileQueryInfoFlags   flags,
			 GCancellable         *cancellable,
			 GError              **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  GFileInfo *info;
  GFileAttributeMatcher *matcher;
  char *basename, *dirname;
  GLocalParentFileInfo parent_info;

  matcher = g_file_attribute_matcher_new (attributes);
  
  basename = g_path_get_basename (local->filename);
  
  dirname = g_path_get_dirname (local->filename);
  _g_local_file_info_get_parent_info (dirname, matcher, &parent_info);
  g_free (dirname);
  
  info = _g_local_file_info_get (basename, local->filename,
				 matcher, flags, &parent_info,
				 error);
  
  g_free (basename);

  g_file_attribute_matcher_unref (matcher);

  return info;
}

static GFileAttributeInfoList *
g_local_file_query_settable_attributes (GFile         *file,
					GCancellable  *cancellable,
					GError       **error)
{
  return g_file_attribute_info_list_ref (local_writable_attributes);
}

static GFileAttributeInfoList *
g_local_file_query_writable_namespaces (GFile         *file,
					GCancellable  *cancellable,
					GError       **error)
{
  return g_file_attribute_info_list_ref (local_writable_namespaces);
}

static gboolean
g_local_file_set_attribute (GFile                      *file,
			    const char                 *attribute,
			    const GFileAttributeValue  *value,
			    GFileQueryInfoFlags         flags,
			    GCancellable               *cancellable,
			    GError                    **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);

  return _g_local_file_info_set_attribute (local->filename,
					   attribute,
					   value,
					   flags,
					   cancellable,
					   error);
}

static gboolean
g_local_file_set_attributes_from_info (GFile                *file,
				       GFileInfo            *info,
				       GFileQueryInfoFlags   flags,
				       GCancellable         *cancellable,
				       GError              **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  int res, chained_res;
  GFileIface* default_iface;

  res = _g_local_file_info_set_attributes (local->filename,
					   info, flags, 
					   cancellable,
					   error);

  if (!res)
    error = NULL; /* Don't write over error if further errors */

  default_iface = g_type_default_interface_peek (G_TYPE_FILE);

  chained_res = (default_iface->set_attributes_from_info) (file, info, flags, cancellable, error);
  
  return res && chained_res;
}

static GFileInputStream *
g_local_file_read (GFile         *file,
		   GCancellable  *cancellable,
		   GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  int fd;
  struct stat buf;
  
  fd = g_open (local->filename, O_RDONLY, 0);
  if (fd == -1)
    {
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Error opening file: %s"),
		   g_strerror (errno));
      return NULL;
    }

  if (fstat(fd, &buf) == 0 && S_ISDIR (buf.st_mode))
    {
      close (fd);
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_IS_DIRECTORY,
		   _("Can't open directory"));
      return NULL;
    }
  
  return _g_local_file_input_stream_new (fd);
}

static GFileOutputStream *
g_local_file_append_to (GFile             *file,
			GFileCreateFlags   flags,
			GCancellable      *cancellable,
			GError           **error)
{
  return _g_local_file_output_stream_append (G_LOCAL_FILE (file)->filename,
					     flags, cancellable, error);
}

static GFileOutputStream *
g_local_file_create (GFile             *file,
		     GFileCreateFlags   flags,
		     GCancellable      *cancellable,
		     GError           **error)
{
  return _g_local_file_output_stream_create (G_LOCAL_FILE (file)->filename,
					     flags, cancellable, error);
}

static GFileOutputStream *
g_local_file_replace (GFile             *file,
		      const char        *etag,
		      gboolean           make_backup,
		      GFileCreateFlags   flags,
		      GCancellable      *cancellable,
		      GError           **error)
{
  return _g_local_file_output_stream_replace (G_LOCAL_FILE (file)->filename,
					      etag, make_backup, flags,
					      cancellable, error);
}


static gboolean
g_local_file_delete (GFile         *file,
		     GCancellable  *cancellable,
		     GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  
  if (g_remove (local->filename) == -1)
    {
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Error removing file: %s"),
		   g_strerror (errno));
      return FALSE;
    }
  
  return TRUE;
}

static char *
strip_trailing_slashes (const char *path)
{
  char *path_copy;
  int len;

  path_copy = g_strdup (path);
  len = strlen (path_copy);
  while (len > 1 && path_copy[len-1] == '/')
    path_copy[--len] = 0;

  return path_copy;
 }

static char *
expand_symlink (const char *link)
{
  char *resolved, *canonical, *parent, *link2;
  char symlink_value[4096];
  ssize_t res;
  
  res = readlink (link, symlink_value, sizeof (symlink_value) - 1);
  if (res == -1)
    return g_strdup (link);
  symlink_value[res] = 0;
  
  if (g_path_is_absolute (symlink_value))
    return canonicalize_filename (symlink_value);
  else
    {
      link2 = strip_trailing_slashes (link);
      parent = g_path_get_dirname (link2);
      g_free (link2);
      
      resolved = g_build_filename (parent, symlink_value, NULL);
      g_free (parent);
      
      canonical = canonicalize_filename (resolved);
      
      g_free (resolved);

      return canonical;
    }
}

static char *
get_parent (const char *path, 
            dev_t      *parent_dev)
{
  char *parent, *tmp;
  struct stat parent_stat;
  int num_recursions;
  char *path_copy;

  path_copy = strip_trailing_slashes (path);
  
  parent = g_path_get_dirname (path_copy);
  if (strcmp (parent, ".") == 0 ||
      strcmp (parent, path_copy) == 0)
    {
      g_free (path_copy);
      return NULL;
    }
  g_free (path_copy);

  num_recursions = 0;
  do {
    if (g_lstat (parent, &parent_stat) != 0)
      {
	g_free (parent);
	return NULL;
      }
    
    if (S_ISLNK (parent_stat.st_mode))
      {
	tmp = parent;
	parent = expand_symlink (parent);
	g_free (tmp);
      }
    
    num_recursions++;
    if (num_recursions > 12)
      {
	g_free (parent);
	return NULL;
      }
  } while (S_ISLNK (parent_stat.st_mode));

  *parent_dev = parent_stat.st_dev;
  
  return parent;
}

static char *
expand_all_symlinks (const char *path)
{
  char *parent, *parent_expanded;
  char *basename, *res;
  dev_t parent_dev;

  parent = get_parent (path, &parent_dev);
  if (parent)
    {
      parent_expanded = expand_all_symlinks (parent);
      g_free (parent);
      basename = g_path_get_basename (path);
      res = g_build_filename (parent_expanded, basename, NULL);
      g_free (basename);
      g_free (parent_expanded);
    }
  else
    res = g_strdup (path);
  
  return res;
}

static char *
find_mountpoint_for (const char *file, 
                     dev_t       dev)
{
  char *dir, *parent;
  dev_t dir_dev, parent_dev;

  dir = g_strdup (file);
  dir_dev = dev;

  while (1) 
    {
      parent = get_parent (dir, &parent_dev);
      if (parent == NULL)
        return dir;
    
      if (parent_dev != dir_dev)
        {
          g_free (parent);
          return dir;
        }
    
      g_free (dir);
      dir = parent;
    }
}

static char *
find_topdir_for (const char *file)
{
  char *dir;
  dev_t dir_dev;

  dir = get_parent (file, &dir_dev);
  if (dir == NULL)
    return NULL;

  return find_mountpoint_for (dir, dir_dev);
}

static char *
get_unique_filename (const char *basename, 
                     int         id)
{
  const char *dot;
      
  if (id == 1)
    return g_strdup (basename);

  dot = strchr (basename, '.');
  if (dot)
    return g_strdup_printf ("%.*s.%d%s", dot - basename, basename, id, dot);
  else
    return g_strdup_printf ("%s.%d", basename, id);
}

static gboolean
path_has_prefix (const char *path, 
                 const char *prefix)
{
  int prefix_len;

  if (prefix == NULL)
    return TRUE;

  prefix_len = strlen (prefix);
  
  if (strncmp (path, prefix, prefix_len) == 0 &&
      (prefix_len == 0 || /* empty prefix always matches */
       prefix[prefix_len - 1] == '/' || /* last char in prefix was a /, so it must be in path too */
       path[prefix_len] == 0 ||
       path[prefix_len] == '/'))
    return TRUE;
  
  return FALSE;
}

static char *
try_make_relative (const char *path, 
                   const char *base)
{
  char *path2, *base2;
  char *relative;

  path2 = expand_all_symlinks (path);
  base2 = expand_all_symlinks (base);

  relative = NULL;
  if (path_has_prefix (path2, base2))
    {
      relative = path2 + strlen (base2);
      while (*relative == '/')
	relative ++;
      relative = g_strdup (relative);
    }
  g_free (path2);
  g_free (base2);

  if (relative)
    return relative;
  
  /* Failed, use abs path */
  return g_strdup (path);
}

static char *
escape_trash_name (char *name)
{
  GString *str;
  const gchar hex[16] = "0123456789ABCDEF";
  
  str = g_string_new ("");

  while (*name != 0)
    {
      char c;

      c = *name++;

      if (g_ascii_isprint (c))
	g_string_append_c (str, c);
      else
	{
          g_string_append_c (str, '%');
          g_string_append_c (str, hex[((guchar)c) >> 4]);
          g_string_append_c (str, hex[((guchar)c) & 0xf]);
	}
    }

  return g_string_free (str, FALSE);
}

static gboolean
g_local_file_trash (GFile         *file,
		    GCancellable  *cancellable,
		    GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  struct stat file_stat, home_stat, trash_stat, global_stat;
  const char *homedir;
  char *dirname;
  char *trashdir, *globaldir, *topdir, *infodir, *filesdir;
  char *basename, *trashname, *trashfile, *infoname, *infofile;
  char *original_name, *original_name_escaped;
  uid_t uid;
  char uid_str[32];
  int i;
  char *data;
  gboolean is_homedir_trash;
  time_t t;
  struct tm now;
  char delete_time[32];
  int fd;
  
  if (g_lstat (local->filename, &file_stat) != 0)
    {
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Error trashing file: %s"),
		   g_strerror (errno));
      return FALSE;
    }
    
  homedir = g_get_home_dir ();
  g_stat (homedir, &home_stat);

  is_homedir_trash = FALSE;
  trashdir = NULL;
  if (file_stat.st_dev == home_stat.st_dev)
    {
      is_homedir_trash = TRUE;
      errno = 0;
      trashdir = g_build_filename (g_get_user_data_dir (), "Trash", NULL);
      if (g_mkdir_with_parents (trashdir, 0700) < 0)
	{
          char *display_name;
          int err;

          err = errno;
          display_name = g_filename_display_name (trashdir);
          g_set_error (error, G_IO_ERROR,
                       g_io_error_from_errno (err),
                       _("Unable to create trash dir %s: %s"),
                       display_name, g_strerror (err));
          g_free (display_name);
          g_free (trashdir);
          return FALSE;
	}
      topdir = g_strdup (g_get_user_data_dir ());
    }
  else
    {
      uid = geteuid ();
      g_snprintf (uid_str, sizeof (uid_str), "%lu", (unsigned long)uid);
      
      topdir = find_topdir_for (local->filename);
      if (topdir == NULL)
	{
	  g_set_error (error, G_IO_ERROR,
		       G_IO_ERROR_NOT_SUPPORTED,
		       _("Unable to find toplevel directory for trash"));
	  return FALSE;
	}
      
      /* Try looking for global trash dir $topdir/.Trash/$uid */
      globaldir = g_build_filename (topdir, ".Trash", NULL);
      if (g_lstat (globaldir, &global_stat) == 0 &&
	  S_ISDIR (global_stat.st_mode) &&
	  (global_stat.st_mode & S_ISVTX) != 0)
	{
	  trashdir = g_build_filename (globaldir, uid_str, NULL);

	  if (g_lstat (trashdir, &trash_stat) == 0)
	    {
	      if (!S_ISDIR (trash_stat.st_mode) ||
		  trash_stat.st_uid != uid)
		{
		  /* Not a directory or not owned by user, ignore */
		  g_free (trashdir);
		  trashdir = NULL;
		}
	    }
	  else if (g_mkdir (trashdir, 0700) == -1)
	    {
	      g_free (trashdir);
	      trashdir = NULL;
	    }
	}
      g_free (globaldir);

      if (trashdir == NULL)
	{
	  /* No global trash dir, or it failed the tests, fall back to $topdir/.Trash-$uid */
	  dirname = g_strdup_printf (".Trash-%s", uid_str);
	  trashdir = g_build_filename (topdir, dirname, NULL);
	  g_free (dirname);
	  
	  if (g_lstat (trashdir, &trash_stat) == 0)
	    {
	      if (!S_ISDIR (trash_stat.st_mode) ||
		  trash_stat.st_uid != uid)
		{
		  /* Not a directory or not owned by user, ignore */
		  g_free (trashdir);
		  trashdir = NULL;
		}
	    }
	  else if (g_mkdir (trashdir, 0700) == -1)
	    {
	      g_free (trashdir);
	      trashdir = NULL;
	    }
	}

      if (trashdir == NULL)
	{
	  g_free (topdir);
	  g_set_error (error, G_IO_ERROR,
		       G_IO_ERROR_NOT_SUPPORTED,
		       _("Unable to find or create trash directory"));
	  return FALSE;
	}
    }

  /* Trashdir points to the trash dir with the "info" and "files" subdirectories */

  infodir = g_build_filename (trashdir, "info", NULL);
  filesdir = g_build_filename (trashdir, "files", NULL);
  g_free (trashdir);

  /* Make sure we have the subdirectories */
  if ((g_mkdir (infodir, 0700) == -1 && errno != EEXIST) ||
      (g_mkdir (filesdir, 0700) == -1 && errno != EEXIST))
    {
      g_free (topdir);
      g_free (infodir);
      g_free (filesdir);
      g_set_error (error, G_IO_ERROR,
		   G_IO_ERROR_NOT_SUPPORTED,
		   _("Unable to find or create trash directory"));
      return FALSE;
    }  

  basename = g_path_get_basename (local->filename);
  i = 1;
  trashname = NULL;
  infofile = NULL;
  do {
    g_free (trashname);
    g_free (infofile);
    
    trashname = get_unique_filename (basename, i++);
    infoname = g_strconcat (trashname, ".trashinfo", NULL);
    infofile = g_build_filename (infodir, infoname, NULL);
    g_free (infoname);

    fd = open (infofile, O_CREAT | O_EXCL, 0666);
  } while (fd == -1 && errno == EEXIST);

  g_free (basename);
  g_free (infodir);

  if (fd == -1)
    {
      g_free (filesdir);
      g_free (topdir);
      g_free (trashname);
      g_free (infofile);
      
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Unable to create trashed file: %s"),
		   g_strerror (errno));
      return FALSE;
    }

  close (fd);

  /* TODO: Maybe we should verify that you can delete the file from the trash
     before moving it? OTOH, that is hard, as it needs a recursive scan */

  trashfile = g_build_filename (filesdir, trashname, NULL);

  g_free (filesdir);

  if (g_rename (local->filename, trashfile) == -1)
    {
      g_free (topdir);
      g_free (trashname);
      g_free (infofile);
      g_free (trashfile);
      
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Unable to trash file: %s"),
		   g_strerror (errno));
      return FALSE;
    }

  g_free (trashfile);

  /* TODO: Do we need to update mtime/atime here after the move? */

  /* Use absolute names for homedir */
  if (is_homedir_trash)
    original_name = g_strdup (local->filename);
  else
    original_name = try_make_relative (local->filename, topdir);
  original_name_escaped = escape_trash_name (original_name);
  
  g_free (original_name);
  g_free (topdir);
  
  t = time (NULL);
  localtime_r (&t, &now);
  delete_time[0] = 0;
  strftime(delete_time, sizeof (delete_time), "%Y-%m-%dT%H:%M:%S", &now);

  data = g_strdup_printf ("[Trash Info]\nPath=%s\nDeletionDate=%s\n",
			  original_name_escaped, delete_time);

  g_file_set_contents (infofile, data, -1, NULL);
  g_free (infofile);
  g_free (data);
  
  g_free (original_name_escaped);
  g_free (trashname);
  
  return TRUE;
}

static gboolean
g_local_file_make_directory (GFile         *file,
			     GCancellable  *cancellable,
			     GError       **error)
{
  GLocalFile *local = G_LOCAL_FILE (file);
  
  if (g_mkdir (local->filename, 0755) == -1)
    {
      int errsv = errno;

      if (errsv == EINVAL)
	/* This must be an invalid filename, on e.g. FAT */
	g_set_error (error, G_IO_ERROR,
		     G_IO_ERROR_INVALID_FILENAME,
		     _("Invalid filename"));
      else
	g_set_error (error, G_IO_ERROR,
		     g_io_error_from_errno (errsv),
		     _("Error removing file: %s"),
		     g_strerror (errsv));
      return FALSE;
    }
  
  return TRUE;
}

static gboolean
g_local_file_make_symbolic_link (GFile         *file,
				 const char    *symlink_value,
				 GCancellable  *cancellable,
				 GError       **error)
{
#ifdef HAVE_SYMLINK
  GLocalFile *local = G_LOCAL_FILE (file);
  
  if (symlink (symlink_value, local->filename) == -1)
    {
      int errsv = errno;

      if (errsv == EINVAL)
	/* This must be an invalid filename, on e.g. FAT */
	g_set_error (error, G_IO_ERROR,
		     G_IO_ERROR_INVALID_FILENAME,
		     _("Invalid filename"));
      else
	g_set_error (error, G_IO_ERROR,
		     g_io_error_from_errno (errsv),
		     _("Error making symbolic link: %s"),
		     g_strerror (errsv));
      return FALSE;
    }
  return TRUE;
#else
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Symlinks not supported");
  return FALSE;
#endif
}


static gboolean
g_local_file_copy (GFile                  *source,
		   GFile                  *destination,
		   GFileCopyFlags          flags,
		   GCancellable           *cancellable,
		   GFileProgressCallback   progress_callback,
		   gpointer                progress_callback_data,
		   GError                **error)
{
  /* Fall back to default copy */
  g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Copy not supported");
  return FALSE;
}

static gboolean
g_local_file_move (GFile                  *source,
		   GFile                  *destination,
		   GFileCopyFlags          flags,
		   GCancellable           *cancellable,
		   GFileProgressCallback   progress_callback,
		   gpointer                progress_callback_data,
		   GError                **error)
{
  GLocalFile *local_source = G_LOCAL_FILE (source);
  GLocalFile *local_destination = G_LOCAL_FILE (destination);
  struct stat statbuf;
  gboolean destination_exist, source_is_dir;
  char *backup_name;
  int res;

  res = g_lstat (local_source->filename, &statbuf);
  if (res == -1)
    {
      g_set_error (error, G_IO_ERROR,
		   g_io_error_from_errno (errno),
		   _("Error moving file: %s"),
		   g_strerror (errno));
      return FALSE;
    }
  else
    source_is_dir = S_ISDIR (statbuf.st_mode);
  
  destination_exist = FALSE;
  res = g_lstat (local_destination->filename, &statbuf);
  if (res == 0)
    {
      destination_exist = TRUE; /* Target file exists */

      if (flags & G_FILE_COPY_OVERWRITE)
	{
	  /* Always fail on dirs, even with overwrite */
	  if (S_ISDIR (statbuf.st_mode))
	    {
	      g_set_error (error,
			   G_IO_ERROR,
			   G_IO_ERROR_WOULD_MERGE,
			   _("Can't move directory over directory"));
	      return FALSE;
	    }
	}
      else
	{
	  g_set_error (error,
		       G_IO_ERROR,
		       G_IO_ERROR_EXISTS,
		       _("Target file already exists"));
	  return FALSE;
	}
    }
  
  if (flags & G_FILE_COPY_BACKUP && destination_exist)
    {
      backup_name = g_strconcat (local_destination->filename, "~", NULL);
      if (rename (local_destination->filename, backup_name) == -1)
	{
      	  g_set_error (error,
		       G_IO_ERROR,
		       G_IO_ERROR_CANT_CREATE_BACKUP,
		       _("Backup file creation failed"));
	  g_free (backup_name);
	  return FALSE;
	}
      g_free (backup_name);
      destination_exist = FALSE; /* It did, but no more */
    }

  if (source_is_dir && destination_exist && (flags & G_FILE_COPY_OVERWRITE))
    {
      /* Source is a dir, destination exists (and is not a dir, because that would have failed
	 earlier), and we're overwriting. Manually remove the target so we can do the rename. */
      res = unlink (local_destination->filename);
      if (res == -1)
	{
	  g_set_error (error, G_IO_ERROR,
		       g_io_error_from_errno (errno),
		       _("Error removing target file: %s"),
		       g_strerror (errno));
	  return FALSE;
	}
    }
  
  if (rename (local_source->filename, local_destination->filename) == -1)
    {
      int errsv = errno;
      if (errsv == EXDEV)
	goto fallback;

      if (errsv == EINVAL)
	/* This must be an invalid filename, on e.g. FAT, or
	   we're trying to move the file into itself...
	   We return invalid filename for both... */
	g_set_error (error, G_IO_ERROR,
		     G_IO_ERROR_INVALID_FILENAME,
		     _("Invalid filename"));
      else
	g_set_error (error, G_IO_ERROR,
		     g_io_error_from_errno (errsv),
		     _("Error moving file: %s"),
		     g_strerror (errsv));
      return FALSE;

    }
  return TRUE;

 fallback:

  if (!g_file_copy (source, destination, G_FILE_COPY_OVERWRITE | G_FILE_COPY_ALL_METADATA, cancellable,
		    progress_callback, progress_callback_data,
		    error))
    return FALSE;
  
  return g_file_delete (source, cancellable, error);
}


static GDirectoryMonitor*
g_local_file_monitor_dir (GFile             *file,
			  GFileMonitorFlags  flags,
			  GCancellable      *cancellable)
{
  GLocalFile* local_file = G_LOCAL_FILE(file);
  return _g_local_directory_monitor_new (local_file->filename, flags);
}

static GFileMonitor*
g_local_file_monitor_file (GFile             *file,
			   GFileMonitorFlags  flags,
			   GCancellable      *cancellable)
{
  GLocalFile* local_file = G_LOCAL_FILE(file);
  return _g_local_file_monitor_new (local_file->filename, flags);
}

static void
g_local_file_file_iface_init (GFileIface *iface)
{
  iface->dup = g_local_file_dup;
  iface->hash = g_local_file_hash;
  iface->equal = g_local_file_equal;
  iface->is_native = g_local_file_is_native;
  iface->has_uri_scheme = g_local_file_has_uri_scheme;
  iface->get_uri_scheme = g_local_file_get_uri_scheme;
  iface->get_basename = g_local_file_get_basename;
  iface->get_path = g_local_file_get_path;
  iface->get_uri = g_local_file_get_uri;
  iface->get_parse_name = g_local_file_get_parse_name;
  iface->get_parent = g_local_file_get_parent;
  iface->contains_file = g_local_file_contains_file;
  iface->get_relative_path = g_local_file_get_relative_path;
  iface->resolve_relative_path = g_local_file_resolve_relative_path;
  iface->get_child_for_display_name = g_local_file_get_child_for_display_name;
  iface->set_display_name = g_local_file_set_display_name;
  iface->enumerate_children = g_local_file_enumerate_children;
  iface->query_info = g_local_file_query_info;
  iface->query_filesystem_info = g_local_file_query_filesystem_info;
  iface->find_enclosing_volume = g_local_file_find_enclosing_volume;
  iface->query_settable_attributes = g_local_file_query_settable_attributes;
  iface->query_writable_namespaces = g_local_file_query_writable_namespaces;
  iface->set_attribute = g_local_file_set_attribute;
  iface->set_attributes_from_info = g_local_file_set_attributes_from_info;
  iface->read = g_local_file_read;
  iface->append_to = g_local_file_append_to;
  iface->create = g_local_file_create;
  iface->replace = g_local_file_replace;
  iface->delete_file = g_local_file_delete;
  iface->trash = g_local_file_trash;
  iface->make_directory = g_local_file_make_directory;
  iface->make_symbolic_link = g_local_file_make_symbolic_link;
  iface->copy = g_local_file_copy;
  iface->move = g_local_file_move;
  iface->monitor_dir = g_local_file_monitor_dir;
  iface->monitor_file = g_local_file_monitor_file;
}
