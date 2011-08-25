#ifndef __GYPSY_DEBUG_H__
#define __GYPSY_DEBUG_H__

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
    GYPSY_DEBUG_NMEA = 1 << 0,
    GYPSY_DEBUG_SERVER = 1 << 1,
    GYPSY_DEBUG_CLIENT = 1 << 2
} GypsyDebugFlags;

#define GYPSY_HAS_DEBUG(type) ((gypsy_debug_flags & GYPSY_DEBUG_##type) != FALSE)

extern guint gypsy_debug_flags;

#ifdef __GNUC__

#define GYPSY_NOTE(type,x,a...) G_STMT_START {                    \
        if (G_UNLIKELY (GYPSY_HAS_DEBUG (type)))                      \
            { _gypsy_message ("[" #type "] " G_STRLOC ": " x, ##a); }   \
    } G_STMT_END

#else /* !__GNUC__ */

#define GYPSY_NOTE(type,...) G_STMT_START { \
        if (G_UNLIKELY (GYPSY_HAS_DEBUG (type))) {       \
            char *_fmt = g_strdup_printf (__VA_ARGS__);         \
            _gypsy_message ("[" #type "] " G_STRLOC ": %s", _fmt);      \
            g_free (_fmt);                                              \
        }                                                               \
    } G_STMT_END

#endif

void _gypsy_message (const char *format, ...);

G_END_DECLS

#endif /* __GYPSY_DEBUG_H__ */
