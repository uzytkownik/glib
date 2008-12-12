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

#ifndef G_OS_WIN32
# include <sys/socket.h>
# include <sys/un.h>
# include <netinet/in.h>
#else
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include "glocalsocketaddress.h"

/**
 * SECTION:glocalsocketaddress
 * @short_description: Local socket addresses
 **/

G_DEFINE_TYPE (GLocalSocketAddress, g_local_socket_address, G_TYPE_SOCKET_ADDRESS);

enum
{
  PROP_0,
  PROP_PATH,
};

struct _GLocalSocketAddressPrivate
{
  char *path;
};

static void
g_local_socket_address_finalize (GObject *object)
{
  GLocalSocketAddress *address G_GNUC_UNUSED = G_LOCAL_SOCKET_ADDRESS (object);

  g_free (address->priv->path);

  if (G_OBJECT_CLASS (g_local_socket_address_parent_class)->finalize)
    (G_OBJECT_CLASS (g_local_socket_address_parent_class)->finalize) (object);
}

static void
g_local_socket_address_dispose (GObject *object)
{
  GLocalSocketAddress *address G_GNUC_UNUSED = G_LOCAL_SOCKET_ADDRESS (object);

  if (G_OBJECT_CLASS (g_local_socket_address_parent_class)->dispose)
    (*G_OBJECT_CLASS (g_local_socket_address_parent_class)->dispose) (object);
}

static void
g_local_socket_address_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GLocalSocketAddress *address = G_LOCAL_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_PATH:
        g_value_set_string (value, address->priv->path);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_local_socket_address_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GLocalSocketAddress *address = G_LOCAL_SOCKET_ADDRESS (object);

  switch (prop_id)
    {
      case PROP_PATH:
        g_free (address->priv->path);
        address->priv->path = g_value_dup_string (value);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static gssize
g_local_socket_address_native_size (GSocketAddress *address)
{
  g_return_val_if_fail (G_IS_LOCAL_SOCKET_ADDRESS (address), 0);

#ifdef G_OS_WIN32
  g_error ("local sockets not available on Windows");
  return -1;
#else
  return sizeof (struct sockaddr_un);
#endif
}

static gboolean
g_local_socket_address_to_native (GSocketAddress *address,
                                  gpointer        dest)
{
  GLocalSocketAddress *addr;

#ifdef G_OS_WIN32
  g_error ("local sockets not available on Windows");
  return FALSE;
#else
  struct sockaddr_un *sock;

  g_return_val_if_fail (G_IS_LOCAL_SOCKET_ADDRESS (address), 0);

  addr = G_LOCAL_SOCKET_ADDRESS (address);

  sock = (struct sockaddr_un *) dest;

  sock->sun_family = AF_LOCAL;
  strcpy (sock->sun_path, addr->priv->path);

  return TRUE;
#endif
}

static void
g_local_socket_address_class_init (GLocalSocketAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GSocketAddressClass *gsocketaddress_class = G_SOCKET_ADDRESS_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GLocalSocketAddressPrivate));

  gobject_class->finalize = g_local_socket_address_finalize;
  gobject_class->dispose = g_local_socket_address_dispose;
  gobject_class->set_property = g_local_socket_address_set_property;
  gobject_class->get_property = g_local_socket_address_get_property;

  gsocketaddress_class->to_native = g_local_socket_address_to_native;
  gsocketaddress_class->native_size = g_local_socket_address_native_size;

  g_object_class_install_property (gobject_class,
                                   PROP_PATH,
                                   g_param_spec_string ("path",
                                                        "",
                                                        "",
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void
g_local_socket_address_init (GLocalSocketAddress *address)
{
  address->priv = G_TYPE_INSTANCE_GET_PRIVATE (address,
                                               G_TYPE_LOCAL_SOCKET_ADDRESS,
                                               GLocalSocketAddressPrivate);

  address->priv->path = NULL;
}

/**
 * g_local_socket_address_new:
 * @address: a #GLocalAddress
 * @port: a port number
 *
 * Returns: a new #GLocalSocketAddress with a floating reference
 */
GLocalSocketAddress *
g_local_socket_address_new (const gchar *path)
{
  return G_LOCAL_SOCKET_ADDRESS (g_object_new (G_TYPE_LOCAL_SOCKET_ADDRESS, "path", path, NULL));
}

