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

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef G_CONNECTION_H
#define G_CONNECTION_H

#include <glib-object.h>

#include "giotypes.h"

G_BEGIN_DECLS

#define G_TYPE_CONNECTION                                               \
   (g_connection_get_type())
#define G_CONNECTION(obj)                                               \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_CONNECTION,                      \
                                GConnection))
#define G_CONNECTION_CLASS(klass)                                       \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_CONNECTION,                         \
                             GConnectionClass))
#define IS_G_CONNECTION(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_CONNECTION))
#define IS_G_CONNECTION_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_CONNECTION))
#define G_CONNECTION_GET_CLASS(obj)                                     \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_CONNECTION,                       \
                               GConnectionClass))

typedef struct _GConnectionClass   GConnectionClass;

struct _GConnection
{
  GObject parent;
};

struct _GConnectionClass
{
  GObjectClass      parent_class;
  GSocketAddress *(*get_local_address) (GConnection *self);
  GSocketAddress *(*get_peer_address)  (GConnection *self);
};

GType           g_connection_get_type          (void) G_GNUC_CONST;
GSocketAddress *g_connection_get_local_address (GConnection *self) G_GNUC_PURE;
GSocketAddress *g_connection_get_peer_address  (GConnection *self) G_GNUC_PURE;

G_END_DECLS

#endif
