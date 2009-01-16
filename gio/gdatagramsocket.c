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

#include "gdatagramsocket.h"
#include "gasyncresult.h"
#include "gsimpleasyncresult.h"

G_DEFINE_TYPE (GDatagramSocket, g_datagram_socket, G_TYPE_SOCKET);

static void
g_datagram_socket_class_init (GDatagramSocketClass *klass)
{
}

static void
g_datagram_socket_init (GDatagramSocket *self)
{
}

gboolean
g_datagram_socket_bind (GDatagramSocket  *self,
			GSocketAddress   *local,
			GCancellable     *cancellable,
			GError          **error)
{
  GDatagramSocketClass *klass;
  
  g_return_val_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)), FALSE);
  g_return_val_if_fail (klass->bind, FALSE);

  if (g_socket_set_pending ((GSocket *)self, error))
    {
      gboolean res;

      res = klass->bind (self, local, cancellable, error);
      g_socket_clear_pending ((GSocket *)self);

      return res;
    }
  else
    {
      return FALSE;
    }
}

typedef struct {
  GAsyncReadyCallback callback;
  gpointer user_data;
  GSocketAddress *local;
} BindAsyncData;

static void
bind_callback (GObject      *source,
	       GAsyncResult *res,
	       gpointer      user_data)
{
  BindAsyncData *data;
  GAsyncReadyCallback callback;

  data = (BindAsyncData *)user_data;
  callback = data->callback;
  user_data = data->user_data;

  g_slice_free (BindAsyncData, data);

  callback (source, res, user_data);
}

static void
bind_thread (GSimpleAsyncResult *res,
	     GObject            *object,
	     GCancellable       *cancellable)
{
  gboolean result;
  BindAsyncData *data;
  GError *error;
  GDatagramSocketClass *klass;

  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (object)));
  g_return_if_fail (klass->bind);
  
  error = NULL;
  data = g_simple_async_result_get_op_res_gpointer (res);
  result = klass->bind ((GDatagramSocket *)object, data->local,
			cancellable, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (res, error);
    }
  g_simple_async_result_set_op_res_gboolean (res, result);

  g_simple_async_result_complete_in_idle (res);
}

void
g_datagram_socket_bind_async (GDatagramSocket     *self,
			      GSocketAddress      *local,
			      int                  io_priority,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      void                *user_data)
{
  GDatagramSocketClass *klass;
  BindAsyncData *data;
  GError *error;
  
  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)));

  if (g_socket_set_pending ((GSocket *)self, &error))
    {
      if (klass->bind_async)
	{
	  data = g_slice_new (BindAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
	  klass->bind_async (self, local, io_priority, cancellable,
			     bind_callback, data); 
	}
      else
	{
	  GSimpleAsyncResult *res;

	  data = g_slice_new (BindAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
      
	  res = g_simple_async_result_new ((GObject *)self, bind_callback,
					   data, g_datagram_socket_bind_async);

	  g_simple_async_result_set_op_res_gpointer (res, data, NULL);

	  g_simple_async_result_run_in_thread (res, bind_thread,
					       io_priority, cancellable);
	}
    }
  else
    {
      g_simple_async_report_gerror_in_idle ((GObject *)self, callback,
					    user_data, error);
    }
}

gboolean
g_datagram_socket_bind_finish (GDatagramSocket  *self,
			       GAsyncResult     *_res,
			       GError          **error)
{
  GSimpleAsyncResult *res;

  g_return_val_if_fail ((GObject *)self != g_async_result_get_source_object(_res), FALSE);
  g_return_val_if_fail ((res = G_SIMPLE_ASYNC_RESULT (_res)), FALSE);

  g_simple_async_result_propagate_error (res, error);
  return g_simple_async_result_get_op_res_gboolean (res);
}

gssize
g_datagram_socket_send (GDatagramSocket  *self,
			GSocketAddress   *destination,
			const void       *buf,
			gsize             size,
			GCancellable     *cancellable,
			GError          **error)
{
  GDatagramSocketClass *klass;

  g_return_val_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)), -1);
  g_return_val_if_fail (klass->send, -1);

  if (g_socket_set_pending ((GSocket *)self, error))
    {
      gssize res;

      res = klass->send (self, destination, buf, size, cancellable, error);
      g_socket_clear_pending ((GSocket *)self);

      return res;
    }
  else
    {
      return -1;
    }
}

typedef struct {
  GAsyncReadyCallback callback;
  gpointer user_data;
  GSocketAddress *destination;
  const void *buf;
  gsize size;
} SendAsyncData;

static void
send_callback (GObject      *source,
	       GAsyncResult *res,
	       gpointer      user_data)
{
  SendAsyncData *data;
  GAsyncReadyCallback callback;

  data = (SendAsyncData *)user_data;
  callback = data->callback;
  user_data = data->user_data;

  g_slice_free (SendAsyncData, data);

  callback (source, res, user_data);
}

static void
send_thread (GSimpleAsyncResult *res,
	     GObject            *object,
	     GCancellable       *cancellable)
{
  gboolean result;
  SendAsyncData *data;
  GError *error;
  GDatagramSocketClass *klass;

  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (object)));
  g_return_if_fail (klass->send);
  
  error = NULL;
  data = g_simple_async_result_get_op_res_gpointer (res);
  result = klass->send ((GDatagramSocket *)object,
			data->destination,
			data->buf,
			data->size,
			cancellable, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (res, error);
    }
  g_simple_async_result_set_op_res_gboolean (res, result);

  g_simple_async_result_complete_in_idle (res);
}

void
g_datagram_socket_send_async (GDatagramSocket     *self,
			      GSocketAddress      *destination,
			      const void          *buf,
			      gsize                size,
			      int                  io_priority,
			      GCancellable        *cancellable,
			      GAsyncReadyCallback  callback,
			      void                *user_data)
{
  GDatagramSocketClass *klass;
  SendAsyncData *data;
  GError *error;
  
  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)));

  if (g_socket_set_pending ((GSocket *)self, &error))
    {
      if (klass->send_async)
	{
	  data = g_slice_new (SendAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
	  klass->send_async (self, destination, buf, size, io_priority,
			     cancellable, send_callback, data); 
	}
      else
	{
	  GSimpleAsyncResult *res;

	  data = g_slice_new (SendAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
      
	  res = g_simple_async_result_new ((GObject *)self, send_callback,
					   data, g_datagram_socket_send_async);

	  g_simple_async_result_set_op_res_gpointer (res, data, NULL);

	  g_simple_async_result_run_in_thread (res, send_thread,
					       io_priority, cancellable);
	}
    }
  else
    {
      g_simple_async_report_gerror_in_idle ((GObject *)self, callback,
					    user_data, error);
    }
}

gssize
g_datagram_socket_send_finish (GDatagramSocket  *self,
			       GAsyncResult     *_res,
			       GError          **error)
{
  GSimpleAsyncResult *res;

  g_return_val_if_fail ((GObject *)self != g_async_result_get_source_object(_res), FALSE);
  g_return_val_if_fail ((res = G_SIMPLE_ASYNC_RESULT (_res)), FALSE);

  g_simple_async_result_propagate_error (res, error);
  return g_simple_async_result_get_op_res_gssize (res);
}

gssize
g_datagram_socket_receive (GDatagramSocket  *self,
			   GSocketAddress  **source,
			   void             *buf,
			   gsize             size,
			   GCancellable     *cancellable,
			   GError          **error)
{
  GDatagramSocketClass *klass;

  g_return_val_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)), -1);
  g_return_val_if_fail (klass->receive, -1);

  if (g_socket_set_pending ((GSocket *)self, error))
    {
      gssize res;

      res = klass->receive (self, source, buf, size, cancellable, error);
      g_socket_clear_pending ((GSocket *)self);

      return res;
    }
  else
    {
      return -1;
    }
}

typedef struct {
  GAsyncReadyCallback callback;
  gpointer user_data;
  GSocketAddress **source;
  void *buf;
  gsize size;
} ReceiveAsyncData;

static void
receive_callback (GObject      *source,
		  GAsyncResult *res,
		  gpointer      user_data)
{
  ReceiveAsyncData *data;
  GAsyncReadyCallback callback;

  data = (ReceiveAsyncData *)user_data;
  callback = data->callback;
  user_data = data->user_data;

  g_slice_free (ReceiveAsyncData, data);

  callback (source, res, user_data);
}

static void
receive_thread (GSimpleAsyncResult *res,
		GObject            *object,
		GCancellable       *cancellable)
{
  gboolean result;
  ReceiveAsyncData *data;
  GError *error;
  GDatagramSocketClass *klass;

  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (object)));
  g_return_if_fail (klass->receive);
  
  error = NULL;
  data = g_simple_async_result_get_op_res_gpointer (res);
  result = klass->receive ((GDatagramSocket *)object,
			   data->source,
			   data->buf,
			   data->size,
			   cancellable, &error);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (res, error);
    }
  g_simple_async_result_set_op_res_gboolean (res, result);

  g_simple_async_result_complete_in_idle (res);
}

void
g_datagram_socket_receive_async (GDatagramSocket      *self,
				 GSocketAddress      **source,
				 void                 *buf,
				 gsize                 size,
				 int                   io_priority,
				 GCancellable         *cancellable,
				 GAsyncReadyCallback   callback,
				 void                 *user_data)
{
  GDatagramSocketClass *klass;
  ReceiveAsyncData *data;
  GError *error;
  
  g_return_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)));

  if (g_socket_set_pending ((GSocket *)self, &error))
    {
      if (klass->receive_async)
	{
	  data = g_slice_new (ReceiveAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
	  klass->receive_async (self, source, buf, size, io_priority,
				cancellable, receive_callback, data); 
	}
      else
	{
	  GSimpleAsyncResult *res;

	  data = g_slice_new (ReceiveAsyncData);
	  data->callback = callback;
	  data->user_data = user_data;
	  data->source = source;
	  data->buf = buf;
	  data->size = size;
      
	  res = g_simple_async_result_new ((GObject *)self, receive_callback,
					   data,
					   g_datagram_socket_receive_async);

	  g_simple_async_result_set_op_res_gpointer (res, data, NULL);

	  g_simple_async_result_run_in_thread (res, receive_thread,
					       io_priority, cancellable);
	}
    }
  else
    {
      g_simple_async_report_gerror_in_idle ((GObject *)self, callback,
					    user_data, error);
    }
}

gssize
g_datagram_socket_receive_finish (GDatagramSocket  *self,
				  GAsyncResult     *_res,
				  GError          **error)
{
  GSimpleAsyncResult *res;

  g_return_val_if_fail ((GObject *)self != g_async_result_get_source_object(_res), FALSE);
  g_return_val_if_fail ((res = G_SIMPLE_ASYNC_RESULT (_res)), FALSE);

  g_simple_async_result_propagate_error (res, error);
  return g_simple_async_result_get_op_res_gssize (res);
}

gboolean
g_datagram_socket_has_next (GDatagramSocket *self)
{
  GDatagramSocketClass *klass;
  
  g_return_val_if_fail ((klass = G_DATAGRAM_SOCKET_GET_CLASS (self)), FALSE);
  g_return_val_if_fail (klass->has_next, FALSE);

  if (g_socket_set_pending ((GSocket *)self, NULL))
    {
      gboolean res;

      res = klass->has_next (self);
      g_socket_clear_pending ((GSocket *)self);

      return res;
    }
  else
    {
      return FALSE;
    }
}
