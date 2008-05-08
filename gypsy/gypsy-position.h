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

#ifndef __GYPSY_POSITION_H__
#define __GYPSY_POSITION_H__

#include <glib-object.h>

G_BEGIN_DECLS 

/**
 * GYPSY_POSITION_DBUS_SERVICE:
 *
 * A define containing the address of the Position service
 */
#define GYPSY_POSITION_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_POSITION_DBUS_INTERFACE:
 * 
 * A define containing the name of the Position interface
 */
#define GYPSY_POSITION_DBUS_INTERFACE "org.freedesktop.Gypsy.Position"

#define GYPSY_TYPE_POSITION (gypsy_position_get_type ())
#define GYPSY_POSITION(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_POSITION, GypsyPosition))
#define GYPSY_IS_POSITION(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_POSITION))

/**
 * GypsyPositionFields:
 * @GYPSY_POSITION_FIELDS_NONE: None of the fields are valid
 * @GYPSY_POSITION_FIELDS_LATITUDE: The latitude field is valid
 * @GYPSY_POSITION_FIELDS_LONGITUDE: The longitude field is valid
 * @GYPSY_POSITION_FIELDS_ALTITUDE: The altitude field is valid
 *
 * A bitmask telling which fields in the position_changed callback are valid
 */
typedef enum {
	GYPSY_POSITION_FIELDS_NONE = 0,
	GYPSY_POSITION_FIELDS_LATITUDE = 1 << 0,
	GYPSY_POSITION_FIELDS_LONGITUDE = 1 << 1,
	GYPSY_POSITION_FIELDS_ALTITUDE = 1 << 2
} GypsyPositionFields;

/**
 * GypsyPosition:
 *
 * There are no public fields in #GypsyPosition.
 */
typedef struct _GypsyPosition {
	GObject parent_object;
} GypsyPosition;

typedef struct _GypsyPositionClass {
	GObjectClass parent_class;

	void (*position_changed) (GypsyPosition      *position,
				  GypsyPositionFields fields_set,
				  int                 timestamp,
				  double              latitude,
				  double              longitude,
				  double              altitude);
} GypsyPositionClass;

GType gypsy_position_get_type (void);

GypsyPosition *gypsy_position_new (const char *object_path);

GypsyPositionFields gypsy_position_get_position (GypsyPosition *position,
						 int           *timestamp,
						 double        *latitude,
						 double        *longitude,
						 double        *altitude,
						 GError       **error);

G_END_DECLS

#endif
