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

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_INET4_ADDRESS_H__
#define __G_INET4_ADDRESS_H__

#include <gio/ginetaddress.h>

G_BEGIN_DECLS

#define G_TYPE_INET4_ADDRESS         (g_inet4_address_get_type ())
#define G_INET4_ADDRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_INET4_ADDRESS, GInet4Address))
#define G_INET4_ADDRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_INET4_ADDRESS, GInet4AddressClass))
#define G_IS_INET4_ADDRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_INET4_ADDRESS))
#define G_IS_INET4_ADDRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_INET4_ADDRESS))
#define G_INET4_ADDRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_INET4_ADDRESS, GInet4AddressClass))

typedef struct _GInet4AddressClass   GInet4AddressClass;
typedef struct _GInet4AddressPrivate GInet4AddressPrivate;

struct _GInet4Address
{
  GInetAddress parent;

  GInet4AddressPrivate *priv;
};

struct _GInet4AddressClass
{
  GInetAddressClass parent_class;
};

GType          g_inet4_address_get_type     (void) G_GNUC_CONST;

GInetAddress * g_inet4_address_from_string  (const char *string);

GInetAddress * g_inet4_address_from_bytes   (const guint8 bytes[4]);

GInetAddress * g_inet4_address_new_loopback (void);

GInetAddress * g_inet4_address_new_any      (void);

G_END_DECLS

#endif /* __G_INET4_ADDRESS_H__ */
