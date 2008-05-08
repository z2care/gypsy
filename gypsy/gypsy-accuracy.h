/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>, Ross Burton <ross@openedhand.com>
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

#ifndef __GYPSY_ACCURACY_H__
#define __GYPSY_ACCURACY_H__

#include <glib-object.h>

G_BEGIN_DECLS 

/**
 * GYPSY_ACCURACY_DBUS_SERVICE:
 *
 * A define containing the address of the Accuracy service
 */
#define GYPSY_ACCURACY_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_ACCURACY_DBUS_INTERFACE:
 * 
 * A define containing the name of the Accuracy interface
 */
#define GYPSY_ACCURACY_DBUS_INTERFACE "org.freedesktop.Gypsy.Accuracy"

#define GYPSY_TYPE_ACCURACY (gypsy_accuracy_get_type ())
#define GYPSY_ACCURACY(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_ACCURACY, GypsyAccuracy))
#define GYPSY_IS_ACCURACY(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_ACCURACY))

/**
 * GypsyAccuracyFields:
 * @GYPSY_ACCURACY_FIELDS_NONE: None of the fields are valid
 * @GYPSY_ACCURACY_FIELDS_POSITION: The Position (3D) DOP field is valid
 * @GYPSY_ACCURACY_FIELDS_HORIZONTAL: The Horizonal (2D) DOP field is valid
 * @GYPSY_ACCURACY_FIELDS_VERTICAL: The Vertical (altitude) DOP field is valid
 *
 * A bitmask telling which fields in the accuracy_changed callback are valid
 */
typedef enum {
	GYPSY_ACCURACY_FIELDS_NONE = 0,
	GYPSY_ACCURACY_FIELDS_POSITION = 1 << 0,
	GYPSY_ACCURACY_FIELDS_HORIZONTAL = 1 << 1,
	GYPSY_ACCURACY_FIELDS_VERTICAL = 1 << 2,
} GypsyAccuracyFields;

/**
 * GypsyAccuracy:
 *
 * There are no public fields in #GypsyAccuracy.
 */
typedef struct _GypsyAccuracy {
	GObject parent_object;
} GypsyAccuracy;

typedef struct _GypsyAccuracyClass {
	GObjectClass parent_class;
	void (*accuracy_changed) (GypsyAccuracy *accuracy,
				  GypsyAccuracyFields fields_set,
				  float        pdop,
				  float        hdop,
				  float        vdop);
} GypsyAccuracyClass;

GType gypsy_accuracy_get_type (void);

GypsyAccuracy *gypsy_accuracy_new (const char *object_path);

GypsyAccuracyFields gypsy_accuracy_get_accuracy (GypsyAccuracy *accuracy,
						 double        *pdop,
						 double        *hdop,
						 double        *vdop,
						 GError       **error);

G_END_DECLS

#endif
