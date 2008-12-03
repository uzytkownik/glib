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
 * resolution, for hostnames (g_resolver_lookup_address(),
 * g_resolver_lookup_name() and their async variants) and SRV
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

#ifdef G_OS_WIN32
void
g_resolver_os_init (void)
{
  WSADATA wsadata;
  if (WSAStartup (MAKEWORD (2, 0), &wsadata) != 0)
    g_error ("Windows Sockets could not be initialized");
}
#endif

static GResolver *default_resolver;

/**
 * g_resolver_get_default:
 *
 * Gets the default #GResolver.
 *
 * Return value: the #GResolver. You should unref it when you are done
 * with it.
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

void
g_resolver_set_default (GResolver *resolver)
{
  if (default_resolver)
    g_object_unref (default_resolver);
  default_resolver = g_object_ref (resolver);
}


/**
 * g_resolver_lookup_name:
 * @resolver: a #GResolver
 * @hostname: the hostname to look up
 * @port: the port to use in the returned #GSockAddr<!-- -->s. (Use %0
 * if you don't care.)
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronously resolves @hostname to determine its associated IP
 * address(es) and returns a #GNetworkAddress. (Unless @hostname is
 * the textual form of an IP address, in which case
 * g_resolver_lookup_name() will simply return a #GNetworkAddress with
 * that address and no hostname; it will not try to reverse-resolve
 * the IP address.)
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to a
 * value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a #GNetworkAddress, or %NULL on error
 **/
GNetworkAddress *
g_resolver_lookup_name (GResolver     *resolver,
                        const gchar   *hostname,
                        gushort        port,
                        GCancellable  *cancellable,
                        GError       **error)
{
  GNetworkAddress *addr;
  GSockaddr *sockaddr;

  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (hostname != NULL, NULL);

  /* Check if @hostname is just an IP address */
  sockaddr = g_sockaddr_new_from_string (hostname, port);
  if (sockaddr)
    {
      addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                           "sockaddr", sockaddr,
                           NULL);
      g_sockaddr_free (sockaddr);
      return addr;
    }

  addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                       "hostname", hostname,
                       "port", port,
                       NULL);

  if (!G_RESOLVER_GET_CLASS (resolver)->lookup_name (resolver, addr, cancellable, error))
    {
      g_object_unref (addr);
      return NULL;
    }
  return addr;
}

static void
lookup_name_async_callback (GObject      *source,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  GResolver *resolver = G_RESOLVER (source);
  GSimpleAsyncResult *simple = user_data;
  GError *error = NULL;

  if (!G_RESOLVER_GET_CLASS (resolver)->
      lookup_name_finish (resolver, result, &error))
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
}

/**
 * g_resolver_lookup_name_async:
 * @resolver: a #GResolver
 * @hostname: the hostname to look up the address of
 * @port: the port to use in the returned #GSockAddr<!-- -->s. (Use %0
 * if you don't care.)
 * @cancellable: a #GCancellable, or %NULL
 * @callback: callback to call after resolution completes
 * @user_data: data for @callback
 *
 * Begins asynchronously resolving @hostname to determine its
 * associated IP address(es), and eventually calls @callback, which
 * must call g_resolver_lookup_name_finish() to get the result.
 **/
void
g_resolver_lookup_name_async (GResolver           *resolver,
                              const gchar         *hostname,
                              gushort              port,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  GNetworkAddress *addr;
  GSimpleAsyncResult *simple;
  GSockaddr *sockaddr;

  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (hostname != NULL);

  simple = g_simple_async_result_new (G_OBJECT (resolver), callback, user_data,
                                      g_resolver_lookup_name_async);

  /* Check if @hostname is just an IP address */
  sockaddr = g_sockaddr_new_from_string (hostname, port);
  if (sockaddr)
    {
      addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                           "sockaddr", sockaddr,
                           NULL);
      g_sockaddr_free (sockaddr);

      g_simple_async_result_set_op_res_gpointer (simple, addr, g_object_unref);
      g_simple_async_result_complete_in_idle (simple);
      return;
    }

  addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                       "hostname", hostname,
                       "port", port,
                       NULL);
  g_simple_async_result_set_op_res_gpointer (simple, addr, g_object_unref);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_name_async (resolver, addr, cancellable,
                       lookup_name_async_callback, simple);
}

/**
 * g_resolver_lookup_name_finish:
 * @resolver: a #GResolver
 * @result: the result passed to your #GAsyncReadyCallback
 * @error: return location for a #GError, or %NULL
 *
 * Retrieves the result of a call to
 * g_resolver_lookup_name_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Return value: a #GNetworkAddress, or %NULL on error
 **/
GNetworkAddress *
g_resolver_lookup_name_finish (GResolver     *resolver,
                               GAsyncResult  *result,
                               GError       **error)
{
  GSimpleAsyncResult *simple;
  GNetworkAddress *addr;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == g_resolver_lookup_name_async, NULL);

  addr = g_simple_async_result_get_op_res_gpointer (simple);
  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else
    return g_object_ref (addr);
}


/**
 * g_resolver_lookup_address:
 * @resolver: a #GResolver
 * @sockaddr: the address to reverse-resolve
 * @cancellable: a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Synchronously reverse-resolves @sockaddr to determine its
 * associated hostname.
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a #GNetworkAddress, or %NULL on error
 **/
GNetworkAddress *
g_resolver_lookup_address (GResolver     *resolver,
                           GSockaddr     *sockaddr,
                           GCancellable  *cancellable,
                           GError       **error)
{
  GNetworkAddress *addr;

  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (sockaddr != NULL, NULL);

  addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                       "sockaddr", sockaddr,
                       NULL);

  if (!G_RESOLVER_GET_CLASS (resolver)->lookup_address (resolver, addr, cancellable, error))
    {
      g_object_unref (addr);
      return NULL;
    }
  return addr;
}

static void
lookup_address_async_callback (GObject      *source,
                               GAsyncResult *result,
                               gpointer      user_data)
{
  GResolver *resolver = G_RESOLVER (source);
  GSimpleAsyncResult *simple = user_data;
  GError *error = NULL;

  if (!G_RESOLVER_GET_CLASS (resolver)->
      lookup_address_finish (resolver, result, &error))
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
}

/**
 * g_resolver_lookup_address_async:
 * @resolver: a #GResolver
 * @sockaddr: the address to reverse-resolve
 * @cancellable: a #GCancellable, or %NULL
 * @callback: callback to call after resolution completes
 * @user_data: data for @callback
 *
 * Begins asynchronously reverse-resolving @sockaddr to determine its
 * associated hostname, and eventually calls @callback, which must
 * call g_resolver_lookup_address_finish() to get the final result.
 **/
void
g_resolver_lookup_address_async (GResolver           *resolver,
                                 GSockaddr           *sockaddr,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  GNetworkAddress *addr;
  GSimpleAsyncResult *simple;

  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (sockaddr != NULL);

  addr = g_object_new (G_TYPE_NETWORK_ADDRESS,
                       "sockaddr", sockaddr,
                       NULL);

  simple = g_simple_async_result_new (G_OBJECT (resolver), callback, user_data,
                                      g_resolver_lookup_address_async);
  g_simple_async_result_set_op_res_gpointer (simple, addr, g_object_unref);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_address_async (resolver, addr, cancellable,
                          lookup_address_async_callback, simple);
}

/**
 * g_resolver_lookup_address_finish:
 * @resolver: a #GResolver
 * @result: the result passed to your #GAsyncReadyCallback
 * @error: return location for a #GError, or %NULL
 *
 * Retrieves the result of a previous call to
 * g_resolver_lookup_address_async().
 *
 * If the DNS resolution failed, @error (if non-%NULL) will be set to
 * a value from #GResolverError. If the operation was cancelled,
 * @error will be set to %G_IO_ERROR_CANCELLED.
 *
 * Return value: a #GNetworkAddress, or %NULL on error
 **/
GNetworkAddress *
g_resolver_lookup_address_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  GSimpleAsyncResult *simple;
  GNetworkAddress *addr;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == g_resolver_lookup_address_async, NULL);

  addr = g_simple_async_result_get_op_res_gpointer (simple);
  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else
    return g_object_ref (addr);
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
 *
 * If the DNS resolution fails, @error (if non-%NULL) will be set to
 * a value from #GResolverError.
 *
 * If @cancellable is non-%NULL, it can be used to cancel the
 * operation, in which case @error (if non-%NULL) will be set to
 * %G_IO_ERROR_CANCELLED.
 *
 * Return value: a #GNetworkService, or %NULL on error.
 **/
GNetworkService *
g_resolver_lookup_service (GResolver     *resolver,
                           const gchar   *service,
                           const gchar   *protocol,
                           const gchar   *domain,
                           GCancellable  *cancellable,
                           GError       **error)
{
  GNetworkService *srv;

  g_return_val_if_fail (G_IS_RESOLVER (resolver), NULL);
  g_return_val_if_fail (service != NULL, NULL);
  g_return_val_if_fail (protocol != NULL, NULL);
  g_return_val_if_fail (domain != NULL, NULL);

  srv = g_object_new (G_TYPE_NETWORK_SERVICE,
                      "service", service,
                      "protocol", protocol,
                      "domain", domain,
                      NULL);

  if (!G_RESOLVER_GET_CLASS (resolver)->lookup_service (resolver, srv, cancellable, error))
    {
      g_object_unref (srv);
      return NULL;
    }
  return srv;
}

static void
lookup_service_async_callback (GObject      *source,
                               GAsyncResult *result,
                               gpointer      user_data)
{
  GResolver *resolver = G_RESOLVER (source);
  GSimpleAsyncResult *simple = user_data;
  GError *error = NULL;

  if (!G_RESOLVER_GET_CLASS (resolver)->
      lookup_service_finish (resolver, result, &error))
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
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
 * get the final result.
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
  GNetworkService *srv;
  GSimpleAsyncResult *simple;

  g_return_if_fail (G_IS_RESOLVER (resolver));
  g_return_if_fail (service != NULL);
  g_return_if_fail (protocol != NULL);
  g_return_if_fail (domain != NULL);

  srv = g_object_new (G_TYPE_NETWORK_SERVICE,
                      "service", service,
                      "protocol", protocol,
                      "domain", domain,
                      NULL);

  simple = g_simple_async_result_new (G_OBJECT (resolver), callback, user_data,
                                      g_resolver_lookup_service_async);
  g_simple_async_result_set_op_res_gpointer (simple, srv, g_object_unref);

  return G_RESOLVER_GET_CLASS (resolver)->
    lookup_service_async (resolver, srv, cancellable,
                          lookup_service_async_callback, simple);
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
 * Return value: a #GNetworkService, or %NULL on error.
 **/
GNetworkService *
g_resolver_lookup_service_finish (GResolver     *resolver,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  GSimpleAsyncResult *simple;
  GNetworkService *srv;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == g_resolver_lookup_service_async, NULL);

  srv = g_simple_async_result_get_op_res_gpointer (simple);
  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else
    return g_object_ref (srv);
}


/**
 * g_resolver_error_quark:
 * 
 * Gets the #GResolver Error Quark.
 *
 * Return value: a #GQuark.
 **/
GQuark
g_resolver_error_quark (void)
{
  return g_quark_from_static_string ("g-resolver-error-quark");
}

#define __G_RESOLVER_C__
#include "gioaliasdef.c"
