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

#include "gpacketconnection.h"

G_DEFINE_ABSTRACT_TYPE (GPacketConnection, g_packet_connection, G_TYPE_CONNECTION);

static void
g_packet_connection_class_init (GPacketConnectionClass *klass)
{
  
}

static void
g_packet_connection_init (GPacketConnection *self)
{
  
}

gssize
g_packet_connection_send (GPacketConnection *self,
			  const void        *buf,
			  gsize              len)
{
  GPacketConnectionClass *klass;

  g_return_val_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self), -1);
  g_return_val_if_fail (klass->send, -1);

  return klass->send (self, buf, len);
}

void
g_packet_connection_send_async (GPacketConnection   *self,
				const void          *buf,
				gsize                len,
				int                  io_priority,
				GCancellable        *cancellable,
				GAsyncReadyCallback  callback,
				gpointer             user_data)
{
  GPacketConnectionClass *klass;

  g_return_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self));
  g_return_if_fail (klass->send_async);

  klass->send_async (self, buf, len, io_priority, cancellable,
		     callback, user_data);
}

gssize
g_packet_connection_send_finish (GPacketConnection  *self,
				 GAsyncResult       *res,
				 GError            **error)
{
  GPacketConnectionClass *klass;

  g_return_val_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self), -1);
  g_return_val_if_fail (klass->send_finish, -1);

  return klass->send_finish (self, res, error);
}

gssize
g_packet_connection_receive  (GPacketConnection *self,
			      void              *buf,
			      gsize              len)
{
  GPacketConnectionClass *klass;

  g_return_val_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self), -1);
  g_return_val_if_fail (klass->receive, -1);

  return klass->receive (self, buf, len);
}

void
g_packet_connection_receive_async (GPacketConnection   *self,
				   void                *buf,
				   gsize                len,
				   int                  io_priority,
				   GCancellable        *cancellable,
				   GAsyncReadyCallback  callback,
				   gpointer             user_data)
{
  GPacketConnectionClass *klass;

  g_return_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self));
  g_return_if_fail (klass->receive_async);

  klass->receive_async (self, buf, len, io_priority, cancellable,
			callback, user_data);
}

gssize
g_packet_connection_receive_finish (GPacketConnection  *self,
				    GAsyncResult       *res,
				    GError            **error)
{
  GPacketConnectionClass *klass;

  g_return_val_if_fail (klass = G_PACKET_CONNECTION_GET_CLASS (self), -1);
  g_return_val_if_fail (klass->receive_finish, -1);

  return klass->receive_finish (self, res, error);
}
