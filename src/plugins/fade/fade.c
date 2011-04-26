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

#include "xmms/xmms_sample.h"
#include "xmms/xmms_streamtype.h"
gint xmms_sample_frame_size_get (const xmms_stream_type_t *st);

gint
xmms_sample_frame_size_get (const xmms_stream_type_t *st)
{
	gint format, channels;
	format = xmms_stream_type_get_int (st, XMMS_STREAM_TYPE_FMT_FORMAT);
	channels = xmms_stream_type_get_int (st, XMMS_STREAM_TYPE_FMT_CHANNELS);
	return xmms_sample_size_get (format) * channels;
}


#include <glib.h>


/*
 *  Defines
 */





/*
 * Type definitions
 */

typedef struct xmms_fade_data_St {
	int final_frame[128];
	float current_fade_amount[128];
	int lastsign[128];
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

	XMMS_DBG ("Fade plugin instance created");
	
	xmms_fade_data_t *data = g_new0 (xmms_fade_data_t, 1);
	
	xmms_transition_private_data_set (transition, data);
	
	
	return TRUE;

}


static void
xmms_fade_destroy (xmms_transition_t *transition)
{

	XMMS_DBG ("Fade plugin instance destroyed");
	
	xmms_fade_data_t *data;
	
	data = xmms_transition_private_data_get (transition);
	
	g_free (data);
	

}


static int
xmms_fade_process (xmms_transition_t *transition, gpointer buffer, gint len,
                              xmms_error_t *err)
{
	
	xmms_fade_data_t *fader;
	
	fader = xmms_transition_private_data_get (transition);

	
	int frames, channels, i, j, extra_frames;
	gint16 *samples_s16;
	gint32 *samples_s32;
	float *samples_float;
	
	frames = len / xmms_sample_frame_size_get(transition->format);

	/* Fading in extra frames are ignored, Fading out they are zero'ed */
	
	if (transition->current_frame_number >= transition->total_frames) {
		return 0;
	}
	
	//XMMS_DBG("Processing %d - %d of %d", transition->current_frame_number, frames + transition->current_frame_number, transition->total_frames );
	
	if ((frames + transition->current_frame_number) >= transition->total_frames) {
		extra_frames = frames - (transition->total_frames - transition->current_frame_number);
		frames = transition->total_frames - transition->current_frame_number;
	}

	samples_s16 = (gint16*)buffer;
	samples_s32 = (gint32*)buffer;
	samples_float = (float*)buffer;

	channels = xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_CHANNELS);

	int sign[channels];
	int finalsign[channels];
	
	/* First Run Setup */
	if (transition->current_frame_number == 0) {
		for (j = 0; j < channels; j++) {
			if (transition->direction == OUT) {
				fader->current_fade_amount[j] = 1.0;
			} else {
				fader->current_fade_amount[j] = 0.0;
			}
			fader->final_frame[j] = transition->total_frames;
		}
	}
	
		
	/*	The following is repeated in the way that it is for maximum performance, as it could 
	 *	be called half a million times per second
	 */


	if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_S16)
	{
	
		samples_s16 = (gint16*)buffer;

		/* First Zero Finder */
		if (transition->current_frame_number == 0) {
			for (j = 0; j < channels; j++) {
				if (samples_s16[0*channels + j] >= 0) {
					fader->lastsign[j] = 1;
				} else {
					fader->lastsign[j] = 0;
				}
			}
		}
		
		
		/* Final Zero Finder */
		if ((frames + transition->current_frame_number) >= transition->total_frames) {
			for (j = 0; j < channels; j++) {
				if (samples_s16[(frames - 1)*channels + j] >= 0) {
					finalsign[j] = 1;
				} else {
					finalsign[j] = 0;
				}
			}
			
			for (j = 0; j < channels; j++) {
				for (i = frames - 1; i > -1; i--) {	
					if (samples_s16[i*channels + j] >= 0) {
						sign[j] = 1;
					} else {
						sign[j] = 0;
					}
			
					if (sign[j] != finalsign[j]) {
						fader->final_frame[j] = transition->current_frame_number + i + 1;
						break;
					}
				}
				if (fader->final_frame[j] == transition->total_frames) {
					XMMS_DBG("No Zero Cross in channel %d final slice.. a tiny artifact may be heard..", j);
				}
			}
		
			if (transition->direction == OUT) {
		
				for (j = 0; j < channels; j++) {
					for (i = (frames - (transition->total_frames - fader->final_frame[j]) - 1); i < frames + extra_frames; i++) {
						samples_s16[i*channels + j] = 0;
					}
				}
			}
		}
		

		for (i = 0; i < frames; i++) {
			for (j = 0; j < channels; j++) {
				
				if ((i + transition->current_frame_number + 1) <= fader->final_frame[j]) {
				
					if (samples_s16[i*channels + j] >= 0) {
						sign[j] = 1;
					} else {
						sign[j] = 0;
					}
			
					if (sign[j] != fader->lastsign[j]) {
				
						if (transition->direction == OUT) {	
							fader->current_fade_amount[j] = ((transition->total_frames - (transition->current_frame_number + i)) * 100.0) / transition->total_frames;
						} else {
							fader->current_fade_amount[j] = ((transition->current_frame_number + i) * 100.0) / transition->total_frames;
						}
						fader->current_fade_amount[j] /= 100.0;
						fader->current_fade_amount[j] *= fader->current_fade_amount[j];
					}

					samples_s16[i*channels + j] = samples_s16[i*channels + j] * fader->current_fade_amount[j];
					fader->lastsign[j] = sign[j];
			
				}
			}
		
		}
	
	} else {
	
		if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_S32)
		{
		
			samples_s32 = (gint32*)buffer;
			
			/* First Zero Finder */
			if (transition->current_frame_number == 0) {
				for (j = 0; j < channels; j++) {
					if (samples_s32[0*channels + j] >= 0) {
						fader->lastsign[j] = 1;
					} else {
						fader->lastsign[j] = 0;
					}
				}
			}
		
		
			/* Final Zero Finder */
			if ((frames + transition->current_frame_number) >= transition->total_frames) {
				for (j = 0; j < channels; j++) {
					if (samples_s32[(frames - 1)*channels + j] >= 0) {
						finalsign[j] = 1;
					} else {
						finalsign[j] = 0;
					}
				}
			
				for (j = 0; j < channels; j++) {
					for (i = frames - 1; i > -1; i--) {	
						if (samples_s32[i*channels + j] >= 0) {
							sign[j] = 1;
						} else {
							sign[j] = 0;
						}
			
						if (sign[j] != finalsign[j]) {
							fader->final_frame[j] = transition->current_frame_number + i + 1;
							break;
						}
					}
					if (fader->final_frame[j] == transition->total_frames) {
						XMMS_DBG("No Zero Cross in channel %d final slice.. a tiny artifact may be heard..", j);
					}
				}
		
					if (transition->direction == OUT) {
			
					for (j = 0; j < channels; j++) {
						for (i = (frames - (transition->total_frames - fader->final_frame[j]) - 1); i < frames + extra_frames; i++) {
							samples_s32[i*channels + j] = 0;
						}
					}
				}
			}

			for (i = 0; i < frames; i++) {
				for (j = 0; j < channels; j++) {
	
					if ((i + transition->current_frame_number + 1) <= fader->final_frame[j]) {
	
						if (samples_s32[i*channels + j] >= 0) {
							sign[j] = 1;
						} else {
							sign[j] = 0;
						}
			
						if (sign[j] != fader->lastsign[j]) {
							if (transition->direction == OUT) {	
								fader->current_fade_amount[j] = ((transition->total_frames - (transition->current_frame_number + i)) * 100.0) / transition->total_frames;
							} else {
								fader->current_fade_amount[j] = ((transition->current_frame_number + i) * 100.0) / transition->total_frames;
							}
							fader->current_fade_amount[j] /= 100.0;
							fader->current_fade_amount[j] *= fader->current_fade_amount[j];
						}
	
						samples_s32[i*channels + j] = samples_s32[i*channels + j] * fader->current_fade_amount[j];
						fader->lastsign[j] = sign[j];

					}
				
				}
		
			}
	
	
		} else if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_FLOAT) {
		
			samples_float = (float*)buffer;
			
			/* First Zero Finder */
			if (transition->current_frame_number == 0) {
				for (j = 0; j < channels; j++) {
					if (samples_float[0*channels + j] >= 0) {
						fader->lastsign[j] = 1;
					} else {
						fader->lastsign[j] = 0;
					}
				}
			}
		
		
			/* Final Zero Finder */
			if ((frames + transition->current_frame_number) >= transition->total_frames) {
				for (j = 0; j < channels; j++) {
					if (samples_float[(frames - 1)*channels + j] >= 0) {
						finalsign[j] = 1;
					} else {
						finalsign[j] = 0;
					}
				}
				
				for (j = 0; j < channels; j++) {
					for (i = frames - 1; i > -1; i--) {	
						if (samples_float[i*channels + j] >= 0) {
							sign[j] = 1;
						} else {
							sign[j] = 0;
						}
			
						if (sign[j] != finalsign[j]) {	
							fader->final_frame[j] = transition->current_frame_number + i + 1;
							break;
						}
					}
					if (fader->final_frame[j] == transition->total_frames) {
						XMMS_DBG("No Zero Cross in channel %d final slice.. a tiny artifact may be heard..", j);
					}
				}
		
				if (transition->direction == OUT) {
			
					for (j = 0; j < channels; j++) {
						for (i = (frames - (transition->total_frames - fader->final_frame[j]) - 1); i < frames + extra_frames; i++) {
							samples_float[i*channels + j] = 0;
						}
					}
				}
			}

			for (i = 0; i < frames; i++) {
				for (j = 0; j < channels; j++) {
				
					if ((i + transition->current_frame_number + 1) <= fader->final_frame[j]) {
	
						if (samples_float[i*channels + j] >= 0) {	
							sign[j] = 1;
						} else {
							sign[j] = 0;
						}
			
						if (sign[j] != fader->lastsign[j]) {
							if (transition->direction == OUT) {	
								fader->current_fade_amount[j] = ((transition->total_frames - (transition->current_frame_number + i)) * 100.0) / transition->total_frames;
							} else {
								fader->current_fade_amount[j] = ((transition->current_frame_number + i) * 100.0) / transition->total_frames;
							}
							fader->current_fade_amount[j] /= 100.0;
							fader->current_fade_amount[j] *= fader->current_fade_amount[j];
						}

						samples_float[i*channels + j] = samples_float[i*channels + j] * fader->current_fade_amount[j];
						fader->lastsign[j] = sign[j];

					}
				}
			}
			
		}
	}
	
	transition->current_frame_number += frames;
	
	
	return len;

}

