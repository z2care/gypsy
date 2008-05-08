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
#include <gypsy/gypsy-satellite.h>

GypsyControl *control = NULL;

static void
satellites_changed (GypsySatellite *satellite,
		    gpointer        userdata)
{
	g_print ("Sats changed\n");
}

int 
main (int    argc,
      char **argv)
{
	GMainLoop *mainloop;
	GypsyDevice *device;
	GypsySatellite *satellite;
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
	satellite = gypsy_satellite_new (path);
	g_signal_connect (satellite, "satellites-changed",
			  G_CALLBACK (satellites_changed), NULL);

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
