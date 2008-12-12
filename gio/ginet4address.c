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

#include "ginet4address.h"
#include "gresolverprivate.h"

#include <string.h>

#include "gioalias.h"

/**
 * SECTION:ginet4address
 * @short_description: IPv4 addresses
 **/

G_DEFINE_TYPE (GInet4Address, g_inet4_address, G_TYPE_INET_ADDRESS);

struct _GInet4AddressPrivate
{
  struct in_addr addr;
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

  addr = (guint8 *)&G_INET4_ADDRESS (address)->priv->addr.s_addr;

  return g_strdup_printf ("%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
}

static const guint8 *
g_inet4_address_to_bytes (GInetAddress *address)
{
  return (guint8 *)&G_INET4_ADDRESS (address)->priv->addr.s_addr;
}

static void
g_inet4_address_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GInet4Address *address = G_INET4_ADDRESS (object);
  guint32 addr = g_ntohl (address->priv->addr.s_addr);

  switch (prop_id)
    {
      case PROP_IS_ANY:
        g_value_set_boolean (value, addr == INADDR_ANY);
        break;

      case PROP_IS_LOOPBACK:
	/* 127.0.0.0/8 */
        g_value_set_boolean (value, ((addr & 0xff000000) == 0x7f000000));
        break;

      case PROP_IS_LINK_LOCAL:
	/* 169.254.0.0/16 */
        g_value_set_boolean (value, ((addr & 0xffff0000) == 0xa9fe0000));
        break;

      case PROP_IS_SITE_LOCAL:
	/* 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 */
        g_value_set_boolean (value, ((addr & 0xff000000) == 0x0a000000 ||
				     (addr & 0xfff00000) == 0xac100000 ||
				     (addr & 0xffff0000) == 0xc0a80000));
        break;

      case PROP_IS_MULTICAST:
        g_value_set_boolean (value, IN_MULTICAST (addr));
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
  ginetaddress_class->to_bytes = g_inet4_address_to_bytes;

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
GInetAddress *
g_inet4_address_from_string (const gchar *string)
{
  struct sockaddr_in sin;

#ifndef G_OS_WIN32
  if (!inet_pton (AF_INET, string, &sin.sin_addr))
    return NULL;
#else /* G_OS_WIN32 */
  int len = sizeof (sin);

  g_resolver_os_init ();
  if (WSAStringToAddress ((LPTSTR) string, AF_INET, NULL, (LPSOCKADDR) &sin, &len) != 0)
    return NULL;  
#endif

  return g_inet4_address_from_bytes ((guint8 *)&sin.sin_addr.s_addr);
}

/**
 * g_inet4_address_from_bytes:
 * @bytes: an array of 4 bytes, e.g. {127, 0, 0, 1} for the loopback address
 *
 * Returns: a new #GInet4Address corresponding to @bytes.
 */
GInetAddress *
g_inet4_address_from_bytes (const guint8 bytes[4])
{
  GInet4Address *address = g_object_new (G_TYPE_INET4_ADDRESS, NULL);

  memcpy (&address->priv->addr.s_addr, bytes, sizeof (struct in_addr));
  return (GInetAddress *)address;
}

/**
 * g_inet4_address_new_loopback:
 *
 * Returns: a new #GInet4Address corresponding to the loopback address (127.0.0.1).
 */
GInetAddress *
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
GInetAddress *
g_inet4_address_new_any (void)
{
  guint8 addr[4] = {0, 0, 0, 0};

  return g_inet4_address_from_bytes (addr);
}

#define __G_INET4_ADDRESS_C__
#include "gioaliasdef.c"
