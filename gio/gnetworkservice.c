/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

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

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gnetworkservice.h"
#include "gresolverprivate.h"

#include <stdlib.h>
#include <string.h>

#include "gioalias.h"

/**
 * SECTION:gnetworkservice
 * @short_description: DNS SRV record support
 * @include: gio/gio.h
 *
 * SRV (service) records are used by some network protocols to provide
 * service-specific aliasing and load-balancing; when looking for a
 * service in a given domain, rather than connecting directly to the
 * toplevel hostname (eg, "example.com") or assuming a specific server
 * hostname (eg, "jabber.example.com"), you would look up the
 * appropriate SRV record, and try to connect to the server (or
 * servers) that it indicates. Continuing the example from
 * %GNetworkAddress, a jabber client might do:
 *
 * |[
 * static MyConnection *
 * my_connect_to_jabber_server (const char    *domain,
 *                              GCancellable  *cancellable,
 *                              GError       **error)
 * {
 *   GResolver *resolver;
 *   GNetworkService *service;
 *   GSrvTarget **targets;
 *   MyConnection *conn;
 *   int i;
 *
 *   resolver = g_resolver_get_default ();
 *   service = g_resolver_lookup_service (resolver, "xmpp-client", "tcp", domain,
 *                                        cancellable, error);
 *   g_object_unref (resolver);
 *   if (!service)
 *     return NULL;
 *
 *   targets = g_network_service_get_targets (service);
 *   for (i = 0; targets[i]; i++)
 *     {
 *       conn = my_connect_to_host (g_srv_target_get_hostname (targets[i]),
 *                                  g_srv_target_get_port (targets[i]),
 *                                  cancellable, NULL);
 *       if (conn)
 *         {
 *           g_object_unref (service);
 *           return conn;
 *         }
 *       else if (g_cancellable_set_error_if_cancelled (cancellable, error))
 *         return NULL;
 *     }
 *
 *   g_set_error (error, MY_ERROR_DOMAIN, MY_ERROR_CODE,
 *                "Could not connect to any jabber server for %s",
 *                domain);
 *   g_object_unref (service);
 *   return NULL;
 * }
 * ]|
 **/

struct _GSrvTarget {
  gchar   *hostname;
  guint16  port;

  guint16  priority;
  guint16  weight;
  time_t   expires;
};

/**
 * GNetworkService:
 *
 * A description of a network service. #GResolver returns these as the
 * result of looking up a SRV record. (If you need to a create a
 * #GNetworkService on your own, use g_object_new(), along with the
 * appropriate object properties.)
 **/

struct _GNetworkServicePrivate
{
  gchar *service, *protocol, *domain;
  GSrvTarget **targets;
};

enum {
  PROP_0,
  PROP_SERVICE,
  PROP_PROTOCOL,
  PROP_DOMAIN,
  PROP_TARGETS,
};

static void g_network_service_set_property (GObject      *object,
                                            guint         prop_id,
                                            const GValue *value,
                                            GParamSpec   *pspec);
static void g_network_service_get_property (GObject      *object,
                                            guint         prop_id,
                                            GValue       *value,
                                            GParamSpec   *pspec);

G_DEFINE_TYPE (GNetworkService, g_network_service, G_TYPE_OBJECT)

static void
g_network_service_finalize (GObject *object)
{
  GNetworkService *srv = G_NETWORK_SERVICE (object);
  gint i;

  g_free (srv->priv->service);
  g_free (srv->priv->protocol);
  g_free (srv->priv->domain);

  if (srv->priv->targets)
    {
      for (i = 0; srv->priv->targets[i]; i++)
        g_srv_target_free (srv->priv->targets[i]);
      g_free (srv->priv->targets);
    }

  G_OBJECT_CLASS (g_network_service_parent_class)->finalize (object);
}

static void
g_network_service_class_init (GNetworkServiceClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (GNetworkServicePrivate));

  gobject_class->set_property = g_network_service_set_property;
  gobject_class->get_property = g_network_service_get_property;
  gobject_class->finalize = g_network_service_finalize;

  g_object_class_install_property (gobject_class, PROP_SERVICE,
                                   g_param_spec_string ("service",
                                                        P_("Service"),
                                                        P_("Service name, eg \"ldap\""),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_PROTOCOL,
                                   g_param_spec_string ("protocol",
                                                        P_("Protocol"),
                                                        P_("Network protocol, eg \"tcp\""),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_DOMAIN,
                                   g_param_spec_string ("domain",
                                                        P_("domain"),
                                                        P_("Network domain, eg, \"example.com\""),
                                                        NULL,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_TARGETS,
                                   g_param_spec_pointer ("targets",
                                                         P_("Targets"),
                                                         P_("Targets for this service, an array of GSrvTarget"),
                                                         G_PARAM_READWRITE));
}

static void
g_network_service_init (GNetworkService *srv)
{
  srv->priv = G_TYPE_INSTANCE_GET_PRIVATE (srv, G_TYPE_NETWORK_SERVICE,
                                           GNetworkServicePrivate);
}

static void g_network_service_set_targets (GNetworkService  *srv,
                                           GSrvTarget      **targets);

static void
g_network_service_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GNetworkService *srv = G_NETWORK_SERVICE (object);
  GSrvTarget **targets;
  gint n;

  switch (prop_id) 
    {
    case PROP_SERVICE:
      srv->priv->service = g_value_dup_string (value);
      break;

    case PROP_PROTOCOL:
      srv->priv->protocol = g_value_dup_string (value);
      break;

    case PROP_DOMAIN:
      if (g_hostname_is_non_ascii (g_value_get_string (value)))
        srv->priv->domain = g_hostname_to_ascii (g_value_get_string (value));
      else
        srv->priv->domain = g_value_dup_string (value);
      break;

    case PROP_TARGETS:
      /* Ignore gobject when it tries to set the default value */
      if (!g_value_get_pointer (value))
        return;

      if (srv->priv->targets)
        {
          for (n = 0; srv->priv->targets[n]; n++)
            g_srv_target_free (srv->priv->targets[n]);
          g_free (srv->priv->targets);
        }

      targets = g_value_get_pointer (value);
      for (n = 0; targets[n]; n++)
        ;
      srv->priv->targets = g_new0 (GSrvTarget *, n + 1);
      for (n = 0; targets[n]; n++)
        srv->priv->targets[n] = g_srv_target_copy (targets[n]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

static void
g_network_service_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GNetworkService *srv = G_NETWORK_SERVICE (object);

  switch (prop_id)
    { 
    case PROP_SERVICE:
      g_value_set_string (value, g_network_service_get_service (srv));
      break;

    case PROP_PROTOCOL:
      g_value_set_string (value, g_network_service_get_protocol (srv));
      break;

    case PROP_DOMAIN:
      g_value_set_string (value, g_network_service_get_domain (srv));
      break;

    case PROP_TARGETS:
      g_value_set_pointer (value, srv->priv->targets);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }

}

/**
 * g_network_service_new:
 * @service: the service type to look up (eg, "ldap")
 * @protocol: the networking protocol to use for @service (eg, "tcp")
 * @domain: the DNS domain to look up the service in
 *
 * Creates a new #GNetworkService representing the given @service,
 * @protocol, and @domain. This will initially be unresolved; use the
 * #GSocketConnectable interface to resolve it.
 *
 * Return value: a new #GNetworkService
 **/
GNetworkService *
g_network_service_new (const gchar *service,
                       const gchar *protocol,
                       const gchar *domain)
{
  return g_object_new (G_TYPE_NETWORK_SERVICE,
                       "service", service,
                       "protocol", protocol,
                       "domain", domain,
                       NULL);
}

/**
 * g_network_service_get_service:
 * @srv: a #GNetworkService
 *
 * Gets @srv's service name (eg, "ldap").
 *
 * Return value: @srv's service name
 *
 * Since: 2.20
 **/
const gchar *
g_network_service_get_service (GNetworkService *srv)
{
  g_return_val_if_fail (G_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->service;
}

/**
 * g_network_service_get_protocol:
 * @srv: a #GNetworkService
 *
 * Gets @srv's protocol name (eg, "tcp").
 *
 * Return value: @srv's protocol name
 *
 * Since: 2.20
 **/
const gchar *
g_network_service_get_protocol (GNetworkService *srv)
{
  g_return_val_if_fail (G_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->protocol;
}

/**
 * g_network_service_get_domain:
 * @srv: a #GNetworkService
 *
 * Gets the domain that @srv serves.
 *
 * Return value: @srv's domain name
 *
 * Since: 2.20
 **/
const gchar *
g_network_service_get_domain (GNetworkService *srv)
{
  g_return_val_if_fail (G_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->domain;
}

/**
 * g_network_service_get_targets:
 * @srv: a #GNetworkService
 *
 * Gets @srv's array of #GSrvTarget. These are sorted into the correct
 * order by weight and priority, so you should attempt to connect to
 * the first one first, then the second one if the first one fails,
 * etc, until one succeeds.
 *
 * Note that it is possible for a SRV lookup to succeed but for it to
 * contain no targets (indicating that the domain is aware of the
 * existence of that service, and wishes to explicitly indicate that
 * it doesn't provide it). In this case,
 * g_network_service_get_targets() will return an array of 0 elements
 * (ie, an array whose first element is %NULL).
 *
 * Return value: a %NULL-terminated array of #GSrvTarget (or %NULL if
 * @srv has not been successfully resolved)
 *
 * Since: 2.20
 **/
GSrvTarget **
g_network_service_get_targets (GNetworkService *srv)
{
  g_return_val_if_fail (G_IS_NETWORK_SERVICE (srv), NULL);

  return srv->priv->targets;
}

/**
 * g_network_service_get_expires:
 * @srv: a #GNetworkService
 *
 * Gets the expiration time of the target in @srv with the shortest
 * time-to-live.
 *
 * Return value: @srv's expiration time, or %0 if @srv has no targets.
 *
 * Since: 2.20
 **/
time_t
g_network_service_get_expires (GNetworkService *srv)
{
  time_t expires;
  gint i;

  g_return_val_if_fail (G_IS_NETWORK_SERVICE (srv), 0);

  if (!srv->priv->targets)
    return 0;
  expires = srv->priv->targets[0]->expires;

  for (i = 1; srv->priv->targets[i]; i++)
    {
      if (srv->priv->targets[i]->expires < expires)
        expires = srv->priv->targets[i]->expires;
    }
  return expires;
}


/**
 * GSrvTarget:
 *
 * A single target host/port that a network service is running on.
 */

GType
g_srv_target_get_type (void)
{
  static volatile gsize type_volatile = 0;

  if (g_once_init_enter (&type_volatile))
    {
      GType type = g_boxed_type_register_static (
                        g_intern_static_string ("GSrvTarget"),
			(GBoxedCopyFunc) g_srv_target_copy,
			(GBoxedFreeFunc) g_srv_target_free);
      g_once_init_leave (&type_volatile, type);
    }
  return type_volatile;
}

/**
 * g_srv_target_new:
 * @hostname: the host that the service is running on
 * @port: the port that the service is running on
 * @priority: the target's priority
 * @weight: the target's weight
 * @expires: the target's expiration time
 *
 * Creates a new #GSrvTarget with the given parameters.
 *
 * Return value: a new #GSrvTarget.
 *
 * Since: 2.20
 **/
GSrvTarget *
g_srv_target_new (const gchar *hostname,
                  guint16      port,
                  guint16      priority,
                  guint16      weight,
                  time_t       expires)
{
  GSrvTarget *target = g_slice_new0 (GSrvTarget);

  target->hostname = g_strdup (hostname);
  target->port = port;
  target->priority = priority;
  target->weight = weight;
  target->expires = expires;

  return target;
}

/**
 * g_srv_target_copy:
 * @target: a #GSrvTarget
 *
 * Copies @target
 *
 * Return value: a copy of @target
 *
 * Since: 2.20
 **/
GSrvTarget *
g_srv_target_copy (GSrvTarget *target)
{
  return g_srv_target_new (target->hostname, target->port,
                           target->priority, target->weight,
                           target->expires);
}

/**
 * g_srv_target_free:
 * @target: a #GSrvTarget
 *
 * Frees @target
 *
 * Since: 2.20
 **/
void
g_srv_target_free (GSrvTarget *target)
{
  g_free (target->hostname);
  g_slice_free (GSrvTarget, target);
}

/**
 * g_srv_target_get_hostname:
 * @target: a #GSrvTarget
 *
 * Gets @target's hostname (in ASCII form; if you are going to present
 * this to the user, you should use g_hostname_is_ascii_encoded() to
 * check if it contains encoded Unicode segments, and use
 * g_hostname_to_unicode() to convert it if it does.)
 *
 * Return value: @target's hostname
 *
 * Since: 2.20
 **/
const gchar *
g_srv_target_get_hostname (GSrvTarget *target)
{
  return target->hostname;
}

/**
 * g_srv_target_get_port:
 * @target: a #GSrvTarget
 *
 * Gets @target's port
 *
 * Return value: @target's port
 *
 * Since: 2.20
 **/
guint16
g_srv_target_get_port (GSrvTarget *target)
{
  return target->port;
}

/**
 * g_srv_target_get_priority:
 * @target: a #GSrvTarget
 *
 * Gets @target's priority. You should not need to look at this;
 * #GNetworkService already sorts the targets according to the
 * algorithm in RFC 2782.
 *
 * Return value: @target's priority
 *
 * Since: 2.20
 **/
guint16
g_srv_target_get_priority (GSrvTarget *target)
{
  return target->priority;
}

/**
 * g_srv_target_get_weight:
 * @target: a #GSrvTarget
 *
 * Gets @target's weight. You should not need to look at this;
 * #GNetworkService already sorts the targets according to the
 * algorithm in RFC 2782.
 *
 * Return value: @target's weight
 *
 * Since: 2.20
 **/
guint16
g_srv_target_get_weight (GSrvTarget *target)
{
  return target->weight;
}

gint
compare_target (const void *a, const void *b)
{
  GSrvTarget *ta = *(GSrvTarget **)a;
  GSrvTarget *tb = *(GSrvTarget **)b;

  if (ta->priority == tb->priority)
    {
      /* Arrange targets of the same priority "in any order, except
       * that all those with weight 0 are placed at the beginning of
       * the list"
       */
      if (ta->weight == 0)
        return -1;
      else if (tb->weight == 0)
        return 1;
      else
        return g_random_int_range (-1, 1);
    }
  else
    return ta->priority - tb->priority;
}

/**
 * g_srv_target_array_sort:
 * @targets: a %NULL-terminated array of #GSrvTarget
 *
 * Sorts @targets in place according to the algorithm in RFC 2782.
 **/ 
void
g_srv_target_array_sort (GSrvTarget **targets)
{
  gint num_targets, first, last, i, n, sum;
  GSrvTarget *tmp;

  for (num_targets = 0; targets[num_targets]; num_targets++)
    ;

  if (num_targets == 1 && !strcmp (targets[0]->hostname, "."))
    {
      /* 'A Target of "." means that the service is decidedly not
       * available at this domain.'
       */
      g_srv_target_free (targets[0]);
      targets[0] = NULL;
      return;
    }

  /* Sort by priority, and partly by weight */
  qsort (targets, num_targets, sizeof (GSrvTarget *), compare_target);

  /* For each group of targets with the same priority, rebalance them
   * according to weight.
   */
  for (first = last = 0; first < num_targets; first = last + 1)
    {
      /* Skip @first to a non-0-weight target. */
      while (first < num_targets && targets[first]->weight == 0)
        first++;
      if (first == num_targets)
        break;

      /* Skip @last to the last target of the same priority. */
      last = first;
      while (last < num_targets - 1 &&
             targets[last + 1]->priority == targets[last]->priority)
        last++;

      /* If there's only one non-0 weight target at this priority,
       * we can move on to the next priority level.
       */
      if (last == first)
        continue;

      /* Randomly reorder the non-0 weight targets, giving precedence
       * to the ones with higher weight. RFC 2782 describes this in
       * terms of assigning a running sum to each target and building
       * a new list. We do things slightly differently, but should get
       * the same result.
       */
      for (i = first, sum = 0; i <= last; i++)
        sum += targets[i]->weight;
      while (first < last)
        {
          n = g_random_int_range (0, sum);
          for (i = first; i < last; i++)
            {
              if (n < targets[i]->weight)
                break;
              n -= targets[i]->weight;
            }

          tmp = targets[first];
          targets[first] = targets[i];
          targets[i] = tmp;

          sum -= tmp->weight;
          first++;
        }
    }
}

#define __G_NETWORK_SERVICE_C__
#include "gioaliasdef.c"
