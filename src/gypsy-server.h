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

#ifndef __GYPSY_SERVER_H__
#define __GYPSY_SERVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GYPSY_SERVER_ERROR gypsy_server_error_quark ()

#define GYPSY_TYPE_SERVER (gypsy_server_get_type ())

typedef enum {
	GYPSY_SERVER_ERROR_NO_CLIENT,
} GypsyServerError;

typedef struct _GypsyServer {
	GObject parent_object;
} GypsyServer;

typedef struct _GypsyServerClass {
	GObjectClass parent_class;

	void (*terminate) (GypsyServer *server);
} GypsyServerClass;

GType gypsy_server_get_type (void);

void gypsy_server_remove_clients (GypsyServer *gps,
				  const char  *prev_owner);
G_END_DECLS

#endif
