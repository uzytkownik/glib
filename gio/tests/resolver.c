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

static GResolver *resolver;
static GCancellable *cancellable;
static GMainLoop *loop;
static int nlookups = 0;

static void
usage (void)
{
	fprintf (stderr, "Usage: resolver [-t | -T] [hostname | IP | service/protocol/domain ] ...\n");
	fprintf (stderr, "       Use -t for asynchronous lookups in a thread-enabled program.\n");
	fprintf (stderr, "       Use -T for synchronous lookups in separate threads.\n");
	fprintf (stderr, "       (Use neither for asynchronous lookups in a non-threaded program.)\n");
	exit (1);
}

G_LOCK_DEFINE_STATIC (response);

static void
print_resolved_name (const char *phys, GNetworkAddress *addr,
		     GError *error)
{
  G_LOCK (response);
  printf ("Address: %s\n", phys);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      printf ("Name:    %s\n", g_network_address_get_hostname (addr));
      g_object_unref (addr);
    }
  printf ("\n");

  nlookups--;
  if (nlookups == 0)
    g_main_loop_quit (loop);
  G_UNLOCK (response);
}

static void
print_resolved_addresses (const char *name, GNetworkAddress *addr,
			  GError *error)
{
  GSockaddr **addrs;
  char *phys;
  int i;

  G_LOCK (response);
  printf ("Name:    %s\n", name);
  if (error)
    {
      printf ("Error:   %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      addrs = g_network_address_get_sockaddrs (addr);
      for (i = 0; addrs[i]; i++)
	{
	  phys = g_sockaddr_to_string (addrs[i]);
	  printf ("Address: %s\n", phys);
	  g_free (phys);
	}
      g_object_unref (addr);
    }
  printf ("\n");

  nlookups--;
  if (nlookups == 0)
    g_main_loop_quit (loop);
  G_UNLOCK (response);
}

static void
print_resolved_service (const char *service, GNetworkService *srv,
			GError *error)
{
  GSrvTarget **targets;
  int i;

  G_LOCK (response);
  printf ("Service: %s\n", service);
  if (error)
    {
      printf ("Error: %s\n", error->message);
      g_error_free (error);
    }
  else
    {
      targets = g_network_service_get_targets (srv);
      for (i = 0; targets[i]; i++)
	{
	  printf ("%s:%u (pri %u, weight %u)\n",
		  g_srv_target_get_hostname (targets[i]),
		  g_srv_target_get_port (targets[i]),
		  g_srv_target_get_priority (targets[i]),
		  g_srv_target_get_weight (targets[i]));
	}
      g_object_unref (srv);
    }
  printf ("\n");

  nlookups--;
  if (nlookups == 0)
    g_main_loop_quit (loop);
  G_UNLOCK (response);
}

static gpointer
lookup_thread (gpointer data)
{
  const char *arg = data;
  GError *error = NULL;
  GNetworkAddress *addr;
  GNetworkService *srv;

  if (strchr (arg, '/'))
    {
      /* service/protocol/domain */
      char **parts = g_strsplit (arg, "/", 3);

      if (!parts || !parts[2])
	usage ();

      srv = g_resolver_lookup_service (resolver, parts[0], parts[1], parts[2],
				       cancellable, &error);
      print_resolved_service (arg, srv, error);
    }
  else if (g_hostname_is_ip_address (arg))
    {
      GSockaddr *sockaddr = g_sockaddr_new_from_string (arg, 0);
      addr = g_resolver_lookup_address (resolver, sockaddr, cancellable, &error);
      print_resolved_name (arg, addr, error);
      g_sockaddr_free (sockaddr);
    }
  else
    {
      addr = g_resolver_lookup_name (resolver, arg, 0, cancellable, &error);
      print_resolved_addresses (arg, addr, error);
    }

  return NULL;
}

static void
start_threaded_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    g_thread_create (lookup_thread, argv[i], FALSE, NULL);
}

static void
lookup_addr_callback (GObject *source, GAsyncResult *result,
		      gpointer user_data)
{
  const char *phys = user_data;
  GError *error = NULL;
  GNetworkAddress *addr;

  addr = g_resolver_lookup_address_finish (resolver, result, &error);
  print_resolved_name (phys, addr, error);
}

static void
lookup_name_callback (GObject *source, GAsyncResult *result,
		      gpointer user_data)
{
  const char *name = user_data;
  GError *error = NULL;
  GNetworkAddress *addr;

  addr = g_resolver_lookup_name_finish (resolver, result, &error);
  print_resolved_addresses (name, addr, error);
}

static void
lookup_service_callback (GObject *source, GAsyncResult *result,
			 gpointer user_data)
{
  const char *service = user_data;
  GError *error = NULL;
  GNetworkService *srv;

  srv = g_resolver_lookup_service_finish (resolver, result, &error);
  print_resolved_service (service, srv, error);
}

static void
start_async_lookups (char **argv, int argc)
{
  int i;

  for (i = 0; i < argc; i++)
    {
      if (strchr (argv[i], '/'))
	{
	  /* service/protocol/domain */
	  char **parts = g_strsplit (argv[i], "/", 3);

	  if (!parts || !parts[2])
	    usage ();

	  g_resolver_lookup_service_async (resolver,
					   parts[0], parts[1], parts[2],
					   cancellable,
					   lookup_service_callback, argv[i]);
	}
      else if (g_hostname_is_ip_address (argv[i]))
	{
	  GSockaddr *addr = g_sockaddr_new_from_string (argv[i], 0);
	  g_resolver_lookup_address_async (resolver, addr, cancellable,
					   lookup_addr_callback, argv[i]);
	  g_sockaddr_free (addr);
	}
      else
	{
	  g_resolver_lookup_name_async (resolver, argv[i], 0, cancellable,
					lookup_name_callback,
					argv[i]);
	}
    }
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
  gboolean threaded = FALSE;
#ifdef G_OS_UNIX
  GIOChannel *chan;
  guint watch;
#endif

  if (argc >= 2 && (!strcmp (argv[1], "-t") || !strcmp (argv[1], "-T")))
    {
      g_thread_init (NULL);
      threaded = argv[1][1] == 'T';
      argv++;
      argc--;
    }
  g_type_init ();

  if (argc < 2)
    usage ();

  resolver = g_resolver_get_default ();

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

  nlookups = argc - 1;
  if (threaded)
    start_threaded_lookups (argv + 1, argc - 1);
  else
    start_async_lookups (argv + 1, argc - 1);

  loop = g_main_loop_new (NULL, TRUE);
  g_main_run (loop);
  g_main_loop_unref (loop);

#ifdef G_OS_UNIX
  g_source_remove (watch);
#endif
  g_object_unref (cancellable);

  return 0;
}
