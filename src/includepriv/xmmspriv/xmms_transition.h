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




#ifndef _XMMS_TRANSITION_INT_H_
#define _XMMS_TRANSITION_INT_H_

#include "xmms/xmms_transitionplugin.h"
#include "xmmspriv/xmms_plugin.h"

typedef enum xmms_transition_state_E {
	STARTING,
	STOPPING,
	PAUSING,
	RESUMING,
	SEEKING,
	JUMPING,
	ADVANCING,
	NONE
} xmms_transition_state_t;

struct xmms_transitions_St {
	xmms_transition_state_t playback_transition_state;
	xmms_transition_t *transitions[7];

};


/*
 * Private function prototypes -- do NOT use in plugins.
 */

xmms_transition_t *xmms_transition_clone (xmms_transition_t *transition);
void xmms_transition_destroy (xmms_transition_t *transition);
void xmms_transition_reset (xmms_transition_t *transition);
void xmms_transition_set_format (xmms_transition_t *transition, xmms_stream_type_t *format);
xmms_transitions_t *xmms_transitions_new (void);
void xmms_transitions_destroy (xmms_transitions_t *transitions);

#endif
