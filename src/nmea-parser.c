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

/*
 * NMEA Parser - a set of routines to parse NMEA sentences into structures
 *               that Gypsy uses.
 */

#include <string.h>
#include <stdlib.h>

#include "gypsy-client.h"
#include "nmea-parser.h"

#define IS_EMPTY(x) ((x) == NULL || *(x) == '\0')
typedef gboolean (* TagParseFunc) (NMEAParseContext *ctxt,
				   const char       *data);
struct _tag_parser {
	char *tag_name;
	TagParseFunc parser;
};

/* Splits a NMEA data sentence into fields by replacing the delimiter (,)
   with \0 and putting the start of each field into the fields array */
#define DELIMITER ','
static int
split_sentence (const char   *sentence,
		char         *fields[],
		int           num_fields)
{
	char *begin, *end;
	int field_count = 0;

	begin = end = (char *) sentence;
	while (*begin && field_count < num_fields) {
		/* Move over data looking for , */
		while (*end && *end != DELIMITER) {
			end++;
		}

		fields[field_count] = begin;
		field_count++;

		/* We're at the end of the sentence */
		if (*end == '\0') {
			break;
		}

		*end = '\0';
		end++;

		begin = end;

		if (*begin == '\0') {
			/* This is the last field but it is blank */
			fields[field_count] = begin;
			field_count++;
			break;
		}				
	}

	return field_count;
}

static float
calculate_latitude (const char *value,
		    const char *direction,
		    PositionFields *fields)
{
	float dd = 0.0, minutes = 0.0;
	int digit, degrees = 0;

	/* First 2 digits are degrees */
	digit = g_ascii_digit_value (value[0]);
	if (digit == -1) {
		return 0.0;
	}
	degrees = digit * 10;

	digit = g_ascii_digit_value (value[1]);
	if (digit == -1) {
		return 0.0;
	}
	degrees += digit;

	/* The rest of the string XX.YYYY is the decimal minutes */
	minutes = strtod (&(value[2]), NULL);

	dd = (float) degrees + (minutes / 60.0);
	
	if (direction && *direction == 'S') {
		dd *= -1;
	}

	*fields |= POSITION_LATITUDE;
	return dd;
}

static float
calculate_longitude (const char *value,
		     const char *direction,
		     PositionFields *fields)
{
	float dd = 0.0, minutes = 0.0;
	int digit, degrees = 0;

	/* First 3 digits are degrees */
	digit = g_ascii_digit_value (value[0]);
	if (digit == -1) {
		return 0.0;
	}
	degrees = digit * 100;

	digit = g_ascii_digit_value (value[1]);
	if (digit == -1) {
		return 0.0;
	}
	degrees += (digit * 10);

	digit = g_ascii_digit_value (value[2]);
	if (digit == -1) {
		return 0.0;
	}
	degrees += digit;

	/* The rest of the string XX.YYYY is the decimal minutes */
	minutes = strtod (&(value[3]), NULL);

	dd = (float) degrees + (minutes / 60.0);
	
	if (direction && *direction == 'W') {
		dd *= -1;
	}

	*fields |= POSITION_LONGITUDE;
	return dd;
}

static float
calculate_altitude (const char *value,
		    PositionFields *fields)
{
	float alt;
	char *endptr;

	alt = strtod (value, &endptr);
	if (endptr == value) {
		return 0.0;
	}

	*fields |= POSITION_ALTITUDE;
	return alt;
}

static float
calculate_speed (const char *value,
		 CourseFields *fields)
{
	float speed;
	char *endptr;

	speed = strtod (value, &endptr);
	if (endptr == value) {
		return 0.0;
	}

	*fields |= COURSE_SPEED;
	return speed;
}

static float
calculate_direction (const char *value,
		     CourseFields *fields)
{
	float direction;
	char *endptr;

	direction = strtod (value, &endptr);
	if (endptr == value) {
		return 0.0;
	}

	*fields |= COURSE_DIRECTION;
	return direction;
}

#define SECS_IN_HOURS (60 * 60)
#define SECS_IN_MINS (60)

static int
calculate_timestamp (NMEAParseContext *ctxt,
		     const char       *utc_time)
{
	int hours, minutes, seconds;

	if (ctxt->datestamp == 0) {
#ifdef SHOTGUN_DEBUGGING
		g_debug ("Requested timestamp before RMC was seen");
#endif
		return 0;
	}

	hours = g_ascii_digit_value (utc_time[0]) * 10 + 
		g_ascii_digit_value (utc_time[1]);
	minutes = g_ascii_digit_value (utc_time[2]) * 10 +
		g_ascii_digit_value (utc_time[3]);
	seconds = g_ascii_digit_value (utc_time[4]) * 10 +
		g_ascii_digit_value (utc_time[5]);

	return ctxt->datestamp + (hours * SECS_IN_HOURS) + (minutes * SECS_IN_MINS) + seconds;
}

#define BASE_CENTURY 2000
#define SECONDS_PER_DAY (60 * 60 * 24)
static int
calculate_datestamp (NMEAParseContext *ctxt,
		     const char       *date_str)
{
	int year, month, day;
	static GDate *epoch = NULL;
	int days_since_epoch;

	if (epoch == NULL) {
		epoch = g_date_new_dmy (1, 1, 1970);
	}

	day = g_ascii_digit_value (date_str[0]) * 10 +
		g_ascii_digit_value (date_str[1]);
	month = g_ascii_digit_value (date_str[2]) * 10 +
		g_ascii_digit_value (date_str[3]);
	year = BASE_CENTURY + (g_ascii_digit_value (date_str[4]) * 10 + 
			       g_ascii_digit_value (date_str[5]));

	g_date_set_dmy (ctxt->date, day, month, year);

	days_since_epoch = g_date_days_between (epoch, ctxt->date);

	return days_since_epoch * SECONDS_PER_DAY;
}

/* Sentence parsers */

/* There are 19 fields in the GSV sentence:
   0) Number of messages
   1) Message number
   2) Satellites in view
   3) Satellite PRN number
   4) Elevation in degrees (0 - 90)
   5) Azimuth in degrees to true north (0 - 359)
   6) SNR dB (0 - 99)
   7 - 10) Same as 3 - 6 for second satellite
   11 - 14) Same as 3 - 6 for third satellite
   15 - 18) Same as 3 - 6 for fourth satellite
*/
#define GSV_FIELD(x) (ctxt->fields.gsv_fields[x])
#define GSV_FIRST_SAT 3
#define GSV_LAST_SAT 15
static gboolean
parse_gsv (NMEAParseContext *ctxt,
	   const char       *data)
{
	int field_count, message_number, i;

	field_count = split_sentence (data, ctxt->fields.gsv_fields, 
				      GSV_FIELDS);
	
#ifdef SHOTGUN_DEBUGGING
	{
		int i;

		g_debug ("GSV: Got %d fields, wanted %d", 
			 field_count, GSV_FIELDS);
		for (i = 0; i < field_count; i++) {
			g_debug ("[%d] - %s", i, GSV_FIELD (i));
		}
	}
#endif

	if (field_count < GSV_FIELDS)
		return FALSE;

	message_number = atoi (GSV_FIELD (1));

	if (message_number != ctxt->message_count + 1) {
		g_debug ("Missed message %d - got %d", ctxt->message_count + 1,
			 message_number);
		/* We've missed a message, so clear the satellites */
		gypsy_client_clear_satellites (ctxt->client);

		/* If the message received was #1 then we can continue
		   otherwise we need to skip until we find #1 */
		ctxt->message_count = 0;
		ctxt->number_of_messages = 0;
		if (message_number != 1) {
			return FALSE;
		}
	}

	if (message_number == 1) {
		ctxt->number_of_messages = atoi (GSV_FIELD (0));
	}

	for (i = GSV_FIRST_SAT; i <= GSV_LAST_SAT && i < field_count; i += 4) {
		int id, elevation, azimuth, snr;
		int j;
		gboolean in_use = FALSE;

		/* If the ID field is empty, then we've finished the
		   satellites in this sentence */
		if (IS_EMPTY (GSV_FIELD (i))) {
			break;
		}

		id = atoi (GSV_FIELD (i));
		elevation = IS_EMPTY (GSV_FIELD (i + 1)) ? 0 : 
			atoi (GSV_FIELD (i + 1));
		azimuth = IS_EMPTY (GSV_FIELD (i + 2)) ? 0 :
			atoi (GSV_FIELD (i + 2));
		snr = IS_EMPTY (GSV_FIELD (i + 3)) ? 0 :
			atoi (GSV_FIELD (i + 3));

		/* Check all the in use satellite ids */
		for (j = 0; j < ctxt->in_use_count; j++) {
			if (id == ctxt->in_use[j]) {
				in_use = TRUE;
				break;
			}
		}

		gypsy_client_add_satellite (ctxt->client, id, in_use,
					    elevation, azimuth, snr);
	}

	ctxt->message_count++;
	if (ctxt->message_count == ctxt->number_of_messages) {
		gypsy_client_set_satellites (ctxt->client);
		ctxt->message_count = 0;
	}
	
	return TRUE;
}

/* There are 17 fields in the GSA sentence:
   0) Auto selection of 2D or 3D fix (M = manual, A = automatic)
   1) 3D fix - values include : 1 = no fix, 2 = 2D, 3 = 3D
   2 - 13) PRNs of satellites used for fix (space for 12)
   14) PDOP (dilution of precision)
   15) HDOP (horizontal DOP)
   16) VDOP (vertical DOP)
*/
#define GSA_FIELD(x) (ctxt->fields.gsa_fields[x])
#define GSA_FIRST_SAT 2
#define GSA_LAST_SAT 13
static gboolean
parse_gsa (NMEAParseContext *ctxt,
	   const char       *data)
{
	int field_count;
	int sat_count, i;

	field_count = split_sentence (data, ctxt->fields.gsa_fields, 
				      GSA_FIELDS);

#ifdef SHOTGUN_DEBUGGING
	{
		int i;

		g_debug ("GSA: Got %d fields, wanted %d", 
			 field_count, GSA_FIELDS);
		for (i = 0; i < GSA_FIELDS; i++) {
			g_debug ("[%d] - %s", i, GSA_FIELD (i));
		}
	}
#endif

	if (field_count < GSA_FIELDS)
		return FALSE;

	/* We actually have a real fix type now */
	gypsy_client_set_fix_type (ctxt->client, atoi (GSA_FIELD(1)), FALSE);

	sat_count = 0;
	for (i = GSA_FIRST_SAT; i <= GSA_LAST_SAT; i++) {
		char *sat = GSA_FIELD(i);

		if (sat == NULL || *sat == '\0') {
			break;
		}

		ctxt->in_use[sat_count] = atoi (GSA_FIELD(i));
		sat_count++;
	}
	ctxt->in_use_count = sat_count;

	gypsy_client_set_accuracy
		(ctxt->client,
		 *GSA_FIELD(14) ? ACCURACY_POSITION : 0 |
		 *GSA_FIELD(15) ? ACCURACY_HORIZONTAL : 0 |
		 *GSA_FIELD(16) ? ACCURACY_VERTICAL : 0,
		 g_strtod (GSA_FIELD(14), NULL),
		 g_strtod (GSA_FIELD(15), NULL),
		 g_strtod (GSA_FIELD(16), NULL));

	return TRUE;
}

/* There are 14 fields in the GGA sentence:
   0) UTC time
   1) Latitude
   2) N or S
   3) Longitude
   4) E or W
   5) Fix quality
   6) Number of satellites being tracked
   7) Horizontal dilution of position
   8) Altitude, metres above mean sea level
   9) Alt unit (metres)
   10) Height of geoid (mean sea level) above WGS84 ellipsoid
   11) unit
   12) (empty field) time in seconds since last DGPS update
   13) (empty field) DGPS station ID number
*/
#define GGA_FIELD(x) (ctxt->fields.gga_fields[x])
static gboolean
parse_gga (NMEAParseContext *ctxt,
	   const char       *data)
{
	int field_count;
	float latitude, longitude, altitude;
	PositionFields fields;
	FixType fix_type;
	int timestamp;

	field_count = split_sentence (data, ctxt->fields.gga_fields, 
				      GGA_FIELDS);

#ifdef SHOTGUN_DEBUGGING
	{
		int i;

		g_debug ("GGA: Got %d fields, wanted %d",
			 field_count, GGA_FIELDS);
		for (i = 0; i < GGA_FIELDS; i++) {
			g_debug ("[%d] - %s", i, GGA_FIELD(i));
		}
	}
#endif

	if (field_count < GGA_FIELDS)
		return FALSE;

	timestamp = calculate_timestamp (ctxt, GGA_FIELD(0));
	if (timestamp > 0) {
		gypsy_client_set_timestamp (ctxt->client, timestamp);
	}

	fields = POSITION_NONE;
	latitude = calculate_latitude (GGA_FIELD(1), GGA_FIELD(2), &fields);
	longitude = calculate_longitude (GGA_FIELD(3), GGA_FIELD(4), &fields);
	altitude = calculate_altitude (GGA_FIELD(8), &fields);

	gypsy_client_set_position (ctxt->client, fields, latitude, 
				   longitude, altitude);

	/* We can fake the fix type here by checking what fields are set */
	fix_type = FIX_INVALID;
	if ((fields & POSITION_LATITUDE) && (fields & POSITION_LONGITUDE)) {
		if (fields & POSITION_ALTITUDE) {
			fix_type = FIX_3D;
		} else {
			fix_type = FIX_2D;
		}
	} else {
		fix_type = FIX_NONE;
	}
	gypsy_client_set_fix_type (ctxt->client, fix_type, FALSE);

	gypsy_client_set_accuracy (ctxt->client, ACCURACY_HORIZONTAL,
				   0, g_strtod (GGA_FIELD(7), NULL), 0);
	
	return TRUE;
}

/* There are 12 fields in the RMC sentence:
   0) UTC time
   1) Status (V = No fix, A = Fix)
   2) Latitude
   3) N or S
   4) Longitude
   5) E or W
   6) Speed over the ground in knots
   7) Track made good, degrees true
   8) Date in ddmmyy
   9) Magnetic Variation, degrees
   10) E or W
   11) FAA mode indicator (NMEA 2.3 and later)
*/
#define RMC_FIELD(x) (ctxt->fields.rmc_fields[x])
static gboolean
parse_rmc (NMEAParseContext *ctxt,
	   const char       *data)
{
	int field_count;
	PositionFields position_fields;
	CourseFields course_fields;
	float latitude, longitude;
	float speed, direction;

	field_count = split_sentence (data, ctxt->fields.rmc_fields, 
				      RMC_FIELDS);

#ifdef SHOTGUN_DEBUGGING
	{
		int i;

		g_debug ("RMC: Got %d fields, wanted %d", 
			 field_count, RMC_FIELDS);
		for (i = 0; i < RMC_FIELDS; i++) {
			g_debug ("[%d] - %s", i, RMC_FIELD(i));
		}
	}
#endif

	if (field_count < RMC_FIELDS)
		return FALSE;

	/* We can store the datestamp now */
	ctxt->datestamp = calculate_datestamp (ctxt, RMC_FIELD(8));

	/* Calculate the timestamp first */
	gypsy_client_set_timestamp (ctxt->client, 
				    calculate_timestamp (ctxt, RMC_FIELD(0)));

	/* RMC gives us Latitude and Longitude so we check them as well */
	position_fields = POSITION_NONE;
	latitude = calculate_latitude (RMC_FIELD(2), RMC_FIELD(3), 
				       &position_fields);
	longitude = calculate_longitude (RMC_FIELD(4), RMC_FIELD(5),
					 &position_fields);

	gypsy_client_set_position (ctxt->client, position_fields, latitude, 
				   longitude, 0.0);

	if (*RMC_FIELD(1) == 'A') {
		gypsy_client_set_fix_type (ctxt->client, FIX_2D, TRUE);
	} else {
		gypsy_client_set_fix_type (ctxt->client, FIX_NONE, FALSE);
	}

	course_fields = COURSE_NONE;
	speed = calculate_speed (RMC_FIELD(6), &course_fields);
	direction = calculate_direction (RMC_FIELD(7), &course_fields);

	gypsy_client_set_course (ctxt->client, course_fields, speed, 
				 direction, 0.0);
	
	return TRUE;
}

static struct _tag_parser parsers[] = {
	/* Standard NMEA tags */
	{ "GPRMC", parse_rmc },
	{ "GPGGA", parse_gga },
	{ "GPGSA", parse_gsa },
	{ "GPGSV", parse_gsv },

	/* Proprietry tags */
	{ NULL, NULL }
};

static gboolean
parse_tag (NMEAParseContext *ctxt,
	   const char       *tag,
	   const char       *data)
{
	int i;

	for (i = 0; parsers[i].tag_name; i++) {
		if (strcmp (parsers[i].tag_name, tag) == 0) {
			return parsers[i].parser (ctxt, data);
		}
	}

	/* If it got here it was an unknown tag.
	   This doesn't equal an invalid sentence so just return TRUE */
	return TRUE;
}

/* NMEA sentences are of the form $<data>*<checksum>
   The checksum is the XOR of all the characters in <data>

   check_checksum takes the sentence at the first character after the
   beginning $ and checks the checksum. If there is no checksum
   value found, it returns TRUE 

   NB: Modifies the original sentence by replacing * with \0 */
static gboolean
check_checksum (char *sentence)
{
	char *s = sentence;
	int sum = 0;

/* 	g_debug ("Checking: %s", sentence); */
	while (*s && *s != '*') {
		sum ^= *s;
		s++;
	}

	if (*s == '\0') {
		/* No checksum: return fail */
		g_debug ("Sentence has no checksum: %s", sentence);
		return FALSE;
	}

	/* \0 out the * so we can split the sentence later on */
	*s = '\0';
	s++; /* Move s onto the checksum */

	if (sum == strtol (s, NULL, 16)) {
		return TRUE;
	} else {
		return FALSE;
	}
}
	
/* Sigh: Standard NMEA tags are 5 chars, but weird proprietry devices
   can emit longer */
#define TAG_LENGTH 12
gboolean
nmea_parse_sentence (NMEAParseContext *ctxt,
		     char             *sentence,
		     GError          **error)
{
	char tag[TAG_LENGTH + 1];
	const char *data;
	char *comma;
	int tag_length;

	/* Validate sentence */
	if (*sentence != '$') {
		return FALSE;
	}

	if (check_checksum (sentence + 1) == FALSE) {
		return FALSE;
	}

	/* Find the first comma */
	comma = strchr (sentence + 1, ',');
	if (comma == NULL) {
		return FALSE;
	}

	/* We allow tags to be any length up to TAG_LENGTH */
	tag_length = MIN (TAG_LENGTH, comma - (sentence + 1));
	strncpy (tag, sentence + 1, tag_length);
	tag[tag_length] = '\0';

	/* Skip the "$<TAG>," at the start to get to the data */
	data = sentence + tag_length + 2;

#ifdef SHOTGUN_DEBUGGING
 	g_debug ("<%s> - %s", tag, data);
#endif

	if (parse_tag (ctxt, tag, data) == FALSE) {
		return FALSE;
	}

	return TRUE;
}

NMEAParseContext *
nmea_parse_context_new (GypsyClient *client)
{
	NMEAParseContext *ctxt;

	ctxt = g_new0 (NMEAParseContext, 1);
	ctxt->client = client;
	ctxt->date = g_date_new ();

	return ctxt;
}

void
nmea_parse_context_free (NMEAParseContext *ctxt)
{
	g_date_free (ctxt->date);
	g_free (ctxt);
}
