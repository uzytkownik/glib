/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gnetworkaddress.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gresolverprivate.h"
#include "gsimpleasyncresult.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"

#include <string.h>

#include "gioalias.h"

/**
 * SECTION:gnetworkaddress
 * @short_description: IPv4 and IPv6 network address support
 * @include: gio/gio.h
 *
 * IPv4 and IPv6 network address support. Generally you will use
 * %GResolver to create %GNetworkAddress<!-- -->es. For example,
 * a network client might have code something like:
 *
 * |[
 * static MyConnection *
 * my_connect_to_host (const char    *host,
 *                     guint16        port,
 *                     GCancellable  *cancellable,
 *                     GError       **error)
 * {
 *   GResolver *resolver;
 *   GNetworkAddress *addr;
 *   GInetSocketAddress **sockaddrs;
 *   MyConnection *conn;
 *   int i;
 *
 *   resolver = g_resolver_get_default ();
 *   addr = g_resolver_lookup_name (resolver, host, port,
 *                                  cancellable, error);
 *   g_object_unref (resolver);
 *   if (!addr)
 *     return NULL;
 *
 *   sockaddrs = g_network_address_get_sockaddrs (addr);
 *   for (i = 0; sockaddrs[i]; i++)
 *     {
 *       conn = my_connect (sockaddrs[i], cancellable);
 *       if (conn)
 *         {
 *           g_object_unref (addr);
 *           return conn;
 *         }
 *       else if (g_cancellable_set_error_if_cancelled (cancellable, error))
 *         return NULL;
 *     }
 *
 *   g_set_error (error, MY_ERROR_DOMAIN, MY_ERROR_CODE,
 *                "Could not connect to %s", host);
 *   g_object_unref (addr);
 *   return NULL;
 * }
 * ]|
 **/

/**
 * GNetworkAddress:
 *
 * An IPv4 or IPv6 internet address. #GResolver returns these as the
 * result of resolving a hostname or reverse-resolving an IP address.
 * (If you need to a create a #GNetworkAddress on your own, use
 * g_object_new(), along with the appropriate object properties.)
 *
 * To use a #GNetworkAddress with low-level socket routines, call
 * g_network_address_get_sockaddrs() to get the raw address data.
 **/

struct _GNetworkAddressPrivate {
  gchar *hostname, *ascii_name;
  guint16 port;
  GInetSocketAddress **sockaddrs;
};

enum {
  PROP_0,
  PROP_HOSTNAME,
  PROP_ASCII_NAME,
  PROP_PORT,
  PROP_SOCKADDR,
  PROP_SOCKADDRS,
};

static void g_network_address_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void g_network_address_get_property (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

static void                      g_network_address_connectable_iface_init     (GSocketConnectableIface *iface);
static GSocketAddressEnumerator *g_network_address_connectable_get_enumerator (GSocketConnectable      *connectable);

G_DEFINE_TYPE_WITH_CODE (GNetworkAddress, g_network_address, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_SOCKET_CONNECTABLE,
                                                g_network_address_connectable_iface_init))

static void
g_network_address_finalize (GObject *object)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);
  gint i;

  g_free (addr->priv->hostname);
  g_free (addr->priv->ascii_name);

  if (addr->priv->sockaddrs)
    {
      for (i = 0; addr->priv->sockaddrs[i]; i++)
        g_object_unref (addr->priv->sockaddrs[i]);
      g_free (addr->priv->sockaddrs);
    }

  G_OBJECT_CLASS (g_network_address_parent_class)->finalize (object);
}

static void
g_network_address_class_init (GNetworkAddressClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GNetworkAddressPrivate));

  gobject_class->set_property = g_network_address_set_property;
  gobject_class->get_property = g_network_address_get_property;
  gobject_class->finalize = g_network_address_finalize;

  g_object_class_install_property (gobject_class, PROP_HOSTNAME,
                                   g_param_spec_string ("hostname",
                                                        P_("Hostname"),
                                                        P_("Presentation form of hostname"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_ASCII_NAME,
                                   g_param_spec_string ("ascii-name",
                                                        P_("ASCII Hostname"),
                                                        P_("ASCII form of hostname"),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_PORT,
                                   g_param_spec_uint ("port",
                                                      P_("Port"),
                                                      P_("Network port"),
                                                      0, 65535, 0,
                                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SOCKADDR,
                                   g_param_spec_object ("sockaddr",
                                                        P_("Sockaddr"),
                                                        P_("Used to set a single sockaddr"),
                                                        G_TYPE_INET_SOCKET_ADDRESS,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SOCKADDRS,
                                   g_param_spec_pointer ("sockaddrs",
                                                         P_("Sockaddrs"),
                                                         P_("Sockaddrs for this address, an array of GInetSocketAddress"),
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
g_network_address_connectable_iface_init (GSocketConnectableIface *connectable_iface)
{
  connectable_iface->get_enumerator  = g_network_address_connectable_get_enumerator;
}

static void
g_network_address_init (GNetworkAddress *addr)
{
  addr->priv = G_TYPE_INSTANCE_GET_PRIVATE (addr, G_TYPE_NETWORK_ADDRESS,
                                            GNetworkAddressPrivate);
}

static void g_network_address_set_hostname  (GNetworkAddress     *addr,
                                             const gchar         *hostname);
static void g_network_address_set_sockaddrs (GNetworkAddress     *addr,
                                             GInetSocketAddress **sockaddrs);

static void
g_network_address_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);

  switch (prop_id) 
    {
    case PROP_HOSTNAME:
    case PROP_ASCII_NAME:
      /* Ignore gobject when it tries to set the default value */
      if (!g_value_get_string (value))
        return;

      /* Can only set one or the other */
      g_return_if_fail (addr->priv->hostname == NULL && addr->priv->ascii_name == NULL);

      g_network_address_set_hostname (addr, g_value_get_string (value));
      break;

    case PROP_PORT:
      addr->priv->port = g_value_get_uint (value);
      break;

    case PROP_SOCKADDR:
      /* Ignore gobject when it tries to set the default value */
      if (!g_value_get_object (value))
        return;

      /* Can only set one or the other of sockaddr and sockaddrs */
      g_return_if_fail (addr->priv->sockaddrs == NULL);

      addr->priv->sockaddrs = g_new0 (GInetSocketAddress *, 2);
      addr->priv->sockaddrs[0] = g_value_dup_object (value);
      if (addr->priv->port == 0)
        addr->priv->port = g_inet_socket_address_get_port (addr->priv->sockaddrs[0]);
      break;

    case PROP_SOCKADDRS:
      /* Ignore gobject when it tries to set the default value */
      if (!g_value_get_pointer (value))
        return;

      /* Can only set one or the other of sockaddr and sockaddrs */
      g_return_if_fail (addr->priv->sockaddrs == NULL);

      g_network_address_set_sockaddrs (addr, g_value_get_pointer (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_network_address_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GNetworkAddress *addr = G_NETWORK_ADDRESS (object);

  switch (prop_id)
    { 
    case PROP_HOSTNAME:
      g_value_set_string (value, g_network_address_get_hostname (addr));
      break;

    case PROP_ASCII_NAME:
      g_value_set_string (value, g_network_address_get_ascii_name (addr));
      break;

    case PROP_PORT:
      g_value_set_uint (value, addr->priv->port);
      break;

    case PROP_SOCKADDRS:
      g_value_set_pointer (value, addr->priv->sockaddrs);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_network_address_set_hostname (GNetworkAddress *addr,
                                const gchar     *hostname)
{
  g_return_if_fail (hostname != NULL);

  /* If @addr already has a sockaddr set then we must have been doing
   * reverse-resolution, and so the caller is most likely to want the
   * display hostname. If @addr doesn't have a sockaddr, then we're
   * probably going to do be doing forward resolution, so we'll need
   * the ASCII hostname. We can generate the other version on the fly
   * later if we end up needing it.
   */
  if (addr->priv->sockaddrs)
    addr->priv->hostname = g_hostname_to_unicode (hostname);
  else
    addr->priv->ascii_name = g_hostname_to_ascii (hostname);
}

static void
g_network_address_set_sockaddrs (GNetworkAddress     *addr,
                                 GInetSocketAddress **sockaddrs)
{
  gint n;
  guint16 port;

  g_return_if_fail (sockaddrs != NULL && sockaddrs[0] != NULL);

  if (addr->priv->port == 0)
    port = g_inet_socket_address_get_port (sockaddrs[0]);
  else
    port = 0;

  for (n = 1; sockaddrs[n]; n++)
    ;
  addr->priv->sockaddrs = g_new0 (GInetSocketAddress *, n + 1);

  for (n = 0; sockaddrs[n]; n++)
    {
      if (g_inet_socket_address_get_port (sockaddrs[n]) != port)
        port = 0;
      addr->priv->sockaddrs[n] = g_object_ref (sockaddrs[n]);
    }

  if (port != 0)
    addr->priv->port = port;
}

static void
g_network_address_set_addresses (GNetworkAddress  *addr,
                                 GInetAddress    **addresses)
{
  gint n;

  g_return_if_fail (addresses != NULL && addresses[0] != NULL);

  for (n = 1; addresses[n]; n++)
    ;
  addr->priv->sockaddrs = g_new0 (GInetSocketAddress *, n + 1);

  for (n = 0; addresses[n]; n++)
    {
      addr->priv->sockaddrs[n] = (GInetSocketAddress *)
        g_inet_socket_address_new (addresses[n], addr->priv->port);
    }
}

/**
 * g_network_address_new:
 * @hostname: the hostname
 * @port: the port
 *
 * Creates a new #GNetworkAddress for connecting to the given
 * @hostname and @port. The address will initially be unresolved;
 * use its #GSocketConnectable interface to resolve it.
 *
 * Return value: the new #GNetworkAddress
 **/
GNetworkAddress *
g_network_address_new (const gchar *hostname,
                       guint16      port)
{
  return g_object_new (G_TYPE_NETWORK_ADDRESS,
                       "hostname", hostname,
                       "port", port,
                       NULL);
}

/**
 * g_network_address_get_hostname:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's (UTF-8) hostname. This is the version of the hostname
 * that should be presented to the user; see also
 * g_network_address_get_ascii_name().
 *
 * Return value: @addr's display hostname, or %NULL if @addr has
 * only IP addresses
 *
 * Since: 2.20
 **/
const gchar *
g_network_address_get_hostname (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), NULL);

  if (!addr->priv->hostname && addr->priv->ascii_name)
    addr->priv->hostname = g_hostname_to_unicode (addr->priv->ascii_name);
  return addr->priv->hostname;
}

/**
 * g_network_address_get_ascii_name:
 * @addr: a #GNetworkAddress
 *
 * Gets an ASCII version of @addr's hostname; either the hostname
 * itself for non-Unicode hostnames, or else the ASCII-Compatible
 * Encoding of the name for Unicode names. This is the version of the
 * hostname that can be used in non-IDN-aware network protcols. See
 * also g_network_address_get_hostname().
 *
 * Return value: @addr's ASCII hostname, or %NULL if @addr has
 * only IP addresses
 *
 * Since: 2.20
 **/
const gchar *
g_network_address_get_ascii_name (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), NULL);

  if (!addr->priv->ascii_name && addr->priv->hostname)
    addr->priv->ascii_name = g_hostname_to_ascii (addr->priv->hostname);
  return addr->priv->ascii_name;
}

/**
 * g_network_address_get_port:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's port number
 *
 * Return value: @addr's port (which may be %0)
 *
 * Since: 2.20
 **/
guint16
g_network_address_get_port (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), 0);

  return addr->priv->port;
}

/**
 * g_network_address_get_sockaddrs:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's array of #GInetSocketAddress. These are assumed to be
 * in the "correct" order, so you should attempt to connect to the
 * first one first, then the second one if the first one fails, etc,
 * until one succeeds.
 *
 * Return value: a %NULL-terminated array of #GInetSocketAddress, or
 * %NULL if @addr has only a hostname.
 *
 * Since: 2.20
 **/
GInetSocketAddress **
g_network_address_get_sockaddrs (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), NULL);

  return addr->priv->sockaddrs;
}

#define G_TYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (_g_network_address_address_enumerator_get_type ())
#define G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR, GNetworkAddressAddressEnumerator))

typedef struct {
  GSocketAddressEnumerator parent_instance;

  GNetworkAddress *addr;
  gint i;
} GNetworkAddressAddressEnumerator;

typedef struct {
  GSocketAddressEnumeratorClass parent_class;

} GNetworkAddressAddressEnumeratorClass;

G_DEFINE_TYPE (GNetworkAddressAddressEnumerator, _g_network_address_address_enumerator, G_TYPE_SOCKET_ADDRESS_ENUMERATOR)

static void
g_network_address_address_enumerator_finalize (GObject *object)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (object);

  g_object_unref (addr_enum->addr);

  G_OBJECT_CLASS (_g_network_address_address_enumerator_parent_class)->finalize (object);
}

static GSocketAddress *
g_network_address_address_enumerator_get_next (GSocketAddressEnumerator  *enumerator,
                                               GCancellable              *cancellable,
                                               GError                   **error)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (enumerator);

  if (!addr_enum->addr->priv->sockaddrs)
    {
      GResolver *resolver = g_resolver_get_default ();
      GInetAddress **addresses;

      addresses = g_resolver_lookup_by_name (resolver,
                                             g_network_address_get_ascii_name (addr_enum->addr),
                                             cancellable, error);
      if (!addresses)
        {
          g_object_unref (resolver);
          return NULL;
        }

      g_network_address_set_addresses (addr_enum->addr, addresses);
      g_resolver_free_addresses (resolver, addresses);
      g_object_unref (resolver);
    }

  if (!addr_enum->addr->priv->sockaddrs[addr_enum->i])
    return NULL;
  else
    return (GSocketAddress *)addr_enum->addr->priv->sockaddrs[addr_enum->i++];
}

static void
got_addresses (GObject      *source_object,
               GAsyncResult *result,
               gpointer      user_data)
{
  GSimpleAsyncResult *simple = user_data;
  GNetworkAddressAddressEnumerator *addr_enum =
    g_simple_async_result_get_op_res_gpointer (simple);
  GResolver *resolver = G_RESOLVER (source_object);
  GInetAddress **addresses;
  GError *error = NULL;

  addresses = g_resolver_lookup_by_name_finish (resolver, result, &error);
  if (!addr_enum->addr->priv->sockaddrs)
    {
      if (error)
        {
          g_simple_async_result_set_from_error (simple, error);
          g_error_free (error);
        }
      else
        g_network_address_set_addresses (addr_enum->addr, addresses);
    }
  else if (error)
    g_error_free (error);

  g_resolver_free_addresses (resolver, addresses);
  g_object_unref (resolver);

  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

static void
g_network_address_address_enumerator_get_next_async (GSocketAddressEnumerator  *enumerator,
                                                     GCancellable              *cancellable,
                                                     GAsyncReadyCallback        callback,
                                                     gpointer                   user_data)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (enumerator);
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new (G_OBJECT (enumerator),
                                      callback, user_data,
                                      g_network_address_address_enumerator_get_next_async);

  if (!addr_enum->addr->priv->sockaddrs)
    {
      GResolver *resolver = g_resolver_get_default ();

      g_simple_async_result_set_op_res_gpointer (simple, g_object_ref (addr_enum), g_object_unref);
      g_resolver_lookup_by_name_async (resolver,
                                       g_network_address_get_ascii_name (addr_enum->addr),
                                       cancellable,
                                       got_addresses, simple);
    }
  else
    {
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
    }
}

static GSocketAddress *
g_network_address_address_enumerator_get_next_finish (GSocketAddressEnumerator  *enumerator,
                                                      GAsyncResult              *result,
                                                      GError                   **error)
{
  GNetworkAddressAddressEnumerator *addr_enum =
    G_NETWORK_ADDRESS_ADDRESS_ENUMERATOR (enumerator);
  GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else if (!addr_enum->addr->priv->sockaddrs[addr_enum->i])
    return NULL;
  else
    return (GSocketAddress *)addr_enum->addr->priv->sockaddrs[addr_enum->i++];
}

static void
_g_network_address_address_enumerator_init (GNetworkAddressAddressEnumerator *enumerator)
{
}

static void
_g_network_address_address_enumerator_class_init (GNetworkAddressAddressEnumeratorClass *addrenum_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (addrenum_class);
  GSocketAddressEnumeratorClass *enumerator_class =
    G_SOCKET_ADDRESS_ENUMERATOR_CLASS (addrenum_class);

  enumerator_class->get_next = g_network_address_address_enumerator_get_next;
  enumerator_class->get_next_async = g_network_address_address_enumerator_get_next_async;
  enumerator_class->get_next_finish = g_network_address_address_enumerator_get_next_finish;
  object_class->finalize = g_network_address_address_enumerator_finalize;
}

static GSocketAddressEnumerator *
g_network_address_connectable_get_enumerator (GSocketConnectable *connectable)
{
  GNetworkAddressAddressEnumerator *addr_enum;

  addr_enum = g_object_new (G_TYPE_NETWORK_ADDRESS_ADDRESS_ENUMERATOR, NULL);
  addr_enum->addr = g_object_ref (connectable);

  return (GSocketAddressEnumerator *)addr_enum;
}

#define __G_NETWORK_ADDRESS_C__
#include "gioaliasdef.c"
