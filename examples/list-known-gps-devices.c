/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@sleepfive.com>
 * Copyright (C) 2011
 *
 * This example code is in the public domain.
 */

#include <gypsy/gypsy-discovery.h>

int
main (int argc,
      char **argv)
{
    GypsyDiscovery *discovery;
    GError *error = NULL;
    char **known_devices;
    int i;

    g_type_init ();

    discovery = gypsy_discovery_new ();
    known_devices = gypsy_discovery_list_devices (discovery, &error);

    if (known_devices == NULL) {
        if (error != NULL) {
            g_warning ("Error listing devices: %s", error->message);
            return 0;
        }

        g_print ("No GPS devices found\n");
        return 0;
    }

    for (i = 0; known_devices[i]; i++) {
        g_print ("[%d] %s\n", i + 1, known_devices[i]);
    }

    return 0;
}
