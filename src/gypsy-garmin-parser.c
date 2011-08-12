/*
 * Gypsy
 *
 * A simple to use and understand GPSD replacement
 * that uses D-Bus, GLib and memory allocations
 *
 * Author: Iain Holmes <iain@sleepfive.com>
 * Copyright (C) 2011
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

/* Includes code from Garmin Protocol to NMEA 0183 converter,
   license as below */
/*
  Garmin protocol to NMEA 0183 converter
  Copyright (C) 2004 Manuel Kasper <mk@neon1.net>.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
*/
#include <math.h>
#include <string.h>

#include <glib.h>

#include "gypsy-garmin-parser.h"
#include "garmin.h"

#define KNOTS_TO_KMH 1.852
#define rad2deg(x) ((x) * 180.0 / G_PI)
#define READ_BUFFER_SIZE 1024

struct _GypsyGarminParserPrivate {
    guchar sentence[READ_BUFFER_SIZE];
    gsize bytes_in_buffer;

    GDate *epoch;
    GDate *date;

    double lastcourse;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GYPSY_TYPE_GARMIN_PARSER, GypsyGarminParserPrivate))
G_DEFINE_TYPE (GypsyGarminParser, gypsy_garmin_parser, GYPSY_TYPE_PARSER);

static void
gypsy_garmin_parser_finalize (GObject *object)
{
    GypsyGarminParser *self = (GypsyGarminParser *) object;
    GypsyGarminParserPrivate *priv = self->priv;

    g_date_free (priv->epoch);
    g_date_free (priv->date);

    G_OBJECT_CLASS (gypsy_garmin_parser_parent_class)->finalize (object);
}

static void
gypsy_garmin_parser_dispose (GObject *object)
{
#if 0
    GypsyGarminParser *self = (GypsyGarminParser *) object;
#endif
    G_OBJECT_CLASS (gypsy_garmin_parser_parent_class)->dispose (object);
}

#define SECONDS_PER_WEEK 604800
#define SECONDS_PER_DAY 86400
#define DAYS_PER_WEEK 7

static int
calculate_utc (GypsyGarminParser  *parser,
               D800_Pvt_Data_Type *pvt)
{
    GypsyGarminParserPrivate *priv = parser->priv;
    int tmp = 0, dtmp = 0;
    unsigned long w, x, a, b, c, d, e, f;
    unsigned long day, month, year;
    unsigned long jd;
    int datestamp, days_since_epoch;
    int timestamp;

    /*
      UTC time of position fix
      Reminder:
      pvt->tow = seconds (including fractions) since the start of the week.
      pvt->wn_days = days since 31-DEC-1989 for the start of the current week
      (neither is adjusted for leap seconds)
      pvt->leap_scnds = leap second adjustment required.
    */

    /*
      The receivers can (and do) return times like 86299.999999 instead
      of 86300.0   Rounding is required to get the correct time.
      Just capture wn_days for now.
    */

    tmp = rint (pvt->tow);
    dtmp = pvt->wn_days;

    /*
      If the result is 604800, it's really the first sample
      of the new week, so zero out tmp and increment dtmp
      by a week ( 7 days ).
    */

    if (tmp >= SECONDS_PER_WEEK) {
        dtmp += DAYS_PER_WEEK;
        tmp = 0;
    }

    /*
      At this point we have tmp = seconds since the start
      of the week, and dtmp = the first day of the week.
      We now need to correct for leap seconds.  This may actually
      result in reversing the previous adjustment but the code
      required to combine the two operations wouldn't be clear.
    */
    tmp -= pvt->leap_scnds;
    if (tmp < 0) {
        tmp += SECONDS_PER_WEEK;
        dtmp -= DAYS_PER_WEEK;
    }

    /*
      Now we have tmp = seconds since the start if the week,
      and dtmp = the first day of the week, all corrected for
      rounding and leap seconds.

      We now convert dtmp to today's day number
    */
    dtmp += (tmp / SECONDS_PER_DAY);

    /* Garmin format: number of days since December 31, 1989 */
    jd = dtmp + 2447892;

    w = (unsigned long)((jd - 1867216.25)/36524.25);
    x = w/4;
    a = jd + 1 + w - x;
    b = a + 1524;
    c = (unsigned long)((b - 122.1)/365.25);
    d = (unsigned long)(365.25 * c);
    e = (unsigned long)((b-d)/30.6001);
    f = (unsigned long)(30.6001 * e);

    day = b - d - f;
    month = e - 1;
    if (month > 12)
        month -= 12;
    year = c - 4716;
    if (month == 1 || month == 2)
        year++;

    g_date_set_dmy (priv->date, day, month, year);
    days_since_epoch = g_date_days_between (priv->epoch, priv->date);
    datestamp = days_since_epoch * SECONDS_PER_DAY;

    /* Convert tmp to seconds since midnight */
    tmp %= SECONDS_PER_DAY;
    timestamp = datestamp + tmp;

    return timestamp;
}

/* NB: Speed and course over ground are calculated from
   the north/east velocity and may not be accurate */
static void
calculate_speed_course (GypsyGarminParser  *parser,
                        D800_Pvt_Data_Type *pvt,
                        double             *speed,
                        double             *course)
{
    GypsyGarminParserPrivate *priv = parser->priv;

    *speed = sqrt (pvt->east * pvt->east + pvt->north * pvt->north) * 3.6 / KNOTS_TO_KMH;

    if (*speed < 1.0) {
        if (priv->lastcourse >= 0) {
            *course = priv->lastcourse;
        } else {
            *course = 0; /* Too slow to determine course */
        }
    } else {
        *course = atan2 (pvt->east, pvt->north);
        if (*course < 0) {
            *course += 2 * G_PI;
        }
        *course = rad2deg (*course);
        priv->lastcourse = *course;
    }
}

static gboolean
gypsy_garmin_parser_received_data (GypsyParser   *parser,
                                   const guchar  *data,
                                   gsize          length,
                                   GError       **error)
{
    GypsyGarminParser *garmin = GYPSY_GARMIN_PARSER (parser);
    GypsyGarminParserPrivate *priv = garmin->priv;
    GypsyClient *client;
    G_Packet_t *pGpkt;
    int pktlen;

    client = gypsy_parser_get_client (parser);

    memcpy (priv->sentence + priv->bytes_in_buffer, data, length);
    priv->bytes_in_buffer += length;

    pGpkt = (G_Packet_t*) priv->sentence;
    pktlen = GARMIN_HEADER_SIZE + pGpkt->mDataSize;
    while ((priv->bytes_in_buffer >= GARMIN_HEADER_SIZE) &&
           (priv->bytes_in_buffer >= pktlen)) {
        double speed, course;

        /*g_debug("PacketId: %d   pktlen = %d",
          pGpkt->mPacketId, pktlen);*/

        if (pGpkt->mPacketId == Pid_Pvt_Data) {
            D800_Pvt_Data_Type *pvt;
            int fixtype;

            pvt = (D800_Pvt_Data_Type *) pGpkt->mData;

            gypsy_client_set_timestamp (client, calculate_utc (garmin, pvt));

            switch (pvt->fix) {
            case 0:
            case 1:
                fixtype = FIX_NONE;
                break;

            case 2:
            case 4:
                fixtype = FIX_2D;
                break;

            case 3:
            case 5:
                fixtype = FIX_3D;
                break;

            default:
                fixtype = FIX_INVALID;
                break;
            }
            gypsy_client_set_fix_type (client, fixtype, FALSE);
            gypsy_client_set_position (client,
                                       POSITION_LATITUDE |
                                       POSITION_LONGITUDE |
                                       POSITION_ALTITUDE,
                                       pvt->lat, pvt->lon, pvt->alt);

            calculate_speed_course (garmin, pvt, &speed, &course);
            gypsy_client_set_course (client,
                                     COURSE_SPEED |
                                     COURSE_DIRECTION,
                                     speed, course, 0.0);
        } else if (pGpkt->mPacketId == Pid_SatData_Record) {
            int i;
            cpo_sat_data *sat;

            sat = (cpo_sat_data *)pGpkt->mData;

            gypsy_client_clear_satellites (client);
            for (i = 0; i < SAT_MAX_COUNT; i++) {
                if (((sat[i].status & SAT_STATUS_MASK) == SAT_STATUS_GOOD) &&
                    (sat[i].svid <= MAX_SAT_SVID)) {
                /* FIXME: I think this is only passing in_use satellites to
                   Gypsy, do we want to pass SAT_STATUS_BAD satellites as well
                   with in_use = FALSE? */
                gypsy_client_add_satellite (client, sat[i].svid, TRUE,
                                            sat[i].elev, sat[i].azmth,
                                            sat[i].snr);
                }
            }

            gypsy_client_set_satellites (client);
        } else {
            g_debug ("Untranslated PacketId = %d", pGpkt->mPacketId);
        }

        /* now that we're done with this packet,
           move any remaining data up to the
           beginning of the buffer */
        memmove (priv->sentence, priv->sentence + pktlen,
                 priv->bytes_in_buffer - pktlen);
        priv->bytes_in_buffer -= pktlen;
    }

    return TRUE;
}

static gsize
gypsy_garmin_parser_get_space_in_buffer (GypsyParser *parser)
{
    GypsyGarminParser *garmin = GYPSY_GARMIN_PARSER (parser);
    GypsyGarminParserPrivate *priv = garmin->priv;

    return READ_BUFFER_SIZE - priv->bytes_in_buffer;
}

static void
gypsy_garmin_parser_class_init (GypsyGarminParserClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    GypsyParserClass *p_class = (GypsyParserClass *) klass;

    o_class->dispose = gypsy_garmin_parser_dispose;
    o_class->finalize = gypsy_garmin_parser_finalize;

    p_class->received_data = gypsy_garmin_parser_received_data;
    p_class->get_space_in_buffer = gypsy_garmin_parser_get_space_in_buffer;

    g_type_class_add_private (klass, sizeof (GypsyGarminParserPrivate));
}

static void
gypsy_garmin_parser_init (GypsyGarminParser *self)
{
    GypsyGarminParserPrivate *priv = GET_PRIVATE (self);

    self->priv = priv;

    priv->lastcourse = -1;

    priv->epoch = g_date_new_dmy (1, 1, 1970);
    priv->date = g_date_new ();
}

GypsyParser *
gypsy_garmin_parser_new (GypsyClient *client)
{
    g_return_val_if_fail (GYPSY_IS_CLIENT (client), NULL);

    return (GypsyParser *) g_object_new (GYPSY_TYPE_GARMIN_PARSER,
                                         "client", client,
                                         NULL);
}
