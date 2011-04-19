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


#ifndef _XMMS_TRANSITIONPLUGIN_INT_H_
#define _XMMS_TRANSITIONPLUGIN_INT_H_

#include "xmms/xmms_transitionplugin.h"
#include "xmmspriv/xmms_plugin.h"

xmms_plugin_t *xmms_transition_plugin_new (void);
gboolean xmms_transition_plugin_verify (xmms_plugin_t *_plugin);


gboolean xmms_transition_plugin_method_new (xmms_transition_plugin_t *plugin, xmms_transition_t *transition);
void xmms_transition_plugin_method_destroy (xmms_transition_plugin_t *plugin, xmms_transition_t *transition);
gboolean xmms_transition_plugin_method_process (xmms_transition_plugin_t *plugin, xmms_transition_t *transition,
                                               gpointer buffer, gint len, xmms_error_t *err);

#endif
