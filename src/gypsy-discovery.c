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

#include "gypsy-debug.h"
#include "gypsy-discovery.h"
#include "gypsy-marshal-internal.h"

enum {
        PROP_0,
};

enum {
        DEVICE_ADDED,
        DEVICE_REMOVED,
        LAST_SIGNAL,
};

typedef struct _DeviceInfo {
	char *device_path;
	char *type;
} DeviceInfo;

struct _GypsyDiscoveryPrivate {
        GUdevClient *client;

	/* Contains DeviceInfo */
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
					      char          ***types,
                                              GError         **error);
static gboolean gypsy_discovery_start_scanning (GypsyDiscovery *discovery,
                                                GError        **error);
static gboolean gypsy_discovery_stop_scanning (GypsyDiscovery *discovery,
                                               GError        **error);

#include "gypsy-discovery-glue.h"

const char *internal_type = "internal";
const char *bluetooth_type = "bluetooth";
const char *usb_type = "usb";

static void
device_info_free (gpointer data)
{
	DeviceInfo *di = (DeviceInfo *) data;

	g_free (di->device_path);
	g_slice_free (DeviceInfo, di);
}

static DeviceInfo *
device_info_new (const char *device_path,
		 const char *type)
{
	DeviceInfo *di = g_slice_new (DeviceInfo);
	di->device_path = g_strdup (device_path);
	di->type = (char *) type;

	return di;
}

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

static void
gypsy_discovery_class_init (GypsyDiscoveryClass *klass)
{
        GObjectClass *o_class = (GObjectClass *) klass;

        o_class->dispose = gypsy_discovery_dispose;
        o_class->finalize = gypsy_discovery_finalize;

        g_type_class_add_private (klass, sizeof (GypsyDiscoveryPrivate));
        signals[DEVICE_ADDED] = g_signal_new ("device-added",
					      G_TYPE_FROM_CLASS (klass),
					      G_SIGNAL_RUN_FIRST |
					      G_SIGNAL_NO_RECURSE,
					      0, NULL, NULL,
					      gypsy_marshal_VOID__STRING_STRING,
					      G_TYPE_NONE, 2,
					      G_TYPE_STRING,
					      G_TYPE_STRING);
        signals[DEVICE_REMOVED] = g_signal_new ("device-removed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						0, NULL, NULL,
						gypsy_marshal_VOID__STRING_STRING,
						G_TYPE_NONE, 2,
						G_TYPE_STRING,
						G_TYPE_STRING);
        dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
                                         &dbus_glib_gypsy_discovery_object_info);
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
				 device_info_new (g_value_get_string (path),
						  bluetooth_type));

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
        GypsyDiscoveryPrivate *priv = discovery->priv;
	GError *error = NULL;
	char *default_adapter;
	GPtrArray *devices;
	gboolean ret;

	GYPSY_NOTE (DISCOVERY, "Bluetooth discovery enabled");

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

	if (!get_positioning_devices (discovery, devices, &error)) {
		g_warning ("Error getting positioning devices: %s",
			   error->message);
		g_error_free (error);
		return;
	}
#endif
}

/* A list of all the product IDs we know about */
struct ProductMap {
	char *product_id;
	char *product_name;
};

static struct ProductMap known_ids[] = {
	{ "e8d/3329/100", "MTK GPS Receiver" },
	{ "0e8d/3329/0100", "MTK GPS Receiver" },
	{ "1546/1a4/100", "u-blox AG ANTARIS r4 GPS Receiver" },
	{ "1546/01a4/0100", "u-blox AG ANTARIS r4 GPS Receiver" },
	{ NULL, NULL }
};

static const char *
maybe_add_device (GypsyDiscovery *discovery,
		  GUdevDevice    *device)
{
	GypsyDiscoveryPrivate *priv = discovery->priv;
	GUdevDevice *parent;
	const char *property_id, *property_type, *name;
	int i;

	name = g_udev_device_get_device_file (device);
	if (name == NULL) {
		return NULL;
	}

	/* Find the usb device that owns this TTY */
	parent = g_udev_device_get_parent (device);
	while (parent) {
		property_type = g_udev_device_get_property (parent, "DEVTYPE");

		GYPSY_NOTE (DISCOVERY, "Found UDev type: %s", property_type);
		if (property_type &&
		    g_str_equal (property_type, "usb_device")) {
			break;
		}

		parent = g_udev_device_get_parent (parent);
	}

	property_id = g_udev_device_get_property (parent, "PRODUCT");
	if (property_id == NULL) {
		return NULL;
	}

	for (i = 0; known_ids[i].product_id; i++) {
		if (g_str_equal (property_id,
				 known_ids[i].product_id)) {
			GYPSY_NOTE (DISCOVERY, "Found %s - %s",
				    known_ids[i].product_name,
				    name);
			g_ptr_array_add (priv->known_devices,
					 device_info_new (name, usb_type));

			return name;
		}
	}

	return NULL;
}

static void
remove_device_from_array (GPtrArray  *array,
			  const char *device_path)
{
	int i;

	if (array == NULL || device_path == NULL) {
		return;
	}

	for (i = 0; i < array->len; i++) {
		DeviceInfo *di = array->pdata[i];

		GYPSY_NOTE (DISCOVERY, "Comparing %s -> %s", device_path,
			    di->device_path);
		if (g_str_equal (device_path, di->device_path)) {
			g_ptr_array_remove_index (array, i);
			return;
		}
	}
}

static char *
build_product_id_from_tty (GUdevDevice *tty)
{
	const char *vendor_id, *model_id, *revision_id;

	vendor_id = g_udev_device_get_property (tty, "ID_VENDOR_ID");
	model_id = g_udev_device_get_property (tty, "ID_MODEL_ID");
	revision_id = g_udev_device_get_property (tty, "ID_REVISION");

	if (vendor_id == NULL || model_id == NULL || revision_id == NULL) {
		GYPSY_NOTE (DISCOVERY, "Missing property %s %s %s",
			    vendor_id, model_id, revision_id);
		return NULL;
	}

	return g_strdup_printf ("%s/%s/%s", vendor_id, model_id, revision_id);
}

static const char *
maybe_remove_device (GypsyDiscovery *discovery,
		     GUdevDevice    *device)
{
	GypsyDiscoveryPrivate *priv = discovery->priv;
	const char *property_id, *property_type, *name;
	char *tty_id;
	GUdevDevice *parent;
	int i;

	name = g_udev_device_get_device_file (device);
	if (name == NULL) {
		return NULL;
	}

	/* Find the usb device that owns this TTY */
	parent = g_udev_device_get_parent (device);
	while (parent) {
		property_type = g_udev_device_get_property (parent, "DEVTYPE");

		GYPSY_NOTE (DISCOVERY, "Found UDev type: %s", property_type);
		if (property_type &&
		    g_str_equal (property_type, "usb_device")) {
			break;
		}

		parent = g_udev_device_get_parent (parent);
	}

	property_id = g_udev_device_get_property (parent, "PRODUCT");
	if (property_id == NULL) {
		GYPSY_NOTE (DISCOVERY, "Product ID was NULL");
		return NULL;
	}

	GYPSY_NOTE (DISCOVERY, "Found Product ID %s", property_id);
	for (i = 0; known_ids[i].product_id; i++) {
		if (g_str_equal (property_id,
				 known_ids[i].product_id)) {
			GYPSY_NOTE (DISCOVERY, "Found %s - %s",
				    known_ids[i].product_name,
				    name);
			remove_device_from_array (priv->known_devices,
						  name);

			return name;
		}
	}

	/* When removing a USB device, UDev seems to often (only?) give the
	   parent of the tty as the USB port or hub rather than the device
	   that was removed. But there may be the various components of the
	   product ID on the tty device. Check that against the known
	   database */
	tty_id = build_product_id_from_tty (device);
	if (tty_id == NULL) {
		GYPSY_NOTE (DISCOVERY, "%s is an unknown device.", name);
		return NULL;
	}

	GYPSY_NOTE (DISCOVERY, "Found usb_device with unknown product ID. Falling back to tty IDs: %s", tty_id);
	for (i = 0; known_ids[i].product_id; i++) {
		if (g_str_equal (tty_id,
				 known_ids[i].product_id)) {
			GYPSY_NOTE (DISCOVERY, "Found %s - %s",
				    known_ids[i].product_name, name);
			remove_device_from_array (priv->known_devices, name);
			g_free (tty_id);

			return name;
		}
	}

	GYPSY_NOTE (DISCOVERY, "%s (%s)is an unknown device.", name, tty_id);
	g_free (tty_id);

	return NULL;
}

static void
uevent_occurred_cb (GUdevClient    *client,
                    const char     *action,
                    GUdevDevice    *device,
                    GypsyDiscovery *discovery)
{
        if (strcmp (action, "add") == 0) {
		const char *path;

		GYPSY_NOTE (DISCOVERY, "UDev add event occurred");
		path = maybe_add_device (discovery, device);
		if (path == NULL) {
			GYPSY_NOTE (DISCOVERY, "Was not a known GPS device");
			return;
		}

		GYPSY_NOTE (DISCOVERY, "Was a known GPS device at %s", path);
		g_signal_emit (discovery, signals[DEVICE_ADDED], 0, path, "usb");
        } else if (strcmp (action, "remove") == 0) {
		const char *path;

		GYPSY_NOTE (DISCOVERY, "UDev remove event occurred");
		path = maybe_remove_device (discovery, device);
		if (path == NULL) {
			GYPSY_NOTE (DISCOVERY, "Was not a known GPS device");
			return;
		}

		GYPSY_NOTE (DISCOVERY, "Was a known GPS device at %s", path);
		g_signal_emit (discovery, signals[DEVICE_REMOVED], 0, path, "usb");
        }
}

static void
add_known_udev_devices (GypsyDiscovery *self)
{
	GypsyDiscoveryPrivate *priv = self->priv;
	GList *udev_devices = NULL, *l;

	udev_devices = g_udev_client_query_by_subsystem (priv->client, "tty");
	for (l = udev_devices; l; l = l->next) {
		GUdevDevice *device = l->data;

		maybe_add_device (self, device);
		g_object_unref (device);
	}

	g_list_free (udev_devices);
}

static void
gypsy_discovery_init (GypsyDiscovery *self)
{
        GypsyDiscoveryPrivate *priv = GET_PRIVATE (self);
        const char * const subsystems[] = { "tty" };

        self->priv = priv;

	priv->known_devices = g_ptr_array_new_with_free_func (device_info_free);

        priv->client = g_udev_client_new (subsystems);
        g_signal_connect (priv->client, "uevent",
                          G_CALLBACK (uevent_occurred_cb), self);

	add_known_udev_devices (self);

	setup_bluetooth_discovery (self);
}

static gboolean
gypsy_discovery_list_devices (GypsyDiscovery  *discovery,
                              char          ***devices,
			      char          ***types,
                              GError         **error)
{
	GypsyDiscoveryPrivate *priv = discovery->priv;
	int i;

	*devices = g_new (char *, priv->known_devices->len + 1);
	*types = g_new (char *, priv->known_devices->len + 1);
	for (i = 0; i < priv->known_devices->len; i++) {
		DeviceInfo *di = priv->known_devices->pdata[i];

		(*devices)[i] = g_strdup (di->device_path);
		(*types)[i] = g_strdup (di->type);
	}

	/* NULL terminate the arrays */
	(*devices)[i] = NULL;
	(*types)[i] = NULL;

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
