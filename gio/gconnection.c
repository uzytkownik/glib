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
 * Author: Maciej Piechotka <uzytkownik2@gmail.com>
 */

#include "gconnection.h"

enum
  {
    PROP_0,
    PROP_LOCAL,
    PROP_PEER
  };

static void g_connection_get_property (GObject    *object,
				       guint       prop_id,
				       GValue     *value,
				       GParamSpec *pspec);

G_DEFINE_ABSTRACT_TYPE (GConnection, g_connection, G_TYPE_OBJECT);

static void
g_connection_class_init (GConnectionClass *klass)
{
  GObjectClass *gobject_class = (GObjectClass *)klass;

  gobject_class->get_property = g_connection_get_property;
  
  g_object_class_install_property (gobject_class, PROP_LOCAL,
				   g_param_spec_object ("local-address",
							"local address",
							"address of local side of connection",
							G_TYPE_SOCKET_ADDRESS,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PEER,
				   g_param_spec_object ("peer-address",
							"peer address",
							"address of remote side of connection",
							G_TYPE_SOCKET_ADDRESS,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
g_connection_init (GConnection *self)
{
}

static void
g_connection_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GConnection      *self;
  GConnectionClass *klass;
  
  self = G_CONNECTION (object);
  klass = G_CONNECTION_GET_CLASS (self);

  switch (prop_id)
    {
    case PROP_LOCAL:
      g_value_take_object (value, klass->get_local_address (self));
      break;
    case PROP_PEER:
      g_value_take_object (value, klass->get_peer_address (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GSocketAddress *
g_connection_get_local_address (GConnection *self)
{
  GConnectionClass *klass;
  
  g_return_val_if_fail (klass = G_CONNECTION_GET_CLASS (self), NULL);
  g_return_val_if_fail (klass->get_local_address, NULL);

  return klass->get_local_address (self);
}

GSocketAddress *
g_connection_get_peer_address (GConnection *self)
{
  GConnectionClass *klass;

  g_return_val_if_fail (klass = G_CONNECTION_GET_CLASS (self), NULL);
  g_return_val_if_fail (klass->get_peer_address, NULL);

  return klass->get_peer_address (self);
}
