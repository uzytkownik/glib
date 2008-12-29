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

#ifndef G_NATIVE_DATAGREAM_SOCKET_H
#define G_NATIVE_DATAGREAM_SOCKET_H

#include <glib-object.h>
#include <gio/gdatagramsocket.h>
#include <gio/gioenums.h>

G_BEGIN_DECLS

#define G_TYPE_NATIVE_DATAGREAM_SOCKET                                               \
   (g_native_datagream_socket_get_type())
#define G_NATIVE_DATAGREAM_SOCKET(obj)                                               \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_NATIVE_DATAGREAM_SOCKET,                      \
                                GNativeDatagramSocket))
#define G_NATIVE_DATAGREAM_SOCKET_CLASS(klass)                                       \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_NATIVE_DATAGREAM_SOCKET,                         \
                             GNativeDatagramSocketClass))
#define IS_G_NATIVE_DATAGREAM_SOCKET(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_NATIVE_DATAGREAM_SOCKET))
#define IS_G_NATIVE_DATAGREAM_SOCKET_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_NATIVE_DATAGREAM_SOCKET))
#define G_NATIVE_DATAGREAM_SOCKET_GET_CLASS(obj)                                     \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_NATIVE_DATAGREAM_SOCKET,                       \
                               GNativeDatagramSocketClass))

typedef struct _GNativeDatagramSocket        GNativeDatagramSocket;
typedef struct _GNativeDatagramSocketClass   GNativeDatagramSocketClass;
typedef struct _GNativeDatagramSocketPrivate GNativeDatagramSocketPrivate;

struct _GNativeDatagramSocket
{
  GDatagramSocket parent;
  GNativeDatagramSocketPrivate *priv;
};

struct _GNativeDatagramSocketClass
{
  GDatagramSocketClass parent_class;
};

GType                 g_native_datagream_socket_get_type          (void) G_GNUC_CONST;
GNativeDatagramSocket *g_native_datagream_socket_new              (GInetAddressFamily inet);
GInetAddressFamily     g_native_datagream_socket_get_inet_familly (GNativeDatagramSocket *socket);
G_END_DECLS

#endif
