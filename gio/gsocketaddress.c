/* GNIO - GLib Network Layer of GIO
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

#include <config.h>
#include <glib.h>

#include "gsocketaddress.h"
#include "ginetaddress.h"
#include "ginetsocketaddress.h"
#include "gresolverprivate.h"
#include "gsocketaddressenumerator.h"
#include "gsocketconnectable.h"

#ifdef G_OS_UNIX
#include "gunixsocketaddress.h"
#endif

#include "gioalias.h"

/**
 * SECTION:gsocketaddress
 * @short_description: Abstract base class representing endpoints for socket communication
 * @see_also: #GInetSocketAddress
 *
 * #GSocketAddress is the equivalent of %struct %sockaddr in the BSD sockets API.
 **/

static void                      g_socket_address_connectable_iface_init     (GSocketConnectableIface *iface);
static GSocketAddressEnumerator *g_socket_address_connectable_get_enumerator (GSocketConnectable      *connectable);

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (GSocketAddress, g_socket_address, G_TYPE_OBJECT,
				  G_IMPLEMENT_INTERFACE (G_TYPE_SOCKET_CONNECTABLE,
							 g_socket_address_connectable_iface_init))

static void
g_socket_address_class_init (GSocketAddressClass *klass)
{

}

static void
g_socket_address_connectable_iface_init (GSocketConnectableIface *connectable_iface)
{
  connectable_iface->get_enumerator  = g_socket_address_connectable_get_enumerator;
}

static void
g_socket_address_init (GSocketAddress *address)
{

}

/**
 * g_socket_address_native_size:
 * @address: a #GSocketAddress
 *
 * Returns: the size of the native %struct %sockaddr that @address represents
 */
gssize
g_socket_address_native_size (GSocketAddress *address)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), -1);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->native_size (address);
}

/**
 * g_socket_address_to_native:
 * @address: a #GSocketAddress
 * @dest: a pointer to a memory location that will contain the native
 * %struct %sockaddr.
 * @destlen: the size of @dest. Must be at least as large as
 * g_socket_address_native_size().
 *
 * Converts a #GSocketAddress to a native %struct %sockaddr.
 *
 * Returns: %TRUE if @dest was filled in, %FALSE if @address is invalid
 * or @destlen is too small.
 */
gboolean
g_socket_address_to_native (GSocketAddress *address,
			    gpointer        dest,
			    gsize           destlen)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), FALSE);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->to_native (address, dest, destlen);
}

/**
 * g_socket_address_from_native:
 * @native: a pointer to a %struct %sockaddr
 * @len: the size of the memory location pointed to by @native
 *
 * Returns: a new #GSocketAddress if @native could successfully be converted,
 * otherwise %NULL.
 */
GSocketAddress *
g_socket_address_from_native (gpointer native,
			      gsize    len)
{
  gshort family;

  if (len < sizeof (gshort))
    return NULL;

  family = ((struct sockaddr *) native)->sa_family;

  if (family == AF_UNSPEC)
    return NULL;

  if (family == AF_INET)
    {
      struct sockaddr_in *addr = (struct sockaddr_in *) native;
      GInetAddress *iaddr = g_inet_address_from_bytes ((guint8 *) &(addr->sin_addr), AF_INET);
      GInetSocketAddress *isa;

      isa = g_inet_socket_address_new (iaddr, g_ntohs (addr->sin_port));
      g_object_unref (iaddr);
      return G_SOCKET_ADDRESS (isa);
    }

  if (family == AF_INET6)
    {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *) native;
      GInetAddress *iaddr = g_inet_address_from_bytes ((guint8 *) &(addr->sin6_addr), AF_INET6);
      GInetSocketAddress *isa;

      isa = g_inet_socket_address_new (iaddr, g_ntohs (addr->sin6_port));
      g_object_unref (iaddr);
      return G_SOCKET_ADDRESS (isa);
    }

#ifdef G_OS_UNIX
  if (family == AF_UNIX)
    {
      struct sockaddr_un *addr = (struct sockaddr_un *) native;

      return G_SOCKET_ADDRESS (g_unix_socket_address_new (addr->sun_path));
    }
#endif

  return NULL;
}


#define G_TYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR (_g_socket_address_address_enumerator_get_type ())
#define G_SOCKET_ADDRESS_ADDRESS_ENUMERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR, GSocketAddressAddressEnumerator))

typedef struct {
  GSocketAddressEnumerator parent_instance;

  GSocketAddress *sockaddr;
} GSocketAddressAddressEnumerator;

typedef struct {
  GSocketAddressEnumeratorClass parent_class;

} GSocketAddressAddressEnumeratorClass;

G_DEFINE_TYPE (GSocketAddressAddressEnumerator, _g_socket_address_address_enumerator, G_TYPE_SOCKET_ADDRESS_ENUMERATOR)

static void
g_socket_address_address_enumerator_finalize (GObject *object)
{
  GSocketAddressAddressEnumerator *sockaddr_enum =
    G_SOCKET_ADDRESS_ADDRESS_ENUMERATOR (object);

  if (sockaddr_enum->sockaddr)
    g_object_unref (sockaddr_enum->sockaddr);

  G_OBJECT_CLASS (_g_socket_address_address_enumerator_parent_class)->finalize (object);
}

static GSocketAddress *
g_socket_address_address_enumerator_get_next (GSocketAddressEnumerator  *enumerator,
					      GCancellable              *cancellable,
					      GError                   **error)
{
  GSocketAddressAddressEnumerator *sockaddr_enum =
    G_SOCKET_ADDRESS_ADDRESS_ENUMERATOR (enumerator);

  if (sockaddr_enum->sockaddr)
    {
      GSocketAddress *ret = sockaddr_enum->sockaddr;

      sockaddr_enum->sockaddr = NULL;
      return ret;
    }
  else
    return NULL;
}

static void
_g_socket_address_address_enumerator_init (GSocketAddressAddressEnumerator *enumerator)
{
}

static void
_g_socket_address_address_enumerator_class_init (GSocketAddressAddressEnumeratorClass *sockaddrenum_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (sockaddrenum_class);
  GSocketAddressEnumeratorClass *enumerator_class =
    G_SOCKET_ADDRESS_ENUMERATOR_CLASS (sockaddrenum_class);

  enumerator_class->get_next = g_socket_address_address_enumerator_get_next;
  object_class->finalize = g_socket_address_address_enumerator_finalize;
}

static GSocketAddressEnumerator *
g_socket_address_connectable_get_enumerator (GSocketConnectable *connectable)
{
  GSocketAddressAddressEnumerator *sockaddr_enum;

  sockaddr_enum = g_object_new (G_TYPE_SOCKET_ADDRESS_ADDRESS_ENUMERATOR, NULL);
  sockaddr_enum->sockaddr = g_object_ref (connectable);

  return (GSocketAddressEnumerator *)sockaddr_enum;
}

#define __G_SOCKET_ADDRESS_C__
#include "gioaliasdef.c"
