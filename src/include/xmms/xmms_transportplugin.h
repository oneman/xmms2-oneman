/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003	Peter Alm, Tobias Rundstr�m, Anders Gustafsson
 * 
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *                   
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */




#ifndef __XMMS_TRANSPORTPLUGIN_H__
#define __XMMS_TRANSPORTPLUGIN_H__

#include <glib.h>

/*
 * Type definitions
 */


#include "xmms/xmms_error.h"
#include "xmms/xmms_plugin.h"
#include "xmms/xmms_medialib.h"
#include "xmms/xmms_transport.h"

/*
 * Transport plugin methods
 */

typedef gboolean (*xmms_transport_can_handle_method_t) (const gchar *uri);
typedef guint (*xmms_transport_lmod_method_t) (xmms_transport_t *transport);
typedef gboolean (*xmms_transport_open_method_t) (xmms_transport_t *transport,
						  const gchar *uri);
typedef void (*xmms_transport_close_method_t) (xmms_transport_t *transport);
typedef gint (*xmms_transport_read_method_t) (xmms_transport_t *transport,
					      gchar *buffer, guint length, xmms_error_t *error);
typedef gint (*xmms_transport_seek_method_t) (xmms_transport_t *transport, 
					      guint64 offset, gint whence);
typedef guint64 (*xmms_transport_size_method_t) (xmms_transport_t *transport);
typedef GList *(*xmms_transport_list_method_t) (const gchar *path);

#define XMMS_PLUGIN_METHOD_CAN_HANDLE_TYPE xmms_transport_can_handle_method_t
#define XMMS_PLUGIN_METHOD_INIT_TYPE xmms_transport_open_method_t
#define XMMS_PLUGIN_METHOD_READ_TYPE xmms_transport_read_method_t
#define XMMS_PLUGIN_METHOD_SIZE_TYPE xmms_transport_size_method_t
#define XMMS_PLUGIN_METHOD_SEEK_TYPE xmms_transport_seek_method_t
#define XMMS_PLUGIN_METHOD_LMOD_TYPE xmms_transport_lmod_method_t
#define XMMS_PLUGIN_METHOD_CLOSE_TYPE xmms_transport_close_method_t


/*
 * Public function prototypes
 */

void xmms_transport_ringbuf_resize (xmms_transport_t *transport, gint size);
gpointer xmms_transport_private_data_get (xmms_transport_t *transport);
void xmms_transport_private_data_set (xmms_transport_t *transport, gpointer data);
xmms_plugin_t *xmms_transport_plugin_get (const xmms_transport_t *transport);

#endif
