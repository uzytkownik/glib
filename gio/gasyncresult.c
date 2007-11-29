/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
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
 * Author: Alexander Larsson <alexl@redhat.com>
 */

#include <config.h>
#include "gasyncresult.h"
#include "glibintl.h"

#include "gioalias.h"

/**
 * SECTION:gasyncresult
 * @short_description: Asynchronous Function Results
 * @see_also: #GSimpleAsyncResult
 * 
 * Provides a base class for implementing asynchronous function results.
 * 
 * Asynchronous operations are broken up into two separate operations
 * which are chained together by a #GAsyncReadyCallback. To begin
 * an asynchronous operation, provide a #GAsyncReadyCallback to the 
 * asynchronous function. This callback will be triggered when the 
 * operation has completed, and will be passed a #GAsyncResult instance 
 * filled with the details of the operation's success or failure, the 
 * object the asynchronous function was started for and any error codes 
 * returned. The asynchronous callback function is then expected to call 
 * the corresponding "_finish()" function with the object the function 
 * was called for, and the #GAsyncResult instance, and optionally, 
 * an @error to grab any error conditions that may have occurred.
 * 
 * The purpose of the "_finish()" function is to take the generic 
 * result of type #GAsyncResult and return the specific result
 * that the operation in question yields (e.g. a #GFileEnumerator for
 * a "enumerate children" operation). If the result or error status
 * of the operation is not needed, there is no need to call the
 * "_finish()" function, GIO will take care of cleaning up the
 * result and error information after the #GAsyncReadyCallback 
 * returns.
 * 
 * Example of a typical asynchronous operation flow:
 * |[
 * void _theoretical_frobnitz_async (Theoretical         *t, 
 *                                   GCancellable        *c, 
 *                                   GAsyncReadyCallback *cb,
 *                                   gpointer             u);
 *
 * gboolean _theoretical_frobnitz_finish (Theoretical   *t,
 *                                        GAsyncResult  *res,
 *                                        GError       **e);
 *
 * static void 
 * frobnitz_result_func (GObject      *source_object, 
 *			 GAsyncResult *res, 
 *			 gpointer      user_data)
 * {
 *   gboolean success = FALSE;
 *
 *   success = _theoretical_frobnitz_finish (source_object, res, NULL);
 *
 *   if (success)
 *     g_printf ("Hurray!/n");
 *   else 
 *     g_printf ("Uh oh!/n");
 *
 *   /<!-- -->* ... *<!-- -->/
 *
 *   g_free (res);
 * }
 *
 * int main (int argc, void *argv[])
 * {
 *    /<!-- -->* ... *<!-- -->/
 *
 *    _theoretical_frobnitz_async (theoretical_data, 
 *                                 NULL, 
 *                                 frobnitz_result_func, 
 *                                 NULL);
 *
 *    /<!-- -->* ... *<!-- -->/
 * }
 * ]|
 * 
 * Asynchronous jobs are threaded if #GThread is available, but also may
 * be sent to the Main Event Loop and processed in an idle function.
 **/

static void g_async_result_base_init (gpointer g_class);
static void g_async_result_class_init (gpointer g_class,
				       gpointer class_data);

GType
g_async_result_get_type (void)
{
  static GType async_result_type = 0;

  if (! async_result_type)
    {
      static const GTypeInfo async_result_info =
      {
        sizeof (GAsyncResultIface), /* class_size */
	g_async_result_base_init,   /* base_init */
	NULL,		            /* base_finalize */
	g_async_result_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };

      async_result_type =
	g_type_register_static (G_TYPE_INTERFACE, I_("GAsyncResult"),
				&async_result_info, 0);

      g_type_interface_add_prerequisite (async_result_type, G_TYPE_OBJECT);
    }

  return async_result_type;
}

static void
g_async_result_class_init (gpointer g_class,
			   gpointer class_data)
{
}

static void
g_async_result_base_init (gpointer g_class)
{
}

/**
 * g_async_result_get_user_data:
 * @res: a #GAsyncResult.
 * 
 * Gets the user data from a #GAsyncResult.
 * 
 * Returns: the user data for @res. 
 **/
gpointer
g_async_result_get_user_data (GAsyncResult *res)
{
  GAsyncResultIface *iface;

  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  iface = G_ASYNC_RESULT_GET_IFACE (res);

  return (* iface->get_user_data) (res);
}

/**
 * g_async_result_get_source_object:
 * @res: a #GAsyncResult.
 * 
 * Gets the source object from a #GAsyncResult.
 * 
 * Returns: the source object for the @res.
 **/
GObject *
g_async_result_get_source_object (GAsyncResult *res)
{
  GAsyncResultIface *iface;

  g_return_val_if_fail (G_IS_ASYNC_RESULT (res), NULL);

  iface = G_ASYNC_RESULT_GET_IFACE (res);

  return (* iface->get_source_object) (res);
}

#define __G_ASYNC_RESULT_C__
#include "gioaliasdef.c"
