#ifndef __GYPSY_PARSER_H__
#define __GYPSY_PARSER_H__

#include <glib-object.h>
#include <gypsy-client.h>

G_BEGIN_DECLS

#define GYPSY_TYPE_PARSER                                               \
   (gypsy_parser_get_type())
#define GYPSY_PARSER(obj)                                               \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GYPSY_TYPE_PARSER,                      \
                                GypsyParser))
#define GYPSY_PARSER_CLASS(klass)                                       \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GYPSY_TYPE_PARSER,                         \
                             GypsyParserClass))
#define GYPSY_IS_PARSER(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GYPSY_TYPE_PARSER))
#define GYPSY_IS_PARSER_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GYPSY_TYPE_PARSER))
#define GYPSY_PARSER_GET_CLASS(obj)                                     \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GYPSY_TYPE_PARSER,                       \
                               GypsyParserClass))

typedef struct _GypsyParserPrivate GypsyParserPrivate;
typedef struct _GypsyParser      GypsyParser;
typedef struct _GypsyParserClass GypsyParserClass;

struct _GypsyParser
{
    GObject parent;

    GypsyParserPrivate *priv;
};

struct _GypsyParserClass
{
    GObjectClass parent_class;

    gboolean (*received_data) (GypsyParser  *parser,
                               const guchar *data,
                               gsize         length,
                               GError      **error);
    gsize (*get_space_in_buffer) (GypsyParser *parser);
};

GType gypsy_parser_get_type (void) G_GNUC_CONST;
GypsyClient *gypsy_parser_get_client (GypsyParser *parser);

G_END_DECLS

#endif /* __GYPSY_PARSER_H__ */
