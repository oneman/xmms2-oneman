/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2011 XMMS2 Team
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

#include "xmms/xmms_transitionplugin.h"
#include "xmms/xmms_log.h"

#include <glib.h>


/*
 *  Defines
 */





/*
 * Type definitions
 */

typedef struct xmms_fade_data_St {

} xmms_fade_data_t;


/*
 * Function prototypes
 */

static gboolean xmms_fade_plugin_setup (xmms_transition_plugin_t *plugin);
static gboolean xmms_fade_new (xmms_transition_t *transition);
static void xmms_fade_destroy (xmms_transition_t *transition);
static int xmms_fade_process (xmms_transition_t *transition, gpointer buffer, gint len,
                              xmms_error_t *err);

/*
 * Plugin header
 */

XMMS_TRANSITION_PLUGIN ("fade", "Fade Transition", XMMS_VERSION,
                    "Fading transition plugin",
                    xmms_fade_plugin_setup);

static gboolean
xmms_fade_plugin_setup (xmms_transition_plugin_t *plugin)
{
	xmms_transition_methods_t methods;

	XMMS_TRANSITION_METHODS_INIT (methods);

	methods.new = xmms_fade_new;
	methods.destroy = xmms_fade_destroy;
	methods.process = xmms_fade_process;

	xmms_transition_plugin_methods_set (plugin, &methods);

	xmms_transition_plugin_config_property_register (plugin, "testconfigoption", "test123",
	                                             NULL, NULL);

	XMMS_DBG ("Fade Plugin Method Setup Called");

	return TRUE;
}


/*
 * Member functions
 */


static gboolean
xmms_fade_new (xmms_transition_t *transition)
{

	XMMS_DBG ("Fade Plugin Method New Called");
	return TRUE;

}


static void
xmms_fade_destroy (xmms_transition_t *transition)
{

	XMMS_DBG ("Fade Plugin Method Destroy Called");

}


static int
xmms_fade_process (xmms_transition_t *transition, gpointer buffer, gint len,
                              xmms_error_t *err)
{

	XMMS_DBG ("Fade Plugin Method Process Called");
	return TRUE;

}

