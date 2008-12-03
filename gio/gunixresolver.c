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

#include <resolv.h>
#include <stdio.h>
#include <string.h>

#include "gunixresolver.h"
#include "gresolverprivate.h"

#include "gcancellable.h"
#include "ghostutils.h"
#include "gnetworkaddress.h"
#include "gnetworkservice.h"
#include "gsimpleasyncresult.h"

#include "gioalias.h"

G_DEFINE_TYPE (GUnixResolver, g_unix_resolver, G_TYPE_THREADED_RESOLVER)

static gboolean g_unix_resolver_watch (GIOChannel *iochannel,
                                       GIOCondition condition,
                                       gpointer user_data);

static void
g_unix_resolver_init (GUnixResolver *gur)
{
  int fd;
  GIOChannel *io;

  /* FIXME: how many workers? */
  gur->asyncns = _g_asyncns_new (2);

  fd = _g_asyncns_fd (gur->asyncns);
  io = g_io_channel_unix_new (fd);
  gur->watch = g_io_add_watch (io, G_IO_IN | G_IO_HUP | G_IO_ERR,
                               g_unix_resolver_watch, gur);
  g_io_channel_unref (io);
}

static void
g_unix_resolver_finalize (GObject *object)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (object);

  if (gur->watch)
    g_source_remove (gur->watch);
  _g_asyncns_free (gur->asyncns);

  G_OBJECT_CLASS (g_unix_resolver_parent_class)->finalize (object);
}

/* The various request possibilities:
 *
 * 1. Synchronous: handed off to the base class (GThreadedResolver);
 *    since it's never possible to cancel a synchronous request in a
 *    single-threaded program, the request is done in the calling
 *    thread.
 *
 * 2. Asynchronous: An appropriate _g_asyncns_query_t is created, and
 *    then a GUnixResolverRequest is created with that query and a
 *    GSimpleAsyncResult. Two sub-possibilities:
 *
 *      a. The resolution completes: g_unix_resolver_watch() sees that
 *         the request has completed, and calls
 *         g_unix_resolver_request_complete(), which detaches the
 *         "cancelled" signal handler (if it was present) and then
 *         immediately completes the async_result (since
 *         g_unix_resolver_watch() is already run from main-loop
 *         time.) After completing the async_result, it unrefs it,
 *         causing the req to be freed as well.
 *
 *      b. The resolution is cancelled: request_cancelled() calls
 *         _g_asyncns_cancel() to cancel the resolution. Then it calls
 *         g_unix_resolver_request_complete(), which detaches the
 *         signal handler, and queues async_result to complete in an
 *         idle handler. It then unrefs the async_result to ensure
 *         that after its callback runs, it will be destroyed, in turn
 *         causing the req to be freed. Because the asyncns resolution
 *         was cancelled, g_unix_resolver_watch() will never be
 *         triggered for this req.
 *
 *    Since there's only a single thread, it's not possible for the
 *    request to both complete and be cancelled "at the same time",
 *    and each of the two possibilities takes steps to block the other
 *    from being able to happen later, so it's always safe to free req
 *    after the async_result completes.
 */

typedef struct {
  GUnixResolver *gur;

  _g_asyncns_query_t *qy;
  gpointer resolvable;

  GCancellable *cancellable;
  GSimpleAsyncResult *async_result;

} GUnixResolverRequest;

static void g_unix_resolver_request_free (GUnixResolverRequest *req);
static void request_cancelled (GCancellable *cancellable, gpointer user_data);

static GUnixResolverRequest *
g_unix_resolver_request_new (GUnixResolver *gur,
                             gpointer resolvable,
                             _g_asyncns_query_t *qy,
                             GCancellable *cancellable,
                             GSimpleAsyncResult *async_result)
{
  GUnixResolverRequest *req;

  req = g_slice_new0 (GUnixResolverRequest);
  req->gur = g_object_ref (gur);
  req->qy = qy;
  req->resolvable = g_object_ref (resolvable);

  if (cancellable)
    {
      req->cancellable = g_object_ref (cancellable);
      g_signal_connect (cancellable, "cancelled",
                        G_CALLBACK (request_cancelled), req);
    }

  req->async_result = g_object_ref (async_result);

  g_simple_async_result_set_op_res_gpointer (req->async_result, req, (GDestroyNotify)g_unix_resolver_request_free);

  return req;
}

static void
g_unix_resolver_request_free (GUnixResolverRequest *req)
{
  g_object_unref (req->resolvable);

  /* We don't have to free req->cancellable and req->async_result,
   * since they must already have been freed if we're here.
   */

  g_slice_free (GUnixResolverRequest, req);
}

static void
g_unix_resolver_request_complete (GUnixResolverRequest *req,
                                  gboolean need_idle)
{
  if (req->cancellable)
    {
      g_signal_handlers_disconnect_by_func (req->cancellable, request_cancelled, req);
      g_object_unref (req->cancellable);
      req->cancellable = NULL;
    }

  if (need_idle)
    g_simple_async_result_complete_in_idle (req->async_result);
  else
    g_simple_async_result_complete (req->async_result);

  /* If we completed_in_idle, that will have taken an extra ref on
   * req->async_result; if not, then we're already done. Either way we
   * need to unref the async_result to make sure it eventually is
   * destroyed, causing req to be freed.
   */
  g_object_unref (req->async_result);
}

static void
request_cancelled (GCancellable *cancellable, gpointer user_data)
{
  GUnixResolverRequest *req = user_data;
  GError *error = NULL;

  _g_asyncns_cancel (req->gur->asyncns, req->qy);
  req->qy = NULL;

  g_cancellable_set_error_if_cancelled (cancellable, &error);
  g_simple_async_result_set_from_error (req->async_result, error);
  g_error_free (error);

  g_unix_resolver_request_complete (req, TRUE);
}

static gboolean
g_unix_resolver_watch (GIOChannel *iochannel, GIOCondition condition,
                       gpointer user_data)
{
  GUnixResolver *gur = user_data;
  _g_asyncns_query_t *qy;
  GUnixResolverRequest *req;

  if (condition & (G_IO_HUP | G_IO_ERR))
    {
      /* Shouldn't happen. Should we create a new asyncns? FIXME */
      g_warning ("asyncns died");
      gur->watch = 0;
      return FALSE;
    }

  while (_g_asyncns_wait (gur->asyncns, FALSE) == 0 &&
         (qy = _g_asyncns_getnext (gur->asyncns)) != NULL)
    {
      req = _g_asyncns_getuserdata (gur->asyncns, qy);
      g_unix_resolver_request_complete (req, FALSE);
    }

  return TRUE;
}

static void
resolve_async (GUnixResolver *gur, gpointer resolvable,
               _g_asyncns_query_t *qy, GCancellable *cancellable,
               GAsyncReadyCallback callback, gpointer user_data,
               gpointer tag)
{
  GSimpleAsyncResult *result;
  GUnixResolverRequest *req;

  result = g_simple_async_result_new (G_OBJECT (gur), callback, user_data, tag);
  req = g_unix_resolver_request_new (gur, resolvable, qy, cancellable, result);
  g_object_unref (result);
  _g_asyncns_setuserdata (gur->asyncns, qy, req);
}

static void
lookup_name_async (GResolver *resolver, GNetworkAddress *addr,
		   GCancellable *cancellable, GAsyncReadyCallback callback,
		   gpointer user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  _g_asyncns_query_t *qy;
  struct addrinfo hints;
  char service[8];

  g_network_address_get_addrinfo_hints (addr, service, &hints);
  qy = _g_asyncns_getaddrinfo (gur->asyncns,
                               g_network_address_get_ascii_name (addr),
                               service, &hints);
  resolve_async (gur, addr, qy, cancellable,
                 callback, user_data, lookup_name_async);
}

static gboolean
lookup_name_finish (GResolver *resolver, GAsyncResult *result,
		    GError **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  struct addrinfo *res;
  int retval;
  gboolean success;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), FALSE);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == lookup_name_async, FALSE);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  retval = _g_asyncns_getaddrinfo_done (req->gur->asyncns, req->qy, &res);
  success = g_network_address_set_from_addrinfo (req->resolvable, res, retval, error);
  if (res)
    freeaddrinfo (res);

  return success;
}


static void
lookup_address_async (GResolver *resolver, GNetworkAddress *addr,
		      GCancellable *cancellable, GAsyncReadyCallback callback,
		      gpointer user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  _g_asyncns_query_t *qy;
  GSockaddr *sockaddr;
  int socklen;

  sockaddr = g_network_address_get_sockaddrs (addr)[0];
  socklen = g_sockaddr_size (sockaddr);
  qy = _g_asyncns_getnameinfo (gur->asyncns,
                               (struct sockaddr *)sockaddr, socklen,
                               NI_NAMEREQD, TRUE, FALSE);
  resolve_async (gur, addr, qy, cancellable,
                 callback, user_data, lookup_address_async);
}

static gboolean
lookup_address_finish (GResolver *resolver, GAsyncResult *result,
		       GError **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  char host[NI_MAXHOST];
  int retval;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), FALSE);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == lookup_address_async, FALSE);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  retval = _g_asyncns_getnameinfo_done (req->gur->asyncns, req->qy,
                                        host, sizeof (host), NULL, 0);
  return g_network_address_set_from_nameinfo (req->resolvable, host, retval, error);
}


static void
lookup_service_async (GResolver *resolver, GNetworkService *srv,
		      GCancellable *cancellable, GAsyncReadyCallback callback,
		      gpointer user_data)
{
  GUnixResolver *gur = G_UNIX_RESOLVER (resolver);
  _g_asyncns_query_t *qy;
  char *dname;

  dname = g_network_service_get_rrname (srv);
  qy = _g_asyncns_res_query (gur->asyncns, dname, C_IN, T_SRV);
  g_free (dname);
  resolve_async (gur, srv, qy, cancellable,
                 callback, user_data, lookup_service_async);
}

static gboolean
lookup_service_finish (GResolver *resolver, GAsyncResult *result,
		       GError **error)
{
  GSimpleAsyncResult *simple;
  GUnixResolverRequest *req;
  guchar *answer;
  int len, herr;
  gboolean success;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), FALSE);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == lookup_service_async, FALSE);

  if (g_simple_async_result_propagate_error (simple, error))
    return FALSE;

  req = g_simple_async_result_get_op_res_gpointer (simple);
  len = _g_asyncns_res_done (req->gur->asyncns, req->qy, &answer);
  if (len < 0)
    herr = h_errno;
  else
    herr = 0;

  success = g_network_service_set_from_res_query (req->resolvable, answer, len, herr, error);
  _g_asyncns_freeanswer (answer);
  return success;
}


static void
g_unix_resolver_class_init (GUnixResolverClass *unix_class)
{
  GResolverClass *resolver_class = G_RESOLVER_CLASS (unix_class);
  GObjectClass *object_class = G_OBJECT_CLASS (unix_class);

  resolver_class->lookup_name_async     = lookup_name_async;
  resolver_class->lookup_name_finish    = lookup_name_finish;
  resolver_class->lookup_address_async  = lookup_address_async;
  resolver_class->lookup_address_finish = lookup_address_finish;
  resolver_class->lookup_service_async  = lookup_service_async;
  resolver_class->lookup_service_finish = lookup_service_finish;

  object_class->finalize = g_unix_resolver_finalize;
}

#define __G_UNIX_RESOLVER_C__
#include "gioaliasdef.c"
