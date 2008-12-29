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
#include <sys/un.h>

#endif

G_BEGIN_DECLS

extern struct addrinfo _g_resolver_addrinfo_hints;

void           _g_resolver_os_init                 (void);

GInetAddress **_g_resolver_addresses_from_addrinfo (const char       *hostname,
						    struct addrinfo  *res,
						    gint              gai_retval,
						    GError          **error);

void           _g_resolver_address_to_sockaddr     (GInetAddress            *address,
						    struct sockaddr_storage *sa,
						    gsize                   *sa_len);
char          *_g_resolver_name_from_nameinfo      (GInetAddress     *address,
						    const gchar      *name,
						    gint              gni_retval,
						    GError          **error);

#if defined(G_OS_UNIX)
GSrvTarget   **_g_resolver_targets_from_res_query  (const gchar      *rrname,
						    guchar           *answer,
						    gint              len,
						    gint              herr,
						    GError          **error);
#elif defined(G_OS_WIN32)
GSrvTarget   **_g_resolver_targets_from_DnsQuery   (const gchar      *rrname,
						    DNS_STATUS        status,
						    DNS_RECORD       *results,
						    GError          **error);
#endif

G_END_DECLS

#endif /* __G_RESOLVERPRIVATE_H__ */
