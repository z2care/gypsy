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
