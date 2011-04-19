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

/**
 * @file
 * Output plugin helper
 */
 
#include "xmms/xmms_transitionplugin.h"
#include "xmmspriv/xmms_transitionplugin.h"
#include "xmmspriv/xmms_transition.h"
#include "xmms/xmms_log.h"

struct xmms_transition_St {

	xmms_transition_plugin_t *plugin;
	gpointer plugin_data;
	gint duration;
	gint frames;
	gboolean enabled;

};

struct xmms_transitions_St {

	xmms_transition_t *starting;
	xmms_transition_t *stopping;
	xmms_transition_t *pausing;
	xmms_transition_t *resuming;
	xmms_transition_t *seeking;
	xmms_transition_t *jumping;
	xmms_transition_t *advancing;
	
	xmms_transition_t *transitions[7];

};

static xmms_transition_t *xmms_transition_add (xmms_transition_plugin_t *plugin, xmms_transition_t *transition);
static void xmms_transition_remove (xmms_transition_t *transition);
static xmms_transition_t *xmms_transition_new (xmms_transition_plugin_t *plugin);
static void xmms_transition_destroy (xmms_transition_t *transition);




gpointer
xmms_transition_private_data_get (xmms_transition_t *transition)
{
	g_return_val_if_fail (transition, NULL);
	g_return_val_if_fail (transition->plugin, NULL);

	return transition->plugin_data;
}

void
xmms_transition_private_data_set (xmms_transition_t *transition, gpointer data)
{
	g_return_if_fail (transition);
	g_return_if_fail (transition->plugin);

	transition->plugin_data = data;
}



void
xmms_transitions_destroy (xmms_transitions_t *transitions)
{

	g_return_if_fail (transitions);


	xmms_transition_destroy(transitions->starting);

	g_free (transitions);

	XMMS_DBG ("Destroying transitions");

}

xmms_transitions_t *
xmms_transitions_new ()
{

	xmms_transitions_t *transitions = g_new0 (xmms_transitions_t, 1);

	XMMS_DBG ("Setting up transitions");
	
	// read all the config values and load up all the plugins into the structs

	xmms_transition_plugin_t *plugin;
	xmms_config_property_t *cv;
	const gchar *name = "fade";
	
		cv = xmms_config_property_register ("transition.plugin",
	                                    "fade",
	                                    NULL, NULL);
	
	name = xmms_config_property_get_string (cv);
	xmms_log_info ("Using transition plugin: %s", name);
	plugin = (xmms_transition_plugin_t *)xmms_plugin_find (XMMS_PLUGIN_TYPE_TRANSITION, name);

	transitions->starting = xmms_transition_new(plugin);


	XMMS_DBG ("Transitions setup complete");
	
	return transitions;


}
 
 
static xmms_transition_t *
xmms_transition_new (xmms_transition_plugin_t *plugin)
{

	xmms_transition_t *transition = g_new0 (xmms_transition_t, 1);

	return xmms_transition_add (plugin, transition);

}
 
static xmms_transition_t *
xmms_transition_add (xmms_transition_plugin_t *plugin, xmms_transition_t *transition)
{
	
	XMMS_DBG ("Adding transition");

	transition->plugin = plugin;

	xmms_transition_plugin_method_new (plugin, transition);

	transition->enabled = true;

	return transition;

}
 
static void
xmms_transition_remove (xmms_transition_t *transition)
{

	g_return_if_fail (transition);

	XMMS_DBG ("Removing transition");

	xmms_transition_plugin_method_destroy (transition->plugin, transition);

	transition->enabled = false;

	//g_free (transition);


}


static void
xmms_transition_destroy (xmms_transition_t *transition)
{

	g_return_if_fail (transition);

	XMMS_DBG ("Destroying transition");

	xmms_transition_plugin_method_destroy (transition->plugin, transition);

	xmms_object_unref (transition->plugin);

	g_free (transition);


}



 /** @} */
