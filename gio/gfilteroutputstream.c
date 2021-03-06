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
 * Author: Christian Kellner <gicmo@gnome.org> 
 */

#include "config.h"
#include "gfilteroutputstream.h"
#include "gsimpleasyncresult.h"
#include "goutputstream.h"
#include "glibintl.h"

#include "gioalias.h"

/**
 * SECTION:gfilteroutputstream
 * @short_description: Filter Output Stream
 * @include: gio/gio.h
 *
 **/

enum {
  PROP_0,
  PROP_BASE_STREAM,
  PROP_CLOSE_BASE
};

static void     g_filter_output_stream_set_property (GObject      *object,
                                                     guint         prop_id,
                                                     const GValue *value,
                                                     GParamSpec   *pspec);

static void     g_filter_output_stream_get_property (GObject    *object,
                                                     guint       prop_id,
                                                     GValue     *value,
                                                     GParamSpec *pspec);
static void     g_filter_output_stream_dispose      (GObject *object);


static gssize   g_filter_output_stream_write        (GOutputStream *stream,
                                                     const void    *buffer,
                                                     gsize          count,
                                                     GCancellable  *cancellable,
                                                     GError       **error);
static gboolean g_filter_output_stream_flush        (GOutputStream    *stream,
                                                     GCancellable  *cancellable,
                                                     GError          **error);
static gboolean g_filter_output_stream_close        (GOutputStream  *stream,
                                                     GCancellable   *cancellable,
                                                     GError        **error);
static void     g_filter_output_stream_write_async  (GOutputStream        *stream,
                                                     const void           *buffer,
                                                     gsize                 count,
                                                     int                   io_priority,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              data);
static gssize   g_filter_output_stream_write_finish (GOutputStream        *stream,
                                                     GAsyncResult         *result,
                                                     GError              **error);
static void     g_filter_output_stream_flush_async  (GOutputStream        *stream,
                                                     int                   io_priority,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              data);
static gboolean g_filter_output_stream_flush_finish (GOutputStream        *stream,
                                                     GAsyncResult         *result,
                                                     GError              **error);
static void     g_filter_output_stream_close_async  (GOutputStream        *stream,
                                                     int                   io_priority,
                                                     GCancellable         *cancellable,
                                                     GAsyncReadyCallback   callback,
                                                     gpointer              data);
static gboolean g_filter_output_stream_close_finish (GOutputStream        *stream,
                                                     GAsyncResult         *result,
                                                     GError              **error);



G_DEFINE_TYPE (GFilterOutputStream, g_filter_output_stream, G_TYPE_OUTPUT_STREAM)

#define GET_PRIVATE(inst) G_TYPE_INSTANCE_GET_PRIVATE (inst, \
  G_TYPE_FILTER_OUTPUT_STREAM, GFilterOutputStreamPrivate)

typedef struct
{
  gboolean close_base;
} GFilterOutputStreamPrivate;

static void
g_filter_output_stream_class_init (GFilterOutputStreamClass *klass)
{
  GObjectClass *object_class;
  GOutputStreamClass *ostream_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = g_filter_output_stream_get_property;
  object_class->set_property = g_filter_output_stream_set_property;
  object_class->dispose      = g_filter_output_stream_dispose;
    
  ostream_class = G_OUTPUT_STREAM_CLASS (klass);
  ostream_class->write_fn = g_filter_output_stream_write;
  ostream_class->flush = g_filter_output_stream_flush;
  ostream_class->close_fn = g_filter_output_stream_close;
  ostream_class->write_async  = g_filter_output_stream_write_async;
  ostream_class->write_finish = g_filter_output_stream_write_finish;
  ostream_class->flush_async  = g_filter_output_stream_flush_async;
  ostream_class->flush_finish = g_filter_output_stream_flush_finish;
  ostream_class->close_async  = g_filter_output_stream_close_async;
  ostream_class->close_finish = g_filter_output_stream_close_finish;

  g_type_class_add_private (klass, sizeof (GFilterOutputStreamPrivate));

  g_object_class_install_property (object_class,
                                   PROP_BASE_STREAM,
                                   g_param_spec_object ("base-stream",
                                                         P_("The Filter Base Stream"),
                                                         P_("The underlying base stream the io ops will be done on"),
                                                         G_TYPE_OUTPUT_STREAM,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | 
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));

  g_object_class_install_property (object_class,
                                   PROP_CLOSE_BASE,
                                   g_param_spec_boolean ("close-base-stream",
                                                         P_("Close Base Stream"),
                                                         P_("If the base stream be closed when the filter stream is"),
                                                         TRUE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
                                                         G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB));
}

static void
g_filter_output_stream_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  GFilterOutputStream *filter_stream;
  GObject *obj;

  filter_stream = G_FILTER_OUTPUT_STREAM (object);

  switch (prop_id) 
    {
    case PROP_BASE_STREAM:
      obj = g_value_dup_object (value);
      filter_stream->base_stream = G_OUTPUT_STREAM (obj);
      break;

    case PROP_CLOSE_BASE:
      g_filter_output_stream_set_close_base_stream (filter_stream,
                                                    g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_output_stream_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  GFilterOutputStream *filter_stream;

  filter_stream = G_FILTER_OUTPUT_STREAM (object);

  switch (prop_id)
    {
    case PROP_BASE_STREAM:
      g_value_set_object (value, filter_stream->base_stream);
      break;

    case PROP_CLOSE_BASE:
      g_value_set_boolean (value, GET_PRIVATE (filter_stream)->close_base);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_filter_output_stream_dispose (GObject *object)
{
  GFilterOutputStream *stream;

  stream = G_FILTER_OUTPUT_STREAM (object);

  G_OBJECT_CLASS (g_filter_output_stream_parent_class)->dispose (object);
  
  if (stream->base_stream)
    {
      g_object_unref (stream->base_stream);
      stream->base_stream = NULL;
    }
}


static void
g_filter_output_stream_init (GFilterOutputStream *stream)
{
}

/**
 * g_filter_output_stream_get_base_stream:
 * @stream: a #GFilterOutputStream.
 * 
 * Gets the base stream for the filter stream.
 *
 * Returns: a #GOutputStream.
 **/
GOutputStream *
g_filter_output_stream_get_base_stream (GFilterOutputStream *stream)
{
  g_return_val_if_fail (G_IS_FILTER_OUTPUT_STREAM (stream), NULL);

  return stream->base_stream;
}

/**
 * g_filter_output_stream_get_close_base_stream:
 * @stream: a #GFilterOutputStream.
 *
 * Returns whether the base stream will be closed when @stream is
 * closed.
 *
 * Return value: %TRUE if the base stream will be closed.
 **/
gboolean
g_filter_output_stream_get_close_base_stream (GFilterOutputStream *stream)
{
  g_return_val_if_fail (G_IS_FILTER_OUTPUT_STREAM (stream), FALSE);

  return GET_PRIVATE (stream)->close_base;
}

/**
 * g_filter_output_stream_set_close_base_stream:
 * @stream: a #GFilterOutputStream.
 * @close_base: %TRUE to close the base stream.
 *
 * Sets whether the base stream will be closed when @stream is closed.
 **/
void
g_filter_output_stream_set_close_base_stream (GFilterOutputStream *stream,
                                              gboolean             close_base)
{
  GFilterOutputStreamPrivate *priv;

  g_return_if_fail (G_IS_FILTER_OUTPUT_STREAM (stream));

  close_base = !!close_base;

  priv = GET_PRIVATE (stream);

  if (priv->close_base != close_base)
    {
      priv->close_base = close_base;
      g_object_notify (G_OBJECT (stream), "close-base-stream");
    }
}

static gssize
g_filter_output_stream_write (GOutputStream  *stream,
                              const void     *buffer,
                              gsize           count,
                              GCancellable   *cancellable,
                              GError        **error)
{
  GFilterOutputStream *filter_stream;
  gssize nwritten;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  nwritten = g_output_stream_write (filter_stream->base_stream,
                                    buffer,
                                    count,
                                    cancellable,
                                    error);

  return nwritten;
}

static gboolean
g_filter_output_stream_flush (GOutputStream  *stream,
                              GCancellable   *cancellable,
                              GError        **error)
{
  GFilterOutputStream *filter_stream;
  gboolean res;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  res = g_output_stream_flush (filter_stream->base_stream,
                               cancellable,
                               error);

  return res;
}

static gboolean
g_filter_output_stream_close (GOutputStream  *stream,
                              GCancellable   *cancellable,
                              GError        **error)
{
  gboolean res = TRUE;

  if (GET_PRIVATE (stream)->close_base)
    {
      GFilterOutputStream *filter_stream;

      filter_stream = G_FILTER_OUTPUT_STREAM (stream);

      res = g_output_stream_close (filter_stream->base_stream,
                                   cancellable,
                                   error);
    }

  return res;
}

static void
g_filter_output_stream_write_async (GOutputStream       *stream,
                                    const void          *buffer,
                                    gsize                count,
                                    int                  io_priority,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             data)
{
  GFilterOutputStream *filter_stream;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  g_output_stream_write_async (filter_stream->base_stream,
                               buffer,
                               count,
                               io_priority,
                               cancellable,
                               callback,
                               data);

}

static gssize
g_filter_output_stream_write_finish (GOutputStream  *stream,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  GFilterOutputStream *filter_stream;
  gssize nwritten;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  nwritten = g_output_stream_write_finish (filter_stream->base_stream,
                                           result,
                                           error);

  return nwritten;
}

static void
g_filter_output_stream_flush_async (GOutputStream       *stream,
                                    int                  io_priority,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             data)
{
  GFilterOutputStream *filter_stream;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  g_output_stream_flush_async (filter_stream->base_stream,
                               io_priority,
                               cancellable,
                               callback,
                               data);
}

static gboolean
g_filter_output_stream_flush_finish (GOutputStream  *stream,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  GFilterOutputStream *filter_stream;
  gboolean res;

  filter_stream = G_FILTER_OUTPUT_STREAM (stream);

  res = g_output_stream_flush_finish (filter_stream->base_stream,
                                      result,
                                      error);

  return res;
}

static void
g_filter_output_stream_close_ready (GObject       *object,
                                    GAsyncResult  *result,
                                    gpointer       user_data)
{
  GSimpleAsyncResult *simple = user_data;
  GError *error = NULL;

  g_output_stream_close_finish (G_OUTPUT_STREAM (object), result, &error);

  if (error)
    {
      g_simple_async_result_set_from_error (simple, error);
      g_error_free (error);
    }

  g_simple_async_result_complete (simple);
  g_object_unref (simple);
}

static void
g_filter_output_stream_close_async (GOutputStream       *stream,
                                    int                  io_priority,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  GSimpleAsyncResult *simple;

  simple = g_simple_async_result_new (G_OBJECT (stream),
                                      callback, user_data,
                                      g_filter_output_stream_close_async);

  if (GET_PRIVATE (stream)->close_base)
    {
      GFilterOutputStream *filter_stream = G_FILTER_OUTPUT_STREAM (stream);

      g_output_stream_close_async (filter_stream->base_stream,
                                  io_priority, cancellable,
                                  g_filter_output_stream_close_ready,
                                  g_object_ref (simple));
    }
  else
    /* do nothing */
    g_simple_async_result_complete_in_idle (simple);

  g_object_unref (simple);
}

static gboolean
g_filter_output_stream_close_finish (GOutputStream  *stream,
                                     GAsyncResult   *result,
                                     GError        **error)
{
  GSimpleAsyncResult *simple;

  g_return_val_if_fail (g_simple_async_result_is_valid (
    result, G_OBJECT (stream), g_filter_output_stream_close_async), FALSE);

  simple = G_SIMPLE_ASYNC_RESULT (result);

  return !g_simple_async_result_propagate_error (simple, error);
}

#define __G_FILTER_OUTPUT_STREAM_C__
#include "gioaliasdef.c"
