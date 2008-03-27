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

#ifndef __GYPSY_CLIENT_H__
#define __GYPSY_CLIENT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GYPSY_TYPE_CLIENT (gypsy_client_get_type ())

/* NMEA only allows space for 12 strictly speaking, but some GPS devices send
   all of the satellites it can see. */
#define MAX_SATELLITES 32

typedef enum {
	POSITION_NONE = 0,
	POSITION_LATITUDE = 1 << 0,
	POSITION_LONGITUDE = 1 << 1,
	POSITION_ALTITUDE = 1 << 2
} PositionFields;

typedef enum {
	COURSE_NONE = 0,
	COURSE_SPEED = 1 << 0,
	COURSE_DIRECTION = 1 << 1,
	COURSE_CLIMB = 1 << 2
} CourseFields;

typedef enum {
	FIX_INVALID = 0,
	FIX_NONE,
	FIX_2D,
	FIX_3D
} FixType;

typedef enum {
	ACCURACY_NONE = 0,
	ACCURACY_POSITION = 1 << 0, /* 3D */
	ACCURACY_HORIZONAL = 1 << 1, /* 2D */
	ACCURACY_VERTICAL = 1 << 2, /* Altitude */
} AccuracyFields;

typedef struct _GypsyClientSatellite {
	int satellite_id;
	gboolean in_use;
	int elevation;
	int azimuth;
	int snr;
} GypsyClientSatellite;

typedef struct _GypsyClient {
	GObject parent_object;
} GypsyClient;

typedef struct _GypsyClientClass {
	GObjectClass parent_class;

	void (*accuracy_changed) (GypsyClient *client,
				  int          fields_set,
				  float        pdop,
				  float        hdop,
				  float        vdop);
	void (*position_changed) (GypsyClient *client,
				  PositionFields fields_set,
				  int          timestamp,
				  float        latitude,
				  float        longitude,
				  float        altitude);
	void (*course_changed) (GypsyClient *client,
				CourseFields fields_set,
				int          timestamp,
				float        speed,
				float        direction,
				float        climb);
	void (*connection_changed) (GypsyClient *client,
				    gboolean     connected);
	void (*fix_status_changed) (GypsyClient *client,
				    FixType      fix);
	void (*time_changed) (GypsyClient *client,
			      int          timestamp);
} GypsyClientClass;

GType gypsy_client_get_type (void);

void gypsy_client_set_position (GypsyClient   *client,
				PositionFields fields_set,
				float          latitude,
				float          longitude,
				float          altitude);
void gypsy_client_set_course (GypsyClient *client,
			      CourseFields fields_set,
			      float        speed,
			      float        direction,
			      float        climb);
void gypsy_client_set_timestamp (GypsyClient *client,
				 int          utc_time);
void gypsy_client_set_fix_type (GypsyClient *client,
				FixType      type,
				gboolean     weak);

void gypsy_client_add_satellite (GypsyClient *client,
				 int          number,
				 gboolean     in_use,
				 int          elevation,
				 int          azimuth,
				 int          snr);
void gypsy_client_clear_satellites (GypsyClient *client);
void gypsy_client_set_satellites (GypsyClient *client);

void gypsy_client_set_accuracy (GypsyClient *client,
				AccuracyFields fields_set,
				double pdop,
				double hdop,
				double vdop);

G_END_DECLS

#endif
