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

	gint t;

	for(t = 0; t < 7; t++) {
		xmms_transition_destroy(transitions->transitions[t]);
	}
	g_free (transitions);

	XMMS_DBG ("Destroying transitions");

}

static void
update_transitions_config (xmms_object_t *object, xmmsv_t *data,
                          gpointer userdata)
{


	XMMS_DBG ("Update transitions config called");

}

xmms_transitions_t *
xmms_transitions_new ()
{

	xmms_transitions_t *transitions = g_new0 (xmms_transitions_t, 1);

	const char *transition_strings[] = { "starting", "stopping", "pausing", "resuming", "seeking", "jumping", "advancing" };

	XMMS_DBG ("Setting up transitions");

	xmms_transition_plugin_t *plugin;
	xmms_config_property_t *cv;
	
	int t;
	gint stack_no; // well call the 'chain' of plugins a 'stack' since we use that chain word elsewhere
	xmms_config_property_t *cfg;
	gchar key[64];
	const gchar *name;
		

	for(t = 0; t < 7; t++) {
	
		for (stack_no = 0; TRUE; stack_no++) {

			g_snprintf (key, sizeof (key), "transition.%s.%i", transition_strings[t], stack_no);
		
			if ((!stack_no)) {
				xmms_config_property_register (key, "", update_transitions_config,
			                               NULL);
			}

			cfg = xmms_config_lookup (key);
			
			// Single Source starting/stopping/pausing/resuming
			if (t < 4) { 

				if (!cfg) {
					// no config exists
					XMMS_DBG ("No config for %s transition plugin number %d in stack", transition_strings[t], stack_no);
					transitions->transitions[t] = xmms_transition_new(NULL);
					break;
				}
			
				name = xmms_config_property_get_string (cfg);

				if (!name[0]) {
					// config exists, but is empty
					XMMS_DBG ("No value for %s transition plugin number %d in stack", transition_strings[t],stack_no);
					transitions->transitions[t] = xmms_transition_new(NULL);
					break;
				}

				// config exists and is something
				XMMS_DBG ("Trying %s plugin for %s transition as effect number %d in stack", name, transition_strings[t], stack_no);
				
				plugin = (xmms_transition_plugin_t *)xmms_plugin_find (XMMS_PLUGIN_TYPE_TRANSITION, name);

				if (plugin == NULL) {
					XMMS_DBG ("Cant find plugin called %s", name);
					transitions->transitions[t] = xmms_transition_new(NULL);
					break;
				} else {
					transitions->transitions[t] = xmms_transition_new(plugin);
					XMMS_DBG ("%s plugin set for %s transition as effect %d in stack", name, transition_strings[t], stack_no);
				}
		
			} else {
				// Dual Source Seeking, Jumping Advancing
				XMMS_DBG ("Skipping dual source %s transition", transition_strings[t]);
				break;
			}
		}
	}

	XMMS_DBG ("Transitions setup complete");
	
	return transitions;


}
 
 
static xmms_transition_t *
xmms_transition_new (xmms_transition_plugin_t *plugin)
{

	xmms_transition_t *transition = g_new0 (xmms_transition_t, 1);

	if (plugin != NULL) {
		return xmms_transition_add (plugin, transition);
	} else {
		transition->enabled = false;
		return transition;
	}
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
