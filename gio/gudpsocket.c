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

#include "gudpsocket.h"

#ifdef G_OS_WIN32
#include "winsock2.h"

#define SHUT_RD (SD_REVEIVE)
#define SHUT_WR (SD_SEND)
#define SHUT_RDWR (SD_BOTH)
typedef int sockaddr_t;
#define get_errno (WSAGetLastError ())
#else /* G_OS_WIN32 */
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

typedef int SOCKET;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define get_errno (errno)
#endif /* G_OS_WIN32 */

#include "gcancellable.h"
#include "gioerror.h"

struct _GUDPSocketPrivate
{
  SOCKET socket;
  volatile GInetSocketAddress *local;
};

static void            g_udp_socket_finalize          (GObject              *object);
static void            g_udp_socket_dispose           (GObject              *object);
static GSocketAddress *g_udp_socket_get_local_address (GSocket              *self);
static gboolean        g_udp_socket_bind              (GDatagramSocket      *self,
						       GSocketAddress       *local,
						       GCancellable         *cancellable,
						       GError              **error);
/*
static void            g_udp_socket_bind_async        (GDatagramSocket      *self,
						       GSocketAddress       *local,
						       int                   io_priority,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       void                 *user_data);
static gboolean        g_udp_socket_bind_finish       (GDatagramSocket      *self,
						       GAsyncResult         *res,
						       GError              **error);
*/
static gssize          g_udp_socket_send              (GDatagramSocket      *self,
						       GSocketAddress       *destination,
						       const void           *buf,
						       gsize                 size,
						       GCancellable         *cancellable,
						       GError              **error);
/*
static void            g_udp_socket_send_async        (GDatagramSocket      *self,
						       GSocketAddress       *destination,
						       const void           *buf,
						       gsize                 size,
						       int                   io_priority,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       void                 *user_data);
static gssize          g_udp_socket_send_finish       (GDatagramSocket      *self,
						       GAsyncResult         *res,
						       GError              **error);
*/
static gssize          g_udp_socket_receive           (GDatagramSocket      *self,
						       GSocketAddress      **source,
						       void                 *buf,
						       gsize                 size,
						       GCancellable         *cancellable,
						       GError              **error);
/*
static void            g_udp_socket_receive_async     (GDatagramSocket      *self,
						       GSocketAddress      **source,
						       void                 *buf,
						       gsize                 size,
						       int                   io_priority,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       void                 *user_data);
static gssize          g_udp_socket_receive_finish    (GDatagramSocket      *self,
						       GAsyncResult         *res,
						       GError              **error);
*/

G_DEFINE_TYPE (GUDPSocket, g_udp_socket, G_TYPE_DATAGRAM_SOCKET);

static void
g_udp_socket_class_init (GUDPSocketClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;
  GSocketClass *socket_class = (GSocketClass *)klass;
  GDatagramSocketClass *datagram_socket_class = (GDatagramSocketClass *)klass;
  
  g_type_class_add_private (gobject_class, sizeof (GUDPSocketPrivate));
  
  gobject_class->finalize = g_udp_socket_finalize;
  gobject_class->dispose = g_udp_socket_dispose;

  socket_class->get_local_address = g_udp_socket_get_local_address;

  datagram_socket_class->bind = g_udp_socket_bind;
  /*
  datagram_socket_class->bind_async = g_udp_socket_bind_async;
  datagram_socket_class->bind_finish = g_udp_socket_bind_finish;
  */
  datagram_socket_class->send = g_udp_socket_send;
  /*
  datagram_socket_class->send_async = g_udp_socket_send_async;
  datagram_socket_class->send_finish = g_udp_socket_send_finish;
  */
  datagram_socket_class->receive = g_udp_socket_receive;
  /*
  datagram_socket_class->receive_async = g_udp_socket_receive_async;
  datagram_socket_class->receive_finish = g_udp_socket_receive_finish;
  */
}

static void
g_udp_socket_init (GUDPSocket *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), G_TYPE_UDP_SOCKET,
					    GUDPSocketPrivate);
  self->priv->socket = INVALID_SOCKET;
  self->priv->local = NULL;
}

static void
g_udp_socket_dispose (GObject *object)
{
  GUDPSocket *self = (GUDPSocket *)object;

  if (self->priv->local != NULL)
    {
      g_object_unref ((GObject *)self->priv->local);
    }

  if (G_OBJECT_CLASS (g_udp_socket_parent_class)->dispose)
    G_OBJECT_CLASS (g_udp_socket_parent_class)->dispose (object);
}

static void
g_udp_socket_finalize (GObject *object)
{
  GUDPSocket *self = (GUDPSocket *)object;

  if (self->priv->socket != INVALID_SOCKET)
    {
      shutdown (self->priv->socket, SHUT_RDWR);
    }
  
  if (G_OBJECT_CLASS (g_udp_socket_parent_class)->finalize)
    G_OBJECT_CLASS (g_udp_socket_parent_class)->finalize (object);
}

static GSocketAddress *
g_udp_socket_get_local_address (GSocket *self)
{
  GUDPSocket *socket;
  struct sockaddr_storage sockaddr;
  gsize sockaddr_len;
  GSocketAddress *address;
  
  g_return_val_if_fail ((socket = G_UDP_SOCKET (self)), NULL);
  g_return_val_if_fail (socket->priv, NULL);
  
  if (socket->priv->local)
    return (GSocketAddress *)socket->priv->local;

  sockaddr_len = sizeof (struct sockaddr_storage);
  if (getsockname (socket->priv->socket,
		   (struct sockaddr *)&sockaddr,
		   &sockaddr_len) == SOCKET_ERROR)
    {
      return NULL;
    }

  address = g_socket_address_from_native (&sockaddr, sockaddr_len);
  if (!g_atomic_pointer_compare_and_exchange ((volatile gpointer *)&socket->priv->local, NULL, address))
    {
      /* The address has been set. Avoid memory leak */
      g_object_unref (address);
    }

  return (GSocketAddress *)socket->priv->local;
}

static gboolean
g_udp_socket_bind (GDatagramSocket  *self,
		   GSocketAddress   *local,
		   GCancellable     *cancellable,
		   GError          **error)
{
  GUDPSocket *socket;
  struct sockaddr *sockaddr;
  gssize socklen;

  g_return_val_if_fail ((socket = G_UDP_SOCKET (self)), FALSE);
  g_return_val_if_fail (socket->priv, FALSE);
  
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;
  
  socklen = g_socket_address_native_size (local);
  g_return_val_if_fail (socklen < 0, FALSE);
  
  sockaddr = g_alloca (socklen);
  if (!g_socket_address_to_native (local, sockaddr, socklen))
    {
      /*
       * It should be handled inside g_socket_address_to_native.
       * Here it is assumed that the errno has not been cleared.
       */
      g_set_error (error, g_io_error_quark (),
		   g_io_error_from_errno (get_errno),
		   "%s", g_strerror (get_errno));
      return FALSE;
    }

  if (bind (socket->priv->socket, sockaddr, socklen) == SOCKET_ERROR)
    {
      g_set_error (error, g_io_error_quark (),
		   g_io_error_from_errno (get_errno),
		   "%s", g_strerror (get_errno));
      return FALSE;
    }

  return TRUE;
}
/*
static void
g_udp_socket_bind_async (GDatagramSocket      *self,
			 GSocketAddress       *local,
			 int                   io_priority,
			 GCancellable         *cancellable,
			 GAsyncReadyCallback   callback,
			 void                 *user_data);
static gboolean
g_udp_socket_bind_finish (GDatagramSocket      *self,
			  GAsyncResult         *res,
			  GError              **error);
*/
static gssize
g_udp_socket_send (GDatagramSocket      *self,
		   GSocketAddress       *destination,
		   const void           *buf,
		   gsize                 size,
		   GCancellable         *cancellable,
		   GError              **error)
{
  GUDPSocket *socket;
  struct sockaddr *sockaddr;
  socklen_t socklen;
  gssize _socklen;

  g_return_val_if_fail ((socket = G_UDP_SOCKET (self)), -1);
  g_return_val_if_fail (socket->priv, -1);
  
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return -1;
  
  _socklen = g_socket_address_native_size (destination);
  g_return_val_if_fail (_socklen < 0, FALSE);
  socklen = (socklen_t)_socklen;
  
  sockaddr = g_alloca (socklen);
  if (!g_socket_address_to_native (destination, sockaddr, socklen))
    {
      g_set_error (error, g_io_error_quark (),
		   g_io_error_from_errno (get_errno),
		   "%s", g_strerror (get_errno));
      return -1;
    }

  return sendto (socket->priv->socket, buf, size, 0, sockaddr, socklen);
}
/*
static void
g_udp_socket_send_async (GDatagramSocket      *self,
			 GSocketAddress       *destination,
			 const void           *buf,
			 gsize                 size,
			 int                   io_priority,
			 GCancellable         *cancellable,
			 GAsyncReadyCallback   callback,
			 void                 *user_data);
static gssize
g_udp_socket_send_finish (GDatagramSocket      *self,
			  GAsyncResult         *res,
			  GError              **error);
*/
static gssize
g_udp_socket_receive (GDatagramSocket      *self,
		      GSocketAddress      **source,
		      void                 *buf,
		      gsize                 size,
		      GCancellable         *cancellable,
		      GError              **error)
  {
  GUDPSocket *socket;
  struct sockaddr_storage sockaddr;
  socklen_t socklen;
  gssize recv;

  g_return_val_if_fail ((socket = G_UDP_SOCKET (self)), -1);
  g_return_val_if_fail (socket->priv, -1);
  
  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  socklen = sizeof (struct sockaddr_storage);
  recv = recvfrom (socket->priv->socket, buf, size, 0,
		   (struct sockaddr *)&sockaddr, &socklen);

  if (recv == SOCKET_ERROR)
    {
      g_set_error (error, g_io_error_quark (),
		   g_io_error_from_errno (get_errno),
		   "%s", g_strerror (get_errno));
      return -1;
    }
  
  if (source != NULL)
    {
      *source = g_socket_address_from_native (&sockaddr, socklen);
    }

  return recv;
}
/*
static void
g_udp_socket_receive_async (GDatagramSocket      *self,
			    GSocketAddress      **source,
			    void                 *buf,
			    gsize                 size,
			    int                   io_priority,
			    GCancellable         *cancellable,
			    GAsyncReadyCallback   callback,
			    void                 *user_data);
static gssize
g_udp_socket_receive_finish (GDatagramSocket      *self,
			     GAsyncResult         *res,
			     GError              **error);
*/

GUDPSocket *
g_udp_socket_new ()
{
  return (GUDPSocket *)g_object_new (G_TYPE_UDP_SOCKET, NULL);
}

