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

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef G_SOCKET_H
#define G_SOCKET_H

#include <glib-object.h>

#include "giotypes.h"

G_BEGIN_DECLS

#define G_TYPE_SOCKET                                                   \
   (g_socket_get_type())
#define G_SOCKET(obj)                                                   \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_SOCKET,                          \
                                GSocket))
#define G_SOCKET_CLASS(klass)                                           \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_SOCKET,                             \
                             GSocketClass))
#define IS_G_SOCKET(obj)                                                \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_SOCKET))
#define IS_G_SOCKET_CLASS(klass)                                        \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_SOCKET))
#define G_SOCKET_GET_CLASS(obj)                                         \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_SOCKET,                           \
                               GSocketClass))

typedef struct _GSocketClass   GSocketClass;
typedef struct _GSocketPrivate GSocketPrivate;

struct _GSocket
{
  GObject parent;
  GSocketPrivate *priv;
};

struct _GSocketClass
{
  GObjectClass parent_class;
  GSocketAddress *(*get_local_address) (GSocket *self);
};

GType           g_socket_get_type          (void) G_GNUC_CONST;
GSocketAddress *g_socket_get_local_address (GSocket          *self) G_GNUC_PURE;

/* For implementations: */
gboolean        g_socket_has_pending       (GSocket          *socket);
gboolean        g_socket_set_pending       (GSocket          *socket,
					    GError          **error);
void            g_socket_clear_pending     (GSocket          *socket);

G_END_DECLS

#endif
