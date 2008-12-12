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

#ifndef G_OS_WIN32
# include <sys/socket.h>
# include <sys/un.h>
# include <netinet/in.h>
#else
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#include "gsocketaddress.h"
#include "ginetsocketaddress.h"
#include "ginet4address.h"
#include "ginet6address.h"
#include "glocalsocketaddress.h"

/**
 * SECTION:gsocketaddress
 * @short_description: Abstract base class representing endpoints for socket communication
 * @see_also: #GInetSocketAddress
 *
 * #GSocketAddress is the equivalent of %struct %sockaddr in the BSD sockets API.
 **/

G_DEFINE_ABSTRACT_TYPE (GSocketAddress, g_socket_address, G_TYPE_INITIALLY_UNOWNED);

static void
g_socket_address_class_init (GSocketAddressClass *klass)
{

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
 * @dest: a pointer to a memory location that will contain the native %struct %sockaddr.
 *
 * Converts a #GSocketAddress to a native %struct %sockaddr. @dest must be able to hold at least
 * as many bytes as g_socket_address_native_size() returns for @address.
 *
 * Returns: the size of the native %struct %sockaddr that @address represents
 */
gboolean
g_socket_address_to_native (GSocketAddress *address, gpointer dest)
{
  g_return_val_if_fail (G_IS_SOCKET_ADDRESS (address), FALSE);

  return G_SOCKET_ADDRESS_GET_CLASS (address)->to_native (address, dest);
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
g_socket_address_from_native (gpointer native, gsize len)
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

      return G_SOCKET_ADDRESS (g_inet_socket_address_new (G_INET_ADDRESS (g_inet4_address_from_bytes ((guint8 *) &(addr->sin_addr))), g_ntohs (addr->sin_port)));
    }

  if (family == AF_INET6)
    {
      struct sockaddr_in6 *addr = (struct sockaddr_in6 *) native;

      return G_SOCKET_ADDRESS (g_inet_socket_address_new (G_INET_ADDRESS (g_inet6_address_from_bytes ((guint8 *) &(addr->sin6_addr))), g_ntohs (addr->sin6_port)));
    }

#ifndef G_OS_WIN32
  if (family == AF_LOCAL)
    {

      struct sockaddr_un *addr = (struct sockaddr_un *) native;

      return G_SOCKET_ADDRESS (g_local_socket_address_new (addr->sun_path));
    }
#else
  g_error ("local sockets not supported on Windows");
#endif

  return NULL;
}
