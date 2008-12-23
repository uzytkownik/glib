/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Christian Kellner, Samuel Cormier-Iijima
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

#include "gstreamconnection.h"

enum
  {
    PROP_0,
    PROP_INPUT,
    PROP_OUTPUT
  };

G_DEFINE_ABSTRACT_TYPE (GStreamConnection, g_stream_connection, G_TYPE_CONNECTION);

static void g_stream_connection_get_property (GObject    *object,
					      guint       prop_id,
					      GValue     *value,
					      GParamSpec *pspec);

static void
g_stream_connection_class_init (GStreamConnectionClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;
  
  gobject_class->get_property = g_stream_connection_get_property;
  
  g_object_class_install_property (gobject_class, PROP_INPUT,
				   g_param_spec_object ("input-stream",
							"input stream",
							"input stream for connection",
							G_TYPE_INPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_OUTPUT,
				   g_param_spec_object ("output-stream",
							"output stream",
							"output stream for connection",
							G_TYPE_OUTPUT_STREAM,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
g_stream_connection_init (GStreamConnection *self)
{
}

static void
g_stream_connection_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
  GStreamConnection      *self;
  GStreamConnectionClass *klass;
  
  self = G_STREAM_CONNECTION (object);
  klass = G_STREAM_CONNECTION_GET_CLASS (self);

  switch (prop_id)
    {
    case PROP_INPUT:
      g_value_take_object (value, klass->get_input_stream (self));
      break;
    case PROP_OUTPUT:
      g_value_take_object (value, klass->get_output_stream (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GInputStream *
g_stream_connection_get_input_stream  (GStreamConnection *self)
{
  GStreamConnectionClass *klass;

  g_return_val_if_fail (klass = G_STREAM_CONNECTION_GET_CLASS (self), NULL);
  g_return_val_if_fail (klass->get_input_stream, NULL);

  return klass->get_input_stream (self);
}

GOutputStream *
g_stream_connection_get_output_stream (GStreamConnection *self)
{
  GStreamConnectionClass *klass;

  g_return_val_if_fail (klass = G_STREAM_CONNECTION_GET_CLASS (self), NULL);
  g_return_val_if_fail (klass->get_output_stream, NULL);

  return klass->get_output_stream (self);
}
