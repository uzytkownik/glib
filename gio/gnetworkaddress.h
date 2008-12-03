/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
 */

#if !defined (__GIO_GIO_H_INSIDE__) && !defined (GIO_COMPILATION)
#error "Only <gio/gio.h> can be included directly."
#endif

#ifndef __G_NETWORK_ADDRESS_H__
#define __G_NETWORK_ADDRESS_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define G_TYPE_NETWORK_ADDRESS         (g_network_address_get_type ())
#define G_NETWORK_ADDRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_NETWORK_ADDRESS, GNetworkAddress))
#define G_NETWORK_ADDRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_NETWORK_ADDRESS, GNetworkAddressClass))
#define G_IS_NETWORK_ADDRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_NETWORK_ADDRESS))
#define G_IS_NETWORK_ADDRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_NETWORK_ADDRESS))
#define G_NETWORK_ADDRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_NETWORK_ADDRESS, GNetworkAddressClass))

typedef struct _GNetworkAddressClass   GNetworkAddressClass;
typedef struct _GNetworkAddressPrivate GNetworkAddressPrivate;

struct _GNetworkAddress
{
  GObject parent_instance;

  /*< private >*/
  GNetworkAddressPrivate *priv;
};

struct _GNetworkAddressClass
{
  GObjectClass parent_class;

};

GType             g_network_address_get_type       (void) G_GNUC_CONST;

const gchar      *g_network_address_get_hostname   (GNetworkAddress  *addr);
const gchar      *g_network_address_get_ascii_name (GNetworkAddress  *addr);
gushort           g_network_address_get_port       (GNetworkAddress  *addr);
GSockaddr       **g_network_address_get_sockaddrs  (GNetworkAddress  *addr);


GType g_sockaddr_get_type (void) G_GNUC_CONST;
#define G_TYPE_SOCKADDR (g_sockaddr_get_type ())

GSockaddr        *g_sockaddr_new_from_string       (const gchar      *ip_addr,
						    gushort           port);
GSockaddr        *g_sockaddr_copy                  (GSockaddr        *sockaddr);
void              g_sockaddr_free                  (GSockaddr        *sockaddr);
gushort           g_sockaddr_get_port              (GSockaddr        *sockaddr);
gchar            *g_sockaddr_to_string             (GSockaddr        *sockaddr);
gsize             g_sockaddr_size                  (GSockaddr        *sockaddr);

G_END_DECLS

#endif /* __G_NETWORK_ADDRESS_H__ */
