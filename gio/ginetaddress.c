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

#include "ginetaddress.h"
#include "gioenums.h"
#include "gioenumtypes.h"
#include "gresolverprivate.h"

#include "gioalias.h"

/**
 * SECTION:ginetaddress
 * @short_description: A raw IPv4/IPv6 addresses
 *
 * #GInetAddress represents an IPv4 or IPv6 internet address.
 **/

G_DEFINE_TYPE (GInetAddress, g_inet_address, G_TYPE_OBJECT);

struct _GInetAddressPrivate
{
  GInetAddressFamily family;
  union {
    struct in_addr ipv4;
    struct in6_addr ipv6;
  } addr;
};

enum
{
  PROP_0,
  PROP_FAMILY,
  PROP_BYTES,
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

static void
g_inet_address_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  GInetAddress *address = G_INET_ADDRESS (object);

  switch (prop_id) 
    {
    case PROP_FAMILY:
      address->priv->family = g_value_get_enum (value);
      break;

    case PROP_BYTES:
      memcpy (&address->priv->addr, g_value_get_pointer (value),
	      address->priv->family == AF_INET ?
	      sizeof (address->priv->addr.ipv4) :
	      sizeof (address->priv->addr.ipv6));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_inet_address_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GInetAddress *address = G_INET_ADDRESS (object);

  switch (prop_id)
    {
    case PROP_FAMILY:
      g_value_set_enum (value, address->priv->family);
      break;

    case PROP_BYTES:
      g_value_set_pointer (value, &address->priv->addr);
      break;

    case PROP_IS_ANY:
      g_value_set_boolean (value, g_inet_address_is_any (address));
      break;

    case PROP_IS_LOOPBACK:
      g_value_set_boolean (value, g_inet_address_is_loopback (address));
      break;

    case PROP_IS_LINK_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_link_local (address));
      break;

    case PROP_IS_SITE_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_site_local (address));
      break;

    case PROP_IS_MULTICAST:
      g_value_set_boolean (value, g_inet_address_is_multicast (address));
      break;

    case PROP_IS_MC_GLOBAL:
      g_value_set_boolean (value, g_inet_address_is_mc_global (address));
      break;

    case PROP_IS_MC_LINK_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_mc_link_local (address));
      break;

    case PROP_IS_MC_NODE_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_mc_node_local (address));
      break;

    case PROP_IS_MC_ORG_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_mc_org_local (address));
      break;

    case PROP_IS_MC_SITE_LOCAL:
      g_value_set_boolean (value, g_inet_address_is_mc_site_local (address));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
g_inet_address_class_init (GInetAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GInetAddressPrivate));

  gobject_class->set_property = g_inet_address_set_property;
  gobject_class->get_property = g_inet_address_get_property;

  g_object_class_install_property (gobject_class, PROP_FAMILY,
                                   g_param_spec_enum ("family",
						      "",
						      "",
						      G_TYPE_INET_ADDRESS_FAMILY,
						      G_INET_ADDRESS_INVALID,
						      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_BYTES,
                                   g_param_spec_pointer ("bytes",
							 "",
							 "",
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_ANY,
                                   g_param_spec_boolean ("is-any",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_LINK_LOCAL,
                                   g_param_spec_boolean ("is-link-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_LOOPBACK,
                                   g_param_spec_boolean ("is-loopback",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_SITE_LOCAL,
                                   g_param_spec_boolean ("is-site-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MULTICAST,
                                   g_param_spec_boolean ("is-multicast",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MC_GLOBAL,
                                   g_param_spec_boolean ("is-mc-global",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MC_LINK_LOCAL,
                                   g_param_spec_boolean ("is-mc-link-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MC_NODE_LOCAL,
                                   g_param_spec_boolean ("is-mc-node-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MC_ORG_LOCAL,
                                   g_param_spec_boolean ("is-mc-org-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));

  g_object_class_install_property (gobject_class, PROP_IS_MC_SITE_LOCAL,
                                   g_param_spec_boolean ("is-mc-site-local",
                                                         "",
                                                         "",
                                                         FALSE,
                                                         G_PARAM_READABLE | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_STATIC_NAME));
}

static void
g_inet_address_init (GInetAddress *address)
{
  address->priv = G_TYPE_INSTANCE_GET_PRIVATE (address,
                                               G_TYPE_INET_ADDRESS,
                                               GInetAddressPrivate);
}

/**
 * g_inet_address_from_string:
 * @string: a string representation of an IP address
 *
 * Returns: a new #GInetAddress corresponding to @string, or %NULL if
 * @address_string could not be parsed.
 */
GInetAddress *
g_inet_address_from_string (const gchar *string)
{
#ifdef G_OS_WIN32
  struct sockaddr_storage sa;
  struct sockaddr_in *sin = (struct sockaddr_in *)&sa;
  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&sa;
  gint len;

  memset (&sa, 0, sizeof (sa));
  _g_resolver_os_init ();

  len = sizeof (sa);
  if (WSAStringToAddress ((LPTSTR) string, AF_INET, NULL, (LPSOCKADDR) &sa, &len) == 0)
    return g_inet_address_from_bytes ((guint8 *)&sin.sin_addr, AF_INET);
  else if (WSAStringToAddress ((LPTSTR) string, AF_INET6, NULL, (LPSOCKADDR) &sa, &len) == 0)
    return g_inet_address_from_bytes ((guint8 *)&sin6.sin6_addr, AF_INET6);

#else /* !G_OS_WIN32 */

  struct in_addr in_addr;
  struct in6_addr in6_addr;

  if (inet_pton (AF_INET, string, &in_addr) > 0)
    return g_inet_address_from_bytes ((guint8 *)&in_addr, AF_INET);
  else if (inet_pton (AF_INET6, string, &in6_addr) > 0)
    return g_inet_address_from_bytes ((guint8 *)&in6_addr, AF_INET6);
#endif

  return NULL;
}

static inline gboolean
g_inet_address_family_is_valid (GInetAddressFamily family)
{
  return family == AF_INET || family == AF_INET6;
}

/**
 * g_inet_address_from_bytes:
 * @bytes: raw address data
 * @family: the address family of @bytes
 *
 * Creates a new #GInetAddress from the given @family and @bytes.
 * @bytes should be 4 bytes for %G_INET_ADDRESS_IPV4 and 16 bytes for
 * %G_INET_ADDRESS_IPV6.
 *
 * Returns: a new #GInetAddress corresponding to @family and @bytes.
 */
GInetAddress *
g_inet_address_from_bytes (const guint8       *bytes,
			   GInetAddressFamily  family)
{
  g_return_val_if_fail (g_inet_address_family_is_valid (family), NULL);

  return g_object_new (G_TYPE_INET_ADDRESS,
		       "family", family,
		       "bytes", bytes,
		       NULL);
}

/**
 * g_inet4_address_new_loopback:
 * @family: the address family
 *
 * Returns: a new #GInetAddress corresponding to the loopback address
 * for @family.
 */
GInetAddress *
g_inet_address_new_loopback (GInetAddressFamily  family)
{
  g_return_val_if_fail (g_inet_address_family_is_valid (family), NULL);

  if (family == AF_INET)
    {    
      guint8 addr[4] = {127, 0, 0, 1};

      return g_inet_address_from_bytes (addr, family);
    }
  else
    return g_inet_address_from_bytes (in6addr_loopback.s6_addr, family);
}

/**
 * g_inet_address_new_any:
 * @family: the address family
 *
 * Returns: a new #GInetAddress corresponding to the "any" address
 * for @family.
 */
GInetAddress *
g_inet_address_new_any (GInetAddressFamily  family)
{
  g_return_val_if_fail (g_inet_address_family_is_valid (family), NULL);

  if (family == AF_INET)
    {    
      guint8 addr[4] = {0, 0, 0, 0};

      return g_inet_address_from_bytes (addr, family);
    }
  else
    return g_inet_address_from_bytes (in6addr_any.s6_addr, family);
}


/**
 * g_inet_address_to_string:
 * @address: a #GInetAddress
 *
 * Returns: a representation of @address as a string, which should be freed after use.
 */
gchar *
g_inet_address_to_string (GInetAddress *address)
{
  gchar buffer[INET6_ADDRSTRLEN];
#ifdef G_OS_WIN32
  DWORD buflen = sizeof (buffer), addrlen;
  struct sockaddr_storage sa;
  struct sockaddr_in *sin = (struct sockaddr_in *)&sa;
  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&sa;
#endif

  g_return_val_if_fail (G_IS_INET_ADDRESS (address), NULL);

  _g_resolver_os_init ();

#ifdef G_OS_WIN32
  sa.ss_family = address->priv->family;
  if (address->priv->family == AF_INET)
    {
      addrlen = sizeof (*sin);
      memcpy (sin->sin_addr, &address->priv->addr, sizeof (sin->sin_addr));
    }
  else
    {
      addrlen = sizeof (*sin6);
      memcpy (sin6->sin6_addr, &address->priv->addr, sizeof (sin6->sin6_addr));
    }
  if (WSAAddressToString ((LPSOCKADDR) &sa, addrlen, NULL, buffer, &buflen) != 0)
    return NULL;

#else /* !G_OS_WIN32 */

  if (address->priv->family == AF_INET)
    inet_ntop (AF_INET, &address->priv->addr.ipv4, buffer, sizeof (buffer));
  else
    inet_ntop (AF_INET6, &address->priv->addr.ipv6, buffer, sizeof (buffer));
#endif

  return g_strdup (buffer);
}

/**
 * g_inet_address_to_bytes:
 * @address: a #GInetAddress
 *
 * Returns: a pointer to an internal array of the bytes in @address,
 * which should not be modified, stored, or freed.
 */
const guint8 *
g_inet_address_to_bytes (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), NULL);

  return (guint8 *)&address->priv->addr;
}

/**
 * g_inet_address_get_family:
 * @address: a #GInetAddress
 *
 * Returns: @address's family
 */
GInetAddressFamily
g_inet_address_get_family (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  return address->priv->family;
}

/**
 * g_inet_address_is_any:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is the any address. See
 * g_inet4_address_new_any() and g_inet6_address_new_any()
 */
gboolean
g_inet_address_is_any (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      guint32 addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      return addr4 == INADDR_ANY;
    }
  else
    return IN6_IS_ADDR_UNSPECIFIED (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_loopback:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is the loopback address. See
 * g_inet4_address_new_loopback() and g_inet6_address_new_loopback()
 */
gboolean
g_inet_address_is_loopback (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      guint32 addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 127.0.0.0/8 */
      return ((addr4 & 0xff000000) == 0x7f000000);
    }
  else
    return IN6_IS_ADDR_LOOPBACK (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_link_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a link-local address.
 */
gboolean
g_inet_address_is_link_local (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      guint32 addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 169.254.0.0/16 */
      return ((addr4 & 0xffff0000) == 0xa9fe0000);
    }
  else
    return IN6_IS_ADDR_LINKLOCAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_site_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a site-local address.
 */
gboolean
g_inet_address_is_site_local (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      guint32 addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      /* 10.0.0.0/8, 172.16.0.0/12, 192.168.0.0/16 */
      return ((addr4 & 0xff000000) == 0x0a000000 ||
	      (addr4 & 0xfff00000) == 0xac100000 ||
	      (addr4 & 0xffff0000) == 0xc0a80000);
    }
  else
    return IN6_IS_ADDR_SITELOCAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_multicast:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a multicast address.
 */
gboolean
g_inet_address_is_multicast (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    {
      guint32 addr4 = g_ntohl (address->priv->addr.ipv4.s_addr);

      return IN_MULTICAST (addr4);
    }
  else
    return IN6_IS_ADDR_MULTICAST (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_mc_global:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a global multicast address.
 */
gboolean
g_inet_address_is_mc_global (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
    return IN6_IS_ADDR_MC_GLOBAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_mc_link_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a link-local multicast address.
 */
gboolean
g_inet_address_is_mc_link_local (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
    return IN6_IS_ADDR_MC_LINKLOCAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_mc_node_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a node-local multicast address.
 */
gboolean
g_inet_address_is_mc_node_local (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
    return IN6_IS_ADDR_MC_NODELOCAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_mc_org_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is an organization-local multicast address.
 */
gboolean
g_inet_address_is_mc_org_local  (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
    return IN6_IS_ADDR_MC_ORGLOCAL (&address->priv->addr.ipv6.s6_addr);
}

/**
 * g_inet_address_is_mc_site_local:
 * @address: a #GInetAddress
 *
 * Returns: %TRUE is @address is a site-local multicast address.
 */
gboolean
g_inet_address_is_mc_site_local (GInetAddress *address)
{
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  if (address->priv->family == AF_INET)
    return FALSE;
  else
    return IN6_IS_ADDR_MC_SITELOCAL (&address->priv->addr.ipv6.s6_addr);
}

#define __G_INET_ADDRESS_C__
#include "gioaliasdef.c"
