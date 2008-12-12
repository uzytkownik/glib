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
#include <string.h>
#include <glib.h>

#ifndef G_OS_WIN32
# include <netinet/in.h>
# include <arpa/inet.h>
#else
# include <winsock2.h>
# include <ws2tcpip.h>
# define IN_LOOPBACKNET 127
#endif

#include "ginet6address.h"

/**
 * SECTION:ginet6address
 * @short_description: IPv6 addresses
 **/

G_DEFINE_TYPE (GInet6Address, g_inet6_address, G_TYPE_INET_ADDRESS);

struct _GInet6AddressPrivate
{
  union {
    guint8  u6_addr8[16];
    guint16 u6_addr16[8];
    guint32 u6_addr32[4];
  } addr;
};

static gchar *
g_inet6_address_to_string (GInetAddress *address)
{
  gchar *addr = g_malloc (48);

  g_return_val_if_fail (G_IS_INET6_ADDRESS (address), NULL);

#ifndef G_OS_WIN32
  inet_ntop (AF_INET6, G_INET6_ADDRESS (address)->priv->addr.u6_addr8, addr, 48);

  return addr;
#else
  g_error ("IPv6 not implemented on Windows");

  return NULL;
#endif
}

static void
g_inet6_address_class_init (GInet6AddressClass *klass)
{
  GInetAddressClass *ginetaddress_class = G_INET_ADDRESS_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GInet6AddressPrivate));

  ginetaddress_class->to_string = g_inet6_address_to_string;
}

static void
g_inet6_address_init (GInet6Address *address)
{
  address->priv = G_TYPE_INSTANCE_GET_PRIVATE (address,
                                               G_TYPE_INET6_ADDRESS,
                                               GInet6AddressPrivate);
}

/**
 * g_inet6_address_from_string:
 * @string: a string
 */
GInet6Address *
g_inet6_address_from_string (const char *string)
{
  struct in6_addr addr;

#ifndef G_OS_WIN32
  if(!inet_pton (AF_INET6, string, &addr))
    {
      g_warning ("Could not parse IP address %s", string);
      return NULL;
    }

  return g_inet6_address_from_bytes (addr.s6_addr);
#else
  g_error ("IPv6 not implemented on Windows");

  return NULL;
#endif
}

/**
 * g_inet6_address_from_bytes:
 * @bytes: an array of bytes
 *
 * Returns: a new #GInet6Address corresponding to @bytes
 */
GInet6Address *
g_inet6_address_from_bytes (const guint8 bytes[16])
{
  GInet6Address *address = G_INET6_ADDRESS (g_object_new (G_TYPE_INET6_ADDRESS, NULL));

  g_memmove (address->priv->addr.u6_addr8, bytes, 16);

  return address;
}

/**
 * g_inet6_address_to_bytes:
 * @address: a #GInet6Address
 *
 * Returns: a pointer to an internal array of bytes in @address,
 * which should not be modified, stored, or freed.
 */
const guint8 *
g_inet6_address_to_bytes (GInet6Address *address)
{
  return address->priv->addr.u6_addr8;
}

/**
 * g_inet6_address_new_loopback:
 *
 * Returns: a new #GInet6Address corresponding to the IPv6 loopback address ::1.
 */
GInet6Address *
g_inet6_address_new_loopback (void)
{
  guint8 bytes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

  return g_inet6_address_from_bytes (bytes);
}

/**
 * g_inet6_address_new_any:
 *
 * Returns: a new #GInet6Address corresponding to the IPv6 any address ::0.
 */
GInet6Address *
g_inet6_address_new_any (void)
{
  guint8 bytes[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  return g_inet6_address_from_bytes (bytes);
}
