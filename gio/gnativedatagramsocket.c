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

#include "gnativedatagramsocket.h"

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
#include "gioenums.h"
#include "gioenumtypes.h"
#include "gsocketaddress.h"

struct _GNativeDatagramSocketPrivate
{
  SOCKET socket;
  GInetAddressFamily familly;
  volatile GInetSocketAddress *local;
};

enum
  {
    PROP_0,
    PROP_AF
  };

static void           g_native_datagram_socket_get_property       (GObject              *object,
								   guint                 prop_id,
								   GValue               *value,
								   GParamSpec           *pspec);
static void           g_native_datagram_socket_set_property       (GObject              *object,
								   guint                 prop_id,
								   const GValue         *value,
								   GParamSpec           *pspec);
static void            g_native_datagram_socket_finalize          (GObject              *object);
static void            g_native_datagram_socket_dispose           (GObject              *object);
static GSocketAddress *g_native_datagram_socket_get_local_address (GSocket              *self);
static gboolean        g_native_datagram_socket_bind              (GDatagramSocket      *self,
								   GSocketAddress       *local,
								   GCancellable         *cancellable,
								   GError              **error);
/*
static void            g_native_datagram_socket_bind_async        (GDatagramSocket      *self,
								   GSocketAddress       *local,
								   int                   io_priority,
								   GCancellable         *cancellable,
								   GAsyncReadyCallback   callback,
								   void                 *user_data);
static gboolean        g_native_datagram_socket_bind_finish       (GDatagramSocket      *self,
								   GAsyncResult         *res,
								   GError              **error);
*/
static gssize          g_native_datagram_socket_send              (GDatagramSocket      *self,
								   GSocketAddress       *destination,
								   const void           *buf,
								   gsize                 size,
								   GCancellable         *cancellable,
								   GError              **error);
/*
static void            g_native_datagram_socket_send_async        (GDatagramSocket      *self,
								   GSocketAddress       *destination,
								   const void           *buf,
								   gsize                 size,
								   int                   io_priority,
								   GCancellable         *cancellable,
								   GAsyncReadyCallback   callback,
								   void                 *user_data);
static gssize          g_native_datagram_socket_send_finish       (GDatagramSocket      *self,
								   GAsyncResult         *res,
								   GError              **error);
*/
static gssize          g_native_datagram_socket_receive           (GDatagramSocket      *self,
								   GSocketAddress      **source,
								   void                 *buf,
								   gsize                 size,
								   GCancellable         *cancellable,
								   GError              **error);
/*
static void            g_native_datagram_socket_receive_async     (GDatagramSocket      *self,
						       GSocketAddress      **source,
						       void                 *buf,
						       gsize                 size,
						       int                   io_priority,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       void                 *user_data);
static gssize          g_native_datagram_socket_receive_finish    (GDatagramSocket      *self,
						       GAsyncResult         *res,
						       GError              **error);
*/

G_DEFINE_TYPE (GNativeDatagramSocket, g_native_datagram_socket, G_TYPE_DATAGRAM_SOCKET);

static void
g_native_datagram_socket_class_init (GNativeDatagramSocketClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;
  GSocketClass *socket_class = (GSocketClass *)klass;
  GDatagramSocketClass *datagram_socket_class = (GDatagramSocketClass *)klass;
  
  g_type_class_add_private (gobject_class, sizeof (GNativeDatagramSocketPrivate));

  gobject_class->set_property = g_native_datagram_socket_set_property;
  gobject_class->get_property = g_native_datagram_socket_get_property;
  gobject_class->finalize = g_native_datagram_socket_finalize;
  gobject_class->dispose = g_native_datagram_socket_dispose;

  g_object_class_install_property (gobject_class, PROP_AF,
				   g_param_spec_enum ("address-familly",
						      "address familly",
						      "Adress familly of the socket",
						      G_TYPE_INET_ADDRESS_FAMILY,
						      G_INET_ADDRESS_IPV4,
						      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT | G_PARAM_CONSTRUCT_ONLY));
  
  socket_class->get_local_address = g_native_datagram_socket_get_local_address;

  datagram_socket_class->bind = g_native_datagram_socket_bind;
  /*
  datagram_socket_class->bind_async = g_native_datagram_socket_bind_async;
  datagram_socket_class->bind_finish = g_native_datagram_socket_bind_finish;
  */
  datagram_socket_class->send = g_native_datagram_socket_send;
  /*
  datagram_socket_class->send_async = g_native_datagram_socket_send_async;
  datagram_socket_class->send_finish = g_native_datagram_socket_send_finish;
  */
  datagram_socket_class->receive = g_native_datagram_socket_receive;
  /*
  datagram_socket_class->receive_async = g_native_datagram_socket_receive_async;
  datagram_socket_class->receive_finish = g_native_datagram_socket_receive_finish;
  */
}

static void
g_native_datagram_socket_init (GNativeDatagramSocket *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), G_TYPE_NATIVE_DATAGRAM_SOCKET,
					    GNativeDatagramSocketPrivate);
  self->priv->socket = INVALID_SOCKET;
  self->priv->local = NULL;
}

static void
g_native_datagram_socket_get_property (GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec)
{
  GNativeDatagramSocket *socket;

  g_return_if_fail ((socket = G_NATIVE_DATAGRAM_SOCKET (object)));
  g_return_if_fail (socket->priv);
  
  switch (prop_id)
    {
    case PROP_AF:
      g_value_set_enum (value, socket->priv->familly);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_native_datagram_socket_set_property (GObject      *object,
				       guint         prop_id,
				       const GValue *value,
				       GParamSpec   *pspec)
{
  GNativeDatagramSocket *dgram_socket;

  g_return_if_fail ((dgram_socket = G_NATIVE_DATAGRAM_SOCKET (object)));
  g_return_if_fail (dgram_socket->priv);
  
  switch (prop_id)
    {
    case PROP_AF:
      dgram_socket->priv->familly = g_value_get_enum (value);
      dgram_socket->priv->socket = socket (dgram_socket->priv->familly,
					   SOCK_STREAM, 0);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_native_datagram_socket_dispose (GObject *object)
{
  GNativeDatagramSocket *self = (GNativeDatagramSocket *)object;

  if (self->priv->local != NULL)
    {
      g_object_unref ((GObject *)self->priv->local);
    }

  if (G_OBJECT_CLASS (g_native_datagram_socket_parent_class)->dispose)
    G_OBJECT_CLASS (g_native_datagram_socket_parent_class)->dispose (object);
}

static void
g_native_datagram_socket_finalize (GObject *object)
{
  GNativeDatagramSocket *self = (GNativeDatagramSocket *)object;

  if (self->priv->socket != INVALID_SOCKET)
    {
      shutdown (self->priv->socket, SHUT_RDWR);
    }
  
  if (G_OBJECT_CLASS (g_native_datagram_socket_parent_class)->finalize)
    G_OBJECT_CLASS (g_native_datagram_socket_parent_class)->finalize (object);
}

static GSocketAddress *
g_native_datagram_socket_get_local_address (GSocket *self)
{
  GNativeDatagramSocket *socket;
  struct sockaddr_storage sockaddr;
  gsize sockaddr_len;
  GSocketAddress *address;
  
  g_return_val_if_fail ((socket = G_NATIVE_DATAGRAM_SOCKET (self)), NULL);
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
g_native_datagram_socket_bind (GDatagramSocket  *self,
			       GSocketAddress   *local,
			       GCancellable     *cancellable,
			       GError          **error)
{
  GNativeDatagramSocket *socket;
  struct sockaddr *sockaddr;
  gssize socklen;

  g_return_val_if_fail ((socket = G_NATIVE_DATAGRAM_SOCKET (self)), FALSE);
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
g_native_datagram_socket_bind_async (GDatagramSocket      *self,
				     GSocketAddress       *local,
				     int                   io_priority,
				     GCancellable         *cancellable,
				     GAsyncReadyCallback   callback,
				     void                 *user_data);
static gboolean
g_native_datagram_socket_bind_finish (GDatagramSocket      *self,
				      GAsyncResult         *res,
				      GError              **error);
*/
static gssize
g_native_datagram_socket_send (GDatagramSocket      *self,
		   GSocketAddress       *destination,
		   const void           *buf,
		   gsize                 size,
		   GCancellable         *cancellable,
		   GError              **error)
{
  GNativeDatagramSocket *socket;
  struct sockaddr *sockaddr;
  socklen_t socklen;
  gssize _socklen;

  g_return_val_if_fail ((socket = G_NATIVE_DATAGRAM_SOCKET (self)), -1);
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
g_native_datagram_socket_send_async (GDatagramSocket      *self,
				     GSocketAddress       *destination,
				     const void           *buf,
				     gsize                 size,
				     int                   io_priority,
				     GCancellable         *cancellable,
				     GAsyncReadyCallback   callback,
				     void                 *user_data);
static gssize
g_native_datagram_socket_send_finish (GDatagramSocket      *self,
				      GAsyncResult         *res,
				      GError              **error);
*/
static gssize
g_native_datagram_socket_receive (GDatagramSocket      *self,
				  GSocketAddress      **source,
				  void                 *buf,
				  gsize                 size,
				  GCancellable         *cancellable,
				  GError              **error)
  {
  GNativeDatagramSocket *socket;
  struct sockaddr_storage sockaddr;
  socklen_t socklen;
  gssize recv;

  g_return_val_if_fail ((socket = G_NATIVE_DATAGRAM_SOCKET (self)), -1);
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
g_native_datagram_socket_receive_async (GDatagramSocket      *self,
					GSocketAddress      **source,
					void                 *buf,
					gsize                 size,
					int                   io_priority,
					GCancellable         *cancellable,
					GAsyncReadyCallback   callback,
					void                 *user_data);
static gssize
g_native_datagram_socket_receive_finish (GDatagramSocket      *self,
					 GAsyncResult         *res,
					 GError              **error);
*/

GNativeDatagramSocket *
g_native_datagram_socket_new (GInetAddressFamily inet)
{
  return (GNativeDatagramSocket *)g_object_new (G_TYPE_NATIVE_DATAGRAM_SOCKET,
						 "address-familly", inet,
						 NULL);
}

