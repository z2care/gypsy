/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GYPSY_DISCOVERY_H__
#define __GYPSY_DISCOVERY_H__

#include <glib-object.h>


G_BEGIN_DECLS

#define GYPSY_DISCOVERY_DBUS_SERVICE "org.freedesktop.Gypsy"
#define GYPSY_DISCOVERY_DBUS_PATH "/org/freedesktop/Gypsy/Discovery"
#define GYPSY_DISCOVERY_DBUS_INTERFACE "org.freedesktop.Gypsy.Discovery"

#define GYPSY_TYPE_DISCOVERY			\
	(gypsy_discovery_get_type())
#define GYPSY_DISCOVERY(obj)					\
	(G_TYPE_CHECK_INSTANCE_CAST ((obj),			\
				     GYPSY_TYPE_DISCOVERY,	\
				     GypsyDiscovery))
#define GYPSY_DISCOVERY_CLASS(klass)			\
	(G_TYPE_CHECK_CLASS_CAST ((klass),		\
				  GYPSY_TYPE_DISCOVERY,	\
				  GypsyDiscoveryClass))
#define GYPSY_IS_DISCOVERY(obj)					\
	(G_TYPE_CHECK_INSTANCE_TYPE ((obj),			\
				     GYPSY_TYPE_DISCOVERY))
#define GYPSY_IS_DISCOVERY_CLASS(klass)				\
	(G_TYPE_CHECK_CLASS_TYPE ((klass),			\
				  GYPSY_TYPE_DISCOVERY))
#define GYPSY_DISCOVERY_GET_CLASS(obj)				\
	(G_TYPE_INSTANCE_GET_CLASS ((obj),			\
				    GYPSY_TYPE_DISCOVERY,	\
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
GypsyDiscovery *gypsy_discovery_new (void);
char **gypsy_discovery_list_devices (GypsyDiscovery *discovery,
				     GError        **error);
gboolean gypsy_discovery_start_scanning (GypsyDiscovery *discovery,
					 GError        **error);
gboolean gypsy_discovery_stop_scanning (GypsyDiscovery *discovery,
					GError        **error);


G_END_DECLS

#endif /* __GYPSY_DISCOVERY_H__ */
