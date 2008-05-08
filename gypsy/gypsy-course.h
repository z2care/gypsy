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

#ifndef __GYPSY_COURSE_H__
#define __GYPSY_COURSE_H__

#include <glib-object.h>

#include <gypsy/gypsy-device.h>

G_BEGIN_DECLS 

/**
 * GYPSY_COURSE_DBUS_SERVICE:
 *
 * A define containing the address of the Course service
 */
#define GYPSY_COURSE_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_COURSE_DBUS_INTERFACE:
 *
 * A define containing the name of the Course interface
 */
#define GYPSY_COURSE_DBUS_INTERFACE "org.freedesktop.Gypsy.Course"

#define GYPSY_TYPE_COURSE (gypsy_course_get_type ())
#define GYPSY_COURSE(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_COURSE, GypsyCourse))
#define GYPSY_IS_COURSE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_COURSE))

/**
 * GypsyCourseFields:
 * @GYPSY_COURSE_FIELDS_NONE: None of the fields are valid
 * @GYPSY_COURSE_FIELDS_SPEED: The speed field is valid
 * @GYPSY_COURSE_FIELDS_DIRECTION: The direction field is valid
 * @GYPSY_COURSE_FIELDS_CLIMB: The climb field is valid
 *
 * A bitfield telling which fields in the course_changed callback are valid
 */
typedef enum {
	GYPSY_COURSE_FIELDS_NONE = 0,
	GYPSY_COURSE_FIELDS_SPEED = 1 << 0,
	GYPSY_COURSE_FIELDS_DIRECTION = 1 << 1,
	GYPSY_COURSE_FIELDS_CLIMB = 1 << 2
} GypsyCourseFields;

/**
 * GypsyCourse:
 *
 * There are no public fields in #GypsyCourse.
 */
typedef struct _GypsyCourse {
	GObject parent_object;
} GypsyCourse;

typedef struct _GypsyCourseClass {
	GObjectClass parent_class;

	void (*course_changed) (GypsyCourse      *course,
				GypsyCourseFields fields_set,
				double            timestamp,
				double            speed,
				double            direction,
				double            climb);
} GypsyCourseClass;

GType gypsy_course_get_type (void);

GypsyCourse *gypsy_course_new (const char *object_path);

GypsyCourseFields gypsy_course_get_course (GypsyCourse *course,
					   int         *timestamp,
					   double      *speed,
					   double      *direction,
					   double      *climb,
					   GError     **error);

G_END_DECLS

#endif
