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
#include "gresolverprivate.h"
#include "ghostutils.h"

#include <string.h>

#include "gioalias.h"

/**
 * SECTION:gnetworkaddress
 * @short_description: IPv4 and IPv6 network address support
 * @include: gio/gio.h
 *
 * IPv4 and IPv6 network address support
 **/

static GResolverError
g_resolver_error_from_addrinfo_error (gint err)
{
  switch (err)
    {
    case EAI_FAIL:
    case EAI_NODATA:
    case EAI_NONAME:
      return G_RESOLVER_ERROR_NOT_FOUND;

    case EAI_AGAIN:
      return G_RESOLVER_ERROR_TEMPORARY_FAILURE;

    default:
      return G_RESOLVER_ERROR_INTERNAL;
    }
}

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
  gushort port;
  GSockaddr **sockaddrs;
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

G_DEFINE_TYPE (GNetworkAddress, g_network_address, G_TYPE_OBJECT);

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
        g_sockaddr_free (addr->priv->sockaddrs[i]);
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
                                   g_param_spec_boxed ("sockaddr",
                                                       P_("Sockaddr"),
                                                       P_("Used to set a single sockaddr"),
                                                       G_TYPE_SOCKADDR,
                                                       G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_SOCKADDRS,
                                   g_param_spec_pointer ("sockaddrs",
                                                         P_("Sockaddrs"),
                                                         P_("Sockaddrs for this address, an array of GSockaddr"),
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
g_network_address_init (GNetworkAddress *addr)
{
  addr->priv = G_TYPE_INSTANCE_GET_PRIVATE (addr, G_TYPE_NETWORK_ADDRESS,
                                            GNetworkAddressPrivate);
}

static void g_network_address_set_hostname  (GNetworkAddress  *addr,
                                             const gchar      *hostname);
static void g_network_address_set_sockaddrs (GNetworkAddress  *addr,
                                             GSockaddr       **sockaddrs);

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
      if (!g_value_get_boxed (value))
        return;

      /* Can only set one or the other of sockaddr and sockaddrs */
      g_return_if_fail (addr->priv->sockaddrs == NULL);

      addr->priv->sockaddrs = g_new0 (GSockaddr *, 2);
      addr->priv->sockaddrs[0] = g_value_dup_boxed (value);
      if (addr->priv->port == 0)
        addr->priv->port = g_sockaddr_get_port (addr->priv->sockaddrs[0]);
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

/* A gio-internal method to set the hostname from a getnameinfo()
 * response.
 */
gboolean
g_network_address_set_from_nameinfo (GNetworkAddress  *addr,
                                     const gchar      *name,
                                     gint              gni_retval,
                                     GError          **error)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), FALSE);
  g_return_val_if_fail (addr->priv->sockaddrs != NULL, FALSE);

  if (gni_retval != 0)
    {
      gchar *phys;

      phys = g_sockaddr_to_string (addr->priv->sockaddrs[0]);
      g_set_error (error, G_RESOLVER_ERROR,
                   g_resolver_error_from_addrinfo_error (gni_retval),
                   _("Error reverse-resolving '%s': %s"),
                   phys ? phys : "(unknown)", gai_strerror (gni_retval));
      g_free (phys);
      return FALSE;
    }

  g_network_address_set_hostname (addr, name);
  return TRUE;
}

static void
g_network_address_set_sockaddrs (GNetworkAddress  *addr,
                                 GSockaddr       **sockaddrs)
{
  gint n;
  gushort port;

  g_return_if_fail (sockaddrs != NULL && sockaddrs[0] != NULL);

  if (addr->priv->port == 0)
    port = g_sockaddr_get_port (sockaddrs[0]);
  else
    port = 0;

  for (n = 1; sockaddrs[n]; n++)
    ;
  addr->priv->sockaddrs = g_new0 (GSockaddr *, n + 1);

  for (n = 0; sockaddrs[n]; n++)
    {
      if (g_sockaddr_get_port (sockaddrs[n]) != port)
        port = 0;
      addr->priv->sockaddrs[n] = g_sockaddr_copy (sockaddrs[n]);
    }

  if (port != 0)
    addr->priv->port = port;
}

/* Private method to prepare args to getaddrinfo() */
void
g_network_address_get_addrinfo_hints (GNetworkAddress *addr,
                                      gchar            service[8],
                                      struct addrinfo *hints)
{
  g_snprintf (service, sizeof (service), "%u", addr->priv->port);
  memset (hints, 0, sizeof (struct addrinfo));
#ifdef AI_NUMERICSERV
  hints->ai_flags = AI_NUMERICSERV;
#endif
#ifdef AI_ADDRCONFIG
  hints->ai_flags |= AI_ADDRCONFIG;
#endif
  /* These two don't actually matter, they just get copied into the
   * returned addrinfo structures (and then we ignore them). But if
   * we leave them unset, we'll get back duplicate answers.
   */
  hints->ai_socktype = SOCK_STREAM;
  hints->ai_protocol = IPPROTO_TCP;
}

/* A gio-internal method that sets sockaddrs from a getaddrinfo()
 * response.
 */
gboolean
g_network_address_set_from_addrinfo  (GNetworkAddress  *addr,
                                      struct addrinfo  *res,
                                      gint              gai_retval,
                                      GError          **error)
{
  struct addrinfo *ai;
  GSockaddr **sockaddrs;
  gint n;

  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), FALSE);

  if (gai_retval != 0)
    {
      g_set_error (error, G_RESOLVER_ERROR,
		   g_resolver_error_from_addrinfo_error (gai_retval),
		   _("Error resolving '%s': %s"),
		   g_network_address_get_hostname (addr),
                   gai_strerror (gai_retval));
      return FALSE;
    }

  g_return_val_if_fail (res != NULL, FALSE);

  for (ai = res, n = 0; ai; ai = ai->ai_next, n++)
    ;
  sockaddrs = alloca ((n + 1) * sizeof (GSockaddr *));
  for (ai = res, n = 0; ai; ai = ai->ai_next, n++)
    sockaddrs[n] = (GSockaddr *)ai->ai_addr;
  sockaddrs[n] = NULL;

  g_network_address_set_sockaddrs (addr, sockaddrs);
  return TRUE;
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
gushort
g_network_address_get_port (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), 0);

  return addr->priv->port;
}

/**
 * g_network_address_get_sockaddrs:
 * @addr: a #GNetworkAddress
 *
 * Gets @addr's array of #GSockaddr. These are assumed to be in the
 * "correct" order, so you should attempt to connect to the first one
 * first, then the second one if the first one fails, etc, until one
 * succeeds.
 *
 * Return value: a %NULL-terminated array of #GSockaddr, or %NULL if
 * @addr has only a hostname.
 *
 * Since: 2.20
 **/
GSockaddr **
g_network_address_get_sockaddrs (GNetworkAddress *addr)
{
  g_return_val_if_fail (G_IS_NETWORK_ADDRESS (addr), NULL);

  return addr->priv->sockaddrs;
}


/**
 * GSockaddr:
 *
 * A type representing an IP address and port.
 *
 * #GSockaddr is simply a typedef for the system <literal><type>struct
 * sockaddr</type></literal> type, and you may freely cast a
 * #GSockaddr to a <literal><type>struct sockaddr *</type></literal>
 * in order to pass it to a low-level socket routine, or cast a
 * pointer to a <literal><type>struct sockaddr_in</type></literal>, or
 * <literal><type>struct sockaddr_in6</type></literal> to a #GSockaddr
 * in order to pass it to a gio routine. (However, #GSockaddr<!-- -->s
 * created with g_sockaddr_new_from_string() or g_sockaddr_copy() must
 * be freed with g_sockaddr_free(), and other #GSockaddr<!-- -->s must
 * not be.)
 */

GType
g_sockaddr_get_type (void)
{
  static volatile gsize type_volatile = 0;

  if (g_once_init_enter (&type_volatile))
    {
      GType type = g_boxed_type_register_static (
                        g_intern_static_string ("GSockaddr"),
			(GBoxedCopyFunc) g_sockaddr_copy,
			(GBoxedFreeFunc) g_sockaddr_free);
      g_once_init_leave (&type_volatile, type);
    }
  return type_volatile;
}

/**
 * g_sockaddr_new_from_string:
 * @ip_addr: an IPv4 or IPv6 numeric address
 * @port: port to use in the #GSockaddr, or %0 if you don't care
 *
 * Parses @ip_addr and returns a corresponding #GSockaddr.
 *
 * Return value: a new #GSockaddr, or %NULL if @ip_addr is not an
 * IPv4 or IPv6 address.
 *
 * Since: 2.20
 **/
GSockaddr *
g_sockaddr_new_from_string (const gchar *ip_addr,
                            gushort      port)
{
#ifdef G_OS_WIN32
  gint len;
#endif
  struct sockaddr_storage sa;
  struct sockaddr_in *sin = (struct sockaddr_in *)&sa;
  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)&sa;

  memset (&sa, 0, sizeof (sa));
  g_resolver_os_init ();

#ifdef G_OS_WIN32
  len = sizeof (sa);
  if (WSAStringToAddress ((LPTSTR) ip_addr, AF_INET, NULL, (LPSOCKADDR) &sa, &len) == 0)
    sin->sin_port = port;
  else if (WSAStringToAddress ((LPTSTR) ip_addr, AF_INET6, NULL, (LPSOCKADDR) &sa, &len) == 0)
    sin6->sin6_port = port;
  else
    return NULL;

#else /* !G_OS_WIN32 */

  if (inet_pton (AF_INET, ip_addr, &sin->sin_addr) > 0)
    {
      sin->sin_family = AF_INET;
      sin->sin_port = port;
    }
  else if (inet_pton (AF_INET6, ip_addr, &sin6->sin6_addr) > 0)
    {
      sin6->sin6_family = AF_INET6;
      sin6->sin6_port = port;
    }
  else
    return NULL;
#endif

  return g_sockaddr_copy ((GSockaddr *)&sa);
}

/**
 * g_sockaddr_copy:
 * @sockaddr: a #GSockaddr
 *
 * Copies @sockaddr
 *
 * Return value: a copy of @sockaddr
 *
 * Since: 2.20
 **/
GSockaddr *
g_sockaddr_copy (GSockaddr *sockaddr)
{
  return g_slice_copy (g_sockaddr_size (sockaddr), sockaddr);
}

/**
 * g_sockaddr_free:
 * @sockaddr: a #GSockaddr
 *
 * Frees @sockaddr
 *
 * Since: 2.20
 **/
void
g_sockaddr_free (GSockaddr *sockaddr)
{
  g_slice_free1 (g_sockaddr_size (sockaddr), sockaddr);
}

/**
 * g_sockaddr_get_port:
 * @sockaddr: a #GSockaddr
 *
 * Gets @sockaddr's port number
 *
 * Return value: @sockaddr's port (which may be %0)
 *
 * Since: 2.20
 **/
gushort
g_sockaddr_get_port (GSockaddr *sockaddr)
{
  struct sockaddr *sa = (struct sockaddr *)sockaddr;

  if (sa->sa_family == AF_INET)
    return ((struct sockaddr_in *)sa)->sin_port;
  else if (sa->sa_family == AF_INET6)
    return ((struct sockaddr_in6 *)sa)->sin6_port;
  else
    g_return_val_if_reached (0);
}

/**
 * g_sockaddr_to_string:
 * @sockaddr: a #GSockaddr
 *
 * Converts @sockaddr to string form (ignoring its port)
 *
 * Return value: the string form of @sockaddr, which must be freed
 *
 * Since: 2.20
 **/
gchar *
g_sockaddr_to_string (GSockaddr *sockaddr)
{
  struct sockaddr *sa = (struct sockaddr *)sockaddr;
  gchar buffer[INET6_ADDRSTRLEN];
#ifdef G_OS_WIN32
  DWORD buflen = sizeof (buffer);
#else
  struct sockaddr_in *sin = (struct sockaddr_in *)sockaddr;
  struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sockaddr;
#endif

  g_resolver_os_init ();

#ifdef G_OS_WIN32
  if (WSAAddressToString ((LPSOCKADDR) sa, g_sockaddr_size (sockaddr), NULL,
			  buffer, &buflen) == 0)
    return g_strdup (buffer);
  else
    return NULL;

#else /* !G_OS_WIN32 */

  switch (sa->sa_family)
    {
    case AF_INET:
      inet_ntop (AF_INET, &sin->sin_addr, buffer, sizeof (buffer));
      return g_strdup (buffer);

    case AF_INET6:
      inet_ntop (AF_INET6, &sin6->sin6_addr, buffer, sizeof (buffer));
      return g_strdup (buffer);

    default:
      return NULL;
    }
#endif
}

/**
 * g_sockaddr_size:
 * @sockaddr: a #GSockaddr
 *
 * Determines the size of @sockaddr, which is needed as a parameter to
 * some standard library routines like bind() or connect().
 *
 * Return value: the size of @sockaddr
 *
 * Since: 2.20
 **/
gsize
g_sockaddr_size (GSockaddr *sockaddr)
{
  struct sockaddr *sa = (struct sockaddr *)sockaddr;

  if (sa->sa_family == AF_INET)
    return sizeof (struct sockaddr_in);
  else if (sa->sa_family == AF_INET6)
    return sizeof (struct sockaddr_in6);
  else
    g_return_val_if_reached (0);
}

#define __G_NETWORK_ADDRESS_C__
#include "gioaliasdef.c"
