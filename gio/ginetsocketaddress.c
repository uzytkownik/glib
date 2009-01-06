/* GNIO - GLib Network Layer of GIO
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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#include <config.h>
#include <glib.h>
#include <string.h>

#include "ginetsocketaddress.h"
#include "ginetaddress.h"
#include "gresolverprivate.h"

#include "gioalias.h"

/**
 * SECTION:ginetsocketaddress
 * @short_description: Internet socket addresses
 **/

G_DEFINE_TYPE (GInetSocketAddress, g_inet_socket_address, G_TYPE_SOCKET_ADDRESS);

enum {
  PROP_0,
  PROP_ADDRESS,
  PROP_PORT
};

struct _GInetSocketAddressPrivate
{
  GInetAddress *address;
  guint16       port;
};

static void
g_inet_socket_address_finalize (GObject *object)
{
  GInetSocketAddress *address G_GNUC_UNUSED = G_INET_SOCKET_ADDRESS (object);

  if (G_OBJECT_CLASS (g_inet_socket_address_parent_class)->finalize)
    (*G_OBJECT_CLASS (g_inet_socket_address_parent_class)->finalize) (object);
}

static void
g_inet_socket_address_dispose (GObject *object)
{
  GInetSocketAddress *address G_GNUC_UNUSED = G_INET_SOCKET_ADDRESS (object);

  g_object_unref (address->priv->address);

  if (G_OBJECT_CLASS (g_inet_socket_address_parent_class)->dispose)
    (*G_OBJECT_CLASS (g_inet_socket_address_parent_class)->dispose) (object);
}

static void
g_inet_socket_address_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GInetSocketAddress *address = G_INET_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_ADDRESS:
        g_value_set_object (value, address->priv->address);
        break;

      case PROP_PORT:
        g_value_set_uint (value, address->priv->port);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_inet_socket_address_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GInetSocketAddress *address = G_INET_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_ADDRESS:
        address->priv->address = g_object_ref (g_value_get_object (value));
        break;

      case PROP_PORT:
        address->priv->port = (guint16) g_value_get_uint (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gssize
g_inet_socket_address_native_size (GSocketAddress *address)
{
  GInetSocketAddress *addr;
  GInetAddressFamily family;

  g_return_val_if_fail (G_IS_INET_SOCKET_ADDRESS (address), 0);

  addr = G_INET_SOCKET_ADDRESS (address);
  family = g_inet_address_get_family (addr->priv->address);

  if (family == AF_INET)
    return sizeof (struct sockaddr_in);
  else if (family == AF_INET6)
    return sizeof (struct sockaddr_in6);
  else
    return -1;
}

static gboolean
g_inet_socket_address_to_native (GSocketAddress *address,
                                 gpointer        dest,
				 gsize           destlen)
{
  GInetSocketAddress *addr;
  GInetAddressFamily family;

  g_return_val_if_fail (G_IS_INET_SOCKET_ADDRESS (address), 0);

  addr = G_INET_SOCKET_ADDRESS (address);
  family = g_inet_address_get_family (addr->priv->address);

  if (family == AF_INET)
    {
      struct sockaddr_in *sock = (struct sockaddr_in *) dest;

      if (destlen < sizeof (*sock))
	return FALSE;

      sock->sin_family = AF_INET;
      sock->sin_port = g_htons (addr->priv->port);
      memcpy (&(sock->sin_addr.s_addr), g_inet_address_to_bytes (addr->priv->address), sizeof (sock->sin_addr));
      memset (sock->sin_zero, 0, sizeof (sock->sin_zero));
      return TRUE;
    }
  else if (family == AF_INET6)
    {
      struct sockaddr_in6 *sock = (struct sockaddr_in6 *) dest;

      if (destlen < sizeof (*sock))
	return FALSE;

      memset (sock, 0, sizeof (sock));
      sock->sin6_family = AF_INET6;
      sock->sin6_port = g_htons (addr->priv->port);
      memcpy (&(sock->sin6_addr.s6_addr), g_inet_address_to_bytes (addr->priv->address), sizeof (sock->sin6_addr));
      return TRUE;
    }
  else
    return FALSE;
}

static GDatagramSocket
g_inet_socket_address_create_datagram_socket (GSocketAddress *object)
{
  GInetSocketAddress *self;
  GInetAddressFamilly familly;
  
  g_return_val_if_fail ((self = G_INET_SOCKET_ADDRESS (object)), NULL);
  family = g_inet_address_get_family (addr->priv->address);

  return g_native_datagream_socket_new (familly);
}

static void
g_inet_socket_address_class_init (GInetSocketAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GSocketAddressClass *gsocketaddress_class = G_SOCKET_ADDRESS_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GInetSocketAddressPrivate));

  gobject_class->finalize = g_inet_socket_address_finalize;
  gobject_class->dispose = g_inet_socket_address_dispose;
  gobject_class->set_property = g_inet_socket_address_set_property;
  gobject_class->get_property = g_inet_socket_address_get_property;

  gsocketaddress_class->to_native = g_inet_socket_address_to_native;
  gsocketaddress_class->native_size = g_inet_socket_address_native_size;
  gsocketaddress_class->create_datagram_socket = g_inet_socket_address_create_datagram_socket;

  g_object_class_install_property (gobject_class, PROP_ADDRESS,
                                   g_param_spec_object ("address",
                                                        "address",
                                                        "address",
                                                        G_TYPE_INET_ADDRESS,
                                                        G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK));

  g_object_class_install_property (gobject_class, PROP_PORT,
                                   g_param_spec_uint ("port",
                                                      "port",
                                                      "port",
                                                      0,
                                                      65535,
                                                      0,
                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NICK));
}

static void
g_inet_socket_address_init (GInetSocketAddress *address)
{
  address->priv = G_TYPE_INSTANCE_GET_PRIVATE (address,
                                               G_TYPE_INET_SOCKET_ADDRESS,
                                               GInetSocketAddressPrivate);

  address->priv->address = NULL;
  address->priv->port = 0;
}

/**
 * g_inet_socket_address_new:
 * @address: a #GInetAddress
 * @port: a port number
 *
 * Returns: a new #GInetSocketAddress
 */
GSocketAddress *
g_inet_socket_address_new (GInetAddress *address,
                           guint16       port)
{
  return g_object_new (G_TYPE_INET_SOCKET_ADDRESS,
		       "address", address,
		       "port", port,
		       NULL);
}

/**
 * g_inet_socket_address_get_address:
 * @address: a #GInetSocketAddress
 *
 * Returns: the #GInetAddress for @address, which must be g_object_ref()'d if
 * it will be stored
 */
GInetAddress *
g_inet_socket_address_get_address (GInetSocketAddress *address)
{
  g_return_val_if_fail (G_IS_INET_SOCKET_ADDRESS (address), NULL);

  return address->priv->address;
}

/**
 * g_inet_socket_address_get_port:
 * @address: a #GInetSocketAddress
 *
 * Returns: the port for @address
 */
guint16
g_inet_socket_address_get_port (GInetSocketAddress *address)
{
  g_return_val_if_fail (G_IS_INET_SOCKET_ADDRESS (address), 0);

  return address->priv->port;
}

#define __G_INET_SOCKET_ADDRESS_C__
#include "gioaliasdef.c"
