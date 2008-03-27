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

#ifndef __NMEA_PARSER_H__
#define __NMEA_PARSER_H__

#include <glib.h>

#include "gypsy-client.h"

#define GSV_FIELDS 19
#define GSA_FIELDS 17
#define GGA_FIELDS 14
#define RMC_FIELDS 12

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
	int in_use[MAX_SATELLITES]; /* The satellites that are in use */

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
