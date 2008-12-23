/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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

#ifndef G_PACKET_CONNECTION_H
#define G_PACKET_CONNECTION_H

#include "gconnection.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define G_TYPE_PACKET_CONNECTION                                        \
   (g_packet_connection_get_type())
#define G_PACKET_CONNECTION(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_PACKET_CONNECTION,               \
                                GPacketConnection))
#define G_PACKET_CONNECTION_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_PACKET_CONNECTION,                  \
                             GPacketConnectionClass))
#define IS_G_PACKET_CONNECTION(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_PACKET_CONNECTION))
#define IS_G_PACKET_CONNECTION_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_PACKET_CONNECTION))
#define G_PACKET_CONNECTION_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_PACKET_CONNECTION,                \
                               GPacketConnectionClass))

typedef struct _GPacketConnection      GPacketConnection;
typedef struct _GPacketConnectionClass GPacketConnectionClass;

struct _GPacketConnection
{
  GConnection parent;
};

struct _GPacketConnectionClass
{
  GConnectionClass   parent_class;
  gssize           (*send)           (GPacketConnection    *self,
				      const void           *buf,
			              gsize                 len);
  void             (*send_async)     (GPacketConnection    *self,
				      const void           *buf,
				      gsize                 len,
				      int                   io_priority,
				      GCancellable         *cancellable,
				      GAsyncReadyCallback   callback,
				      gpointer              user_data);
  gssize           (*send_finish)    (GPacketConnection    *self,
				      GAsyncResult         *res,
				      GError              **error);
  gssize           (*receive)        (GPacketConnection    *self,
				      void                 *buf,
				      gsize                 len);
  void             (*receive_async)  (GPacketConnection    *self,
				      void                 *buf,
				      gsize                 len,
				      int                   io_priority,
				      GCancellable         *cancellable,
				      GAsyncReadyCallback   callback,
				      gpointer              user_data);
  gssize           (*receive_finish) (GPacketConnection    *self,
				      GAsyncResult         *res,
				      GError              **error);
};

GType  g_packet_connection_get_type       (void) G_GNUC_CONST;
gssize g_packet_connection_send           (GPacketConnection *self,
					   const void        *buf,
					   gsize              len);
void   g_packet_connection_send_async     (GPacketConnection    *self,
					   const void           *buf,
					   gsize                 len,
					   int                   io_priority,
					   GCancellable         *cancellable,
					   GAsyncReadyCallback   callback,
					   gpointer              user_data);
gssize g_packet_connection_send_finish    (GPacketConnection    *self,
					   GAsyncResult         *res,
					   GError              **error);
gssize g_packet_connection_receive        (GPacketConnection *self,
					   void              *buf,
					   gsize              len);
void   g_packet_connection_receive_async  (GPacketConnection    *self,
					   void                 *buf,
					   gsize                 len,
					   int                   io_priority,
					   GCancellable         *cancellable,
					   GAsyncReadyCallback   callback,
					   gpointer              user_data);
gssize g_packet_connection_receive_finish (GPacketConnection    *self,
					   GAsyncResult         *res,
					   GError              **error);

G_END_DECLS

#endif
