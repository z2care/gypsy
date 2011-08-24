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
 * GypsyDiscovery - GPS device discovery service
 */
#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <gudev/gudev.h>

#include "gypsy-discovery.h"

enum {
        PROP_0,
};

enum {
        DEVICE_ADDED,
        DEVICE_REMOVED,
        LAST_SIGNAL,
};

struct _GypsyDiscoveryPrivate {
        GUdevClient *client;
	GPtrArray *known_devices;

#ifdef HAVE_BLUEZ
	DBusGConnection *connection;
	DBusGProxy *adapter_proxy;
	DBusGProxy *manager_proxy;
#endif
};

#define BLUEZ_SERVICE "org.bluez"
#define BLUEZ_MANAGER_PATH "/"
#define BLUEZ_MANAGER_IFACE "org.bluez.Manager"
#define BLUEZ_ADAPTER_IFACE "org.bluez.Adapter"
#define BLUEZ_DEVICE_IFACE "org.bluez.Device"

#ifndef DBUS_TYPE_G_OBJECT_PATH_ARRAY
#define DBUS_TYPE_G_OBJECT_PATH_ARRAY \
        (dbus_g_type_get_collection("GPtrArray", DBUS_TYPE_G_OBJECT_PATH))
#endif

#ifndef DBUS_TYPE_G_DICTIONARY
#define DBUS_TYPE_G_DICTIONARY \
        (dbus_g_type_get_map("GHashTable", G_TYPE_STRING, G_TYPE_VALUE))
#endif

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GYPSY_TYPE_DISCOVERY, GypsyDiscoveryPrivate))
G_DEFINE_TYPE (GypsyDiscovery, gypsy_discovery, G_TYPE_OBJECT);
static guint32 signals[LAST_SIGNAL] = {0,};

static gboolean gypsy_discovery_list_devices (GypsyDiscovery  *discovery,
                                              char          ***devices,
                                              GError         **error);
static gboolean gypsy_discovery_start_scanning (GypsyDiscovery *discovery,
                                                GError        **error);
static gboolean gypsy_discovery_stop_scanning (GypsyDiscovery *discovery,
                                               GError        **error);

#include "gypsy-discovery-glue.h"

static void
gypsy_discovery_finalize (GObject *object)
{
        GypsyDiscovery *self = (GypsyDiscovery *) object;
	GypsyDiscoveryPrivate *priv = self->priv;

	if (priv->known_devices) {
		g_ptr_array_free (priv->known_devices, TRUE);
		priv->known_devices = NULL;
	}

        G_OBJECT_CLASS (gypsy_discovery_parent_class)->finalize (object);
}

static void
gypsy_discovery_dispose (GObject *object)
{
        GypsyDiscovery *self = (GypsyDiscovery *) object;
        GypsyDiscoveryPrivate *priv = self->priv;

        if (priv->client) {
                g_object_unref (priv->client);
                priv->client = NULL;
        }

#ifdef HAVE_BLUEZ
	if (priv->manager_proxy) {
		g_object_unref (priv->manager_proxy);
		priv->manager_proxy = NULL;
	}

	if (priv->adapter_proxy) {
		g_object_unref (priv->adapter_proxy);
		priv->adapter_proxy = NULL;
	}
#endif

        G_OBJECT_CLASS (gypsy_discovery_parent_class)->dispose (object);
}

#if 0
static void
gypsy_discovery_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
        GypsyDiscovery *self = (GypsyDiscovery *) object;

        switch (prop_id) {

        default:
                break;
        }
}

static void
gypsy_discovery_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
        GypsyDiscovery *self = (GypsyDiscovery *) object;

        switch (prop_id) {

        default:
                break;
        }
}
#endif

static void
gypsy_discovery_class_init (GypsyDiscoveryClass *klass)
{
        GObjectClass *o_class = (GObjectClass *) klass;

        o_class->dispose = gypsy_discovery_dispose;
        o_class->finalize = gypsy_discovery_finalize;
/*
        o_class->set_property = gypsy_discovery_set_property;
        o_class->get_property = gypsy_discovery_get_property;
*/
        g_type_class_add_private (klass, sizeof (GypsyDiscoveryPrivate));
        dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                         &dbus_glib_gypsy_discovery_object_info);

        signals[DEVICE_ADDED] = g_signal_new ("device-added",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      0, NULL, NULL,
					      g_cclosure_marshal_VOID__STRING,
					      G_TYPE_NONE, 1,
					      G_TYPE_STRV);
        signals[DEVICE_REMOVED] = g_signal_new ("device-removed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						0, NULL, NULL,
						g_cclosure_marshal_VOID__STRING,
						G_TYPE_NONE, 1,
						G_TYPE_STRV);
}

static void
uevent_occurred_cb (GUdevClient    *client,
                    const char     *action,
                    GUdevDevice    *device,
                    GypsyDiscovery *discovery)
{
        if (strcmp (action, "add") == 0) {
        } else if (strcmp (action, "remove") == 0) {
        }
}

#ifdef HAVE_BLUEZ
static gboolean
class_is_positioning_device (GValue *value)
{
	int class_id = g_value_get_uint (value);

	return ((class_id >> 16) & 0x1);
}

static gboolean
get_positioning_devices (GypsyDiscovery *discovery,
			 GPtrArray *devices,
			 GError   **error)
{
	GypsyDiscoveryPrivate *priv = discovery->priv;
	int i;

	if (devices == NULL) {
		/* It didn't fail, there just weren't any devices */
		return TRUE;
	}

	for (i = 0; i < devices->len; i++) {
		DBusGProxy *proxy;
		GHashTable *properties;
		GValue *path, *class;
		gboolean ret;

		proxy = dbus_g_proxy_new_for_name (priv->connection,
						   BLUEZ_SERVICE,
						   devices->pdata[i],
						   BLUEZ_DEVICE_IFACE);
		ret = dbus_g_proxy_call (proxy, "GetProperties", error,
					 G_TYPE_INVALID,
					 DBUS_TYPE_G_DICTIONARY, &properties,
					 G_TYPE_INVALID);
		if (ret == FALSE) {
			g_object_unref (proxy);
			return FALSE;
		}

		class = g_hash_table_lookup (properties, "Class");
		if (class == NULL) {
			g_object_unref (proxy);
			g_hash_table_destroy (properties);
			continue;
		}

		if (class_is_positioning_device (class) == FALSE) {
			g_object_unref (proxy);
			g_hash_table_destroy (properties);
			continue;
		}

		path = g_hash_table_lookup (properties, "Address");
		if (path == NULL) {
			g_object_unref (proxy);
			g_hash_table_destroy (properties);
			continue;
		}

		g_ptr_array_add (priv->known_devices,
				 g_value_dup_string (path));

		g_object_unref (proxy);
		g_hash_table_destroy (properties);
	}

	return TRUE;
}
#endif

static void
setup_bluetooth_discovery (GypsyDiscovery *discovery)
{
#ifdef HAVE_BLUEZ
        GypsyDiscoveryPrivate *priv = self->priv;
	GError *error = NULL;
	char *default_adapter;
	GPtrArray *devices;
	gboolean ret;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (!priv->connection) {
		g_warning ("Error getting D-Bus connection: %s\n"
			   "Continuing without Bluetooth discovery",
			   error->message);
		g_error_free (error);
		return;
	}

	priv->manager_proxy = dbus_g_proxy_new_for_name (priv->connection,
							 BLUEZ_SERVICE,
							 BLUEZ_MANAGER_PATH,
							 BLUEZ_MANAGER_IFACE);

	ret = dbus_g_proxy_call (priv->manager_proxy, "DefaultAdapter",
				 &error, G_TYPE_INVALID,
				 DBUS_TYPE_G_OBJECT_PATH, &default_adapter,
				 G_TYPE_INVALID);
	if (ret == FALSE) {
		g_warning ("Error getting default adapter path: %s\n"
			   "Continuing without Bluetooth discovery",
			   error->message);
		g_error_free (error);

		/* Shut down BT discovery */
		g_object_unref (priv->manager_proxy);
		priv->manager_proxy = NULL;

		priv->connection = NULL;
		return;
	}

	priv->adapter_proxy = dbus_g_proxy_new_for_name (priv->connection,
							 BLUEZ_SERVICE,
							 default_adapter,
							 BLUEZ_ADAPTER_IFACE);
	ret = dbus_g_proxy_call (priv->adapter_proxy, "ListDevices", &error,
				 G_TYPE_INVALID,
				 DBUS_TYPE_G_OBJECT_PATH_ARRAY, &devices,
				 G_TYPE_INVALID);
	if (ret == FALSE) {
		g_warning ("Error getting devices on %s: %s\n", default_adapter,
			   error->message);
		g_error_free (error);
		return;
	}

	if (!get_positioning_devices (self, devices, &error)) {
		g_warning ("Error getting positioning devices: %s",
			   error->message);
		g_error_free (error);
		return;
	}
#endif
}

static void
gypsy_discovery_init (GypsyDiscovery *self)
{
        GypsyDiscoveryPrivate *priv = GET_PRIVATE (self);
        const char * const subsystems[] = { "" };

        self->priv = priv;

	priv->known_devices = g_ptr_array_new ();

        priv->client = g_udev_client_new (subsystems);
        g_signal_connect (priv->client, "uevent",
                          G_CALLBACK (uevent_occurred_cb), self);

	/* FIXME: Get devices UDev knows about */

	setup_bluetooth_discovery (self);
}

static gboolean
gypsy_discovery_list_devices (GypsyDiscovery  *discovery,
                              char          ***devices,
                              GError         **error)
{
	GypsyDiscoveryPrivate *priv = discovery->priv;
	int i;

	*devices = g_new (char *, priv->known_devices->len);
	for (i = 0; i < priv->known_devices->len; i++) {
		(*devices)[i] = g_strdup (priv->known_devices->pdata[i]);
	}

        return TRUE;
}

static gboolean
gypsy_discovery_start_scanning (GypsyDiscovery *discovery,
                                GError        **error)
{
	g_warning ("Scanning not implemented");
        return TRUE;
}

static gboolean
gypsy_discovery_stop_scanning (GypsyDiscovery *discovery,
                               GError        **error)
{
	g_warning ("Scanning not implemented");
        return TRUE;
}
