/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Maciej Piechotka
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
 * Author: Maciej Piechotka <uzytkownik2@gmail.com>
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef G_DATAGRAM_SOCKET_H
#define G_DATAGRAM_SOCKET_H

#include <glib-object.h>

#include "giotypes.h"
#include "gsocket.h"

G_BEGIN_DECLS

#define G_TYPE_DATAGRAM_SOCKET                                          \
   (g_datagram_socket_get_type())
#define G_DATAGRAM_SOCKET(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_DATAGRAM_SOCKET,                 \
                                GDatagramSocket))
#define G_DATAGRAM_SOCKET_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_DATAGRAM_SOCKET,                    \
                             GDatagramSocketClass))
#define IS_G_DATAGRAM_SOCKET(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_DATAGRAM_SOCKET))
#define IS_G_DATAGRAM_SOCKET_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_DATAGRAM_SOCKET))
#define G_DATAGRAM_SOCKET_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_DATAGRAM_SOCKET,                  \
                               GDatagramSocketClass))

typedef struct _GDatagramSocketClass GDatagramSocketClass;

struct _GDatagramSocket
{
  GSocket parent;
};

struct _GDatagramSocketClass
{
  GSocketClass parent_class;
  gboolean (*bind)           (GDatagramSocket      *self,
			      GSocketAddress       *local,
			      GCancellable         *cancellable,
			      GError              **error);
  gssize   (*send)           (GDatagramSocket      *self,
			      GSocketAddress       *destination,
			      const void           *buf,
			      gsize                 size,
			      GCancellable         *cancellable,
			      GError              **error);
  gssize   (*receive)        (GDatagramSocket      *self,
			      GSocketAddress      **source,
			      void                 *buf,
			      gsize                 size,
			      GCancellable         *cancellable,
			      GError              **error);
  /* Optional */
  void     (*bind_async)     (GDatagramSocket      *self,
			      GSocketAddress       *local,
			      int                   io_priority,
			      GCancellable         *cancellable,
			      GAsyncReadyCallback   callback,
			      void                 *user_data);
  gboolean (*bind_finish)    (GDatagramSocket      *self,
			      GAsyncResult         *res,
			      GError              **error);
  void     (*send_async)     (GDatagramSocket      *self,
			      GSocketAddress       *destination,
			      const void           *buf,
			      gsize                 size,
			      int                   io_priority,
			      GCancellable         *cancellable,
			      GAsyncReadyCallback   callback,
			      void                 *user_data);
  gssize   (*send_finish)    (GDatagramSocket      *self,
			      GAsyncResult         *res,
			      GError              **error);
  void     (*receive_async)  (GDatagramSocket      *self,
			      GSocketAddress      **source,
			      void                 *buf,
			      gsize                 size,
			      int                   io_priority,
			      GCancellable         *cancellable,
			      GAsyncReadyCallback   callback,
			      void                 *user_data);
  gssize   (*receive_finish) (GDatagramSocket      *self,
			      GAsyncResult         *res,
			      GError              **error);
};

GType    g_datagram_socket_get_type       (void) G_GNUC_CONST;
gboolean g_datagram_socket_bind           (GDatagramSocket      *self,
					   GSocketAddress       *local,
					   GCancellable         *cancellable,
					   GError              **error);
void     g_datagram_socket_bind_async     (GDatagramSocket      *self,
					   GSocketAddress       *local,
					   int                   io_priority,
					   GCancellable         *cancellable,
					   GAsyncReadyCallback   callback,
					   void                 *user_data);
gboolean g_datagram_socket_bind_finish    (GDatagramSocket      *self,
					   GAsyncResult         *res,
					   GError              **error);
gssize   g_datagram_socket_send           (GDatagramSocket      *self,
					   GSocketAddress       *destination,
					   const void           *buf,
					   gsize                 size,
					   GCancellable         *cancellable,
					   GError              **error);
void     g_datagram_socket_send_async     (GDatagramSocket      *self,
					   GSocketAddress       *destination,
					   const void           *buf,
					   gsize                 size,
					   int                   io_priority,
					   GCancellable         *cancellable,
					   GAsyncReadyCallback   callback,
					   void                 *user_data);
gssize   g_datagram_socket_send_finish    (GDatagramSocket      *self,
					   GAsyncResult         *res,
					   GError              **error);
gssize   g_datagram_socket_receive        (GDatagramSocket      *self,
					   GSocketAddress      **source,
					   void                 *buf,
					   gsize                 size,
					   GCancellable         *cancellable,
					   GError              **error);
void     g_datagram_socket_receive_async  (GDatagramSocket      *self,
					   GSocketAddress      **source,
					   void                 *buf,
					   gsize                 size,
					   int                   io_priority,
					   GCancellable         *cancellable,
					   GAsyncReadyCallback   callback,
					   void                 *user_data);
gssize   g_datagram_socket_receive_finish (GDatagramSocket      *self,
					   GAsyncResult         *res,
					   GError              **error);

G_END_DECLS

#endif
