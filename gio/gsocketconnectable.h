/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_SOCKET_CONNECTABLE_H__
#define __G_SOCKET_CONNECTABLE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define G_TYPE_SOCKET_CONNECTABLE            (g_socket_connectable_get_type ())
#define G_SOCKET_CONNECTABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SOCKET_CONNECTABLE, GSocketConnectable))
#define G_IS_SOCKET_CONNECTABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), G_TYPE_SOCKET_CONNECTABLE))
#define G_SOCKET_CONNECTABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), G_TYPE_SOCKET_CONNECTABLE, GSocketConnectableIface))

/**
 * GSocketConnectable:
 *
 * Interface for objects that contain or generate #GSocketAddress<!-- -->es.
 **/
typedef struct _GSocketConnectableIface   GSocketConnectableIface;

/**
 * GSocketConnectableIter:
 *
 * An opaque type used to iterate a #GSocketConnectable.
 **/
typedef struct _GSocketConnectableIter    GSocketConnectableIter;

/**
 * GSocketConnectableIface:
 * @g_iface: The parent interface.
 * @tell: Tells the current location within a stream.
 * @can_seek: Checks if seeking is supported by the stream.
 * @seek: Seeks to a location within a stream.
 * @can_truncate: Chekcs if truncation is suppored by the stream.
 * @truncate_fn: Truncates a stream.
 *
 * Provides an interface for implementing connectable functionality on I/O Streams.
 **/
struct _GSocketConnectableIface
{
  GTypeInterface g_iface;

  /* Virtual Table */

  GSocketConnectableIter * (* get_iter)        (GSocketConnectable      *connectable);
  void                     (* free_iter)       (GSocketConnectable      *connectable,
						GSocketConnectableIter  *iter);

  GSocketAddress *         (* get_next)        (GSocketConnectable      *connectable,
						GSocketConnectableIter  *iter,
						GCancellable            *cancellable,
						GError                 **error);

  void                     (* get_next_async)  (GSocketConnectable      *connectable,
						GSocketConnectableIter  *iter,
						GCancellable            *cancellable,
						GAsyncReadyCallback      callback,
						gpointer                 user_data);
  GSocketAddress *         (* get_next_finish) (GSocketConnectable      *connectable,
						GAsyncResult            *result,
						GError                 **error);
};

GType                    g_socket_connectable_get_type        (void) G_GNUC_CONST;

GSocketConnectableIter * g_socket_connectable_get_iter        (GSocketConnectable      *connectable);
void                     g_socket_connectable_free_iter       (GSocketConnectable      *connectable,
							       GSocketConnectableIter  *iter);

GSocketAddress *         g_socket_connectable_get_next        (GSocketConnectable      *connectable,
							       GSocketConnectableIter  *iter,
							       GCancellable            *cancellable,
							       GError                 **error);

void                     g_socket_connectable_get_next_async  (GSocketConnectable      *connectable,
							       GSocketConnectableIter  *iter,
							       GCancellable            *cancellable,
							       GAsyncReadyCallback      callback,
							       gpointer                 user_data);
GSocketAddress *         g_socket_connectable_get_next_finish (GSocketConnectable      *connectable,
							       GAsyncResult            *result,
							       GError                 **error);

G_END_DECLS


#endif /* __G_SOCKET_CONNECTABLE_H__ */
