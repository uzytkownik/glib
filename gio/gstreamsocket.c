/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Maciej Piechotka
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
 * Author: Maciej Piechotka <uzytkownik2@gmail.com>
 */

#include "gstreamsocket.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"


static void g_stream_socket_dispose (GObject *object);
static void g_stream_socket_finalize (GObject *object);

enum
  {
    PROP_0,
    PROP_REMOTE
  };

static void g_stream_socket_get_property (GObject    *object,
					  guint       prop_id,
					  GValue     *value,
					  GParamSpec *pspec);

G_DEFINE_TYPE (GStreamSocket, g_stream_socket, G_TYPE_SOCKET);

static void
g_stream_socket_class_init (GStreamSocketClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *)klass;

    gobject_class->get_property = g_stream_socket_get_property;
}

static void
g_stream_socket_init (GStreamSocket *self)
{
}

static void
g_stream_socket_get_property (GObject    *object,
			      guint       prop_id,
			      GValue     *value,
			      GParamSpec *pspec)
{
  GStreamSocket      *self;
  GStreamSocketClass *klass;
  
  self = G_STREAM_SOCKET (object);
  klass = G_STREAM_SOCKET_GET_CLASS (self);

  switch (prop_id)
    {
    case PROP_REMOTE:
      g_value_take_object (value, klass->get_remote_address (self));
      break;
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GSocketAddress *
g_stream_socket_get_remote_address (GStreamSocket *self)
{
  GStreamSocketClass *klass;

  g_return_val_if_fail ((klass = G_STREAM_SOCKET_GET_CLASS (self)), NULL);
  g_return_val_if_fail (klass->get_remote_address, NULL);

  return klass->get_remote_address (self);
}

GStreamConnection *
g_stream_socket_connect (GStreamSocket *self,
			 GCancellable *cancellable,
			 GError **error)
{
  GStreamSocketClass *klass;

  g_return_val_if_fail ((klass = G_STREAM_SOCKET_GET_CLASS (self)), NULL);
  g_return_val_if_fail (klass->connect, NULL);

  return klass->connect (self, cancellable, error);
}

typedef struct {
  GAsyncReadyCallback callback;
  gpointer user_data;
} ConnectAsyncData;

static void
connect_callback (GObject      *source,
		  GAsyncResult *res,
		  gpointer      user_data)
{
  ConnectAsyncData *data;
  GAsyncReadyCallback callback;

  data = (ConnectAsyncData *)user_data;
  callback = data->callback;
  user_data = data->user_data;

  g_slice_free (ConnectAsyncData, data);

  callback (source, res, user_data);
}

static void
connect_thread (GSimpleAsyncResult *res,
		GObject            *object,
		GCancellable       *cancellable)
{
  GStreamConnection *result;
  ConnectAsyncData *data;
  GError *error;
  GStreamSocketClass *klass;

  g_return_if_fail ((klass = G_STREAM_SOCKET_GET_CLASS (object)));
  g_return_if_fail (klass->connect);
  
  error = NULL;
  data = g_simple_async_result_get_op_res_gpointer (res);
  result = klass->connect ((GStreamSocket *)object,
			   cancellable, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (res, error);
    }
  g_simple_async_result_set_op_res_gpointer (res, result, g_object_unref);

  g_simple_async_result_complete_in_idle (res);
}

void
g_stream_socket_connect_async (GStreamSocket       *self,
			       int                  io_priority,
			       GCancellable        *cancellable,
			       GAsyncReadyCallback  callback,
			       void                *user_data)
{
  GStreamSocketClass *klass;
  ConnectAsyncData *data;
  GError *error;
  
  g_return_if_fail ((klass = G_STREAM_SOCKET_GET_CLASS (self)));

  if (g_socket_set_pending ((GSocket *)self, &error))
    {
      if (klass->connect_async)
	{
	  data = g_slice_new (ConnectAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
	  klass->connect_async (self, io_priority, cancellable,
				connect_callback, data); 
	}
      else
	{
	  GSimpleAsyncResult *res;

	  data = g_slice_new (ConnectAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;

	  res = g_simple_async_result_new ((GObject *)self, connect_callback,
					   data,
					   g_stream_socket_connect_async);

	  g_simple_async_result_set_op_res_gpointer (res, data, NULL);

	  g_simple_async_result_run_in_thread (res, connect_thread,
					       io_priority, cancellable);
	}
    }
  else
    {
      g_simple_async_report_gerror_in_idle ((GObject *)self, callback,
					    user_data, error);
    }
}

GStreamConnection *
g_stream_socket_connect_finish (GStreamSocket  *self,
				GAsyncResult   *_res,
				GError        **error)
{
  GSimpleAsyncResult *res;
  GStreamConnection *data;

  g_return_val_if_fail ((GObject *)self != g_async_result_get_source_object(_res), FALSE);
  g_return_val_if_fail ((res = G_SIMPLE_ASYNC_RESULT (_res)), FALSE);

  g_simple_async_result_propagate_error (res, error);

  data = g_simple_async_result_get_op_res_gpointer (res);
  if (data)
    g_object_ref ((GObject *)data);
  return data;
}
