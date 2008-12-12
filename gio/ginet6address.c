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

#include "ginet6address.h"
#include "gresolverprivate.h"

#include <string.h>

#include "gioalias.h"

/**
 * SECTION:ginet6address
 * @short_description: IPv6 addresses
 **/

G_DEFINE_TYPE (GInet6Address, g_inet6_address, G_TYPE_INET_ADDRESS);

struct _GInet6AddressPrivate
{
  struct in6_addr addr;
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
g_inet6_address_to_string (GInetAddress *address)
{
  gchar *addr = g_malloc (INET6_ADDRSTRLEN);

#ifndef G_OS_WIN32
  inet_ntop (AF_INET6, G_INET6_ADDRESS (address)->priv->addr.s6_addr,
	     addr, INET6_ADDRSTRLEN);

#else

  struct sockaddr_in6 sin6;
  DWORD len = INET6_ADDRSTRLEN;

  g_resolver_os_init ();
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = G_INET6_ADDRESS (address)->priv->addr;
  WSAAddressToString ((LPSOCKADDR) &sin6, sizeof (sin6), NULL, addr, &len);
#endif

  return addr;
}

static void
g_inet6_address_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  GInet6Address *address = G_INET6_ADDRESS (object);
  GInet6AddressPrivate *priv = address->priv;

  switch (prop_id)
    {
      case PROP_IS_ANY:
        g_value_set_boolean (value, IN6_IS_ADDR_UNSPECIFIED (priv->addr.s6_addr));
        break;

      case PROP_IS_LOOPBACK:
        g_value_set_boolean (value, IN6_IS_ADDR_LOOPBACK (priv->addr.s6_addr));
        break;

      case PROP_IS_LINK_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_LINKLOCAL (priv->addr.s6_addr));
        break;

      case PROP_IS_SITE_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_SITELOCAL (priv->addr.s6_addr));
        break;

      case PROP_IS_MULTICAST:
        g_value_set_boolean (value, IN6_IS_ADDR_MULTICAST (priv->addr.s6_addr));
        break;

      case PROP_IS_MC_GLOBAL:
        g_value_set_boolean (value, IN6_IS_ADDR_MC_GLOBAL (priv->addr.s6_addr));
        break;

      case PROP_IS_MC_LINK_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_MC_LINKLOCAL (priv->addr.s6_addr));
        break;

      case PROP_IS_MC_NODE_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_MC_NODELOCAL (priv->addr.s6_addr));
        break;

      case PROP_IS_MC_ORG_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_MC_ORGLOCAL (priv->addr.s6_addr));
        break;

      case PROP_IS_MC_SITE_LOCAL:
        g_value_set_boolean (value, IN6_IS_ADDR_MC_SITELOCAL (priv->addr.s6_addr));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
g_inet6_address_class_init (GInet6AddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GInetAddressClass *ginetaddress_class = G_INET_ADDRESS_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GInet6AddressPrivate));

  gobject_class->get_property = g_inet6_address_get_property;

  ginetaddress_class->to_string = g_inet6_address_to_string;

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
  struct sockaddr_in6 sin6;

#ifndef G_OS_WIN32
  if (!inet_pton (AF_INET6, string, &sin6.sin6_addr))
    return NULL;

#else /* G_OS_WIN32 */

  int len = sizeof (sin6);

  g_resolver_os_init ();
  if (WSAStringToAddress ((LPTSTR) string, AF_INET6, NULL, (LPSOCKADDR) &sin6, &len) != 0)
    return NULL;  
#endif

  return g_inet6_address_from_bytes ((guint8 *)&sin6.sin6_addr.s6_addr);
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
  GInet6Address *address = g_object_new (G_TYPE_INET6_ADDRESS, NULL);

  memcpy (&address->priv->addr, bytes, sizeof (struct in6_addr));

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
  return address->priv->addr.s6_addr;
}

/**
 * g_inet6_address_new_loopback:
 *
 * Returns: a new #GInet6Address corresponding to the IPv6 loopback address ::1.
 */
GInet6Address *
g_inet6_address_new_loopback (void)
{
  return g_inet6_address_from_bytes (in6addr_loopback.s6_addr);
}

/**
 * g_inet6_address_new_any:
 *
 * Returns: a new #GInet6Address corresponding to the IPv6 any address ::0.
 */
GInet6Address *
g_inet6_address_new_any (void)
{
  return g_inet6_address_from_bytes (in6addr_any.s6_addr);
}

#define __G_INET6_ADDRESS_C__
#include "gioaliasdef.c"
