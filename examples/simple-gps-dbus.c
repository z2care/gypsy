/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007
 *
 * This example code is in the public domain.
 */

/*
 * simple-gps-dbus: A simple gps example using pure D-Bus.
 */

#include <glib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus.h>

static DBusHandlerResult
parse_position_changed (DBusConnection *conn,
			DBusMessage    *message,
			void           *user_data)
{
	int timestamp, fields;
	double latitude, longitude, altitude;
	DBusError error;

	dbus_error_init (&error);
	if (!dbus_message_get_args (message, &error,
				    DBUS_TYPE_INT32, &fields,
				    DBUS_TYPE_INT32, &timestamp,
				    DBUS_TYPE_DOUBLE, &latitude,
				    DBUS_TYPE_DOUBLE, &longitude,
				    DBUS_TYPE_DOUBLE, &altitude,
				    DBUS_TYPE_INVALID)) {
		g_warning ("Could not get position: %s", error.message);

		dbus_error_free (&error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	g_print ("Latitude: %f\nLongitude: %f\nAltitude: %f\n",
		 latitude, longitude, altitude);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
parse_course_changed (DBusConnection *conn,
		      DBusMessage    *message,
		      void           *user_data)
{
	int timestamp, fields;
	double speed, direction, climb;
	DBusError error;

	dbus_error_init (&error);
	if (!dbus_message_get_args (message, &error,
				    DBUS_TYPE_INT32, &fields,
				    DBUS_TYPE_INT32, &timestamp,
				    DBUS_TYPE_DOUBLE, &speed,
				    DBUS_TYPE_DOUBLE, &direction,
				    DBUS_TYPE_DOUBLE, &climb,
				    DBUS_TYPE_INVALID)) {
		g_warning ("Could not get position: %s", error.message);

		dbus_error_free (&error);
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	g_print ("Speed: %f\nDirection: %f\nClimb: %f\n",
		 speed, direction, climb);

        return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
signal_filter (DBusConnection *conn,
               DBusMessage    *message,
               void           *user_data)
{
        if (dbus_message_is_signal (message, "org.freedesktop.gypsy.Client",
                                    "PositionChanged")) {
                return parse_position_changed (conn, message, user_data);
        } 

        if (dbus_message_is_signal (message, "org.freedesktop.gypsy.Client",
                                    "CourseChanged")) {
                return parse_course_changed (conn, message, user_data);
        } 

        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusConnection *
get_connection (void)
{
        GError *error = NULL;
        DBusGConnection *conn;

        conn = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
        if (conn == NULL) {
                g_error ("Error getting bus: %s", error->message);
        }

        return dbus_g_connection_get_connection (conn);
}

#define POSITION_CHANGED_MATCH "type='signal',interface='org.freedesktop.gypsy.Client',member='PositionChanged'"
#define COURSE_CHANGED_MATCH "type='signal',interface='org.freedesktop.gypsy.Client',member='CourseChanged'"

int
main (int    argc,
      char **argv)
{
	DBusConnection *conn;
	DBusError error;
	GMainLoop *mainloop;

	g_type_init ();
	conn = get_connection ();

	dbus_error_init (&error);
	dbus_bus_add_match (conn, POSITION_CHANGED_MATCH, &error);
	if (dbus_error_is_set (&error)) {
		g_error ("Error adding match: %s", error.message);
	}

	dbus_bus_add_match (conn, COURSE_CHANGED_MATCH, &error);
	if (dbus_error_is_set (&error)) {
		g_error ("Error adding match: %s", error.message);
	}

	if (!dbus_connection_add_filter (conn, signal_filter,
					 NULL, NULL)) {
		g_error ("Error adding filter");
	}

	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);

	return 0;
}
