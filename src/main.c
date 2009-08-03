/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <glib.h>

#include <dbus/dbus-protocol.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "gypsy-server.h"

#define GYPSY_NAME "org.freedesktop.Gypsy"
#define DEFAULT_PID_FILE LOCALSTATEDIR"/run/Gypsy.pid"

static GMainLoop *mainloop;
/* This is a bit ugly, but it works */
char* nmea_log = NULL;

static void
gypsy_terminate (GObject *object,
		 gpointer userdata)
{
	g_main_loop_quit (mainloop);
}

static void
write_pidfile (const char *pidfile)
{
	FILE *f;

	if ((f = fopen (pidfile, "w")) == NULL) {
		g_printerr ("Opening %s failed: %s\n", pidfile, strerror (errno));
		return;
	}
	if (fprintf (f, "%d", getpid ()) < 0) {
		g_printerr ("Writing to %s failed: %s\n", pidfile, strerror (errno));
	}

	if (fclose (f)) {
		g_printerr ("Closing %s failed: %s\n", pidfile, strerror (errno));
	}
}

static void
name_owner_changed (DBusGProxy  *proxy,
		    const char  *name,
		    const char  *prev_owner,
		    const char  *new_owner,
		    GypsyServer *server)
{
	if (strcmp (new_owner, "") == 0 && strcmp (name, prev_owner) == 0) {
		gypsy_server_remove_clients (server, prev_owner);
	}
}

int
main (int    argc,
      char **argv)
{
	GOptionContext *context;
	DBusGConnection *conn;
	DBusGProxy *proxy;
	GError *error = NULL;
	guint32 request_name_ret;
	GypsyServer *gypsy;
	gboolean become_daemon = FALSE;
	char *pidfile = NULL;
	char *user_pidfile = NULL;

	GOptionEntry entries[] = {
		{ "nmea-log", 0, 0, G_OPTION_ARG_FILENAME, &nmea_log, "Log NMEA data to FILE.[device]", "FILE" },
		{ "no-daemon", 0, 0, G_OPTION_ARG_NONE, &become_daemon, "Don't become a daemon", NULL },
		{ "pid-file", 0, 0, G_OPTION_ARG_FILENAME, &user_pidfile, "Specify the location of a PID file", "FILE" },
		{ NULL }
	};

	context = g_option_context_new ("- GPS daemon");
	g_option_context_add_main_entries (context, entries, NULL);
	
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
#if GLIB_CHECK_VERSION(2, 14, 0)
		char *help;
		help = g_option_context_get_help (context, TRUE, NULL);
		g_print (help);
		g_free (help);
#else
		g_printerr ("Cannot parse arguments: %s\n", error->message);
#endif
		g_error_free (error);
		return 1;
	}
	
	pidfile = g_strdup (user_pidfile ? user_pidfile : DEFAULT_PID_FILE);

	/* Tricky: become_daemon is FALSE by default, so unless it's TRUE
	   because of a CLI option, it'll become TRUE after this
	*/
	become_daemon = !become_daemon;
	if (become_daemon) {
		if (daemon (0, 0) < 0) {
			int saved_errno;

			saved_errno = errno;
			g_printerr ("Could not daemonize: %s [error %u]\n",
				    g_strerror (saved_errno), saved_errno);
			return 1;
		}
		write_pidfile (pidfile);
	}
	g_free (pidfile);

	g_option_context_free (context);

	umask (022);

	g_type_init ();

	mainloop = g_main_loop_new (NULL, FALSE);
	
	conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (!conn) {
		g_error ("Error getting bus: %s", error->message);
		return 1;
	}
	
	proxy = dbus_g_proxy_new_for_name (conn, 
					   DBUS_SERVICE_DBUS,
					   DBUS_PATH_DBUS,
					   DBUS_INTERFACE_DBUS);
	if (!org_freedesktop_DBus_request_name (proxy, GYPSY_NAME,
						0,  &request_name_ret,
						&error)) {
		g_error ("Error registering D-Bus service %s: %s", 
			 GYPSY_NAME, error->message);
		return 1;
	}

	/* Just quit if GPS is already running */
	if (request_name_ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return 1;
	}

	gypsy = g_object_new (GYPSY_TYPE_SERVER, NULL);
	g_signal_connect (G_OBJECT (gypsy), "terminate",
			  G_CALLBACK (gypsy_terminate), NULL);

	dbus_g_proxy_add_signal (proxy, "NameOwnerChanged",
				 G_TYPE_STRING, G_TYPE_STRING, 
				 G_TYPE_STRING, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (proxy, "NameOwnerChanged",
				     G_CALLBACK (name_owner_changed),
				     gypsy, NULL);

	dbus_g_connection_register_g_object (conn, "/org/freedesktop/Gypsy", G_OBJECT (gypsy));

	g_main_loop_run (mainloop);

	return 0;
}
