/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007 Iain Holmes
 * Copyright (C) 2007 Openedhand, Ltd
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

/*
 * GypsyServer - The main control object that creates GPS connection objects.
 */
#include <glib.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gypsy-server.h"
#include "gypsy-client.h"

enum {
	TERMINATE,
	LAST_SIGNAL,
};

typedef struct _GypsyServerPrivate {
	DBusGConnection *connection;
	GHashTable *connections;
	int client_count; /* When client_count returns to 0, 
			     we quit the daemon after TERMINATE_TIMEOUT */
	guint32 terminate_id;
} GypsyServerPrivate;

static guint32 signals[LAST_SIGNAL] = {0, };

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE  ((obj), GYPSY_TYPE_SERVER, GypsyServerPrivate))

G_DEFINE_TYPE (GypsyServer, gypsy_server, G_TYPE_OBJECT);

#define GYPSY_GPS_PATH "/org/freedesktop/Gypsy/"
#define TERMINATE_TIMEOUT 10000 /* 10 second timeout */

static void gypsy_server_create (GypsyServer            *gps,
				 const char             *IN_device_path,
				 DBusGMethodInvocation *context);
static void gypsy_server_shutdown (GypsyServer           *gps,
				   const char            *IN_device_path,
				   DBusGMethodInvocation *context);
#include "gypsy-server-glue.h"

GQuark
gypsy_server_error_quark (void)
{
	static GQuark quark = 0;
	if (quark == 0) {
		quark = g_quark_from_static_string ("gypsy-server-error-quark");
	}

	return quark;
}

/* We don't want to terminate at the moment, as there is no way to be
   restarted until D-Bus 1.2 which has the System bus activation stuff
*/
static gboolean
gypsy_terminate (gpointer data)
{
	GypsyServer *server = data;

	g_signal_emit (server, signals[TERMINATE], 0);

	return FALSE;
}

/* IN_args contains that path to the GPS device we wish to open */
static void
gypsy_server_create (GypsyServer           *gps,
		     const char            *IN_device_path,
		     DBusGMethodInvocation *context)
{
	GypsyServerPrivate *priv;
	GypsyClient *client;
	char *path, *device_name, *sender;
	GList *list;

	priv = GET_PRIVATE (gps);

	/* We might be in the termination timeout when we receive a new
	   create request, so cancel that timeout */
	if (priv->terminate_id > 0) {
		g_source_remove (priv->terminate_id);
		priv->terminate_id = 0;
	}

	g_debug ("Creating client for %s", IN_device_path);
	device_name = g_path_get_basename (IN_device_path);
	g_debug ("Device name: %s", device_name);
	path = g_strdup_printf ("%s%s", GYPSY_GPS_PATH, 
				g_strdelimit (device_name, ":", '_'));
	g_free (device_name);

	client = (GypsyClient *) dbus_g_connection_lookup_g_object (priv->connection, path);
	if (client == NULL) {
		/* If there isn't already an object registered on that path
		   create and register it */
		client = g_object_new (GYPSY_TYPE_CLIENT, 
				       "device_path", IN_device_path,
				       NULL);
	
		dbus_g_connection_register_g_object (priv->connection, path,
						     G_OBJECT (client));
	} else {
		/* Ref the client so that when one client calls shutdown
		   we won't destroy another clients object */
		g_object_ref (client);
	}

	g_debug ("Registered client on %s", path);

	/* Update the hash of open connnctions */
	sender = dbus_g_method_get_sender (context);
	list = g_hash_table_lookup (priv->connections, sender);
	list = g_list_prepend (list, client);
	g_hash_table_insert (priv->connections, sender, list);

	priv->client_count++;

	dbus_g_method_return (context, path);
	g_free (path);
}

static void
gypsy_server_shutdown (GypsyServer           *gps,
		       const char            *IN_device_path,
		       DBusGMethodInvocation *context)
{
	GypsyServerPrivate *priv;
	GypsyClient *client;
	GList *list, *owner;
	char *path, *device_name, *sender;

	priv = GET_PRIVATE (gps);

	g_debug ("Finding client for %s", IN_device_path);
	device_name = g_path_get_basename (IN_device_path);
	g_debug ("Device name: %s", device_name);
	path = g_strdup_printf ("%s%s", GYPSY_GPS_PATH, device_name);

	client = (GypsyClient *) dbus_g_connection_lookup_g_object (priv->connection, path);
	g_free (path);

	if (client == NULL) {
		dbus_g_method_return_error (context,
					    g_error_new (GYPSY_SERVER_ERROR,
							 GYPSY_SERVER_ERROR_NO_CLIENT,
							 "No such client: %s",
							 device_name));
	} else {
		if (--priv->client_count == 0) {
			if (priv->terminate_id == 0) {
				priv->terminate_id = g_timeout_add (TERMINATE_TIMEOUT,
								    gypsy_terminate,
								    gps);
			}
		}

		/* Update the hash of open connnctions */
		sender = dbus_g_method_get_sender (context);
		list = g_hash_table_lookup (priv->connections, sender);
		owner = g_list_find (list, client);
		if (owner) {
			g_object_unref (owner->data);
		}
		list = g_list_remove (list, client);
		g_hash_table_insert (priv->connections, sender, list);

		dbus_g_method_return (context);

	}

	g_free (device_name);
}

static void
finalize (GObject *object) 
{
 	GypsyServerPrivate *priv = GET_PRIVATE (object);

	g_hash_table_destroy (priv->connections);
	((GObjectClass *) gypsy_server_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyServerPrivate *priv = GET_PRIVATE (object);

	if (priv->connection) {
		dbus_g_connection_unref (priv->connection);
		priv->connection = NULL;
	}

	((GObjectClass *) gypsy_server_parent_class)->dispose (object);
}

static void
gypsy_server_class_init (GypsyServerClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;
	
	o_class->finalize = finalize;
	o_class->dispose = dispose;

	g_type_class_add_private (klass, sizeof (GypsyServerPrivate));
	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
					 &dbus_glib_gypsy_server_object_info);

	signals[TERMINATE] = g_signal_new ("terminate",
					   G_TYPE_FROM_CLASS (klass),
					   G_SIGNAL_RUN_FIRST |
					   G_SIGNAL_NO_RECURSE,
					   G_STRUCT_OFFSET (GypsyServerClass,
							    terminate),
					   NULL, NULL,
					   g_cclosure_marshal_VOID__VOID,
					   G_TYPE_NONE, 0);
}

static void
gypsy_server_init (GypsyServer *gps)
{
	GypsyServerPrivate *priv = GET_PRIVATE (gps);
	GError *error = NULL;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (priv->connection == NULL) {
		g_warning ("Error connecting to system bus:\n%s",
			   error->message);
		g_error_free (error);
		return;
	}
	dbus_g_connection_ref (priv->connection);

	priv->connections = g_hash_table_new_full (g_str_hash, g_str_equal,
						   g_free, NULL);

	priv->client_count = 0;
	priv->terminate_id = 0;
}

void
gypsy_server_remove_clients (GypsyServer *gps,
			     const char  *prev_owner)
{
	GypsyServerPrivate *priv = GET_PRIVATE (gps);
	char *key;
	GList *list = NULL;

	if (g_hash_table_lookup_extended (priv->connections, prev_owner,
					  (gpointer) &key, (gpointer) &list)) {
		GList *l;

		for (l = list; l; l = l->next) {
			g_object_unref (l->data);
			if (--priv->client_count == 0) {
				if (priv->terminate_id == 0) {
					priv->terminate_id = 
						g_timeout_add (TERMINATE_TIMEOUT,
							       gypsy_terminate,
							       gps);
				}
			}
		}
		g_list_free (list);
		g_hash_table_remove (priv->connections, prev_owner);
	}
}
	
