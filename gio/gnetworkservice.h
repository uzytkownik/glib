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

#ifndef __G_NETWORK_SERVICE_H__
#define __G_NETWORK_SERVICE_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define G_TYPE_NETWORK_SERVICE         (g_network_service_get_type ())
#define G_NETWORK_SERVICE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_NETWORK_SERVICE, GNetworkService))
#define G_NETWORK_SERVICE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_NETWORK_SERVICE, GNetworkServiceClass))
#define G_IS_NETWORK_SERVICE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_NETWORK_SERVICE))
#define G_IS_NETWORK_SERVICE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_NETWORK_SERVICE))
#define G_NETWORK_SERVICE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_NETWORK_SERVICE, GNetworkServiceClass))

typedef struct _GNetworkServiceClass   GNetworkServiceClass;
typedef struct _GNetworkServicePrivate GNetworkServicePrivate;

struct _GNetworkService
{
  GObject parent_instance;

  /*< private >*/
  GNetworkServicePrivate *priv;
};

struct _GNetworkServiceClass
{
  GObjectClass parent_class;

};

GType             g_network_service_get_type      (void) G_GNUC_CONST;

GNetworkService  *g_network_service_new           (const gchar      *service,
						   const gchar      *protocol,
						   const gchar      *domain);

const gchar      *g_network_service_get_service   (GNetworkService  *srv);
const gchar      *g_network_service_get_protocol  (GNetworkService  *srv);
const gchar      *g_network_service_get_domain    (GNetworkService  *srv);
GSrvTarget      **g_network_service_get_targets   (GNetworkService  *srv);
time_t            g_network_service_get_expires   (GNetworkService  *srv);

GType g_srv_target_get_type (void) G_GNUC_CONST;
#define G_TYPE_SRV_TARGET (g_srv_target_get_type ())

GSrvTarget       *g_srv_target_new                (const gchar      *hostname,
						   guint16           port,
						   guint16           priority,
						   guint16           weight,
						   time_t            expires);
GSrvTarget       *g_srv_target_copy               (GSrvTarget       *target);
void              g_srv_target_free               (GSrvTarget       *target);

const gchar      *g_srv_target_get_hostname       (GSrvTarget       *target);
guint16           g_srv_target_get_port           (GSrvTarget       *target);
guint16           g_srv_target_get_priority       (GSrvTarget       *target);
guint16           g_srv_target_get_weight         (GSrvTarget       *target);

void              g_srv_target_array_sort         (GSrvTarget      **targets);

G_END_DECLS

#endif /* __G_NETWORK_SERVICE_H__ */

