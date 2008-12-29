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

#ifndef G_UDP_SOCKET_H
#define G_UDP_SOCKET_H

#include <glib-object.h>
#include <gio/gdatagramsocket.h>
#include <gio/gioenums.h>

G_BEGIN_DECLS

#define G_TYPE_UDP_SOCKET                                               \
   (g_udp_socket_get_type())
#define G_UDP_SOCKET(obj)                                               \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                G_TYPE_UDP_SOCKET,                      \
                                GUDPSocket))
#define G_UDP_SOCKET_CLASS(klass)                                       \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             G_TYPE_UDP_SOCKET,                         \
                             GUDPSocketClass))
#define IS_G_UDP_SOCKET(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                G_TYPE_UDP_SOCKET))
#define IS_G_UDP_SOCKET_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             G_TYPE_UDP_SOCKET))
#define G_UDP_SOCKET_GET_CLASS(obj)                                     \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               G_TYPE_UDP_SOCKET,                       \
                               GUDPSocketClass))

typedef struct _GUDPSocket        GUDPSocket;
typedef struct _GUDPSocketClass   GUDPSocketClass;
typedef struct _GUDPSocketPrivate GUDPSocketPrivate;

struct _GUDPSocket
{
  GDatagramSocket parent;
  GUDPSocketPrivate *priv;
};

struct _GUDPSocketClass
{
  GDatagramSocketClass parent_class;
};

GType               g_udp_socket_get_type         (void) G_GNUC_CONST;
GUDPSocket         *g_udp_socket_new              (GInetAddressFamily inet);
GInetAddressFamily  g_udp_socket_get_inet_familly (GUDPSocket *socket);
G_END_DECLS

#endif
