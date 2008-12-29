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
print_resolved_name (const char *phys,
                     char       *name,
                     GError     *error)
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
      printf ("Name:    %s\n", name);
      g_free (name);
    }
  printf ("\n");

  nlookups--;
  if (nlookups == 0)
    g_main_loop_quit (loop);
  G_UNLOCK (response);
}

static void
print_resolved_addresses (const char    *name,
                          GInetAddress **addresses,
			  GError        *error)
{
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
      for (i = 0; addresses[i]; i++)
	{
	  phys = g_inet_address_to_string (addresses[i]);
	  printf ("Address: %s\n", phys);
	  g_free (phys);
          g_object_unref (addresses[i]);
	}
      g_free (addresses);
    }
  printf ("\n");

  nlookups--;
  if (nlookups == 0)
    g_main_loop_quit (loop);
  G_UNLOCK (response);
}

static void
print_resolved_service (const char  *service,
                        GSrvTarget **targets,
			GError      *error)
{
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
      for (i = 0; targets[i]; i++)
	{
	  printf ("%s:%u (pri %u, weight %u)\n",
		  g_srv_target_get_hostname (targets[i]),
		  g_srv_target_get_port (targets[i]),
		  g_srv_target_get_priority (targets[i]),
		  g_srv_target_get_weight (targets[i]));
          g_srv_target_free (targets[i]);
	}
      g_free (targets);
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

  if (strchr (arg, '/'))
    {
      GSrvTarget **targets;
      /* service/protocol/domain */
      char **parts = g_strsplit (arg, "/", 3);

      if (!parts || !parts[2])
	usage ();

      targets = g_resolver_lookup_service (resolver,
                                           parts[0], parts[1], parts[2],
                                           cancellable, &error);
      print_resolved_service (arg, targets, error);
    }
  else if (g_hostname_is_ip_address (arg))
    {
      GInetAddress *addr = g_inet_address_from_string (arg);
      char *name;

      name = g_resolver_lookup_by_address (resolver, addr, cancellable, &error);
      print_resolved_name (arg, name, error);
      g_object_unref (addr);
    }
  else
    {
      GInetAddress **addresses;

      addresses = g_resolver_lookup_by_name (resolver, arg, cancellable, &error);
      print_resolved_addresses (arg, addresses, error);
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
lookup_by_addr_callback (GObject *source, GAsyncResult *result,
                         gpointer user_data)
{
  const char *phys = user_data;
  GError *error = NULL;
  char *name;

  name = g_resolver_lookup_by_address_finish (resolver, result, &error);
  print_resolved_name (phys, name, error);
}

static void
lookup_by_name_callback (GObject *source, GAsyncResult *result,
                         gpointer user_data)
{
  const char *name = user_data;
  GError *error = NULL;
  GInetAddress **addresses;

  addresses = g_resolver_lookup_by_name_finish (resolver, result, &error);
  print_resolved_addresses (name, addresses, error);
}

static void
lookup_service_callback (GObject *source, GAsyncResult *result,
			 gpointer user_data)
{
  const char *service = user_data;
  GError *error = NULL;
  GSrvTarget **targets;

  targets = g_resolver_lookup_service_finish (resolver, result, &error);
  print_resolved_service (service, targets, error);
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
          GInetAddress *addr = g_inet_address_from_string (argv[i]);

	  g_resolver_lookup_by_address_async (resolver, addr, cancellable,
                                              lookup_by_addr_callback, argv[i]);
	  g_object_unref (addr);
	}
      else
	{
	  g_resolver_lookup_by_name_async (resolver, argv[i], cancellable,
                                           lookup_by_name_callback,
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
