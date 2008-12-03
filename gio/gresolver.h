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

#ifndef __G_RESOLVER_H__
#define __G_RESOLVER_H__

#include <gio/giotypes.h>

G_BEGIN_DECLS

#define G_TYPE_RESOLVER         (g_resolver_get_type ())
#define G_RESOLVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_RESOLVER, GResolver))
#define G_RESOLVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_RESOLVER, GResolverClass))
#define G_IS_RESOLVER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_RESOLVER))
#define G_IS_RESOLVER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_RESOLVER))
#define G_RESOLVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_RESOLVER, GResolverClass))

struct _GResolver {
  GObject parent_instance;

};

typedef struct {
  GObjectClass parent_class;

  gboolean ( *lookup_name)           (GResolver           *resolver,
				      GNetworkAddress     *address,
				      GCancellable        *cancellable,
				      GError             **error);
  void     ( *lookup_name_async)     (GResolver           *resolver,
				      GNetworkAddress     *address,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data);
  gboolean ( *lookup_name_finish)    (GResolver           *resolver,
				      GAsyncResult        *result,
				      GError             **error);

  gboolean ( *lookup_address)        (GResolver           *resolver,
				      GNetworkAddress     *address,
				      GCancellable        *cancellable,
				      GError             **error);
  void     ( *lookup_address_async)  (GResolver           *resolver,
				      GNetworkAddress     *address,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data);
  gboolean ( *lookup_address_finish) (GResolver           *resolver,
				      GAsyncResult        *result,
				      GError             **error);

  gboolean ( *lookup_service)        (GResolver           *resolver,
				      GNetworkService     *srv,
				      GCancellable        *cancellable,
				      GError             **error);
  void     ( *lookup_service_async)  (GResolver           *resolver,
				      GNetworkService     *srv,
				      GCancellable        *cancellable,
				      GAsyncReadyCallback  callback,
				      gpointer             user_data);
  gboolean ( *lookup_service_finish) (GResolver           *resolver,
				      GAsyncResult        *result,
				      GError             **error);

  /* Padding for future expansion */
  void (*_g_reserved1) (void);
  void (*_g_reserved2) (void);
  void (*_g_reserved3) (void);
  void (*_g_reserved4) (void);
  void (*_g_reserved5) (void);

} GResolverClass;

GType            g_resolver_get_type               (void) G_GNUC_CONST;
GResolver       *g_resolver_get_default            (void);
void             g_resolver_set_default            (GResolver           *resolver);

GNetworkAddress *g_resolver_lookup_name            (GResolver           *resolver,
						    const char          *hostname,
						    gushort              port,
						    GCancellable        *cancellable,
						    GError             **error);
void             g_resolver_lookup_name_async      (GResolver           *resolver,
						    const char          *hostname,
						    gushort              port,
						    GCancellable        *cancellable,
						    GAsyncReadyCallback  callback,
						    gpointer             user_data);
GNetworkAddress *g_resolver_lookup_name_finish     (GResolver           *resolver,
						    GAsyncResult        *result,
						    GError             **error);

GNetworkAddress *g_resolver_lookup_address         (GResolver           *resolver,
						    GSockaddr           *sockaddr,
						    GCancellable        *cancellable,
						    GError             **error);
void             g_resolver_lookup_address_async   (GResolver           *resolver,
						    GSockaddr           *sockaddr,
						    GCancellable        *cancellable,
						    GAsyncReadyCallback  callback,
						    gpointer             user_data);
GNetworkAddress *g_resolver_lookup_address_finish  (GResolver           *resolver,
						    GAsyncResult        *result,
						    GError             **error);

GNetworkService *g_resolver_lookup_service         (GResolver           *resolver,
						    const char          *service,
						    const char          *protocol,
						    const char          *domain,
						    GCancellable        *cancellable,
						    GError             **error);
void             g_resolver_lookup_service_async   (GResolver           *resolver,
						    const char          *service,
						    const char          *protocol,
						    const char          *domain,
						    GCancellable        *cancellable,
						    GAsyncReadyCallback  callback,
						    gpointer             user_data);
GNetworkService *g_resolver_lookup_service_finish  (GResolver           *resolver,
						    GAsyncResult        *result,
						    GError             **error);

/**
 * G_RESOLVER_ERROR:
 *
 * Error domain for #GResolver. Errors in this domain will be from the
 * #GResolverError enumeration. See #GError for more information on
 * error domains.
 **/
#define G_RESOLVER_ERROR (g_resolver_error_quark ())
GQuark g_resolver_error_quark (void);

G_END_DECLS

#endif /* __G_RESOLVER_H__ */
