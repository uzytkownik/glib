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
#include "gseekable.h"
#include "glibintl.h"

#include "gioalias.h"

/**
 * SECTION:gseekable
 * @short_description: Stream seeking interface
 * @see_also: #GInputStream, #GOutputStream
 * 
 * #GSeekable is implemented by streams (implementations of 
 * #GInputStream or #GOutputStream) that support seeking.
 * 
 **/


static void g_seekable_base_init (gpointer g_class);


GType
g_seekable_get_type (void)
{
  static GType seekable_type = 0;

  if (!seekable_type)
    {
      static const GTypeInfo seekable_info =
      {
        sizeof (GSeekableIface), /* class_size */
	g_seekable_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      seekable_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("GSeekable"),
				&seekable_info, 0);

      g_type_interface_add_prerequisite (seekable_type, G_TYPE_OBJECT);
    }

  return seekable_type;
}

static void
g_seekable_base_init (gpointer g_class)
{
}

/**
 * g_seekable_tell:
 * @seekable: a #GSeekable.
 * 
 * Tells the current position within the stream.
 * 
 * Returns: the offset from the beginning of the buffer.
 **/
goffset
g_seekable_tell (GSeekable *seekable)
{
  GSeekableIface *iface;

  g_return_val_if_fail (G_IS_SEEKABLE (seekable), 0);

  iface = G_SEEKABLE_GET_IFACE (seekable);

  return (* iface->tell) (seekable);
}

/**
 * g_seekable_can_seek:
 * @seekable: a #GSeekable.
 * 
 * Tests if the stream supports the #GSeekableIface.
 * 
 * Returns: %TRUE if @seekable can be seeked. %FALSE otherwise.
 **/
gboolean
g_seekable_can_seek (GSeekable *seekable)
{
  GSeekableIface *iface;
  
  g_return_val_if_fail (G_IS_SEEKABLE (seekable), FALSE);

  iface = G_SEEKABLE_GET_IFACE (seekable);

  return (* iface->can_seek) (seekable);
}

/**
 * g_seekable_seek:
 * @seekable: a #GSeekable.
 * @offset: a #goffset.
 * @type: a #GSeekType.
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Seeks in the stream by the given @offset, modified by @type.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. 
 * 
 * Returns: %TRUE if successful. If an error
 *     has occured, this function will return %FALSE and set @error
 *     appropriately if present.
 **/
gboolean
g_seekable_seek (GSeekable     *seekable,
		 goffset        offset,
		 GSeekType      type,
		 GCancellable  *cancellable,
		 GError       **error)
{
  GSeekableIface *iface;
  
  g_return_val_if_fail (G_IS_SEEKABLE (seekable), FALSE);

  iface = G_SEEKABLE_GET_IFACE (seekable);

  return (* iface->seek) (seekable, offset, type, cancellable, error);
}

/**
 * g_seekable_can_truncate:
 * @seekable: a #GSeekable.
 * 
 * Tests if the stream can be truncated.
 * 
 * Returns: %TRUE if the stream can be truncated, %FALSE otherwise.
 **/
gboolean
g_seekable_can_truncate (GSeekable *seekable)
{
  GSeekableIface *iface;
  
  g_return_val_if_fail (G_IS_SEEKABLE (seekable), FALSE);

  iface = G_SEEKABLE_GET_IFACE (seekable);

  return (* iface->can_truncate) (seekable);
}

/**
 * g_seekable_truncate:
 * @seekable: a #GSeekable.
 * @offset: a #goffset.
 * @cancellable: optional #GCancellable object, %NULL to ignore. 
 * @error: a #GError location to store the error occuring, or %NULL to 
 * ignore.
 * 
 * Truncates a stream with a given #offset. 
 * 
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the cancellable object from another thread. If the operation
 * was cancelled, the error %G_IO_ERROR_CANCELLED will be returned. If an
 * operation was partially finished when the operation was cancelled the
 * partial result will be returned, without an error.
 * 
 * Returns: %TRUE if successful. If an error
 *     has occured, this function will return %FALSE and set @error
 *     appropriately if present. 
 **/
gboolean
g_seekable_truncate (GSeekable     *seekable,
		     goffset        offset,
		     GCancellable  *cancellable,
		     GError       **error)
{
  GSeekableIface *iface;
  
  g_return_val_if_fail (G_IS_SEEKABLE (seekable), FALSE);

  iface = G_SEEKABLE_GET_IFACE (seekable);

  return (* iface->truncate_fn) (seekable, offset, cancellable, error);
}

#define __G_SEEKABLE_C__
#include "gioaliasdef.c"
