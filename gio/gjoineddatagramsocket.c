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

#include "gjoineddatagramsocket.h"

struct _GJoinedDatagramSocketPrivate
{
  GSList *sockets;
};

G_DEFINE_TYPE (GJoinedDatagramSocket, g_joined_datagram_socket, G_TYPE_DATAGRAM_SOCKET);

static void
g_joined_datagram_socket_class_init (GJoinedDatagramSocketClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;

  g_type_class_add_private (gobject_class, sizeof (struct _GJoinedDatagramSocketPrivate));
}

static void
g_joined_datagram_socket_init (GJoinedDatagramSocket *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
					    G_TYPE_JOINED_DATAGRAM_SOCKET,
					    GJoinedDatagramSocketPrivate);
}

static gboolean
g_joined_datagram_socket_bind  (GDatagramSocket      *self,
				GSocketConnectable   *local,
				GCancellable         *cancellable,
				GError              **error)
{
  GJoinedDatagramSocket *socket;
  GSocketAddressEnumerator *enumerator;
  GError *err;
  GSocketAddress *sockaddr;
  gboolean loop_exit_normal;
  
  g_return_val_if_fail ((socket = G_JOINED_DATAGRAM_SOCKET (self)), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;
  
  err = NULL;
  enumerator = g_socket_connectable_get_enumerator (local);
  loop_exit_normal = TRUE;
  while ((sockaddr = g_socket_address_enumerator_get_next (enumerator,
							   cancellable,
							   &err)))
    {
      GDatagramSocket *rsock;

      rsock = g_socket_create_datagram_socket (sockaddr);
      if (!g_datagram_socket_bind (rsock, sockaddr, cancellable, &err))
	{
	  g_object_unref (rsock);
	  loop_exit_normal = FALSE;
	  break;
	}

      g_slist_prepend (socket->priv->sockets, rsock);
    }

  if (err || loop_exit_normal)
    {
      while (socket->priv->sockets)
	{
	  g_object_unref (socket->priv->sockets->data);
	  socket->priv->sockets = g_slist_delete_link (socket->priv->sockets);
	}

      if (err && error)
	*error = err;
      return FALSE;
    }

  g_slist_reverse (socket->priv->sockets);

  return TRUE;
}

static void
g_joined_datagram_socket_bind_async (GDatagramSocket      *self,
				     GSocketConnectable   *local,
				     int                   io_priority,
				     GCancellable         *cancellable,
				     GAsyncReadyCallback   callback,
				     void                 *user_data)
{
  
}

static gboolean
g_joined_datagram_socket_bind_finish (GDatagramSocket      *self,
				      GAsyncResult         *res,
				      GError              **error);
static gssize
g_joined_datagram_socket_send (GDatagramSocket     *self,
			       GSocketConnectable  *destination,
			       const void          *buf,
			       gsize                size,
			       GCancellable        *cancellable,
			       GError             **error);
static void
g_joined_datagram_socket_send_async (GDatagramSocket      *self,
				     GSocketConnectable   *destination,
				     const void           *buf,
				     gsize                 size,
				     int                   io_priority,
				     GCancellable         *cancellable,
				     GAsyncReadyCallback   callback,
				     void                 *user_data);
static gssize
g_joined_datagram_socket_send_finish (GDatagramSocket      *self,
				      GAsyncResult         *res,
				      GError              **error);
static gssize
g_joined_datagram_socket_receive (GDatagramSocket      *self,
				  GSocketAddress      **source,
				  void                 *buf,
				  gsize                 size,
				  GCancellable         *cancellable,
				  GError              **error);
static void
g_joined_datagram_socket_receive_async (GDatagramSocket      *self,
					GSocketAddress      **source,
					void                 *buf,
					gsize                 size,
					int                   io_priority,
					GCancellable         *cancellable,
					GAsyncReadyCallback   callback,
					void                 *user_data);
static gssize
g_joined_datagram_socket_receive_finish (GDatagramSocket      *self,
					 GAsyncResult         *res,
					 GError              **error);
static gboolean
g_joined_datagram_socket_support_address (GSocket        *socket,
					  GSocketAddress *address);
static gboolean
g_joined_datagram_socket_has_next (GDatagramSocket *self);
