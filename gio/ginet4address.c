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

#ifndef G_OS_WIN32
# include <netinet/in.h>
# include <arpa/inet.h>
#else
# include <winsock2.h>
# define IN_LOOPBACKNET 127
#endif

#include "ginet4address.h"

#include "gioalias.h"

/**
 * SECTION:ginet4address
 * @short_description: IPv4 addresses
 **/

G_DEFINE_TYPE (GInet4Address, g_inet4_address, G_TYPE_INET_ADDRESS);

struct _GInet4AddressPrivate
{
  union
  {
    guint8  u4_addr8[4];
    guint32 u4_addr32;
  } addr;
};

enum
{
  PROP_0,
  PROP_IS_ANY,
  PROP_IS_LOOPBACK,
  PROP_IS_LINK_LOCAL,
  PROP_IS_SITE_LOCAL,
  PROP_IS_MULTICAST,
  PROP_IS_MC_GLOBAL,
  PROP_IS_MC_LINK_LOCAL,
  PROP_IS_MC_NODE_LOCAL,
  PROP_IS_MC_ORG_LOCAL,
  PROP_IS_MC_SITE_LOCAL,
};

static gchar *
g_inet4_address_to_string (GInetAddress *address)
{
  guint8 *addr;

  g_return_val_if_fail (G_IS_INET4_ADDRESS (address), NULL);

  addr = G_INET4_ADDRESS (address)->priv->addr.u4_addr8;

  return g_strdup_printf ("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

static void
g_inet4_address_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GInet4Address *address = G_INET4_ADDRESS (object);
  GInet4AddressPrivate *priv = address->priv;

  guint32 addr = g_ntohl (priv->addr.u4_addr32);

  switch (prop_id)
    {
      case PROP_IS_ANY:
        g_value_set_boolean (value, addr == INADDR_ANY);
        break;

      case PROP_IS_LOOPBACK:
        g_value_set_boolean (value, (priv->addr.u4_addr8[0] == IN_LOOPBACKNET));
        break;

      case PROP_IS_LINK_LOCAL:
        g_value_set_boolean (value, ((addr & 0xffff0000) == 0xa9fe0000));
        break;

      case PROP_IS_SITE_LOCAL:
        g_value_set_boolean (value, ((addr & 0xff000000) == (10 << 24)) || ((addr & 0xfff00000) == 0xac100000) || ((addr & 0xffff0000) == 0xc0a80000));
        break;

      case PROP_IS_MULTICAST:
        g_value_set_boolean (value, IN_MULTICAST (g_ntohl (G_INET4_ADDRESS (address)->priv->addr.u4_addr32)));
        break;

      case PROP_IS_MC_GLOBAL:
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_IS_MC_LINK_LOCAL:
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_IS_MC_NODE_LOCAL:
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_IS_MC_ORG_LOCAL:
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_IS_MC_SITE_LOCAL:
        g_value_set_boolean (value, FALSE);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
g_inet4_address_class_init (GInet4AddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GInetAddressClass *ginetaddress_class = G_INET_ADDRESS_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GInet4AddressPrivate));

  gobject_class->get_property = g_inet4_address_get_property;

  ginetaddress_class->to_string = g_inet4_address_to_string;

  g_object_class_override_property (gobject_class, PROP_IS_ANY, "is-any");
  g_object_class_override_property (gobject_class, PROP_IS_LOOPBACK, "is-loopback");
  g_object_class_override_property (gobject_class, PROP_IS_LINK_LOCAL, "is-link-local");
  g_object_class_override_property (gobject_class, PROP_IS_SITE_LOCAL, "is-site-local");
  g_object_class_override_property (gobject_class, PROP_IS_MULTICAST, "is-multicast");
  g_object_class_override_property (gobject_class, PROP_IS_MC_GLOBAL, "is-mc-global");
  g_object_class_override_property (gobject_class, PROP_IS_MC_LINK_LOCAL, "is-mc-link-local");
  g_object_class_override_property (gobject_class, PROP_IS_MC_ORG_LOCAL, "is-mc-org-local");
  g_object_class_override_property (gobject_class, PROP_IS_MC_SITE_LOCAL, "is-mc-site-local");
}

static void
g_inet4_address_init (GInet4Address *address)
{
  address->priv = G_TYPE_INSTANCE_GET_PRIVATE (address,
                                               G_TYPE_INET4_ADDRESS,
                                               GInet4AddressPrivate);
}

/**
 * g_inet4_address_from_string:
 * @string: an IP address
 *
 * Returns: a new #GInet4Address corresponding to @string.
 */
GInet4Address *
g_inet4_address_from_string (const gchar *string)
{
  struct in_addr addr;

#ifndef G_OS_WIN32
  if (!inet_pton (AF_INET, string, &addr))
    {
      g_warning ("Could not parse IP address %s", string);
      return NULL;
    }
#else
  addr.s_addr = inet_addr (string);
#endif

  return g_inet4_address_from_bytes ((guint8 *)&addr.s_addr);
}

/**
 * g_inet4_address_from_bytes:
 * @bytes: an array of 4 bytes, e.g. {127, 0, 0, 1} for the loopback address
 *
 * Returns: a new #GInet4Address corresponding to @bytes.
 */
GInet4Address *
g_inet4_address_from_bytes (const guint8 bytes[4])
{
  GInet4Address *address = G_INET4_ADDRESS (g_object_new (G_TYPE_INET4_ADDRESS, NULL));

  guint8 *addr = address->priv->addr.u4_addr8;

  addr[0] = bytes[0];
  addr[1] = bytes[1];
  addr[2] = bytes[2];
  addr[3] = bytes[3];

  return address;
}

/**
 * g_inet4_address_to_bytes:
 * @address: a #GInet4Address
 *
 * Returns: a pointer to an internal array of the bytes in @address,
 * which should not be modified, stored, or freed.
 */
const guint8 *
g_inet4_address_to_bytes (GInet4Address *address)
{
  g_return_val_if_fail (G_IS_INET4_ADDRESS (address), NULL);

  return address->priv->addr.u4_addr8;
}

/**
 * g_inet4_address_new_loopback:
 *
 * Returns: a new #GInet4Address corresponding to the loopback address (127.0.0.1).
 */
GInet4Address *
g_inet4_address_new_loopback (void)
{
  guint8 addr[4] = {127, 0, 0, 1};

  return g_inet4_address_from_bytes (addr);
}

/**
 * g_inet4_address_new_any:
 *
 * Returns: a new #GInet4Address corresponding to the any address (0.0.0.0).
 */
GInet4Address *
g_inet4_address_new_any (void)
{
  guint8 addr[4] = {0, 0, 0, 0};

  return g_inet4_address_from_bytes (addr);
}

#define __G_INET4_ADDRESS_C__
#include "gioaliasdef.c"
