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

#ifndef __G_RESOLVERPRIVATE_H__
#define __G_RESOLVERPRIVATE_H__

#include "gresolver.h"

#ifdef G_OS_WIN32

#define WINVER 0x0501 // FIXME?
#include <winsock2.h>
#undef interface
#include <ws2tcpip.h>
#include <windns.h>

#else /* !G_OS_WIN32 */

#define BIND_4_COMPAT

#include <arpa/inet.h>
#include <arpa/nameser.h>
/* We're supposed to define _GNU_SOURCE to get EAI_NODATA, but that
 * won't actually work since <features.h> has already been included at
 * this point. So we define __USE_GNU instead.
 */
#define __USE_GNU
#include <netdb.h>
#undef __USE_GNU
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <resolv.h>
#include <sys/socket.h>

#endif

G_BEGIN_DECLS

#ifdef G_OS_WIN32
void      g_resolver_os_init                   (void);
#else
#define   g_resolver_os_init()
#endif

void      g_network_address_get_addrinfo_hints (GNetworkAddress  *addr,
						gchar             service[8],
						struct addrinfo  *hints);
gboolean  g_network_address_set_from_addrinfo  (GNetworkAddress  *addr,
						struct addrinfo  *res,
						gint              gai_retval,
						GError          **error);

gboolean  g_network_address_set_from_nameinfo  (GNetworkAddress  *addr,
						const gchar      *name,
						gint              gni_retval,
						GError          **error);

gchar    *g_network_service_get_rrname         (GNetworkService  *srv);
#if defined(G_OS_UNIX)
gboolean  g_network_service_set_from_res_query (GNetworkService  *srv,
						guchar           *answer,
						gint              len,
						gint              herr,
						GError          **error);
#elif defined(G_OS_WIN32)
gboolean  g_network_service_set_from_DnsQuery  (GNetworkService  *srv,
						DNS_STATUS        status,
						DNS_RECORD       *results,
						GError          **error);
#endif

G_END_DECLS

#endif /* __G_RESOLVERPRIVATE_H__ */
