#ifndef __GYPSY_DISCOVERY_H__
#define __GYPSY_DISCOVERY_H__

#include <glib-object.h>


G_BEGIN_DECLS

#define GYPSY_TYPE_DISCOVERY                                            \
   (gypsy_discovery_get_type())
#define GYPSY_DISCOVERY(obj)                                            \
   (G_TYPE_CHECK_INSTANCE_CAST ((obj),                                  \
                                GYPSY_TYPE_DISCOVERY,                   \
                                GypsyDiscovery))
#define GYPSY_DISCOVERY_CLASS(klass)                                    \
   (G_TYPE_CHECK_CLASS_CAST ((klass),                                   \
                             GYPSY_TYPE_DISCOVERY,                      \
                             GypsyDiscoveryClass))
#define GYPSY_IS_DISCOVERY(obj)                                         \
   (G_TYPE_CHECK_INSTANCE_TYPE ((obj),                                  \
                                GYPSY_TYPE_DISCOVERY))
#define GYPSY_IS_DISCOVERY_CLASS(klass)                                 \
   (G_TYPE_CHECK_CLASS_TYPE ((klass),                                   \
                             GYPSY_TYPE_DISCOVERY))
#define GYPSY_DISCOVERY_GET_CLASS(obj)                                  \
   (G_TYPE_INSTANCE_GET_CLASS ((obj),                                   \
                               GYPSY_TYPE_DISCOVERY,                    \
                               GypsyDiscoveryClass))

typedef struct _GypsyDiscoveryPrivate GypsyDiscoveryPrivate;
typedef struct _GypsyDiscovery      GypsyDiscovery;
typedef struct _GypsyDiscoveryClass GypsyDiscoveryClass;

struct _GypsyDiscovery
{
    GObject parent;

    GypsyDiscoveryPrivate *priv;
};

struct _GypsyDiscoveryClass
{
    GObjectClass parent_class;
};

GType gypsy_discovery_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GYPSY_DISCOVERY_H__ */
