#include "gypsy-parser.h"

enum {
    PROP_0,
    PROP_CLIENT,
};

struct _GypsyParserPrivate {
    GypsyClient *client;
};

#define GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GYPSY_TYPE_PARSER, GypsyParserPrivate))
G_DEFINE_TYPE (GypsyParser, gypsy_parser, G_TYPE_OBJECT);

static void
gypsy_parser_finalize (GObject *object)
{
    G_OBJECT_CLASS (gypsy_parser_parent_class)->finalize (object);
}

static void
gypsy_parser_dispose (GObject *object)
{
    GypsyParser *self = (GypsyParser *) object;
    GypsyParserPrivate *priv = self->priv;

    if (priv->client) {
        g_object_unref (priv->client);
        priv->client = NULL;
    }

    G_OBJECT_CLASS (gypsy_parser_parent_class)->dispose (object);
}

static void
gypsy_parser_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
    GypsyParser *self = (GypsyParser *) object;
    GypsyParserPrivate *priv = self->priv;

    switch (prop_id) {
    case PROP_CLIENT:
        priv->client = g_value_dup_object (value);
        break;

    default:
        break;
    }
}

static void
gypsy_parser_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
    GypsyParser *self = (GypsyParser *) object;
    GypsyParserPrivate *priv = self->priv;

    switch (prop_id) {
    case PROP_CLIENT:
        g_value_set_object (value, priv->client);
        break;

    default:
        break;
    }
}

static void
gypsy_parser_class_init (GypsyParserClass *klass)
{
    GObjectClass *o_class = (GObjectClass *) klass;
    GParamSpec *pspec;

    o_class->dispose = gypsy_parser_dispose;
    o_class->finalize = gypsy_parser_finalize;
    o_class->set_property = gypsy_parser_set_property;
    o_class->get_property = gypsy_parser_get_property;

    g_type_class_add_private (klass, sizeof (GypsyParserPrivate));

    pspec = g_param_spec_object ("client", "Client",
                                 "The GypsyClient object this parser belongs to",
                                 GYPSY_TYPE_CLIENT,
                                 G_PARAM_READWRITE |
                                 G_PARAM_CONSTRUCT_ONLY |
                                 G_PARAM_STATIC_STRINGS);
    g_object_class_install_property (o_class, PROP_CLIENT, pspec);
}

static void
gypsy_parser_init (GypsyParser *self)
{
    GypsyParserPrivate *priv = GET_PRIVATE (self);

    self->priv = priv;
}

gboolean
gypsy_parser_received_data (GypsyParser  *parser,
                            const guchar *data,
                            guint         length,
                            GError      **error)
{
    GypsyParserClass *klass = GYPSY_PARSER_GET_CLASS (parser);

    if (klass->received_data == NULL) {
        g_error ("%s does not implement received_data",
                 G_OBJECT_TYPE_NAME (parser));
        return FALSE;
    }

    return klass->received_data (parser, data, length, error);
}

gsize
gypsy_parser_get_space_in_buffer (GypsyParser *parser)
{
    GypsyParserClass *klass = GYPSY_PARSER_GET_CLASS (parser);

    if (klass->get_space_in_buffer == NULL) {
        g_error ("%s does not implement get_space_in_buffer",
                 G_OBJECT_TYPE_NAME (parser));
        return FALSE;
    }

    return klass->get_space_in_buffer (parser);
}

GypsyClient *
gypsy_parser_get_client (GypsyParser *parser)
{
    g_return_val_if_fail (GYPSY_IS_PARSER (parser), NULL);

    return parser->priv->client;
}
