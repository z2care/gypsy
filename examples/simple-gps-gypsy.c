/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007 Iain Holmes
 * Copyright (C) 2007 Openedhand Ltd
 *
 * This example code is in the public domain.
 */

/*
 * simple-gps-gypsy: A simple gps example using the libgypsy library.
 */

#include <gypsy/gypsy-control.h>
#include <gypsy/gypsy-device.h>
#include <gypsy/gypsy-position.h>
#include <gypsy/gypsy-course.h>

GypsyControl *control = NULL;

static void
position_changed (GypsyPosition      *position,
		  GypsyPositionFields fields_set,
		  int                 timestamp,
		  double              latitude,
		  double              longitude,
		  double              altitude,
		  gpointer            userdata)
{
	g_print ("%d: %2f, %2f (%1fm)\n", timestamp,
		 (fields_set & GYPSY_POSITION_FIELDS_LATITUDE) ? latitude :-1.0,
		 (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE) ? longitude :-1.0,
		 (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE) ? altitude : -1.0);
}

static void
course_changed (GypsyCourse    *course,
		GypsyCourseFields fields_set,
		int               timestamp,
		double            speed,
		double            direction,
		double            climb,
		gpointer          userdata)
{
	g_print ("%d: %2f, %2f, %2fm/s\n", timestamp,
		 (fields_set & GYPSY_COURSE_FIELDS_SPEED) ? speed : -1.0,
		 (fields_set & GYPSY_COURSE_FIELDS_DIRECTION) ? direction : -1.0,
		 (fields_set & GYPSY_COURSE_FIELDS_CLIMB) ? climb : -1.0);
}

int 
main (int    argc,
      char **argv)
{
	GMainLoop *mainloop;
	GypsyDevice *device;
	GypsyPosition *position;
	GypsyCourse *course;
	GError *error = NULL;
	char *path;

	if (argc < 2) {
		g_print ("Usage: %s device\n", argv[0]);
		return 0;
	}

	g_type_init ();

	control = gypsy_control_get_default ();
	path = gypsy_control_create (control, argv[1], &error);
	if (path == NULL) {
		g_warning ("Error creating client for %s: %s", argv[1],
			   error->message);
		g_error_free (error);
		return 0;
	}

	device = gypsy_device_new (path);
	position = gypsy_position_new (path);
	g_signal_connect (position, "position-changed",
			  G_CALLBACK (position_changed), NULL);
	course = gypsy_course_new (path);
	g_signal_connect (course, "course-changed",
			  G_CALLBACK (course_changed), NULL);

	gypsy_device_start (device, &error);
	if (error != NULL) {
		g_warning ("Error starting %s: %s", argv[1],
			   error->message);
		g_error_free (error);
		return 0;
	}

	mainloop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (mainloop);

	/* FIXME: Need some way to quit the mainloop */
	g_object_unref (control);

	return 0;
}
