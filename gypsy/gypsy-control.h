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

#ifndef __GYPSY_CONTROL_H__
#define __GYPSY_CONTROL_H__

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * GYPSY_CONTROL_DBUS_SERVICE:
 *
 * A define containing the service name of the control service
 */
#define GYPSY_CONTROL_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_CONTROL_DBUS_PATH:
 *
 * A define containing the object path of the Gypsy object
 */
#define GYPSY_CONTROL_DBUS_PATH "/org/freedesktop/Gypsy"

/**
 * GYPSY_CONTROL_DBUS_INTERFACE:
 *
 * A define containing the name of the Control interface
 */
#define GYPSY_CONTROL_DBUS_INTERFACE "org.freedesktop.Gypsy.Server"

#define GYPSY_TYPE_CONTROL (gypsy_control_get_type ())
#define GYPSY_CONTROL(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_CONTROL, GypsyControl))
#define GYPSY_IS_CONTROL(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_CONTROL))

/**
 * GypsyControl:
 *
 * There are no public fields in #GypsyControl.
 */
typedef struct _GypsyControl {
	GObject parent_object;
} GypsyControl;

typedef struct _GypsyControlClass {
	GObjectClass parent_class;
} GypsyControlClass;

GType gypsy_control_get_type (void);

GypsyControl *gypsy_control_get_default (void);
char *gypsy_control_create (GypsyControl *control,
			    const char   *device_name,
			    GError      **error);

G_END_DECLS

#endif
