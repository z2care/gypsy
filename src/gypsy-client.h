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
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GYPSY_CLIENT_H__
#define __GYPSY_CLIENT_H__

#include <glib-object.h>
#include "nmea.h"

G_BEGIN_DECLS

#define GYPSY_TYPE_CLIENT (gypsy_client_get_type ())

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
