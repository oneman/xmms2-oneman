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

/*
 * Private function prototypes -- do NOT use in plugins.
 */



xmms_transitions_t *xmms_transitions_new (void);
void xmms_transitions_destroy (xmms_transitions_t *transitions);

#endif
