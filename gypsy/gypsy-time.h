/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@gnome.org>
 * Copyright (C) 2007
 */

#ifndef __GYPSY_TIME_H__
#define __GYPSY_TIME_H__

#include <glib-object.h>

G_BEGIN_DECLS 

/**
 * GYPSY_TIME_DBUS_SERVICE:
 *
 * A define containing the address of the Time service
 */
#define GYPSY_TIME_DBUS_SERVICE "org.freedesktop.Gypsy"

/**
 * GYPSY_TIME_DBUS_INTERFACE:
 * 
 * A define containing the name of the Time interface
 */
#define GYPSY_TIME_DBUS_INTERFACE "org.freedesktop.Gypsy.Time"

#define GYPSY_TYPE_TIME (gypsy_time_get_type ())
#define GYPSY_TIME(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), GYPSY_TYPE_TIME, GypsyTime))
#define GYPSY_IS_TIME(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GYPSY_TYPE_TIME))

/**
 * GypsyTime:
 *
 * There are no public fields in #GypsyTime.
 */
typedef struct _GypsyTime {
	GObject parent_object;
} GypsyTime;

typedef struct _GypsyTimeClass {
	GObjectClass parent_class;

	void (*time_changed) (GypsyTime *gps_time,
			      int        timestamp);
} GypsyTimeClass;

GType gypsy_time_get_type (void);

GypsyTime *gypsy_time_new (const char *object_path);

gboolean gypsy_time_get_time (GypsyTime *gps_time,
			      int       *timestamp,
			      GError   **error);

G_END_DECLS

#endif
