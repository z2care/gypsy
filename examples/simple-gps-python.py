#! /usr/bin/python

# This example code is in the public domain.

import sys
import gobject
import dbus, dbus.service, dbus.mainloop.glib

# Some constants we need
GYPSY_DBUS_SERVICE = "org.freedesktop.Gypsy"
GYPSY_DBUS_PATH= "/org/freedesktop/Gypsy"

GYPSY_CONTROL_DBUS_INTERFACE = "org.freedesktop.Gypsy.Server"
GYPSY_DEVICE_DBUS_INTERFACE = "org.freedesktop.Gypsy.Device"
GYPSY_POSITION_DBUS_INTERFACE = "org.freedesktop.Gypsy.Position"

GYPSY_POSITION_FIELDS_NONE = 0
GYPSY_POSITION_FIELDS_LATITUDE = 1 << 0
GYPSY_POSITION_FIELDS_LONGITUDE = 1 << 1
GYPSY_POSITION_FIELDS_ALTITUDE = 1 << 2


# Check that the Bluetooth ID of the GPS is specified on the command line
if len(sys.argv) != 2:
    print "$ simple-gps-python.py [bluetooh ID]"
    sys.exit(1)

# Hook into the glib main loop
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

# Connect to the system bus
bus = dbus.SystemBus()

# Get a connection to the Gypsy control server
control = bus.get_object(GYPSY_DBUS_SERVICE, GYPSY_DBUS_PATH)

# Create a client for the specified GPS device
path = control.Create(sys.argv[1], dbus_interface=GYPSY_CONTROL_DBUS_INTERFACE)

# Get a proxy to the client
gps = bus.get_object(GYPSY_DBUS_SERVICE, path)

# Get a proxy to the Position interface, and listen for position changed signals
position = dbus.Interface(gps, dbus_interface=GYPSY_POSITION_DBUS_INTERFACE)
def position_changed(fields_set, timestamp, latitude, longitude, altitude):
    print "%d: %2f, %2f (%1fm)" % (
        timestamp,
        (fields_set & GYPSY_POSITION_FIELDS_LATITUDE) and latitude or -1.0,
        (fields_set & GYPSY_POSITION_FIELDS_LONGITUDE) and longitude or -1.0,
        (fields_set & GYPSY_POSITION_FIELDS_ALTITUDE) and altitude or -1.0)
position.connect_to_signal("PositionChanged", position_changed)

# Get a proxy to the Device interface, and start it up
device = dbus.Interface(gps, dbus_interface=GYPSY_DEVICE_DBUS_INTERFACE)
device.Start()

# Enter the main loop, and let the signals arrive
gobject.MainLoop().run()
