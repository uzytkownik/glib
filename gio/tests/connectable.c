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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gio/gio.h>

static GCancellable *cancellable;
static GMainLoop *loop;

static void
usage (void)
{
	fprintf (stderr, "Usage: connectable [-t] [-s] [hostname | IP | service/protocol/domain ] ...\n");
	fprintf (stderr, "       Use -t to enable threads, -s to do synchronous lookups.\n");
	exit (1);
}

static void
print_sockaddr (GSocketAddress *sockaddr,
		GError         *error)
{
  char *phys;

  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else if (!G_IS_INET_SOCKET_ADDRESS (sockaddr))
    {
      printf ("Error:   Unexpected sockaddr type '%s'\n", g_type_name_from_instance ((GTypeInstance *)sockaddr));
      g_object_unref (sockaddr);
    }
  else
    {
      GInetSocketAddress *isa = G_INET_SOCKET_ADDRESS (sockaddr);
      phys = g_inet_address_to_string (g_inet_socket_address_get_address (isa));
      printf ("Address: %s:%d\n", phys, g_inet_socket_address_get_port (isa));
      g_free (phys);
      g_object_unref (sockaddr);
    }
}

static void
do_sync_lookup (GSocketAddressEnumerator *enumerator)
{
  GSocketAddress *sockaddr;
  GError *error = NULL;

  while ((sockaddr = g_socket_address_enumerator_get_next (enumerator, cancellable, &error)))
    print_sockaddr (sockaddr, error);
}

static void do_async_lookup (GSocketAddressEnumerator *enumerator);

static void
got_next_async (GObject *source, GAsyncResult *result, gpointer user_data)
{
  GSocketAddressEnumerator *enumerator = G_SOCKET_ADDRESS_ENUMERATOR (source);
  GSocketAddress *sockaddr;
  GError *error = NULL;

  sockaddr = g_socket_address_enumerator_get_next_finish (enumerator, result, &error);
  if (sockaddr || error)
    print_sockaddr (sockaddr, error);
  if (sockaddr)
    do_async_lookup (enumerator);
  else
    g_main_loop_quit (loop);
}

static void
do_async_lookup (GSocketAddressEnumerator *enumerator)
{
  g_socket_address_enumerator_get_next_async (enumerator, cancellable,
					      got_next_async, NULL);
}

#ifdef G_OS_UNIX
static int cancel_fds[2];

static void
interrupted (int sig)
{
  signal (SIGINT, SIG_DFL);
  write (cancel_fds[1], "x", 1);
}

static gboolean
async_cancel (GIOChannel *source, GIOCondition cond, gpointer cancellable)
{
  g_cancellable_cancel (cancellable);
  return FALSE;
}
#endif

int
main (int argc, char **argv)
{
  gboolean synchronous = FALSE;
#ifdef G_OS_UNIX
  GIOChannel *chan;
  guint watch;
#endif
  GSocketConnectable *connectable;
  GSocketAddressEnumerator *enumerator;
  char *arg, **parts;

  /* Can't use GOptionContext since we use the options to decide
   * whether or not to call g_thread_init().
   */
  while (argc >=2 && *argv[1] == '-')
    {
      if (!strcmp (argv[1], "-t"))
	g_thread_init (NULL);
      else if (!strcmp (argv[1], "-s"))
	synchronous = TRUE;
      else
	usage ();
      argv++;
      argc--;
    }
  g_type_init ();

  if (argc != 2)
    usage ();
  arg = argv[1];

  cancellable = g_cancellable_new ();

#ifdef G_OS_UNIX
  /* Set up cancellation; we want to cancel if the user ^C's the
   * program, but we can't cancel directly from an interrupt.
   */
  signal (SIGINT, interrupted);

  if (pipe (cancel_fds) == -1)
    {
      perror ("pipe");
      exit (1);
    }
  chan = g_io_channel_unix_new (cancel_fds[0]);
  watch = g_io_add_watch (chan, G_IO_IN, async_cancel, cancellable);
  g_io_channel_unref (chan);
#endif

  if (strchr (arg, '/'))
    {
      /* service/protocol/domain */
      GNetworkService *srv;

      parts = g_strsplit (arg, "/", 3);
      if (!parts || !parts[2])
	usage ();

      srv = g_network_service_new (parts[0], parts[1], parts[2]);
      connectable = G_SOCKET_CONNECTABLE (srv);
    }
  else
    {
      guint16 port;

      parts = g_strsplit (arg, ":", 2);
      if (parts && parts[1])
	{
	  arg = parts[0];
	  port = strtoul (parts[1], NULL, 10);
	}
      else
	port = 0;

      if (g_hostname_is_ip_address (arg))
	{
	  GInetAddress *addr = g_inet_address_from_string (arg);
	  GInetSocketAddress *sockaddr = g_inet_socket_address_new (addr, port);

	  g_object_unref (addr);
	  connectable = G_SOCKET_CONNECTABLE (sockaddr);
	}
      else
	{
	  GNetworkAddress *addr = g_network_address_new (arg, port);
	  connectable = G_SOCKET_CONNECTABLE (addr);
	}
    }

  enumerator = g_socket_connectable_get_enumerator (connectable);
  if (synchronous)
    do_sync_lookup (enumerator);
  else
    {
      do_async_lookup (enumerator);
      loop = g_main_loop_new (NULL, TRUE);
      g_main_run (loop);
      g_main_loop_unref (loop);
    }

#ifdef G_OS_UNIX
  g_source_remove (watch);
#endif
  g_object_unref (cancellable);

  return 0;
}
