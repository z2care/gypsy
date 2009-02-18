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

#ifndef __NMEA_PARSER_H__
#define __NMEA_PARSER_H__

#include <glib.h>

#include "nmea.h"
#include "gypsy-client.h"

typedef struct _NMEAParseContext {
	GypsyClient *client;

	union {
		char *rmc_fields[RMC_FIELDS];
		char *gga_fields[GGA_FIELDS];
		char *gsa_fields[GSA_FIELDS];
		char *gsv_fields[GSV_FIELDS];
	} fields;

	/* This is stored as only RMC supplies it but other sentences
	   can supply the UTC time. We convert UTC time into seconds
	   and add it to this to get the timestamp */
	GDate *date; /* The date from the most recent RMC */
	guint32 datestamp; /* Time from epoch in seconds */

	/* This is used to store the in use details between sentences */
	int in_use_count;
	int in_use[MAX_SAT_SVID]; /* The satellites that are in use */

	/* This is used to store the satellite details between sentences */
	int number_of_messages; /* How many GSV messages we'll get */
	int message_count; /* Number of GSV messages seen */
} NMEAParseContext;

gboolean nmea_parse_sentence (NMEAParseContext *ctxt,
			      char             *sentence,
			      GError          **error);

NMEAParseContext *nmea_parse_context_new (GypsyClient *client);
void nmea_parse_context_free (NMEAParseContext *ctxt);

#endif
