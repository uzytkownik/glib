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
#include "gsocketconnectable.h"
#include "glibintl.h"

#include "gioalias.h"

GType
g_socket_connectable_get_type (void)
{
  static volatile gsize g_define_type_id__volatile = 0;

  if (g_once_init_enter (&g_define_type_id__volatile))
    {
      const GTypeInfo connectable_info =
      {
        sizeof (GSocketConnectableIface), /* class_size */
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	NULL,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	0,
	0,              /* n_preallocs */
	NULL
      };
      GType g_define_type_id =
	g_type_register_static (G_TYPE_INTERFACE, I_("GSocketConnectable"),
				&connectable_info, 0);

      g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_OBJECT);

      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
    }

  return g_define_type_id__volatile;
}

GSocketAddressEnumerator *
g_socket_connectable_get_enumerator (GSocketConnectable *connectable)
{
  GSocketConnectableIface *iface;

  g_return_val_if_fail (G_IS_SOCKET_CONNECTABLE (connectable), NULL);

  iface = G_SOCKET_CONNECTABLE_GET_IFACE (connectable);

  return (* iface->get_enumerator) (connectable);
}

#define __G_SOCKET_CONNECTABLE_C__
#include "gioaliasdef.c"
