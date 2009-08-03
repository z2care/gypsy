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

/**
 * SECTION:gypsy-course
 * @short_description: Object for obtaining course information
 *
 * #GypsyCourse is used whenever the client program wishes to know about
 * GPS course changes. It can report the current course, and has a signal
 * to notify listeners of changes. The course consists of the speed, direction
 * and rate of ascent or descent (called the climb).
 *
 * A #GypsyCourse object is created using gypsy_course_new() using the 
 * D-Bus path of the GPS device. This path is returned from the 
 * gypsy_control_create() function. The client can then find out about the
 * course with gypsy_course_get_course().
 *
 * As the course information changes #GypsyCourse will emit the 
 * course-changed signal. This signal contains the course information if 
 * gypsy-daemon knows it. It has a fields paramater which is a bitmask of
 * #GypsyCourseFields which indicates which of the speed, direction or
 * climb contains valid information. The timestamp will always be valid
 * if it is greater than 0.
 *
 * <informalexample>
 * <programlisting>
 * GypsyCourse *course;
 * GError *error = NULL;
 *
 * . . .
 *
 * / * path comes from the gypsy_control_create() function * /
 * course = gypsy_course_new (path);
 * g_signal_connect (course, "course-changed", G_CALLBACK (course_changed), NULL);
 * 
 * . . .
 *
 * static void 
 * course_changed (GypsyCourse *course, 
 * GypsyCourseFields fields,
 * int timestamp,
 * double speed,
 * double direction,
 * double climb,
 * gpointer userdata)
 * {
 * &nbsp;&nbsp;g_print ("speed: %fm/s\n", (fields & GYPSY_COURSE_FIELDS_SPEED) ? speed : -1.0);
 * }
 * </programlisting>
 * </informalexample>
 */

#include <glib-object.h>

#include <gypsy/gypsy-course.h>
#include <gypsy/gypsy-marshal.h>

#include "gypsy-client-bindings.h"

typedef struct _GypsyCoursePrivate {
	DBusGProxy *proxy;
	char *object_path;
} GypsyCoursePrivate;

enum {
	COURSE_CHANGED,
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_PATH
};

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GYPSY_TYPE_COURSE, GypsyCoursePrivate))

G_DEFINE_TYPE (GypsyCourse, gypsy_course, G_TYPE_OBJECT);

static void course_changed (DBusGProxy  *proxy,
			    int          fields,
			    int          timestamp,
			    double       speed,
			    double       direction,
			    double       climb,
			    GypsyCourse *course);

static guint32 signals[LAST_SIGNAL] = {0, };
static void
finalize (GObject *object)
{
	GypsyCoursePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->object_path) {
		g_free (priv->object_path);
	}

	G_OBJECT_CLASS (gypsy_course_parent_class)->finalize (object);
}

static void
dispose (GObject *object)
{
	GypsyCoursePrivate *priv;

	priv = GET_PRIVATE (object);

	if (priv->proxy) {
		dbus_g_proxy_disconnect_signal (priv->proxy, "CourseChanged",
						G_CALLBACK (course_changed),
						object);
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS (gypsy_course_parent_class)->dispose (object);
}

static void
set_property (GObject      *object,
	      guint         prop_id,
	      const GValue *value,
	      GParamSpec   *pspec)
{
	GypsyCoursePrivate *priv;

	priv = GET_PRIVATE (object);

	switch (prop_id) {
	case PROP_PATH:
		priv->object_path = g_value_dup_string (value);
		break;

	default:
		break;
	}
}

static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	GypsyCoursePrivate *priv;

	priv = GET_PRIVATE (object);
	switch (prop_id) {
	case PROP_PATH:
		g_value_set_string (value, priv->object_path);
		break;

	default:
		break;
	}
}

static void
course_changed (DBusGProxy  *proxy,
		int          fields,
		int          timestamp,
		double       speed,
		double       direction,
		double       climb,
		GypsyCourse *course)
{
	g_signal_emit (course, signals[COURSE_CHANGED], 0,
		       fields, timestamp, speed, direction, climb);
}

static GObject *
constructor (GType                  type,
	     guint                  n_construct_properties,
	     GObjectConstructParam *construct_properties)
{
	GypsyCourse *course;
	GypsyCoursePrivate *priv;
	DBusGConnection *connection;
	GError *error;

	course = GYPSY_COURSE (G_OBJECT_CLASS (gypsy_course_parent_class)->constructor 
			       (type, n_construct_properties,
				construct_properties));

	priv = GET_PRIVATE (course);

	error = NULL;
	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL) {
		g_printerr ("Failed to open connection to bus: %s\n",
			    error->message);
		g_error_free (error);
		
		priv->proxy = NULL;
		return G_OBJECT (course);
	}

	priv->proxy = dbus_g_proxy_new_for_name (connection, 
						 GYPSY_COURSE_DBUS_SERVICE,
						 priv->object_path,
						 GYPSY_COURSE_DBUS_INTERFACE);

	dbus_g_proxy_add_signal (priv->proxy, "CourseChanged",
				 G_TYPE_INT, G_TYPE_INT, G_TYPE_DOUBLE,
				 G_TYPE_DOUBLE, G_TYPE_DOUBLE, G_TYPE_INVALID);
	dbus_g_proxy_connect_signal (priv->proxy, "CourseChanged",
				     G_CALLBACK (course_changed),
				     course, NULL);
				     
	return G_OBJECT (course);
}

static void
gypsy_course_class_init (GypsyCourseClass *klass)
{
	GObjectClass *o_class = (GObjectClass *) klass;

	o_class->finalize = finalize;
	o_class->dispose = dispose;
	o_class->constructor = constructor;
	o_class->set_property = set_property;
	o_class->get_property = get_property;

	g_type_class_add_private (klass, sizeof (GypsyCoursePrivate));

	/**
	 * GypsyCourse:object-path:
	 *
	 * The path of the Gypsy GPS object
	 */
	g_object_class_install_property 
		(o_class, PROP_PATH,
		 g_param_spec_string ("object-path",
				      "Object path",
				      "The DBus path to the object", 
				      "",
				      G_PARAM_WRITABLE |
				      G_PARAM_CONSTRUCT_ONLY |
				      G_PARAM_STATIC_NICK |
				      G_PARAM_STATIC_BLURB |
				      G_PARAM_STATIC_NAME));

	/**
	 * GypsyCourse::course-changed:
	 * @fields: A bitmask of #GypsyCourseFields indicating which of the following fields are valid
	 * @timestamp: The time this change occurred
	 * @speed: The new speed
	 * @direction: The new direction
	 * @climb: The new rate of climb
	 *
	 * The ::course-changed signal is emitted when the GPS device
	 * indicates that one or more of the course fields has changed.
	 * The fields which have changed will be indicated in the @fields
	 * bitmask.
	 */
	signals[COURSE_CHANGED] = g_signal_new ("course-changed",
						G_TYPE_FROM_CLASS (klass),
						G_SIGNAL_RUN_FIRST |
						G_SIGNAL_NO_RECURSE,
						G_STRUCT_OFFSET (GypsyCourseClass, course_changed),
						NULL, NULL,
						gypsy_marshal_VOID__INT_INT_DOUBLE_DOUBLE_DOUBLE,
						G_TYPE_NONE, 5,
						G_TYPE_INT, G_TYPE_INT,
						G_TYPE_DOUBLE, G_TYPE_DOUBLE,
						G_TYPE_DOUBLE);
}

static void
gypsy_course_init (GypsyCourse *course)
{
}

/**
 * gypsy_course_new:
 * @object_path: Object path to the GPS device.
 *
 * Creates a new #GypsyCourse object that listens for course changes
 * from the GPS device found at @object_path.
 *
 * Return value: A #GypsyCourse object
 */
GypsyCourse *
gypsy_course_new (const char *object_path)
{
	return g_object_new (GYPSY_TYPE_COURSE, 
			     "object-path", object_path,
			     NULL);
}

/**
 * gypsy_course_get_course:
 * @course: A #GypsyCourse object
 * @timestamp: Pointer for the timestamp to be returned
 * @speed: Pointer for the speed to be returned
 * @direction: Pointer for the direction to be returned
 * @climb: Pointer for the climb to be returned
 * @error: A #GError for error return
 *
 * Obtains the course details from @course. @timestamp, @speed, @direction and
 * @climb can be #NULL if that detail is not required.
 * 
 * Return value: A bitmask of the fields that are set.
 */
GypsyCourseFields
gypsy_course_get_course (GypsyCourse *course,
			 int         *timestamp,
			 double      *speed,
			 double      *direction,
			 double      *climb,
			 GError     **error)
{
	GypsyCoursePrivate *priv;
	double sp, di, cl;
	int fields, ts;

	g_return_val_if_fail (GYPSY_IS_COURSE (course), GYPSY_COURSE_FIELDS_NONE);

	priv = GET_PRIVATE (course);
	if (!org_freedesktop_Gypsy_Course_get_course (priv->proxy, &fields,
						      &ts, &sp, &di, 
						      &cl, error)) {
		return GYPSY_COURSE_FIELDS_NONE;
	}

	if (timestamp != NULL) {
		*timestamp = ts;
	}

	if (speed != NULL && (fields & GYPSY_COURSE_FIELDS_SPEED)) {
		*speed = sp;
	}

	if (direction != NULL && (fields & GYPSY_COURSE_FIELDS_DIRECTION)) {
		*direction = di;
	}

	if (climb != NULL && (fields & GYPSY_COURSE_FIELDS_CLIMB)) {
		*climb = cl;
	}

	return fields;
}
		
