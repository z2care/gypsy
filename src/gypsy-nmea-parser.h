#ifndef __GYPSY_NMEA_PARSER_H__
#define __GYPSY_NMEA_PARSER_H__

#include <gypsy-parser.h>
#include <gypsy-client.h>

G_BEGIN_DECLS

#define GYPSY_TYPE_NMEA_PARSER                                          \
   (gypsy_nmea_parser_get_type())
#define GYPSY_NMEA_PARSER(obj)                                          \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GYPSY_TYPE_NMEA_PARSER,                 \
                                GypsyNmeaParser))
#define GYPSY_NMEA_PARSER_CLASS(klass)                                  \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GYPSY_TYPE_NMEA_PARSER,                    \
                             GypsyNmeaParserClass))
#define GYPSY_IS_NMEA_PARSER(obj)                                       \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GYPSY_TYPE_NMEA_PARSER))
#define GYPSY_IS_NMEA_PARSER_CLASS(klass)                               \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GYPSY_TYPE_NMEA_PARSER))
#define GYPSY_NMEA_PARSER_GET_CLASS(obj)                                \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GYPSY_TYPE_NMEA_PARSER,                  \
                               GypsyNmeaParserClass))

typedef struct _GypsyNmeaParserPrivate GypsyNmeaParserPrivate;
typedef struct _GypsyNmeaParser      GypsyNmeaParser;
typedef struct _GypsyNmeaParserClass GypsyNmeaParserClass;

struct _GypsyNmeaParser
{
    GypsyParser parent;

    GypsyNmeaParserPrivate *priv;
};

struct _GypsyNmeaParserClass
{
    GypsyParserClass parent_class;
};

GType gypsy_nmea_parser_get_type (void) G_GNUC_CONST;
GypsyParser *gypsy_nmea_parser_new (GypsyClient *client);

G_END_DECLS

#endif /* __GYPSY_NMEA_PARSER_H__ */
