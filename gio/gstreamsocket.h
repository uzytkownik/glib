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

#ifndef G_STREAM_SOCKET_H
#define G_STREAM_SOCKET_H

#include <glib-object.h>

#include "giotypes.h"
#include "gsocket.h"


G_BEGIN_DECLS

#define G_TYPE_STREAM_SOCKET                                            \
   (g_stream_socket_get_type())
#define G_STREAM_SOCKET(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_STREAM_SOCKET,                   \
                                GStreamSocket))
#define G_STREAM_SOCKET_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_STREAM_SOCKET,                      \
                             GStreamSocketClass))
#define IS_G_STREAM_SOCKET(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_STREAM_SOCKET))
#define IS_G_STREAM_SOCKET_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_STREAM_SOCKET))
#define G_STREAM_SOCKET_GET_CLASS(obj)                                  \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_STREAM_SOCKET,                    \
                               GStreamSocketClass))

typedef struct _GStreamSocket      GStreamSocket;
typedef struct _GStreamSocketClass GStreamSocketClass;

struct _GStreamSocket
{
  GSocket parent;
};

struct _GStreamSocketClass
{
  GSocketClass parent_class;
  GSocketAddress    *(*get_remote_address) (GStreamSocket       *self);
  GStreamConnection *(*connect)            (GStreamSocket        *self,
					    GCancellable         *cancllable,
					    GError              **error);
  void               (*connect_async)      (GStreamSocket        *self,
					    int                   io_priority,
					    GCancellable         *cancellable,
					    GAsyncReadyCallback   callback,
					    void                 *user_data);
  GStreamConnection *(*connect_finish)     (GStreamSocket        *self,
					    GAsyncResult         *res,
					    GError              **error);
};

GType              g_stream_socket_get_type           (void) G_GNUC_CONST;
GSocketAddress    *g_stream_socket_get_remote_address (GStreamSocket        *self);
GStreamConnection *g_stream_socket_connect            (GStreamSocket        *self,
						       GCancellable         *cancllable,
						       GError               **error);
void               g_stream_socket_connect_async      (GStreamSocket        *self,
						       int                   io_priority,
						       GCancellable         *cancellable,
						       GAsyncReadyCallback   callback,
						       void                 *user_data);
GStreamConnection *g_stream_socket_connect_finish     (GStreamSocket        *self,
						       GAsyncResult         *res,
						       GError              **error);

G_END_DECLS

#endif
