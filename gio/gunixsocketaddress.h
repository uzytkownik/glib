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
 * Authors: Christian Kellner <gicmo@gnome.org>
 *          Samuel Cormier-Iijima <sciyoshi@gmail.com>
 */

#ifndef __G_UNIX_SOCKET_ADDRESS_H__
#define __G_UNIX_SOCKET_ADDRESS_H__

#include <gio/gsocketaddress.h>

G_BEGIN_DECLS

#define G_TYPE_UNIX_SOCKET_ADDRESS         (g_unix_socket_address_get_type ())
#define G_UNIX_SOCKET_ADDRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddress))
#define G_UNIX_SOCKET_ADDRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddressClass))
#define G_IS_UNIX_SOCKET_ADDRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_UNIX_SOCKET_ADDRESS))
#define G_IS_UNIX_SOCKET_ADDRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_UNIX_SOCKET_ADDRESS))
#define G_UNIX_SOCKET_ADDRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_UNIX_SOCKET_ADDRESS, GUnixSocketAddressClass))

typedef struct _GUnixSocketAddress        GUnixSocketAddress;
typedef struct _GUnixSocketAddressClass   GUnixSocketAddressClass;
typedef struct _GUnixSocketAddressPrivate GUnixSocketAddressPrivate;

struct _GUnixSocketAddress
{
  GSocketAddress parent;

  GUnixSocketAddressPrivate *priv;
};

struct _GUnixSocketAddressClass
{
  GSocketAddressClass parent_class;
};

GType                g_unix_socket_address_get_type    (void) G_GNUC_CONST;

GUnixSocketAddress * g_unix_socket_address_new         (const gchar *path);

G_END_DECLS

#endif /* __G_UNIX_SOCKET_ADDRESS_H__ */
