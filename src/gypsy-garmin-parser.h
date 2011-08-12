#ifndef __GYPSY_GARMIN_PARSER_H__
#define __GYPSY_GARMIN_PARSER_H__

#include <gypsy-parser.h>
#include <gypsy-client.h>


G_BEGIN_DECLS

#define GYPSY_TYPE_GARMIN_PARSER                                        \
   (gypsy_garmin_parser_get_type())
#define GYPSY_GARMIN_PARSER(obj)                                        \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GYPSY_TYPE_GARMIN_PARSER,               \
                                GypsyGarminParser))
#define GYPSY_GARMIN_PARSER_CLASS(klass)                                \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GYPSY_TYPE_GARMIN_PARSER,                  \
                             GypsyGarminParserClass))
#define GYPSY_IS_GARMIN_PARSER(obj)                                     \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GYPSY_TYPE_GARMIN_PARSER))
#define GYPSY_IS_GARMIN_PARSER_CLASS(klass)                             \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GYPSY_TYPE_GARMIN_PARSER))
#define GYPSY_GARMIN_PARSER_GET_CLASS(obj)                              \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GYPSY_TYPE_GARMIN_PARSER,                \
                               GypsyGarminParserClass))

typedef struct _GypsyGarminParserPrivate GypsyGarminParserPrivate;
typedef struct _GypsyGarminParser      GypsyGarminParser;
typedef struct _GypsyGarminParserClass GypsyGarminParserClass;

struct _GypsyGarminParser
{
    GypsyParser parent;

    GypsyGarminParserPrivate *priv;
};

struct _GypsyGarminParserClass
{
    GypsyParserClass parent_class;
};

GType gypsy_garmin_parser_get_type (void) G_GNUC_CONST;
GypsyParser *gypsy_garmin_parser_new (GypsyClient *client);

G_END_DECLS

#endif /* __GYPSY_GARMIN_PARSER_H__ */
