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

#include "gresolver.h"
#include "gresolverprivate.h"
#include "gasyncresult.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gnetworkaddress.h"
#include "gnetworkservice.h"
#include "gsimpleasyncresult.h"

#ifdef G_OS_UNIX
#include "gunixresolver.h"
#endif
#ifdef G_OS_WIN32
#include "gwin32resolver.h"
#endif

#include <stdlib.h>

#include "gioalias.h"

/**
 * SECTION:gresolver
 * @short_description: Asynchronous and cancellable DNS resolver
 * @include: gio/gio.h
 *
 * #GResolver provides cancellable synchronous and asynchronous DNS
 * resolution, for hostnames (g_resolver_lookup_by_address(),
 * g_resolver_lookup_by_name() and their async variants) and SRV
 * (service) records (g_resolver_lookup_service()).
 **/

G_DEFINE_TYPE (GResolver, g_resolver, G_TYPE_OBJECT)

static void
g_resolver_class_init (GResolverClass *resolver_class)
{
  g_resolver_os_init ();
}

static void
g_resolver_init (GResolver *resolver)
{
  ;
}

void
g_resolver_os_init (void)
{
#ifdef G_OS_WIN32
  WSADATA wsadata;

  if (WSAStartup (MAKEWORD (2, 0), &wsadata) != 0)
    g_error ("Windows Sockets could not be initialized");
#endif

#ifdef AI_ADDRCONFIG
  g_resolver_addrinfo_hints.ai_flags |= AI_ADDRCONFIG;
#endif
  /* These two don't actually matter, they just get copied into the
   * returned addrinfo structures (and then we ignore them). But if
   * we leave them unset, we'll get back duplicate answers.
   */
  g_resolver_addrinfo_hints.ai_socktype = SOCK_STREAM;
  g_resolver_addrinfo_hints.ai_protocol = IPPROTO_TCP;
}

static GResolver *default_resolver;

/**
 * g_resolver_get_default:
 *
 * Gets the default #GResolver.
 *
 * Return value: the #GResolver. You should unref it when you are done
 * with it.
 *
 * Since: 2.20
 **/
GResolver *
g_resolver_get_default (void)
{
  if (!default_resolver)
    {
      if (g_thread_supported ())
        default_resolver = g_object_new (G_TYPE_THREADED_RESOLVER, NULL);
      else
        {
#if defined(G_OS_UNIX)
          default_resolver = g_object_new (G_TYPE_UNIX_RESOLVER, NULL);
#elif defined(G_OS_WIN32)
          default_resolver = g_object_new (G_TYPE_WIN32_RESOLVER, NULL);
#endif
        }
    }

  return g_object_ref (default_resolver);
}

/**
 * g_resolver_set_default:
 * @resolver: the new default #GResolver
 *
 * Sets @resolver to be the application's default resolver (reffing
 * @resolver, and unreffing the previous default resolver, if any).
 * Future calls to g_resolver_get_default() will return this resolver.
 *
 * This can be used if an applications wants to perform any sort of
 * DNS caching or "pinning"; it can implement its own #GResolver that
 * calls the original default resolver for DNS operations, and
 * implements its own cache policies on top of that, and then set
 * itself as the default resolver for all later code to use.
 **/
void
g_resolver_set_default (GResolver *resolver)
{
  if (default_resolver)
    g_object_unref (default_resolver);
  default_resolver = g_object_ref (resolver);
}


/**
 * g_resolver_lookup_by_name:
 * @resolver: a #GResolver
 * @hostname: the hostname to look up
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronously resolves @hostname to determine its associated IP
 * address(es). @hostname may be an ASCII-only or UTF-8 hostname, or
 * the textual form of an IP address (in which case this just becomes
 * a wrapper around g_inet_address_from_string()).
 *
 * On success, g_resolver_lookup_by_name() will return a
 * %NULL-terminated array of #GInetAddress, sorted in order of
 * preference. (That is, you should attempt to connect to the first
 * address first, then the second if the first fails, etc.) You must
 * unref each address and free the array when you are done with it.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to a
 * value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a %NULL-terminated array of #GInetAddress, or %NULL
 * on error.
 *
 * Since: 2.20
 **/
GInetAddress **
g_resolver_lookup_by_name (GResolver     *resolver,
                           const gchar   *hostname,
                           GCancellable  *cancellable,
                           GError       **error)
{
  GInetAddress **addrs, *addr;
  gchar *ascii_hostname = NULL;

  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (hostname != NULL, NULL);

  /* Check if @hostname is just an IP address */
  addr = g_inet_address_from_string (hostname);
  if (addr)
    {
      addrs = g_new0 (GInetAddress *, 2);
      addrs[0] = addr;
      return addrs;
    }

  if (g_hostname_is_non_ascii (hostname))
    hostname = ascii_hostname = g_hostname_to_ascii (hostname);

  addrs = G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_name (resolver, hostname, cancellable, error);

  g_free (ascii_hostname);
  return addrs;
}

/**
 * g_resolver_lookup_by_name_async:
 * @resolver: a #GResolver
 * @hostname: the hostname to look up the address of
 * @cancellable: a #GCancellable, or %NULL
 * @callback: callback to call after resolution completes
 * @user_data: data for @callback
 *
 * Begins asynchronously resolving @hostname to determine its
 * associated IP address(es), and eventually calls @callback, which
 * must call g_resolver_lookup_by_name_finish() to get the result. See
 * g_resolver_lookup_by_name() for more details.
 *
 * Since: 2.20
 **/
void
g_resolver_lookup_by_name_async (GResolver           *resolver,
                                 const gchar         *hostname,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  GInetAddress *addr;
  gchar *ascii_hostname = NULL;

  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (hostname != NULL);

  /* Check if @hostname is just an IP address */
  addr = g_inet_address_from_string (hostname);
  if (addr)
    {
      GSimpleAsyncResult *simple;
      GInetAddress **addrs;

      simple = g_simple_async_result_new (G_OBJECT (resolver),
                                          callback, user_data,
                                          g_resolver_lookup_by_name_async);

      addrs = g_new0 (GInetAddress *, 2);
      addrs[0] = addr;
      g_simple_async_result_set_op_res_gpointer (simple, addrs, NULL);
      g_simple_async_result_complete_in_idle (simple);
      g_object_unref (simple);
      return;
    }

  if (g_hostname_is_non_ascii (hostname))
    hostname = ascii_hostname = g_hostname_to_ascii (hostname);

  G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_name_async (resolver, hostname, cancellable, callback, user_data);

  g_free (ascii_hostname);
}

/**
 * g_resolver_lookup_by_name_finish:
 * @resolver: a #GResolver
 * @result: the result passed to your #GAsyncReadyCallback
 * @error: return location for a #GError, or %NULL
 *
 * Retrieves the result of a call to
 * g_resolver_lookup_by_name_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Return value: a %NULL-terminated array of #GInetAddress, or %NULL
 * on error. See g_resolver_lookup_by_name() for more details.
 *
 * Since: 2.20
 **/
GInetAddress **
g_resolver_lookup_by_name_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);

  /* Handle the stringified-IP-addr case */
  if (G_IS_SIMPLE_ASYNC_RESULT (result))
    {
      GSimpleAsyncResult *simple = G_SIMPLE_ASYNC_RESULT (result);

      if (g_simple_async_result_get_source_tag (simple) == g_resolver_lookup_by_name_async)
        {
          g_simple_async_result_propagate_error (simple, error);
          return g_simple_async_result_get_op_res_gpointer (simple);
        }
    }

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_name_finish (resolver, result, error);
}


/**
 * g_resolver_lookup_by_address:
 * @resolver: a #GResolver
 * @address: the address to reverse-resolve
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronously reverse-resolves @address to determine its
 * associated hostname.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a hostname (either ASCII-only, or in ASCII-encoded
 * form), or %NULL on error.
 *
 * Since: 2.20
 **/
gchar *
g_resolver_lookup_by_address (GResolver     *resolver,
                              GInetAddress  *address,
                              GCancellable  *cancellable,
                              GError       **error)
{
  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (G_IS_INET_ADDRESS (address), NULL);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address (resolver, address, cancellable, error);
}

/**
 * g_resolver_lookup_by_address_async:
 * @resolver: a #GResolver
 * @address: the address to reverse-resolve
 * @cancellable: a #GCancellable, or %NULL
 * @callback: callback to call after resolution completes
 * @user_data: data for @callback
 *
 * Begins asynchronously reverse-resolving @address to determine its
 * associated hostname, and eventually calls @callback, which must
 * call g_resolver_lookup_by_address_finish() to get the final result.
 *
 * Since: 2.20
 **/
void
g_resolver_lookup_by_address_async (GResolver           *resolver,
                                    GInetAddress        *address,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (G_IS_INET_ADDRESS (address));

  G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address_async (resolver, address, cancellable, callback, user_data);
}

/**
 * g_resolver_lookup_by_address_finish:
 * @resolver: a #GResolver
 * @result: the result passed to your #GAsyncReadyCallback
 * @error: return location for a #GError, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_by_address_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Return value: a hostname (either ASCII-only, or in ASCII-encoded
 * form), or %NULL on error.
 *
 * Since: 2.20
 **/
gchar *
g_resolver_lookup_by_address_finish (GResolver     *resolver,
                                     GAsyncResult  *result,
                                     GError       **error)
{
  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_by_address_finish (resolver, result, error);
}

static gchar *
g_resolver_get_service_rrname (const char *service,
                               const char *protocol,
                               const char *domain)
{
  gchar *rrname, *ascii_domain = NULL;

  if (g_hostname_is_non_ascii (domain))
    domain = ascii_domain = g_hostname_to_ascii (domain);

  rrname = g_strdup_printf ("_%s._%s.%s", service, protocol, domain);

  g_free (ascii_domain);
  return rrname;
}

/**
 * g_resolver_lookup_service:
 * @resolver: a #GResolver
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronously performs a DNS SRV lookup for the given @service and
 * @protocol in the given @domain and returns a #GNetworkService.
 * @domain may be an ASCII-only or UTF-8 hostname. Note also that the
 * @service and @protocol arguments DO NOT include the leading
 * underscore that appears in the actual DNS entry.
 *
 * On success, g_resolver_lookup_service() will return a
 * %NULL-terminated array of #GSrvTarget, sorted in order of
 * preference. (That is, you should attempt to connect to the first
 * target first, then the second if the first fails, etc.) You must
 * free each target and the array when you are done with it.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a %NULL-terminated array of #GSrvTarget, or %NULL on
 * error.
 *
 * Since: 2.20
 **/
GSrvTarget **
g_resolver_lookup_service (GResolver     *resolver,
                           const gchar   *service,
                           const gchar   *protocol,
                           const gchar   *domain,
                           GCancellable  *cancellable,
                           GError       **error)
{
  GSrvTarget **targets;
  gchar *rrname;

  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (service != NULL, NULL);
  g_return_val_if_fail (protocol != NULL, NULL);
  g_return_val_if_fail (domain != NULL, NULL);

  rrname = g_resolver_get_service_rrname (service, protocol, domain);

  targets = G_RESOLVER_GET_CLASS (resolver)->
    lookup_service (resolver, rrname, cancellable, error);

  g_free (rrname);
  return targets;
}

/**
 * g_resolver_lookup_service_async:
 * @resolver: a #GResolver
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 * @cancellable: a #GCancellable, or %NULL
 * @callback: callback to call after resolution completes
 * @user_data: data for @callback
 *
 * Begins asynchronously performing a DNS SRV lookup for the given
 * @service and @protocol in the given @domain, and eventually calls
 * @callback, which must call g_resolver_lookup_service_finish() to
 * get the final result. See g_resolver_lookup_service() for more
 * details.
 *
 * Since: 2.20
 **/
void
g_resolver_lookup_service_async (GResolver           *resolver,
                                 const gchar         *service,
                                 const gchar         *protocol,
                                 const gchar         *domain,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  gchar *rrname;

  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (service != NULL);
  g_return_if_fail (protocol != NULL);
  g_return_if_fail (domain != NULL);

  rrname = g_resolver_get_service_rrname (service, protocol, domain);

  G_RESOLVER_GET_CLASS (resolver)->
    lookup_service_async (resolver, rrname, cancellable, callback, user_data);

  g_free (rrname);
}

/**
 * g_resolver_lookup_service_finish:
 * @resolver: a #GResolver
 * @result: the result passed to your #GAsyncReadyCallback
 * @error: return location for a #GError, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_service_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Return value: a %NULL-terminated array of #GSrvTarget, or %NULL on
 * error. See g_resolver_lookup_service() for more details.
 *
 * Since: 2.20
 **/
GSrvTarget **
g_resolver_lookup_service_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_service_finish (resolver, result, error);
}


/**
 * g_resolver_error_quark:
 * 
 * Gets the #GResolver Error Quark.
 *
 * Return value: a #GQuark.
 *
 * Since: 2.20
 **/
GQuark
g_resolver_error_quark (void)
{
  return g_quark_from_static_string ("g-resolver-error-quark");
}


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

struct addrinfo g_resolver_addrinfo_hints;

/* Private method to process a getaddrinfo() response. */
GInetAddress **
g_resolver_addresses_from_addrinfo (const char       *hostname,
                                    struct addrinfo  *res,
                                    gint              gai_retval,
                                    GError          **error)
{
  struct addrinfo *ai;
  GInetAddress **addrs;
  GSocketAddress *sockaddr;
  gint n;

  if (gai_retval != 0)
    {
      g_set_error (error, G_RESOLVER_ERROR,
		   g_resolver_error_from_addrinfo_error (gai_retval),
		   _("Error resolving '%s': %s"),
		   hostname, gai_strerror (gai_retval));
      return NULL;
    }

  g_return_val_if_fail (res != NULL, NULL);

  for (ai = res, n = 0; ai; ai = ai->ai_next, n++)
    ;

  addrs = g_new (GInetAddress *, n + 1);
  for (ai = res, n = 0; ai; ai = ai->ai_next)
    {
      sockaddr = g_socket_address_from_native (ai->ai_addr, ai->ai_addrlen);
      if (!sockaddr || !G_IS_INET_SOCKET_ADDRESS (sockaddr))
        continue;

      addrs[n++] = g_object_ref (g_inet_socket_address_get_address ((GInetSocketAddress *)sockaddr));
      g_object_unref (sockaddr);
    }
  addrs[n] = NULL;

  return addrs;
}

/* Private method to set up a getnameinfo() request */
void
g_resolver_address_to_sockaddr (GInetAddress            *address,
                                struct sockaddr_storage *sa,
                                gsize                   *sa_len)
{
  GSocketAddress *isa;

  isa = (GSocketAddress *)g_inet_socket_address_new (address, 0);
  g_socket_address_to_native (isa, (struct sockaddr *)sa);
  *sa_len = g_socket_address_native_size (isa);
  g_object_unref (isa);
}

/* Private method to process a getnameinfo() response. */
char *
g_resolver_name_from_nameinfo (GInetAddress  *address,
                               const gchar   *name,
                               gint           gni_retval,
                               GError       **error)
{
  if (gni_retval != 0)
    {
      gchar *phys;

      phys = g_inet_address_to_string (address);
      g_set_error (error, G_RESOLVER_ERROR,
                   g_resolver_error_from_addrinfo_error (gni_retval),
                   _("Error reverse-resolving '%s': %s"),
                   phys ? phys : "(unknown)", gai_strerror (gni_retval));
      g_free (phys);
      return NULL;
    }

  return g_strdup (name);
}

#if defined(G_OS_UNIX)
/* Private method to process a res_query response into GSrvTargets */
GSrvTarget **
g_resolver_targets_from_res_query (const gchar      *rrname,
                                   guchar           *answer,
                                   gint              len,
                                   gint              herr,
                                   GError          **error)
{
  gint count;
  gchar namebuf[1024];
  guchar *end, *p;
  guint16 type, qclass, rdlength, priority, weight, port;
  guint32 ttl;
  HEADER *header;
  time_t now = time (NULL);
  GSrvTarget *target, **ret;
  GPtrArray *targets;

  if (len < 0)
    {
      GResolverError errnum;
      const gchar *format;

      if (herr == HOST_NOT_FOUND || herr == NO_DATA)
        {
          errnum = G_RESOLVER_ERROR_NOT_FOUND;
          format = _("No service record for '%s'");
        }
      else if (herr == TRY_AGAIN)
        {
          errnum = G_RESOLVER_ERROR_TEMPORARY_FAILURE;
          format = _("Temporarily unable to resolve '%s'");
        }
      else
        {
          errnum = G_RESOLVER_ERROR_INTERNAL;
          format = _("Error resolving '%s'");
        }

      g_set_error (error, G_RESOLVER_ERROR, errnum, format, rrname);
      return NULL;
    }

  targets = g_ptr_array_new ();

  header = (HEADER *)answer;
  p = answer + sizeof (HEADER);
  end = answer + len;

  /* Skip query */
  count = ntohs (header->qdcount);
  while (count-- && p < end)
    {
      p += dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      p += 4;
    }

  /* Read answers */
  count = ntohs (header->ancount);
  while (count-- && p < end)
    {
      p += dn_expand (answer, end, p, namebuf, sizeof (namebuf));
      GETSHORT (type, p);
      GETSHORT (qclass, p);
      GETLONG  (ttl, p);
      GETSHORT (rdlength, p);

      if (type != T_SRV || qclass != C_IN)
        {
          p += rdlength;
          continue;
        }

      GETSHORT (priority, p);
      GETSHORT (weight, p);
      GETSHORT (port, p);
      p += dn_expand (answer, end, p, namebuf, sizeof (namebuf));

      target = g_srv_target_new (namebuf, port, priority, weight, now + ttl);
      g_ptr_array_add (targets, target);
    }

  g_ptr_array_add (targets, NULL);

  ret = (GSrvTarget **) g_ptr_array_free (targets, FALSE);
  g_srv_target_array_sort (ret);
  return ret;
}
#elif defined(G_OS_WIN32)
/* Private method to process a DnsQuery response into GSrvTargets */
GSrvTarget **
g_resolver_targets_from_DnsQuery (const gchar  *rrname,
                                  DNS_STATUS    status,
                                  DNS_RECORD   *results,
                                  GError      **error)
{
  DNS_RECORD *rec;
  GSrvTarget *target, **ret;
  GPtrArray *targets;
  time_t now = time (NULL);

  if (status != ERROR_SUCCESS)
    {
      GResolverError errnum;
      const gchar *format;

      if (status == DNS_ERROR_RCODE_NAME_ERROR)
        {
          errnum = G_RESOLVER_ERROR_NOT_FOUND;
          format = _("No service record for '%s'");
        }
      else if (status == DNS_ERROR_RCODE_SERVER_FAILURE)
        {
          errnum = G_RESOLVER_ERROR_TEMPORARY_FAILURE;
          format = _("Temporarily unable to resolve '%s'");
        }
      else
        {
          errnum = G_RESOLVER_ERROR_INTERNAL;
          format = _("Error resolving '%s'");
        }

      g_set_error (error, G_RESOLVER_ERROR, errnum, format, rrname);
      return NULL;
    }

  targets = g_ptr_array_new ();
  for (rec = results; rec; rec = rec->pNext)
    {
      if (rec->wType != DNS_TYPE_SRV)
        continue;

      target = g_srv_target_new (rec->Data.SRV.pNameTarget,
                                 rec->Data.SRV.wPort,
                                 rec->Data.SRV.wPriority,
                                 rec->Data.SRV.wWeight,
                                 now + rec->dwTtl);
      g_ptr_array_add (targets, target);
    }
  g_ptr_array_add (targets, NULL);

  ret = (GSrvTarget **) g_ptr_array_free (targets, FALSE);
  g_srv_target_array_sort (ret);
  return ret;
}

#endif

#define __G_RESOLVER_C__
#include "gioaliasdef.c"
