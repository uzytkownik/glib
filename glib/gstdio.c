/* gstdio.c - wrappers for C library functions
 *
 * Copyright 2004 Tor Lillqvist
 *
 * GLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#include <wchar.h>
#include <io.h>
#endif

#include "galias.h"
#include "glib.h"
#include "gstdio.h"

#if !defined (G_OS_UNIX) && !defined (G_OS_WIN32)
#error Please port this to your operating system
#endif


/**
 * g_open:
 * @filename: a pathname in the GLib file name encoding
 * @flags: as in open()
 * @mode: as in open()
 *
 * A wrapper for the POSIX open() function. The open() function is used 
 * to convert a pathname into a file descriptor.
 *
 * See the C library manual for more details about open().
 *
 * Returns: a new file descriptor, or -1 if an error occurred
 * 
 * Since: 2.6
 */
int
g_open (const gchar *filename,
	int          flags,
	int          mode)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      int retval = _wopen (wfilename, flags, mode);
      int save_errno = errno;
      
      g_free (wfilename);

      errno = save_errno;
      return retval;
    }
  else
    {    
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      int retval = open (cp_filename, flags, mode);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return open (filename, flags, mode);
#endif
}

/**
 * g_rename:
 * @oldfilename: a pathname in the GLib file name encoding
 * @newfilename: a pathname in the GLib file name encoding
 *
 * A wrapper for the POSIX rename() function. The rename() function 
 * renames a file, moving it between directories if required.
 * 
 * See the C library manual for more details about rename().
 *
 * Returns: 0 if the renaming succeeded, -1 if an error occurred
 * 
 * Since: 2.6
 */
int
g_rename (const gchar *oldfilename,
	  const gchar *newfilename)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *woldfilename = g_utf8_to_utf16 (oldfilename, -1, NULL, NULL, NULL);
      wchar_t *wnewfilename = g_utf8_to_utf16 (newfilename, -1, NULL, NULL, NULL);
      int retval = _wrename (woldfilename, wnewfilename);
      int save_errno = errno;
      
      g_free (woldfilename);
      g_free (wnewfilename);
      
      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_oldfilename = g_locale_from_utf8 (oldfilename, -1, NULL, NULL, NULL);
      gchar *cp_newfilename = g_locale_from_utf8 (newfilename, -1, NULL, NULL, NULL);
      int retval = rename (cp_oldfilename, cp_newfilename);
      int save_errno = errno;

      g_free (cp_oldfilename);
      g_free (cp_newfilename);

      errno = save_errno;
      return retval;
    }
#else
  return rename (oldfilename, newfilename);
#endif
}

/**
 * g_mkdir: 
 * @filename: a pathname in the GLib file name encoding
 * @mode: permissions to use for the newly created directory
 *
 * A wrapper for the POSIX mkdir() function. The mkdir() function 
 * attempts to create a directory with the given name and permissions.
 * 
 * See the C library manual for more details about mkdir().
 *
 * Returns: 0 if the directory was successfully created, -1 if an error 
 *    occurred
 * 
 * Since: 2.6
 */
int
g_mkdir (const gchar *filename,
	 int          mode)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      int retval = _wmkdir (wfilename);
      int save_errno = errno;

      g_free (wfilename);
      
      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      int retval = mkdir (cp_filename);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return mkdir (filename, mode);
#endif
}

/**
 * g_stat: 
 * @filename: a pathname in the GLib file name encoding
 * @buf: a pointer to a <structname>stat</structname> struct, which
 *    will be filled with the file information
 *
 * A wrapper for the POSIX stat() function. The stat() function 
 * returns information about a file.
 * 
 * See the C library manual for more details about stat().
 *
 * Returns: 0 if the directory was successfully created, -1 if an error 
 *    occurred
 * 
 * Since: 2.6
 */
int
g_stat (const gchar *filename,
	struct stat *buf)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      int retval = _wstat (wfilename, (struct _stat *) buf);
      int save_errno = errno;

      g_free (wfilename);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      int retval = stat (cp_filename, buf);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return stat (filename, buf);
#endif
}

/**
 * g_unlink:
 * @filename: a pathname in the GLib file name encoding
 *
 * A wrapper for the POSIX unlink() function. The unlink() function 
 * deletes a name from the filesystem. If this was the last link to the 
 * file and no processes have it opened, the diskspace occupied by the
 * file is freed.
 * 
 * See the C library manual for more details about unlink().
 *
 * Returns: 0 if the directory was successfully created, -1 if an error 
 *    occurred
 * 
 * Since: 2.6
 */
int
g_unlink (const gchar *filename)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      int retval = _wunlink (wfilename);
      int save_errno = errno;

      g_free (wfilename);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      int retval = unlink (cp_filename);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return unlink (filename);
#endif
}

/**
 * g_remove:
 * @filename: a pathname in the GLib file name encoding
 *
 * A wrapper for the POSIX remove() function. The remove() function 
 * deletes a name from the filesystem. It calls unlink() for files
 * and rmdir() for directories.
 * 
 * See the C library manual for more details about remove().
 *
 * Returns: 0 if the directory was successfully created, -1 if an error 
 *    occurred
 * 
 * Since: 2.6
 */
int
g_remove (const gchar *filename)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      int retval = _wremove (wfilename);
      int save_errno = errno;

      g_free (wfilename);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      int retval = remove (cp_filename);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return remove (filename);
#endif
}

/**
 * g_fopen:
 * @filename: a pathname in the GLib file name encoding
 * @mode: a string describing the mode in which the file should be 
 *   opened
 *
 * A wrapper for the POSIX fopen() function. The fopen() function opens
 * a file and associates a new stream with it. 
 * 
 * See the C library manual for more details about fopen().
 *
 * Returns: A <typename>FILE</typename> pointer if the file was successfully
 *    opened, or %NULL if an error occurred
 * 
 * Since: 2.6
 */
FILE *
g_fopen (const gchar *filename,
	 const gchar *mode)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      wchar_t *wmode = g_utf8_to_utf16 (mode, -1, NULL, NULL, NULL);
      FILE *retval = _wfopen (wfilename, wmode);
      int save_errno = errno;

      g_free (wfilename);
      g_free (wmode);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      FILE *retval = fopen (cp_filename, mode);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return fopen (filename, mode);
#endif
}

/**
 * g_freopen:
 * @filename: a pathname in the GLib file name encoding
 * @mode: a string describing the mode in which the file should be 
 *   opened
 * @stream: an existing stream which will be reused, or %NULL
 *
 * A wrapper for the POSIX freopen() function. The freopen() function
 * opens a file and associates it with an existing stream.
 * 
 * See the C library manual for more details about freopen().
 *
 * Returns: A <typename>FILE</typename> pointer if the file was successfully
 *    opened, or %NULL if an error occurred.
 * 
 * Since: 2.6
 */
FILE *
g_freopen (const gchar *filename,
	   const gchar *mode,
	   FILE        *stream)
{
#ifdef G_OS_WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      wchar_t *wmode = g_utf8_to_utf16 (mode, -1, NULL, NULL, NULL);
      FILE *retval = _wfreopen (wfilename, wmode, stream);
      int save_errno = errno;

      g_free (wfilename);
      g_free (wmode);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      FILE *retval = freopen (cp_filename, mode, stream);
      int save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return freopen (filename, mode, stream);
#endif
}
