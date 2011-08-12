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

#include <string.h>
#include <glib.h>

#include "nmea-parser.h"
#include "gypsy-nmea-parser.h"

#define READ_BUFFER_SIZE 1024

struct _GypsyNmeaParserPrivate {
    NMEAParseContext *ctxt;

    char sentence[READ_BUFFER_SIZE + 1]; /* This is for building
                                            the NMEA sentence */
    gsize chars_in_buffer; /* How many characters are in the buffer */
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GYPSY_TYPE_NMEA_PARSER, GypsyNmeaParserPrivate))
G_DEFINE_TYPE (GypsyNmeaParser, gypsy_nmea_parser, GYPSY_TYPE_PARSER);

static void
gypsy_nmea_parser_finalize (GObject *object)
{
    GypsyNmeaParser *self = (GypsyNmeaParser *) object;
    GypsyNmeaParserPrivate *priv = self->priv;

    if (priv->ctxt) {
        nmea_parse_context_free (priv->ctxt);
        priv->ctxt = NULL;
    }

    G_OBJECT_CLASS (gypsy_nmea_parser_parent_class)->finalize (object);
}

static void
gypsy_nmea_parser_dispose (GObject *object)
{
#if 0
    GypsyNmeaParser *self = (GypsyNmeaParser *) object;
#endif
    G_OBJECT_CLASS (gypsy_nmea_parser_parent_class)->dispose (object);
}

static GObject *
gypsy_nmea_parser_constructor (GType                  type,
                               guint                  n_params,
                               GObjectConstructParam *params)
{
    GypsyNmeaParser *parser;
    GypsyNmeaParserPrivate *priv;
    GypsyClient *client;
    GObject *object;

    object = G_OBJECT_CLASS (gypsy_nmea_parser_parent_class)->constructor
        (type, n_params, params);

    parser = GYPSY_NMEA_PARSER (object);
    priv = parser->priv;

    g_object_get (object,
                  "client", &client,
                  NULL);

    priv->ctxt = nmea_parse_context_new (client);

    return object;
}

static gboolean
gypsy_nmea_parser_received_data (GypsyParser  *parser,
                                 const guchar *data,
                                 gsize         length,
                                 GError      **error)
{
    GypsyNmeaParser *nmea = GYPSY_NMEA_PARSER (parser);
    GypsyNmeaParserPrivate *priv = nmea->priv;
    char *eos = NULL;

    memcpy (priv->sentence + priv->chars_in_buffer, data, length);
    priv->chars_in_buffer += length;

    /* Append a '\0' to the data so we never run off the end.
       The '\0' will be overwritten by the next call to received_data */
    *(priv->sentence + priv->chars_in_buffer) = '\0';

    /* NMEA sentences end with <CR><LF>,
       so find the <CR> at the end of each sentence */
    while ((eos = strchr (priv->sentence, '\r'))) {
        int sentence_length;
        /* Account for <LF> */
        sentence_length = (eos - priv->sentence) + 2;
        if (sentence_length > 1) {
            /* terminate the string at the <CR> */
            *eos = '\0';

            g_debug ("NMEA sentence: %s", priv->sentence);
            if (nmea_parse_sentence (priv->ctxt, priv->sentence, NULL) == FALSE) {
                g_debug ("Invalid sentence: %s", priv->sentence);
            }
        }

        if (sentence_length > 0) {
            /* Remove the sentence from the buffer and
               move the rest up including terminating 0 */
            memmove (priv->sentence, eos + 2,
                     (priv->chars_in_buffer - sentence_length) + 1);
            priv->chars_in_buffer -= sentence_length;
        }
    }

    return TRUE;
}

static gsize
gypsy_nmea_parser_get_space_in_buffer (GypsyParser *parser)
{
    GypsyNmeaParser *nmea = GYPSY_NMEA_PARSER (parser);
    GypsyNmeaParserPrivate *priv = nmea->priv;

    return READ_BUFFER_SIZE - priv->chars_in_buffer;
}

static void
gypsy_nmea_parser_class_init (GypsyNmeaParserClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    GypsyParserClass *p_class = (GypsyParserClass *) klass;

    o_class->dispose = gypsy_nmea_parser_dispose;
    o_class->finalize = gypsy_nmea_parser_finalize;
    o_class->constructor = gypsy_nmea_parser_constructor;

    p_class->received_data = gypsy_nmea_parser_received_data;
    p_class->get_space_in_buffer = gypsy_nmea_parser_get_space_in_buffer;

    g_type_class_add_private (klass, sizeof (GypsyNmeaParserPrivate));
}

static void
gypsy_nmea_parser_init (GypsyNmeaParser *self)
{
    GypsyNmeaParserPrivate *priv = GET_PRIVATE (self);

    self->priv = priv;
}

GypsyParser *
gypsy_nmea_parser_new (GypsyClient *client)
{
    g_return_val_if_fail (GYPSY_IS_CLIENT (client), NULL);

    return (GypsyParser *) g_object_new (GYPSY_TYPE_NMEA_PARSER,
                                         "client", client,
                                         NULL);
}
