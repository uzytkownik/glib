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
#include "gsocketconnectable.h"
#include "glibintl.h"

#include "gsimpleasyncresult.h"

#include "gioalias.h"

static void            g_socket_connectable_base_init            (GSocketConnectableIface *connectable_iface);

static void            g_socket_connectable_real_get_next_async  (GSocketConnectable      *connectable,
								  GSocketConnectableIter  *iter,
								  GCancellable            *cancellable,
								  GAsyncReadyCallback      callback,
								  gpointer                 user_data);
static GSocketAddress *g_socket_connectable_real_get_next_finish (GSocketConnectable      *connectable,
								  GAsyncResult            *result,
								  GError                 **error);

GType
g_socket_connectable_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      const GTypeInfo connectable_info =
      {
        sizeof (GSocketConnectableIface), /* class_size */
	(GBaseInitFunc) g_socket_connectable_base_init,   /* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };
      GType g_define_type_id =
	g_type_register_static (G_TYPE_INTERFACE, I_("GSocketConnectable"),
				&connectable_info, 0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}

static void
g_socket_connectable_base_init (GSocketConnectableIface *connectable_iface)
{
  connectable_iface->get_next_async = g_socket_connectable_real_get_next_async;
  connectable_iface->get_next_finish = g_socket_connectable_real_get_next_finish;
}

GSocketConnectableIter *
g_socket_connectable_get_iter (GSocketConnectable *connectable)
{
  GSocketConnectableIface *iface;

  g_return_val_if_fail (G_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  return (* iface->get_iter) (connectable);
}

void
g_socket_connectable_free_iter (GSocketConnectable      *connectable,
				GSocketConnectableIter  *iter)
{
  GSocketConnectableIface *iface;

  g_return_if_fail (G_IS_SOCKET_CONNECTABLE (connectable));

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  (* iface->free_iter) (connectable, iter);
}

GSocketAddress *
g_socket_connectable_get_next (GSocketConnectable      *connectable,
			       GSocketConnectableIter  *iter,
			       GCancellable            *cancellable,
			       GError                 **error)
{
  GSocketConnectableIface *iface;

  g_return_val_if_fail (G_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  return (* iface->get_next) (connectable, iter, cancellable, error);
}

/* Default implementation just calls the synchronous method; this can
 * be used if the implementation already knows all of its addresses,
 * and so the synchronous method will never block.
 */
static void
g_socket_connectable_real_get_next_async (GSocketConnectable      *connectable,
					  GSocketConnectableIter  *iter,
					  GCancellable            *cancellable,
					  GAsyncReadyCallback      callback,
					  gpointer                 user_data)
{
  GSimpleAsyncResult *result;
  GSocketAddress *address;
  GError *error = NULL;

  result = g_simple_async_result_new (G_OBJECT (connectable),
				      callback, user_data,
				      g_socket_connectable_real_get_next_async);
  address = g_socket_connectable_get_next (connectable, iter,
					   cancellable, &error);
  if (address)
    g_simple_async_result_set_op_res_gpointer (result, address, NULL);
  else
    {
      g_simple_async_result_set_from_error (result, error);
      g_error_free (error);
    }
  g_simple_async_result_complete_in_idle (result);
  g_object_unref (result);
}

void
g_socket_connectable_get_next_async (GSocketConnectable      *connectable,
				     GSocketConnectableIter  *iter,
				     GCancellable            *cancellable,
				     GAsyncReadyCallback      callback,
				     gpointer                 user_data)
{
  GSocketConnectableIface *iface;

  g_return_if_fail (G_IS_SOCKET_CONNECTABLE (connectable));

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  (* iface->get_next_async) (connectable, iter, cancellable, callback, user_data);
}

static GSocketAddress *
g_socket_connectable_real_get_next_finish (GSocketConnectable      *connectable,
					   GAsyncResult            *result,
					   GError                 **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (G_IS_SIMPLE_ASYNC_RESULT (result), NULL);
  simple = G_SIMPLE_ASYNC_RESULT (result);
  g_return_val_if_fail (g_simple_async_result_get_source_tag (simple) == g_socket_connectable_real_get_next_async, NULL);

  if (g_simple_async_result_propagate_error (simple, error))
    return NULL;
  else
    return g_simple_async_result_get_op_res_gpointer (simple);
}

GSocketAddress *
g_socket_connectable_get_next_finish (GSocketConnectable      *connectable,
				      GAsyncResult            *result,
				      GError                 **error)
{
  GSocketConnectableIface *iface;

  g_return_val_if_fail (G_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  return (* iface->get_next_finish) (connectable, result, error);
}

#define __G_SOCKET_CONNECTABLE_C__
#include "gioaliasdef.c"
