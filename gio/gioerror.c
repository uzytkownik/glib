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

#include "config.h"
#include <errno.h>
#include "gioerror.h"

#include "gioalias.h"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif

/**
 * SECTION:gioerror
 * @short_description: Error helper functions
 * @include: gio/gio.h
 *
 * Contains helper functions for reporting errors to the user.
 **/

/**
 * g_io_error_quark:
 * 
 * Gets the GIO Error Quark.
 *
 * Return value: a #GQuark.
 **/
GQuark
g_io_error_quark (void)
{
  return g_quark_from_static_string ("g-io-error-quark");
}

/**
 * g_io_error_from_errno:
 * @err_no: Error number as defined in errno.h.
 *
 * Converts errno.h error codes into GIO error codes.
 *
 * Returns: #GIOErrorEnum value for the given errno.h error number.
 **/
GIOErrorEnum
g_io_error_from_errno (gint err_no)
{
  switch (err_no)
    {
#ifdef EEXIST
    case EEXIST:
      return G_IO_ERROR_EXISTS;
      break;
#endif

#ifdef EISDIR
    case EISDIR:
      return G_IO_ERROR_IS_DIRECTORY;
      break;
#endif

#if defined(EACCES) || defined(WSAEACCES)
#ifdef EACCES
    case EACCES:
#endif
#ifdef WSAEACCES
    case WSAEACCES:
#endif
      return G_IO_ERROR_PERMISSION_DENIED;
      break;
#endif

#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
      return G_IO_ERROR_FILENAME_TOO_LONG;
      break;
#endif

#ifdef ENOENT
    case ENOENT:
      return G_IO_ERROR_NOT_FOUND;
      break;
#endif

#ifdef ENOTDIR
    case ENOTDIR:
      return G_IO_ERROR_NOT_DIRECTORY;
      break;
#endif

#ifdef EROFS
    case EROFS:
      return G_IO_ERROR_READ_ONLY;
      break;
#endif

#if defined(ELOOP) || defined(WSAELOOP)
#ifdef ELOOP
    case ELOOP:
#endif
#ifdef WSAELOOP
    case WSAELOOP:
#endif
      return G_IO_ERROR_TOO_MANY_LINKS;
      break;
#endif

#ifdef ENOSPC
    case ENOSPC:
      return G_IO_ERROR_NO_SPACE;
      break;
#endif

#ifdef ENOMEM
    case ENOMEM:
      return G_IO_ERROR_NO_SPACE;
      break;
#endif

#if defined(EINVAL) || defined(WSAEINVAL)
#ifdef EINVAL
    case EINVAL:
#endif
#ifdef WSAEINVAL
    case WSAEINVAL:
#endif
      return G_IO_ERROR_INVALID_ARGUMENT;
      break;
#endif

#ifdef EPERM
    case EPERM:
      return G_IO_ERROR_PERMISSION_DENIED;
      break;
#endif

#if defined(ECANCELED) || defined(WSAECANCELLED)
#ifdef ECANCELED
    case ECANCELED:
#endif
#ifdef WSAECANCELLED
    case WSAECANCELLED:
#endif
      return G_IO_ERROR_CANCELLED;
      break;
#endif

#if defined(ENOTEMPTY) || defined(WSAENOTEMPTY)
#ifdef ENOTEMPTY
    case ENOTEMPTY:
#endif
#ifdef WSAENOTEMPTY
    case WSAENOTEMPTY:
#endif
      return G_IO_ERROR_NOT_EMPTY;
      break;
#endif

#if defined(ENOTEMPTY) || defined(WSAENOTEMPTY)
#ifdef ENOTSUP
    case ENOTSUP:
#endif
#ifdef WSAEOPNOTSUPP
    case WSAEOPNOTSUPP:
#endif
      return G_IO_ERROR_NOT_SUPPORTED;
      break;
#endif

#if defined(ETIMEDOUT) || defined(WSAETIMEDOUT)
#ifdef ETIMEDOUT
    case ETIMEDOUT:
#endif
#ifdef WSAETIMEDOUT
    case WSAETIMEDOUT:
#endif
      return G_IO_ERROR_TIMED_OUT;
      break;
#endif

#ifdef EBUSY
    case EBUSY:
      return G_IO_ERROR_BUSY;
      break;
#endif

#if defined(EWOULDBLOCK) || defined(WSAEWOULDBLOCK)
#ifdef EWOULDBLOCK
    case EWOULDBLOCK:
#endif
#ifdef WSAEWOULDBLOCK
    case WSAEWOULDBLOCK:
#endif
      return G_IO_ERROR_WOULD_BLOCK;
      break;
#endif
      
    default:
      return G_IO_ERROR_FAILED;
      break;
    }
}

#define __G_IO_ERROR_C__
#include "gioaliasdef.c"
