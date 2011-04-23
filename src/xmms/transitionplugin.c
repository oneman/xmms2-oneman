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

#include "xmmspriv/xmms_transitionplugin.h"
#include "xmmspriv/xmms_plugin.h"
#include "xmms/xmms_log.h"

struct xmms_transition_plugin_St {
	xmms_plugin_t plugin;
	xmms_transition_methods_t methods;
};




static void
xmms_transition_plugin_destroy (xmms_object_t *obj)
{
	/* xmms_transition_plugin_t *plugin = (xmms_transition_plugin_t *)obj; */

	xmms_plugin_destroy ((xmms_plugin_t *)obj);
}


xmms_plugin_t *
xmms_transition_plugin_new (void)
{
	xmms_transition_plugin_t *res;

	res = xmms_object_new (xmms_transition_plugin_t, xmms_transition_plugin_destroy);

	return (xmms_plugin_t *)res;
}


void
xmms_transition_plugin_methods_set (xmms_transition_plugin_t *plugin,
                                xmms_transition_methods_t *methods)
{
	g_return_if_fail (plugin);
	g_return_if_fail (plugin->plugin.type == XMMS_PLUGIN_TYPE_TRANSITION);

	XMMS_DBG ("Registering transition '%s'",
	          xmms_plugin_shortname_get ((xmms_plugin_t *)plugin));

	memcpy (&plugin->methods, methods, sizeof (xmms_transition_methods_t));
}


gboolean
xmms_transition_plugin_verify (xmms_plugin_t *_plugin)
{
	xmms_transition_plugin_t *plugin = (xmms_transition_plugin_t *)_plugin;

	g_return_val_if_fail (plugin, FALSE);
	g_return_val_if_fail (_plugin->type == XMMS_PLUGIN_TYPE_TRANSITION, FALSE);

	if (!(plugin->methods.new &&
	      plugin->methods.destroy &&
	      plugin->methods.process)) {
		XMMS_DBG ("Missing: new, destroy or process!");
		return FALSE;
	}

	return TRUE;
}


xmms_config_property_t *
xmms_transition_plugin_config_property_register (xmms_transition_plugin_t *plugin,
                                             const gchar *name,
                                             const gchar *default_value,
                                             xmms_object_handler_t cb,
                                             gpointer userdata)
{
	xmms_plugin_t *p = (xmms_plugin_t *) plugin;

	return xmms_plugin_config_property_register (p, name, default_value,
	                                             cb, userdata);
}


gboolean
xmms_transition_plugin_method_new (xmms_transition_plugin_t *plugin,
                               xmms_transition_t *transition)
{
	gboolean ret = TRUE;

	g_return_val_if_fail (transition, FALSE);
	g_return_val_if_fail (plugin, FALSE);

	if (plugin->methods.new) {
		ret = plugin->methods.new (transition);
	}

	return ret;
}


void
xmms_transition_plugin_method_destroy (xmms_transition_plugin_t *plugin,
                                   xmms_transition_t *transition)
{
	g_return_if_fail (transition);
	g_return_if_fail (plugin);

	if (plugin->methods.destroy) {
		plugin->methods.destroy (transition);
	}
}


gboolean xmms_transition_plugin_method_process (xmms_transition_plugin_t *plugin, xmms_transition_t *transition,
                                               gpointer buffer, gint len, xmms_error_t *err)
{

	g_return_val_if_fail (transition, FALSE);
	g_return_val_if_fail (plugin, FALSE);

	plugin->methods.process (transition, buffer, len, err);
	return TRUE;
}
