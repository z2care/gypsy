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
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GYPSY_DEVICE_H__
#define __GYPSY_DEVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS 

/**
 * GYPSY_DEVICE_DBUS_SERVICE:
 *
 * A define containing the name of the Device service
 */
#define GYPSY_DEVICE_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_DEVICE_DBUS_INTERFACE
 *
 * A define containing the name of the Device interface
 */
#define GYPSY_DEVICE_DBUS_INTERFACE "org.freedesktop.Gypsy.Device"

#define GYPSY_TYPE_DEVICE (gypsy_device_get_type ())
#define GYPSY_DEVICE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_DEVICE, GypsyDevice))
#define GYPSY_IS_DEVICE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_DEVICE))

/**
 * GypsyDeviceFixStatus:
 * @GYPSY_DEVICE_FIX_STATUS_INVALID: The fix is invalid
 * @GYPSY_DEVICE_FIX_STATUS_NONE: A fix has not yet been obtained
 * @GYPSY_DEVICE_FIX_STATUS_2D: A fix with latitude and longitude has been obtained
 * @GYPSY_DEVICE_FIX_STATUS_3D: A fix with latitude, longitude and altitude has been obtained
 *
 * An enumeration representing the various fix states that a GPS device can be in.
 */
typedef enum {
	GYPSY_DEVICE_FIX_STATUS_INVALID = 0,
	GYPSY_DEVICE_FIX_STATUS_NONE,
	GYPSY_DEVICE_FIX_STATUS_2D,
	GYPSY_DEVICE_FIX_STATUS_3D
} GypsyDeviceFixStatus;

/**
 * GypsyDevice:
 *
 * There are no public fields in #GypsyDevice.
 */
typedef struct _GypsyDevice {
	GObject parent_object;
} GypsyDevice;

typedef struct _GypsyDeviceClass {
	GObjectClass parent_class;

	void (*connection_changed) (GypsyDevice *device,
				    gboolean     connected);
	void (*fix_status_changed) (GypsyDevice         *device,
				    GypsyDeviceFixStatus status);
} GypsyDeviceClass;

GType gypsy_device_get_type (void);

GypsyDevice *gypsy_device_new (const char *object_path);

gboolean gypsy_device_start (GypsyDevice *device,
			    GError     **error);
gboolean gypsy_device_stop (GypsyDevice *device,
			    GError     **error);

GypsyDeviceFixStatus gypsy_device_get_fix_status (GypsyDevice *device,
						  GError      **error);
gboolean gypsy_device_get_connection_status (GypsyDevice *device,
					     GError     **error);

G_END_DECLS

#endif
