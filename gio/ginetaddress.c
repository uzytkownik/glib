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
#include "ginet4address.h"
#include "ginet6address.h"

#include "gioalias.h"

/**
 * SECTION:ginetaddress
 * @short_description: Base class for IPv4/IPv6 addresses
 * @see_also: #GInet4Address, #GInet6Address
 *
 * #GInetAddress represents an IPv4 or IPv6 internet address.
 **/

G_DEFINE_ABSTRACT_TYPE (GInetAddress, g_inet_address, G_TYPE_OBJECT);

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

static void
g_inet_address_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
g_inet_address_class_init (GInetAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->get_property = g_inet_address_get_property;

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
  GInetAddress *addr;

  addr = (GInetAddress *)g_inet4_address_from_string (string);
  if (addr)
    return addr;
  else
    return (GInetAddress *)g_inet6_address_from_string (string);
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
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), NULL);

  return G_INET_ADDRESS_GET_CLASS (address)->to_string (address);
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

  return G_INET_ADDRESS_GET_CLASS (address)->to_bytes (address);
}

static gboolean
get_boolean_property (GInetAddress *address, const gchar *property)
{
  gboolean value;

  g_return_val_if_fail (G_IS_INET_ADDRESS (address), FALSE);

  g_object_get (address, property, &value, NULL);

  return value;
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
  return get_boolean_property (address, "is-any");
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
  return get_boolean_property (address, "is-loopback");
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
  return get_boolean_property (address, "is-link-local");
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
  return get_boolean_property (address, "is-site-local");
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
  return get_boolean_property (address, "is-multicast");
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
  return get_boolean_property (address, "is-mc-global");
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
  return get_boolean_property (address, "is-mc-link-local");
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
  return get_boolean_property (address, "is-mc-node-local");
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
  return get_boolean_property (address, "is-mc-org-local");
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
  return get_boolean_property (address, "is-mc-site-local");
}

#define __G_INET_ADDRESS_C__
#include "gioaliasdef.c"
