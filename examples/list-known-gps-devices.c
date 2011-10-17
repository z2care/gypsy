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
    GPtrArray *known_devices;
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

    for (i = 0; i < known_devices->len; i++) {
        GypsyDiscoveryDeviceInfo *di = known_devices->pdata[i];
        g_print ("[%d] %s (%s)\n", i + 1, di->device_path, di->type);
    }

    g_ptr_array_free (known_devices, TRUE);
    return 0;
}
