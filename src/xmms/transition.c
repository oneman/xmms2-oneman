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

static xmms_transition_t *xmms_transition_add (xmms_transition_plugin_t *plugin, xmms_transition_t *transition);
static void xmms_transition_remove (xmms_transition_t *transition);

static gboolean xmms_transition_last_was_enabled (xmms_transition_t *transition);

static xmms_transition_t *xmms_transition_new (xmms_transition_plugin_t *plugin);
static void xmms_transition_load (xmms_transition_t **transition, const gchar *transition_name, int stack_num);

static void xmms_transition_load_config_value (xmms_transition_t **transition, const gchar *transition_name, const gchar *config_name, int stack_num, int dumbass);

static void xmms_transition_destroy_final (xmms_transition_t *transition);

static gboolean xmms_transition_add_plugin (xmms_transition_plugin_t *plugin, xmms_transition_t *transition);
static void xmms_transition_show(xmms_transition_t *transition);

static void xmms_transitions_show (xmms_transitions_t *transitions);


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
		xmms_transition_destroy_final(transitions->transitions[t]);
	}
	g_free (transitions);

	XMMS_DBG ("Destroying transitions");

}

static void
update_transitions_config (xmms_object_t *object, xmmsv_t *data,
                          gpointer userdata)
{


	XMMS_DBG ("Update transitions config called");
	
	
	xmms_transitions_t *transitions = (xmms_transitions_t *)userdata;
	
	xmms_transitions_show (transitions);
	
	

}


static void
xmms_transitions_show (xmms_transitions_t *transitions)
{


	XMMS_DBG ("Displaying transitions setup");
	
	gint t;

	for(t = 0; t < 7; t++) {
		//xmms_transition_show(transitions->transitions[t]);
	}
	
	

}


static void
xmms_transition_load (xmms_transition_t **transition, const gchar *transition_name, int stack_num)
{

	xmms_config_property_t *cfg;
	gchar key[64];
	xmms_transition_plugin_t *plugin;
	const gchar *name;

				g_snprintf (key, sizeof (key), "transition.%s.%i", transition_name, stack_num);

			if (stack_num == 0) {				
				xmms_config_property_register (key, "fade", update_transitions_config, NULL);
			} else {
				xmms_config_property_register (key, "", update_transitions_config, NULL);
			}

				cfg = xmms_config_lookup (key);
				name = xmms_config_property_get_string (cfg);

				if (name[0]) {

					XMMS_DBG ("Trying %s plugin for %s transition as effect number %d in stack", name, transition_name, stack_num);
					plugin = (xmms_transition_plugin_t *)xmms_plugin_find (XMMS_PLUGIN_TYPE_TRANSITION, name);

					if (plugin == NULL) {
						XMMS_DBG ("Cant find plugin called %s", name);
						*transition = xmms_transition_new(NULL);

						return;
					} else {
						if (stack_num == 0) { 
							*transition = xmms_transition_new(plugin);

						} else {
							xmms_transition_add_plugin (plugin, *transition);

						}


						XMMS_DBG ("%s plugin set for %s transition as effect %d in stack", name, transition_name, stack_num);
					}
					
				} else {
				
					// config exists, but is empty
					XMMS_DBG ("No value for %s transition plugin number %d in stack", transition_name,stack_num);
					if (stack_num == 0) { 
						*transition = xmms_transition_new(NULL);

					} else {
												xmms_transition_add_plugin (NULL, *transition);
					}
					
					
				}
				
			xmms_transition_load_config_value(transition, transition_name, "duration", stack_num, 0);
			xmms_transition_load_config_value(transition, transition_name, "offset", stack_num, 1);			
}


static void
xmms_transition_load_config_value(xmms_transition_t **transition, const gchar *transition_name, const gchar *config_name, int stack_num, int dumbass)
{

				xmms_transition_t *t_pointa;
	xmms_transition_plugin_t *plugin;
	xmms_config_property_t *cv;
	
	int t;
	gint stack_no; // well call the 'chain' of plugins a 'stack' since we use that chain word elsewhere
	xmms_config_property_t *cfg;
	gchar key[64];
	const gchar *name;
	int duration;
				
				g_snprintf (key, sizeof (key), "transition.%s.%s.%i", transition_name, config_name, stack_num);
		
				if (dumbass == 0) {
					xmms_config_property_register (key, "60000", update_transitions_config,
				                               NULL);
				} else {
									xmms_config_property_register (key, "", update_transitions_config,
				                               NULL);
				
				}

				cfg = xmms_config_lookup (key);
/*
				if (!cfg) {
					// no config exists
					XMMS_DBG ("No config for duration of %s transition plugin number %d in stack", transition_strings[t], stack_num);
									if (stack_no == 0) { 
					//transitions->transitions[t]->total_frames = 50000;
					}
					break;
				}
	*/		
				duration = xmms_config_property_get_int (cfg);

				if (!(duration > 0)) {
					// config exists, but is empty
					XMMS_DBG ("No value for duration of %s transition plugin number %d in stack", transition_name,stack_num);
								if (stack_num == 0) { 
					//transitions->transitions[t]->total_frames = 50000;
					}
					//break;
				} else {

				// config exists and is something
				XMMS_DBG ("Setting duration to %d for plugin for %s transition as effect number %d in stack", duration, transition_name, stack_num);


				t_pointa = *transition;
				int c;

				for (c = 0; c != stack_num; c++) {

					t_pointa = t_pointa->next;
					

					
				}
				

				if (dumbass == 0) {
									t_pointa->total_frames = duration;
				} else {
													t_pointa->offset = duration;
				}
				
				}
				
				/* end repeat */


}


xmms_transitions_t *
xmms_transitions_new ()
{

	xmms_transitions_t *transitions = g_new0 (xmms_transitions_t, 1);

	const char *transition_strings[] = { "starting", "stopping", "pausing", "resuming", "seeking", "jumping", "advancing" };

	XMMS_DBG ("Setting up transitions");
				xmms_transition_t *t_pointa;
	xmms_transition_plugin_t *plugin;
	xmms_config_property_t *cv;
	
	int t;
	gint stack_no; // well call the 'chain' of plugins a 'stack' since we use that chain word elsewhere
	xmms_config_property_t *cfg;
	gchar key[64];
	const gchar *name;
	int duration;

	for(t = 0; t < 7; t++) {
		
		// Single Source starting/stopping/pausing/resuming
		if (t < 4) { 
		
			for (stack_no = 0; TRUE; stack_no++) {
			


				xmms_transition_load(&transitions->transitions[t], transition_strings[t], stack_no);
				
				//if (transitions->transitions[t]->enabled == FALSE) {
				if (xmms_transition_last_was_enabled(transitions->transitions[t]) == FALSE) {
					// this is wrong because ... the enabled check needs to dive down
					xmms_transition_load(&transitions->transitions[t], transition_strings[t], stack_no + 1);
					break;
				}
				

				
		
				
				
			}
		
		} else {
			// Dual Source Seeking, Jumping Advancing

			/* MIX PLUGIN */


			
				g_snprintf (key, sizeof (key), "transition.%s.mix", transition_strings[t]);
		

					xmms_config_property_register (key, "crossfade", update_transitions_config,
				                               transitions);
	

				cfg = xmms_config_lookup (key);

				if (!cfg) {
					// no config exists
					XMMS_DBG ("No config for %s transition mixing plugin", transition_strings[t]);
					transitions->transitions[t] = xmms_transition_new(NULL);
					//continue;
				} else {
			
				name = xmms_config_property_get_string (cfg);

				if (!name[0]) {
					// config exists, but is empty
					XMMS_DBG ("No value for %s transition mixing plugin", transition_strings[t]);
					transitions->transitions[t] = xmms_transition_new(NULL);
					//continue;
				} else {

				// config exists and is something
				XMMS_DBG ("Trying %s plugin for %s transition mixing effect", name, transition_strings[t]);
				
				plugin = (xmms_transition_plugin_t *)xmms_plugin_find (XMMS_PLUGIN_TYPE_TRANSITION, name);

				if (plugin == NULL) {
					XMMS_DBG ("Cant find plugin called %s", name);
					transitions->transitions[t] = xmms_transition_new(NULL);
					continue;
				} else {
					transitions->transitions[t] = xmms_transition_new(plugin);
					XMMS_DBG ("%s plugin set for %s transition as mixing effect", name, transition_strings[t]);
				}
				}
				}
			
			
			
			
			
			/* IN AND OUT SIDE */
			gint i;
			const gchar *inorout;
			// 0 = out, 1 in
			for (i = 0; i<2;i++) {
			
			if (i == 0) {
				inorout = "incoming";
			} else {
				inorout = "outgoing";
			}
			
			for (stack_no = 0; TRUE; stack_no++) {

				g_snprintf (key, sizeof (key), "transition.%s.%s.%i", transition_strings[t], inorout, stack_no);
		
				if (true) {
					xmms_config_property_register (key, "", update_transitions_config,
				                               transitions);
				}

				cfg = xmms_config_lookup (key);

				if (!cfg) {
					// no config exists
					XMMS_DBG ("No config for %s transition %s plugin number %d in stack", transition_strings[t], inorout, stack_no);
									if (stack_no == 0) { 
					if (i == 0) {
						transitions->transitions[t]->out = xmms_transition_new(NULL);
					} else {
						transitions->transitions[t]->in = xmms_transition_new(NULL);
					}
					}
					break;
				}
			
				name = xmms_config_property_get_string (cfg);

				if (!name[0]) {
					// config exists, but is empty
					XMMS_DBG ("No value for %s transition %s plugin number %d in stack", transition_strings[t], inorout, stack_no);
									if (stack_no == 0) { 
					if (i == 0) {
						transitions->transitions[t]->out = xmms_transition_new(NULL);
					} else {
						transitions->transitions[t]->in = xmms_transition_new(NULL);
					}
					}
					break;
				}

				// config exists and is something
				XMMS_DBG ("Trying %s plugin for %s transition as %s effect number %d in stack", name, transition_strings[t], inorout, stack_no);
				
				plugin = (xmms_transition_plugin_t *)xmms_plugin_find (XMMS_PLUGIN_TYPE_TRANSITION, name);

				if (plugin == NULL) {
					XMMS_DBG ("Cant find plugin called %s", name);
					if (i == 0) {
						transitions->transitions[t]->out = xmms_transition_new(NULL);
					} else {
						transitions->transitions[t]->in = xmms_transition_new(NULL);
					}
					break;
				} else {
				
					if (i == 0) {
					if (stack_no == 0) { 
						transitions->transitions[t]->out = xmms_transition_new(plugin);
					} else {
						xmms_transition_add_plugin (plugin, transitions->transitions[t]->out);
					}
					} else {
					if (stack_no == 0) { 
						transitions->transitions[t]->in = xmms_transition_new(plugin);
					} else {
						xmms_transition_add_plugin (plugin, transitions->transitions[t]->in);
					}
					}
					XMMS_DBG ("%s plugin set for %s transition as %s effect %d in stack", name, transition_strings[t], inorout, stack_no);
				}
				
			}
				
			
		}
		}
	
	}

	XMMS_DBG ("Transitions setup complete");
	
	
	xmms_transitions_show(transitions);
	
	
	return transitions;


}
 
 
xmms_transition_t *
xmms_transition_clone (xmms_transition_t *transition)
{

	xmms_transition_t *transition_next;
	xmms_transition_t *transition_clone = g_new0 (xmms_transition_t, 1);


	XMMS_DBG ("Cloning transition for use");


	if (transition->plugin != NULL) {
		xmms_transition_add (transition->plugin, transition_clone);
	}
	
	if (transition->next != NULL) {
		transition_next = transition->next;
		while (transition_next != NULL) {
			
			/* this needs to be change so we copy the config options */
			xmms_transition_add_plugin (transition_next->plugin, transition_clone);
			
			transition_next = transition_next->next;
		}
	
	
	}
	
	

	//transition_clone->total_frames = transition->total_frames;
	//transition_clone->enabled = true;
	return transition_clone;

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
	transition->total_frames = 150000;
	transition->current_frame_number = 0;

	return transition;

}

static void
xmms_transition_show(xmms_transition_t *transition)
{

	XMMS_DBG ("Showing transition");

	if (transition->enabled) {
		XMMS_DBG ("Enabled");
	}

	if (transition->next) {
		XMMS_DBG ("Stacked a level...");
		xmms_transition_show(transition->next);
	}

}


// this adds a plugin to a transition going down the stack
static gboolean
xmms_transition_add_plugin (xmms_transition_plugin_t *plugin, xmms_transition_t *transition)
{
	
	XMMS_DBG ("Adding plugin to transition");

		if (transition->next == NULL) {
			transition->next = xmms_transition_new (plugin);
			return TRUE;
		} else {
			return xmms_transition_add_plugin (plugin, transition->next);
		}
	
	XMMS_DBG ("A bad problem happened adding the plugin to the transition");
	return FALSE;

}


void
xmms_transition_reset (xmms_transition_t *transition)
{
	
	//XMMS_DBG ("Resetting transition");
	transition->current_frame_number = 0;
	if (transition->next != NULL) {
		xmms_transition_reset (transition->next);
	}

}


static gboolean xmms_transition_last_was_enabled (xmms_transition_t *transition)
{

	//XMMS_DBG ("lookin");

	if (transition->next == NULL) {
		return transition->enabled;
	} else {
		return xmms_transition_last_was_enabled (transition->next);
	}

}



void
xmms_transition_set_format (xmms_transition_t *transition, xmms_stream_type_t *format)
{
	
	//XMMS_DBG ("Setting transition format");
	transition->format = format;
	if (transition->next != NULL) {
		xmms_transition_set_format (transition->next, format);
	}

}

void
xmms_transition_set_direction (xmms_transition_t *transition, xmms_transition_direction_t direction)
{
	
	XMMS_DBG ("Setting transition direction");
	transition->direction = direction;
	if (transition->next != NULL) {
		xmms_transition_set_direction (transition->next, direction);
	}

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

void
xmms_transition_destroy (xmms_transition_t *transition)
{

	g_return_if_fail (transition);

	XMMS_DBG ("Destroying transition");

	if (transition->next != NULL) {
		xmms_transition_destroy(transition->next);
	}
	if (transition->in != NULL) {
		xmms_transition_destroy(transition->in);
	}	
	if (transition->out != NULL) {
		xmms_transition_destroy(transition->out);
	}
	if (transition->plugin != NULL) {
		xmms_transition_plugin_method_destroy (transition->plugin, transition);
		//xmms_object_unref (transition->plugin);
	}
	
	g_free (transition);

}



static void
xmms_transition_destroy_final (xmms_transition_t *transition)
{

	g_return_if_fail (transition);

	XMMS_DBG ("Destroying transition");

	if (transition->next != NULL) {
		xmms_transition_destroy_final(transition->next);
	}
	if (transition->in != NULL) {
		xmms_transition_destroy_final(transition->in);
	}	
	if (transition->out != NULL) {
		xmms_transition_destroy_final(transition->out);
	}
	if (transition->plugin != NULL) {
		xmms_transition_plugin_method_destroy (transition->plugin, transition);
		xmms_object_unref (transition->plugin);
	}
	
	g_free (transition);

}


 /** @} */
