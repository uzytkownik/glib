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

#include <stdio.h>
#include <string.h>

#include "gthreadedresolver.h"
#include "gresolverprivate.h"

#include "gcancellable.h"
#include "gnetworkaddress.h"
#include "gnetworkservice.h"
#include "gsimpleasyncresult.h"

#include "gioalias.h"

G_DEFINE_TYPE (GThreadedResolver, g_threaded_resolver, G_TYPE_RESOLVER)

static void threaded_resolver_thread (gpointer thread_data, gpointer pool_data);

static void
g_threaded_resolver_init (GThreadedResolver *gtr)
{
  if (g_thread_supported ())
    {
      gtr->thread_pool = g_thread_pool_new (threaded_resolver_thread, gtr,
					    -1, FALSE, NULL);
    }
}

static void
finalize (GObject *object)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (object);

  g_thread_pool_free (gtr->thread_pool, FALSE, FALSE);

  G_OBJECT_CLASS (g_threaded_resolver_parent_class)->finalize (object);
}

/* A GThreadedResolverRequest represents a request in progress
 * (usually, but see case 1). It is refcounted, to make sure that it
 * doesn't get freed too soon. In particular, it can't be freed until
 * (a) the resolver thread has finished resolving, (b) the calling
 * thread has received an answer, and (c) no other thread could be in
 * the process of trying to cancel it.
 *
 * The possibilities:
 *
 * 1. Synchronous non-cancellable request: in this case, the request
 *    is simply done in the calling thread, without using
 *    GThreadedResolverRequest at all.
 *
 * 2. Synchronous cancellable request: A req is created with a GCond,
 *    and 3 refs (for the resolution thread, the calling thread, and
 *    the cancellation signal handler).
 *
 *      a. If the resolution completes successfully, the thread pool
 *         function (threaded_resolver_thread()) will call
 *         g_threaded_resolver_request_complete(), which will detach
 *         the "cancelled" signal handler (dropping one ref on req)
 *         and signal the GCond, and then unref the req. The calling
 *         thread receives the signal from the GCond, processes the
 *         response, and unrefs the req, causing it to be freed.
 *
 *      b. If the resolution is cancelled before completing,
 *         request_cancelled() will call
 *         g_threaded_resolver_request_complete(), which will detach
 *         the signal handler (as above, unreffing the req), set
 *         req->error to indicate that it was cancelled, and signal
 *         the GCond. The calling thread receives the signal from the
 *         GCond, processes the response, and unrefs the req.
 *         Eventually, the resolver thread finishes resolving (or
 *         times out in the resolver) and calls
 *         g_threaded_resolver_request_complete() again, but
 *         _request_complete() does nothing this time since the
 *         request is already complete. The thread pool func then
 *         unrefs the req, causing it to be freed.
 *
 * 3. Asynchronous request: A req is created with a GSimpleAsyncResult
 *    (and no GCond). The calling thread's ref on req is set up to be
 *    automatically dropped when the async_result is freed. Two
 *    sub-possibilities:
 *
 *      a. If the resolution completes, the thread pool function
 *         (threaded_resolver_thread()) will call
 *         g_threaded_resolver_request_complete(), which will detach
 *         the "cancelled" signal handler (if it was present)
 *         (unreffing the req), queue the async_result to complete in
 *         an idle handler, unref the async_result (which is still
 *         reffed by the idle handler though), and then unref the req.
 *         The main thread then invokes the async_result's callback
 *         and processes the response. When it finishes, the
 *         async_result drops the ref that was taken by
 *         g_simple_async_result_complete_in_idle(), which causes the
 *         async_result to be freed, which causes req to be unreffed
 *         and freed.
 *
 *      b. If the resolution is cancelled, request_cancelled() will
 *         call g_threaded_resolver_request_complete(), which will
 *         detach the signal handler (as above, unreffing the req) set
 *         req->error to indicate that it was cancelled, and queue and
 *         unref the async_result. The main thread completes the
 *         async_request and unrefs it and the req, as above.
 *         Eventually, the resolver thread finishes resolving (or
 *         times out in the resolver) and calls
 *         g_threaded_resolver_request_complete() again, but
 *         _request_complete() does nothing this time since the
 *         request is already complete. The thread pool func then
 *         unrefs the req, causing it to be freed.
 *
 * g_threaded_resolver_request_complete() ensures that if the request
 * completes and cancels "at the same time" that only one of the two
 * conditions gets processed.
 */

typedef gboolean (*GThreadedResolverResolveFunc) (gpointer, GError **);

typedef struct {
  gpointer resolvable;
  GThreadedResolverResolveFunc resolve_func;

  GCancellable *cancellable;
  GError *error;

  GMutex *mutex;
  guint ref_count;

  GCond *cond;
  GSimpleAsyncResult *async_result;
  gboolean complete;

} GThreadedResolverRequest;

static void g_threaded_resolver_request_unref (GThreadedResolverRequest *req);
static void request_cancelled (GCancellable *cancellable, gpointer req);
static void request_cancelled_disconnect_notify (gpointer req, GClosure *closure);

static GThreadedResolverRequest *
g_threaded_resolver_request_new (gpointer                      resolvable,
				 GThreadedResolverResolveFunc  resolve_func,
				 GCancellable                 *cancellable,
				 GSimpleAsyncResult           *async_result)
{
  GThreadedResolverRequest *req;

  req = g_slice_new0 (GThreadedResolverRequest);
  req->resolvable = g_object_ref (resolvable);
  req->resolve_func = resolve_func;

  /* Initial refcount is 2; one for the caller and one for resolve_func */
  req->ref_count = 2;

  req->mutex = g_mutex_new ();
  /* Initially locked; caller must unlock */
  g_mutex_lock (req->mutex);

  if (cancellable)
    {
      req->ref_count++;
      req->cancellable = g_object_ref (cancellable);
      g_signal_connect_data (cancellable, "cancelled",
			     G_CALLBACK (request_cancelled), req,
			     request_cancelled_disconnect_notify, 0);
    }

  if (async_result)
    {
      req->async_result = g_object_ref (async_result);
      /* Drop the caller's ref when @async_result is destroyed */
      g_simple_async_result_set_op_res_gpointer (req->async_result, req, (GDestroyNotify)g_threaded_resolver_request_unref);
    }
  else
    req->cond = g_cond_new ();

  return req;
}

static void
g_threaded_resolver_request_unref (GThreadedResolverRequest *req)
{
  guint ref_count;

  g_mutex_lock (req->mutex);
  ref_count = --req->ref_count;
  g_mutex_unlock (req->mutex);
  if (ref_count > 0)
    return;

  if (req->resolvable)
    g_object_unref (req->resolvable);

  g_mutex_free (req->mutex);

  if (req->cond)
    g_cond_free (req->cond);

  if (req->error)
    g_error_free (req->error);

  /* We don't have to free req->cancellable or req->async_result,
   * since (if set), they must already have been freed by
   * request_complete() in order to get here.
   */

  g_slice_free (GThreadedResolverRequest, req);
}

static void
g_threaded_resolver_request_complete (GThreadedResolverRequest *req,
				      gboolean                  cancelled)
{
  g_mutex_lock (req->mutex);
  if (req->complete)
    {
      /* The req was cancelled, and now it has finished resolving as
       * well. But we have nowhere to send the result, so just return.
       */
      g_mutex_unlock (req->mutex);
      return;
    }

  req->complete = TRUE;
  g_mutex_unlock (req->mutex);

  if (req->cancellable)
    {
      /* Possibly propagate a cancellation error */
      if (cancelled && !req->error)
        g_cancellable_set_error_if_cancelled (req->cancellable, &req->error);

      /* Drop the signal handler's ref on @req */
      g_signal_handlers_disconnect_by_func (req->cancellable, request_cancelled, req);
      g_object_unref (req->cancellable);
      req->cancellable = NULL;
    }

  if (req->cond)
    g_cond_signal (req->cond);
  else if (req->async_result)
    {
      if (req->error)
        g_simple_async_result_set_from_error (req->async_result, req->error);
      g_simple_async_result_complete_in_idle (req->async_result);

      /* Drop our ref on the async_result, which will eventually cause
       * it to drop its ref on req.
       */
      g_object_unref (req->async_result);
      req->async_result = NULL;
    }
}

static void
request_cancelled (GCancellable *cancellable,
                   gpointer      user_data)
{
  GThreadedResolverRequest *req = user_data;

  g_threaded_resolver_request_complete (req, TRUE);

  /* We can't actually cancel the resolver thread; it will eventually
   * complete on its own and call request_complete() again, which will
   * do nothing the second time.
   */
}

static void
request_cancelled_disconnect_notify (gpointer  req,
                                     GClosure *closure)
{
  g_threaded_resolver_request_unref (req);
}

static void
threaded_resolver_thread (gpointer thread_data,
                          gpointer pool_data)
{
  GThreadedResolverRequest *req = thread_data;

  req->resolve_func (req->resolvable, &req->error);
  g_threaded_resolver_request_complete (req, FALSE);
  g_threaded_resolver_request_unref (req);
}  

static gboolean
resolve_sync (GThreadedResolver             *gtr,
	      gpointer                       resolvable,
              GThreadedResolverResolveFunc   resolve_func,
	      GCancellable                  *cancellable,
              GError                       **error)
{
  GThreadedResolverRequest *req;
  gboolean success;

  if (!cancellable || !gtr->thread_pool)
    return resolve_func (resolvable, error);

  req = g_threaded_resolver_request_new (resolvable, resolve_func,
					 cancellable, NULL);
  g_thread_pool_push (gtr->thread_pool, req, NULL);
  g_cond_wait (req->cond, req->mutex);
  g_mutex_unlock (req->mutex);

  if (req->error)
    {
      g_propagate_error (error, req->error);
      req->error = NULL;
      success = FALSE;
    }
  else
    success = TRUE;

  g_threaded_resolver_request_unref (req);
  return success;
}

static void
resolve_async (GThreadedResolver            *gtr,
	       gpointer                      resolvable,
               GThreadedResolverResolveFunc  resolve_func,
	       GCancellable                 *cancellable,
               GAsyncReadyCallback           callback,
	       gpointer                      user_data,
               gpointer                      tag)
{
  GThreadedResolverRequest *req;
  GSimpleAsyncResult *result;

  result = g_simple_async_result_new (G_OBJECT (gtr), callback, user_data, tag);
  req = g_threaded_resolver_request_new (resolvable, resolve_func,
					 cancellable, result);
  g_object_unref (result);

  g_thread_pool_push (gtr->thread_pool, req, NULL);
  g_mutex_unlock (req->mutex);
}

static gboolean
resolve_finish (GResolver     *resolver,
                GAsyncResult  *result,
		gpointer       tag,
                GError       **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), FALSE);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == tag, FALSE);

  return !g_simple_async_result_propagate_error (simple, error);
}

static gboolean
do_lookup_name (gpointer   data,
                GError   **error)
{
  GNetworkAddress *addr = data;
  struct addrinfo hints, *res = NULL;
  gchar service[8];
  gint retval;

  g_network_address_get_addrinfo_hints (addr, service, &hints);
  retval = getaddrinfo (g_network_address_get_ascii_name (addr),
                        service, &hints, &res);
  g_network_address_set_from_addrinfo (addr, res, retval, error);
  if (res)
    freeaddrinfo (res);
  return retval == 0;
}

static gboolean
lookup_name (GResolver        *resolver,
             GNetworkAddress  *addr,
             GCancellable     *cancellable,
             GError          **error)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  if (!cancellable || !gtr->thread_pool)
    return do_lookup_name (addr, error);
  else
    return resolve_sync (gtr, addr, do_lookup_name, cancellable, error);
}

static void
lookup_name_async (GResolver           *resolver,
                   GNetworkAddress     *addr,
		   GCancellable        *cancellable,
                   GAsyncReadyCallback  callback,
		   gpointer             user_data)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  resolve_async (gtr, addr, do_lookup_name, cancellable,
		 callback, user_data, lookup_name_async);
}

static gboolean
lookup_name_finish (GResolver     *resolver,
                    GAsyncResult  *result,
		    GError       **error)
{
  return resolve_finish (resolver, result, lookup_name_async, error);
}


static gboolean
do_lookup_address (gpointer   data,
                   GError   **error)
{
  GNetworkAddress *addr = data;
  GSockaddr **sockaddrs;
  gchar name[NI_MAXHOST];
  gint retval;

  sockaddrs = g_network_address_get_sockaddrs (addr);
  g_return_val_if_fail (sockaddrs && sockaddrs[0], FALSE);

  retval = getnameinfo ((struct sockaddr *)sockaddrs[0],
                        g_sockaddr_size (sockaddrs[0]),
                        name, sizeof (name), NULL, 0, NI_NAMEREQD);
  g_network_address_set_from_nameinfo (addr, name, retval, error);
  return retval == 0;
}

static gboolean
lookup_address (GResolver        *resolver,
                GNetworkAddress  *addr,
		GCancellable     *cancellable,
                GError          **error)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  if (!cancellable || !gtr->thread_pool)
    return do_lookup_address (addr, error);
  else
    return resolve_sync (gtr, addr, do_lookup_address, cancellable, error);
}

static void
lookup_address_async (GResolver           *resolver,
                      GNetworkAddress     *addr,
		      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
		      gpointer             user_data)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  resolve_async (gtr, addr, do_lookup_address, cancellable,
		 callback, user_data, lookup_address_async);
}

static gboolean
lookup_address_finish (GResolver     *resolver,
                       GAsyncResult  *result,
		       GError       **error)
{
  return resolve_finish (resolver, result, lookup_address_async, error);
}


static gboolean
do_lookup_service (gpointer   data,
                   GError   **error)
{
  GNetworkService *srv = data;
  gboolean success;
  gchar *dname;
#if defined(G_OS_UNIX)
  gint len, herr;
  guchar answer[1024];
#elif defined(G_OS_WIN32)
  DNS_STATUS status;
  DNS_RECORD *results;
#endif

  dname = g_network_service_get_rrname (srv);

#if defined(G_OS_UNIX)
  len = res_query (dname, C_IN, T_SRV, answer, sizeof (answer));
  herr = h_errno;
  success = g_network_service_set_from_res_query (srv, answer, len, herr, error);
#elif defined(G_OS_WIN32)
  status = DnsQuery_A (dname, DNS_TYPE_SRV, DNS_QUERY_STANDARD,
                       NULL, &results, NULL);
  success = g_network_service_set_from_DnsQuery (srv, status, results, error);
  DnsRecordListFree (results, DnsFreeRecordList);
#endif

  g_free (dname);
  return success;
}

static gboolean
lookup_service (GResolver        *resolver,
                GNetworkService  *srv,
		GCancellable     *cancellable,
                GError          **error)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  if (!cancellable || !gtr->thread_pool)
    return do_lookup_service (srv, error);
  else
    return resolve_sync (gtr, srv, do_lookup_service, cancellable, error);
}

static void
lookup_service_async (GResolver           *resolver,
                      GNetworkService     *srv,
		      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
		      gpointer             user_data)
{
  GThreadedResolver *gtr = G_THREADED_RESOLVER (resolver);

  resolve_async (gtr, srv, do_lookup_service, cancellable,
		 callback, user_data, lookup_service_async);
}

static gboolean
lookup_service_finish (GResolver     *resolver,
                       GAsyncResult  *result,
		       GError       **error)
{
  return resolve_finish (resolver, result, lookup_service_async, error);
}


static void
g_threaded_resolver_class_init (GThreadedResolverClass *threaded_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (threaded_class);
  GObjectClass *object_class = G_OBJECT_CLASS (threaded_class);

  resolver_class->lookup_name           = lookup_name;
  resolver_class->lookup_name_async     = lookup_name_async;
  resolver_class->lookup_name_finish    = lookup_name_finish;
  resolver_class->lookup_address        = lookup_address;
  resolver_class->lookup_address_async  = lookup_address_async;
  resolver_class->lookup_address_finish = lookup_address_finish;
  resolver_class->lookup_service        = lookup_service;
  resolver_class->lookup_service_async  = lookup_service_async;
  resolver_class->lookup_service_finish = lookup_service_finish;

  object_class->finalize = finalize;
}

#define __G_THREADED_RESOLVER_C__
#include "gioaliasdef.c"
