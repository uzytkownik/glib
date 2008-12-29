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

#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gsocket.h"
#include "gioerror.h"

enum
  {
    PROP_0,
    PROP_LOCAL
  };

struct _GSocketPrivate
{
  volatile int pending;
};

static void g_socket_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec);

G_DEFINE_ABSTRACT_TYPE (GSocket, g_socket, G_TYPE_OBJECT);

static void
g_socket_class_init (GSocketClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (gobject_class, sizeof (GSocketPrivate));
  
  gobject_class->get_property = g_socket_get_property;
  
  g_object_class_install_property (gobject_class, PROP_LOCAL,
				   g_param_spec_object ("local-address",
							"local address",
							"binded address",
							G_TYPE_SOCKET_ADDRESS,
							G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
g_socket_init (GSocket *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
					    G_TYPE_SOCKET,
					    GSocketPrivate);
}

static void
g_socket_get_property (GObject    *object,
		       guint       prop_id,
		       GValue     *value,
		       GParamSpec *pspec)
{
  GSocket      *self;
  GSocketClass *klass;
  
  self = G_SOCKET (object);
  klass = G_SOCKET_GET_CLASS (self);

  switch (prop_id)
    {
    case PROP_LOCAL:
      g_value_take_object (value, klass->get_local_address (self));
      break;
       break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

GSocketAddress *
g_socket_get_local_address (GSocket *self)
{
  GSocketClass *klass;

  g_return_val_if_fail ((klass = G_SOCKET_GET_CLASS (self)), NULL);
  g_return_val_if_fail (klass->get_local_address, NULL);

  return klass->get_local_address (self);
}

gboolean
g_socket_has_pending (GSocket *socket)
{
  return g_atomic_int_get (&socket->priv->pending);
}

gboolean
g_socket_set_pending (GSocket  *socket,
		      GError  **error)
{
  if (g_atomic_int_compare_and_exchange (&socket->priv->pending, 0, 1))
    {
      return TRUE;
    }
  else
    {
      g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PENDING,
			   _("Socket has outstanding operation"));
      return FALSE;
    }
}

void
g_socket_clear_pending (GSocket *socket)
{
  g_atomic_int_set (&socket->priv->pending, 0);
}

