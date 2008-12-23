/*  GIO - GLib Input, Output and Streaming Library
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

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef G_STREAM_CONNECTION_H
#define G_STREAM_CONNECTION_H

#include "gconnection.h"
#include <gio/gio.h>

G_BEGIN_DECLS

#define G_TYPE_STREAM_CONNECTION                                        \
   (g_stream_connection_get_type())
#define G_STREAM_CONNECTION(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_STREAM_CONNECTION,               \
                                GStreamConnection))
#define G_STREAM_CONNECTION_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_STREAM_CONNECTION,                  \
                             GStreamConnectionClass))
#define IS_G_STREAM_CONNECTION(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_STREAM_CONNECTION))
#define IS_G_STREAM_CONNECTION_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_STREAM_CONNECTION))
#define G_STREAM_CONNECTION_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_STREAM_CONNECTION,                \
                               GStreamConnectionClass))

typedef struct _GStreamConnection      GStreamConnection;
typedef struct _GStreamConnectionClass GStreamConnectionClass;

struct _GStreamConnection
{
  GConnection parent;
};

struct _GStreamConnectionClass
{
  GConnectionClass    parent_class;
  GInputStream     *(*get_input_stream)  (GStreamConnection *self);
  GOutputStream    *(*get_output_stream) (GStreamConnection *self);
};

GType          g_stream_connection_get_type          (void) G_GNUC_CONST;
GInputStream  *g_stream_connection_get_input_stream  (GStreamConnection *self) G_GNUC_PURE;
GOutputStream *g_stream_connection_get_output_stream (GStreamConnection *self) G_GNUC_PURE;

G_END_DECLS

#endif
