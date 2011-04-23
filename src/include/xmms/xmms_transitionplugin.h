/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2009 XMMS2 Team
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




#ifndef _XMMS_TRANSITIONPLUGIN_H_
#define _XMMS_TRANSITIONPLUGIN_H_

#include <glib.h>
#include <string.h> /* for memset() */

#include "xmmsc/xmmsc_idnumbers.h"
#include "xmms/xmms_sample.h"
#include "xmms/xmms_plugin.h"
#include "xmms/xmms_error.h"
#include "xmms/xmms_config.h"
#include "xmms/xmms_streamtype.h"
#include "xmms/xmms_medialib.h"

G_BEGIN_DECLS

/**
 * @defgroup TransitionPlugin TransitionPlugin
 * @ingroup XMMSPlugin
 * @{
 */

typedef enum xmms_transition_direction_E {
	OUT,
	IN
} xmms_transition_direction_t;

typedef struct xmms_transitions_St xmms_transitions_t;
struct xmms_transition_plugin_St;
typedef struct xmms_transition_plugin_St xmms_transition_plugin_t;

typedef struct xmms_transition_St xmms_transition_t;
struct xmms_transition_St {

xmms_transition_direction_t direction; /* single source only */

	xmms_stream_type_t *format;
	int callback;

	xmms_transition_plugin_t *plugin;
	gpointer plugin_data;
	gint duration;
	gint total_frames;
	gint current_frame_number;
	gboolean enabled; /* only for top level */
	xmms_transition_t *next;
	/* dual source only */
	xmms_transition_t *in;
	xmms_transition_t *out;
	
	
	/*
		xmms_ringbuf_t *outring;	// first source
	xmms_ringbuf_t *inring;		// second source
	gboolean readlast;
	void *last;
	*/

};

/*
typedef struct xmms_xtransition_St {
	gboolean setup;
	int current_frame_number;
	int total_frames;
	int final_frame[2][128];
	float current_fade_amount[2][128];
	int lastsign[2][128];
	xmms_stream_type_t *format;
	xmms_ringbuf_t *outring;	// first source
	xmms_ringbuf_t *inring;		// second source
	gboolean readlast;
	void *last;
} xmms_xtransition_t; 
*/

/**
 * The current API version.
 */
#define XMMS_TRANSITION_API_VERSION 1

/**
 * Transition Plugins Yadda
 * Two types single and dual source
 */
typedef struct xmms_transition_methods_St {
	/**
	 * Initiate the transition plugin.
	 *
	 * This function should setup everything that is required to function
	 * Blocking calls in this function are to be
	 * avoided as far as possible as this would freeze the startup of xmms2d.
	 *
	 * @param transition an transition object
	 * @return TRUE on successful init, otherwise FALSE
	 */
	gboolean (*new)(xmms_transition_t *transition);

	/**
	 * Destroy the transition plugin.
	 *
	 * Tear down the data initialized in new.
	 *
	 * @param transition an transition object
	 */
	void (*destroy)(xmms_transition_t *transition);

	/**
	 * Process audio data.
	 *
	 *
	 * @param transition an transition object
	 * @param buffer a buffer with audio data to write to
	 * @param size the number of bytes in the buffer
	 * @param err an error struct
	 */
	int (*process)(xmms_transition_t *transition, gpointer buffer,
	              gint len, xmms_error_t *err);

} xmms_transition_methods_t;

/**
 * Register the transition plugin.
 *
 * @param shname short name of the plugin
 * @param name long name of the plugin
 * @param ver the version of the plugin, usually the XMMS_VERSION macro
 * @param desc a description of the plugin
 * @param setupfunc the function that sets up the plugin functions
 */
#define XMMS_TRANSITION_PLUGIN(shname, name, ver, desc, setupfunc) XMMS_PLUGIN(XMMS_PLUGIN_TYPE_TRANSITION, XMMS_TRANSITION_API_VERSION, shname, name, ver, desc, (gboolean (*)(gpointer))setupfunc)

/**
 * Initialize the #xmms_transition_methods_t struct.
 *
 * This should be run before any functions are associated.
 *
 * @param m the #xmms_transition_methods_t struct to initialize
 */
#define XMMS_TRANSITION_METHODS_INIT(m) memset (&m, 0, sizeof (xmms_transition_methods_t))


/**
 * Register the transition plugin functions.
 *
 * Performs basic validation, see #xmms_transition_methods_St for more information.
 *
 * @param transition a transition plugin object
 * @param methods a struct pointing to the plugin specific functions
 */
void xmms_transition_plugin_methods_set (xmms_transition_plugin_t *transition, xmms_transition_methods_t *methods);


/**
 * Retrieve the private data for the plugin that was set with
 * #xmms_transition_private_data_set.
 *
 * @param transition an transition object
 * @return the private data
 */
gpointer xmms_transition_private_data_get (xmms_transition_t *transition);

/**
 * Set the private data for the plugin that can be retrived
 * with #xmms_transition_private_data_get later.
 *
 * @param transition an transition object
 * @param data the private data
 */
void xmms_transition_private_data_set (xmms_transition_t *transition, gpointer data);


/**
 * Read a number of bytes of data from the transition buffer into a buffer.
 *
 * @param transition an transition object
 * @param buffer a buffer to store the read data in
 * @param len the number of bytes to read
 * @return the number of bytes read
 */
//gint xmms_transition_read (xmms_transition_t *transition, char *buffer, gint len);


/**
 * Register a configuration directive in the plugin setup function.
 *
 * As an optional, but recomended functionality the plugin can decide to
 * subscribe on the configuration value and will thus be notified when it
 * changes by passing a callback, and if needed, userdata.
 *
 * @param plugin an transition plugin object
 * @param name the name of the configuration directive
 * @param default_value the default value of the configuration directive
 * @param cb the function to call on configuration value changes
 * @param userdata a user specified variable to be passed to the callback
 * @return a #xmms_config_property_t based on the given input
 */
xmms_config_property_t *xmms_transition_plugin_config_property_register (xmms_transition_plugin_t *plugin, const gchar *name, const gchar *default_value, xmms_object_handler_t cb, gpointer userdata);

/**
 * Register a configuration directive.
 *
 * As an optional, but recomended functionality the plugin can decide to
 * subscribe on the configuration value and will thus be notified when it
 * changes by passing a callback, and if needed, userdata.
 *
 * @param transition an transition object
 * @param name the name of the configuration directive
 * @param default_value the default value of the configuration directive
 * @param cb the function to call on configuration value changes
 * @param userdata a user specified variable to be passed to the callback
 * @return a #xmms_config_property_t based on the given input
 */

xmms_config_property_t *xmms_transition_config_property_register (xmms_transition_t *transition, const gchar *name, const gchar *default_value, xmms_object_handler_t cb, gpointer userdata);

/**
 * Lookup a configuration directive for the transition plugin.
 *
 * @param transition an transition object
 * @param path the path to the configuration value
 * @return a #xmms_config_property_t found at the given path
 */
xmms_config_property_t *xmms_transition_config_lookup (xmms_transition_t *transition, const gchar *path);

/** @} */

G_END_DECLS

#endif
