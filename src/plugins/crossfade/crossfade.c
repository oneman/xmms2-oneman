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

#include <math.h>

int crossfade_slice_float(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len);
int crossfade_slice_s16(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len);
int crossfade_slice_s32(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len);


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
#include <stdlib.h>


/*
 *  Defines
 */


#define ENOUGH_CHANNELS_FOR_JEAN_LUC_PICARD 128


/*
 * Type definitions
 */

typedef struct xmms_crossfade_data_St {
	int lastsign[2][ENOUGH_CHANNELS_FOR_JEAN_LUC_PICARD];
	float next_fade_amount, next_in_fade_amount;
	float current_fade_amount[2][ENOUGH_CHANNELS_FOR_JEAN_LUC_PICARD];
	int bias, bias_channel;
	
} xmms_crossfade_data_t;


/*
 * Function prototypes
 */

static gboolean xmms_crossfade_plugin_setup (xmms_transition_plugin_t *plugin);
static gboolean xmms_crossfade_new (xmms_transition_t *transition);
static void xmms_crossfade_destroy (xmms_transition_t *transition);
static int xmms_crossfade_mix (xmms_xtransition_t *xtransition, gpointer buffer, gint len,
                              xmms_error_t *err);

/*
 * Plugin header
 */

XMMS_TRANSITION_PLUGIN ("crossfade", "Crossfade Transition", XMMS_VERSION,
                    "Crossfading transition plugin",
                    xmms_crossfade_plugin_setup);

static gboolean
xmms_crossfade_plugin_setup (xmms_transition_plugin_t *plugin)
{
	xmms_transition_methods_t methods;

	XMMS_TRANSITION_METHODS_INIT (methods);

	methods.new = xmms_crossfade_new;
	methods.destroy = xmms_crossfade_destroy;
	//methods.process = xmms_crossfade_process;

	methods.mix = xmms_crossfade_mix;

	xmms_transition_plugin_methods_set (plugin, &methods);

	xmms_transition_plugin_config_property_register (plugin, "testconfigoptionx", "test123",
	                                             NULL, NULL);

	XMMS_DBG ("Crossfade plugin setup");

	return TRUE;
}


/*
 * Member functions
 */


static gboolean
xmms_crossfade_new (xmms_transition_t *transition)
{

	XMMS_DBG ("Crossfade plugin instance created");
	
	xmms_crossfade_data_t *data = g_new0 (xmms_crossfade_data_t, 1);
	
	xmms_transition_private_data_set (transition, data);
	
	
	return TRUE;

}


static void
xmms_crossfade_destroy (xmms_transition_t *transition)
{

	XMMS_DBG ("Crossfade plugin instance destroyed");
	
	xmms_crossfade_data_t *data;
	
	data = xmms_transition_private_data_get (transition);
	
	g_free (data);
	

}


static int
xmms_crossfade_mix (xmms_xtransition_t *xtransition, gpointer buffer, gint len,
                              xmms_error_t *err)
{


xmms_transition_t *transition;

transition = xtransition->transition;

			//XMMS_DBG ("cossfade called with %d", len);
			xmms_xtransition_t *oldtrans;
			oldtrans = xtransition->last;
			
	int bytes, ret, clen;
		
		guint8 oldbuffer[8192];

		if (xtransition->setup == FALSE) {


			//bytes = xmms_ringbuf_bytes_used (transition->outring);

			transition->current_frame_number = 0;
			//transition->total_frames = bytes / xmms_sample_frame_size_get(transition->format);


		srand (time (NULL));
		transition->total_frames = (rand () % 125000) + 25001;
		XMMS_DBG ("frames: %d", transition->total_frames);


			//transition->total_frames = 50000;


			xtransition->setup = TRUE;
			//XMMS_DBG ("Got %d frames for cross transition", transition->total_frames);
			
		
		}
		
		clen = MIN (len, ((transition->total_frames - transition->current_frame_number) * xmms_sample_frame_size_get(transition->format)));
		
		// ok so if we are readin old we read old
		if (xtransition->readlast) {
			//XMMS_DBG ("reading old in the old way");
			ret = xmms_crossfade_mix(oldtrans, &oldbuffer, clen, err);
			//ret = xmms_ringbuf_read (transition->outring, &oldbuffer, clen);
			//XMMS_DBG ("got old %d " , ret);
			
			if (( ret < clen ) && (oldtrans->setup == false)) {
			
						XMMS_DBG ("old transition did finish, switching to ring, needed %d more", clen - ret);
						xtransition->readlast = false;
						ret = xmms_ringbuf_read (xtransition->inring, &oldbuffer + ret, clen - ret);
						XMMS_DBG ("got ring old %d " , ret);
			
			}
			
			
			if (( ret < clen ) && (oldtrans->setup == true)) {
			
						XMMS_DBG ("something fucked up");
			
			
			}
			
			
			if (oldtrans->setup == false) {
			
						XMMS_DBG ("old transition is old goodbye");
									xtransition->readlast = false;
			
			}
			
			
		} else {
			ret = xmms_ringbuf_read (xtransition->outring, &oldbuffer, clen);
			//XMMS_DBG ("got old %d " , ret);	
		}
		
		ret = xmms_ringbuf_read (xtransition->inring, buffer, clen);
		
		//XMMS_DBG ("got new %d " , ret);
		
		//XMMS_DBG ("crossfading %d " , transition->current_frame_number + (len / xmms_sample_frame_size_get(transition->format)));

		if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_S16)
		{
			ret = crossfade_slice_s16(transition, &oldbuffer, buffer, buffer, clen);
		}
				
		if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_S32)
		{
			ret = crossfade_slice_s32(transition, &oldbuffer, buffer, buffer, clen);
		}
				
		if (xmms_stream_type_get_int(transition->format, XMMS_STREAM_TYPE_FMT_FORMAT) == XMMS_SAMPLE_FORMAT_FLOAT)
		{
			ret = crossfade_slice_float(transition, &oldbuffer, buffer, buffer, clen);			
		}


		transition->current_frame_number = transition->current_frame_number + (len / xmms_sample_frame_size_get(transition->format));

		/* Finish Transition */

		if (clen < len) {
			//XMMS_DBG ("extra %d", len - clen);
			xmms_ringbuf_read (xtransition->inring, buffer + clen, len - clen);
		}

		if (transition->current_frame_number >= transition->total_frames) {
		// kill the old
					if (xtransition->readlast) { 
					  if(oldtrans->setup == true) {
					 			XMMS_DBG ("had to kill the young before the old");
							oldtrans->readlast = false;
							oldtrans->setup = false;
						}
					}
		
			xmms_ringbuf_set_eor(xtransition->outring, true);
			xtransition->setup = FALSE;
			xtransition->readlast = FALSE;
			XMMS_DBG ("done with cross transition");
		}		
		
	return len;

}



int
crossfade_slice_float(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len) {

	xmms_crossfade_data_t *data;

	data = xmms_transition_private_data_get (transition);
	
	float *samples_from = (float*)sample_buffer_from;
	float *samples_to = (float*)sample_buffer_to;
	float *faded_samples = (float*)faded_sample_buffer;

	/* well as with the sample type we dont really want to hard code the number of channels */
	int number_of_channels, frames_in_slice;
	number_of_channels = 2;
	
	frames_in_slice = len / xmms_sample_frame_size_get(transition->format);

	

	int i ,j, n;
	float next_fade_amount, next_in_fade_amount;
		float next_fade_amount_biased, next_in_fade_amount_biased;
	int sign[2][number_of_channels];
	
	if (transition->current_frame_number == 0) {
	for (n = 0; n < 2; n++) {
		for (j = 0; j < number_of_channels; j++) {
			data->current_fade_amount[n][j] = 0;
			sign[n][j] = 0;
			data->lastsign[n][j] = -1;
		}
	}
	}
	
	int tcfn, ttfn;
	int bias, biased_cfn, biased_tfn, bias_channel;
	tcfn = transition->current_frame_number;
	ttfn = transition->total_frames;




	if (transition->current_frame_number == 0) {
		srand (time (NULL));
		bias = (rand () % 6) + 1;
		bias_channel = rand () % number_of_channels;
		data->bias = bias;
		data->bias_channel = bias_channel;
		XMMS_DBG ("bias amount: %d bias channel: %d", bias, bias_channel);
	} else {
		bias = data->bias;
		bias_channel = data->bias_channel;
	}



	biased_cfn = transition->current_frame_number * bias;
	biased_tfn = transition->total_frames / bias;


	int miss1, miss2;
	
	miss1 = true;
		miss2 = true;



	for (i = 0; i < frames_in_slice; i++) {
		for (j = 0; j < number_of_channels; j++) {


				next_in_fade_amount = cos(3.14159*0.5*(((float)(i + (tcfn)))  + 0.5)/(float)ttfn);

				next_in_fade_amount = next_in_fade_amount * next_in_fade_amount;
				next_in_fade_amount = 1 - next_in_fade_amount;
											
				next_fade_amount = cos(3.14159*0.5*(((float)(i + (tcfn))) + 0.5)/(float)ttfn);

				next_fade_amount = next_fade_amount * next_fade_amount;
				
				
				next_in_fade_amount_biased = cos(3.14159*0.5*(((float)((i * bias) + (biased_cfn)))  + 0.5)/(float)ttfn);

				next_in_fade_amount_biased = next_in_fade_amount_biased * next_in_fade_amount_biased;
				next_in_fade_amount_biased = 1 - next_in_fade_amount_biased;
											
				next_fade_amount_biased = cos(3.14159*0.5*(((float)((i * bias) + (biased_cfn))) + 0.5)/(float)ttfn);

				next_fade_amount_biased = next_fade_amount_biased * next_fade_amount_biased;
				
			

			if (transition->current_frame_number == 0) {
				data->current_fade_amount[1][j] = next_in_fade_amount;
				data->current_fade_amount[0][j] = next_fade_amount;
			}
		
	
		if (samples_from[i*number_of_channels + j] >= 0) {
				sign[0][j] = 1;
			} else {
				sign[0][j] = 0;
			}
		

			if (samples_to[i*number_of_channels + j] >= 0) {
				sign[1][j] = 1;
			} else {
				sign[1][j] = 0;
			}




//if (((j == 1) && (transition->current_frame_number >= (bias))) || ((j == 0) && (transition->current_frame_number < (bias)))) 
//{
	for (n = 0; n < 2; n++) {	
			
			if(data->lastsign[n][j] == -1)
				data->lastsign[n][j] = sign[n][j];
			
			if (sign[n][j] != data->lastsign[n][j]) {
			 if(n == 1) {
			// XMMS_DBG("fade amount in is: %f ", next_in_fade_amount);
				if (j == bias_channel) {
					if ((biased_cfn + (i * bias) ) <= (transition->total_frames)) {
					 data->current_fade_amount[n][j] = next_in_fade_amount_biased;
					 					miss1 = false;
					}

				} else {
					data->current_fade_amount[n][j] = next_in_fade_amount;
				}
			} else {
			
				if (j == bias_channel) {
					if ((biased_cfn + (i * bias) ) <= (transition->total_frames)) {
					  data->current_fade_amount[n][j] = next_fade_amount_biased;
					  					miss2 = false;
					}
					
				} else {
					data->current_fade_amount[n][j] = next_fade_amount;
				}
			
			}
				//XMMS_DBG ("Zero Crossing at");
			/*	if(current_fade_amount[n][j] < 0)
					current_fade_amount[n][j] = 0;
				if(current_fade_amount[n][j] > 100)
					current_fade_amount[n][j] = 100;
			*/
			}

		}
		//}
		


			faded_samples[i*number_of_channels + j] = ((((samples_from[i*number_of_channels + j] * data->current_fade_amount[0][j]) ) + ((samples_to[i*number_of_channels + j] * data->current_fade_amount[1][j]) )) );

			data->lastsign[0][j] = sign[0][j];
			data->lastsign[1][j] = sign[1][j];

		}
		
	}

		if ((biased_cfn + (i * bias) ) <= (transition->total_frames) && ((miss1) || (miss2))) 
			XMMS_DBG ("I missed");

	return len;

}


int
crossfade_slice_s16(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len) {

	/* ok lets handle those void * and hard code it to float for now, is it possible to cast without a new var?? */
	
	gint16 *samples_from = (gint16*)sample_buffer_from;
	gint16 *samples_to = (gint16*)sample_buffer_to;
	gint16 *faded_samples = (gint16*)faded_sample_buffer;

	/* well as with the sample type we dont really want to hard code the number of channels */
	int number_of_channels, frames_in_slice;
	number_of_channels = 2;
	
	frames_in_slice = len / xmms_sample_frame_size_get(transition->format);
	

	int i ,j, n;
	int sign[2][number_of_channels], lastsign[2][number_of_channels];
	float next_fade_amount, next_in_fade_amount;
	float current_fade_amount[2][number_of_channels];
	
	for (n = 0; n < 2; n++) {
		for (j = 0; j < number_of_channels; j++) {
			current_fade_amount[n][j] = 0;
			sign[n][j] = 0;
			lastsign[n][j] = -1;
		}
	}
	
	int fuckit;
	fuckit = 0;
	

	for (i = 0; i < frames_in_slice; i++) {
		for (j = 0; j < number_of_channels; j++) {



				next_in_fade_amount = cos(3.14159*0.5*(((float)(i + (transition->current_frame_number)))  + 0.5)/(float)transition->total_frames);

				next_in_fade_amount = next_in_fade_amount * next_in_fade_amount;
				next_in_fade_amount = 1 - next_in_fade_amount;

				
				if (fuckit == 1) {
					next_in_fade_amount = 100;
				}

							
				
				next_fade_amount = cos(3.14159*0.5*(((float)(i + (transition->current_frame_number))) + 0.5)/(float)transition->total_frames);

				next_fade_amount = next_fade_amount * next_fade_amount;

				
				if (fuckit == 1) {
					next_fade_amount = 0;
				}
			//}
			
			// oh knows this could cause us to fade a different amount on a non zero crossing
			if (current_fade_amount[0][j] == 0) {
				current_fade_amount[1][j] = next_in_fade_amount;
				current_fade_amount[0][j] = next_fade_amount;
			}
		
	
		if (samples_from[i*number_of_channels + j] >= 0) {
				sign[0][j] = 1;
			} else {
				sign[0][j] = 0;
			}
		

			if (samples_to[i*number_of_channels + j] >= 0) {
				sign[1][j] = 1;
			} else {
				sign[1][j] = 0;
			}

	for (n = 0; n < 2; n++) {	
			
			if(lastsign[n][j] == -1)
				lastsign[n][j] = sign[n][j];
			
			if (sign[n][j] != lastsign[n][j]) {
			 if(n == 1) {
			 	//XMMS_DBG("fade amount in is: %f ", next_in_fade_amount);
				current_fade_amount[n][j] = next_in_fade_amount;
			} else {
				current_fade_amount[n][j] = next_fade_amount;
			}
				//XMMS_DBG ("Zero Crossing at %d", current_sample_number);
				if(current_fade_amount[n][j] < 0)
					current_fade_amount[n][j] = 0;
				if(current_fade_amount[n][j] > 100)
					current_fade_amount[n][j] = 100;
			}

		}

			faded_samples[i*number_of_channels + j] = ((((samples_from[i*number_of_channels + j] * current_fade_amount[0][j]) ) + ((samples_to[i*number_of_channels + j] * current_fade_amount[1][j]) )) );

			lastsign[0][j] = sign[0][j];
			lastsign[1][j] = sign[1][j];

		}
		
	}

	return len;

}


int
crossfade_slice_s32(xmms_transition_t *transition, void *sample_buffer_from, void *sample_buffer_to, void *faded_sample_buffer, int len) {

	/* ok lets handle those void * and hard code it to float for now, is it possible to cast without a new var?? */
	
	gint *samples_from = (gint*)sample_buffer_from;
	gint *samples_to = (gint*)sample_buffer_to;
	gint *faded_samples = (gint*)faded_sample_buffer;

	/* well as with the sample type we dont really want to hard code the number of channels */
	int number_of_channels, frames_in_slice;
	number_of_channels = 2;
	
	frames_in_slice = len / xmms_sample_frame_size_get(transition->format);
	

	int i ,j, n;
	int sign[2][number_of_channels], lastsign[2][number_of_channels];
	float next_fade_amount, next_in_fade_amount;
	float current_fade_amount[2][number_of_channels];
	
	for (n = 0; n < 2; n++) {
		for (j = 0; j < number_of_channels; j++) {
			current_fade_amount[n][j] = 0;
			sign[n][j] = 0;
			lastsign[n][j] = -1;
		}
	}
	
	int fuckit;
	fuckit = 0;
	
	for (i = 0; i < frames_in_slice; i++) {
		for (j = 0; j < number_of_channels; j++) {



				next_in_fade_amount = cos(3.14159*0.5*(((float)(i + (transition->current_frame_number)))  + 0.5)/(float)transition->total_frames);

				next_in_fade_amount = next_in_fade_amount * next_in_fade_amount;
				next_in_fade_amount = 1 - next_in_fade_amount;

				
				if (fuckit == 1) {
					next_in_fade_amount = 100;
				}

							
				
				next_fade_amount = cos(3.14159*0.5*(((float)(i + (transition->current_frame_number))) + 0.5)/(float)transition->total_frames);

				next_fade_amount = next_fade_amount * next_fade_amount;

				
				if (fuckit == 1) {
					next_fade_amount = 0;
				}

			
			// oh knows this could cause us to fade a different amount on a non zero crossing
			if (current_fade_amount[0][j] == 0) {
				current_fade_amount[1][j] = next_in_fade_amount;
				current_fade_amount[0][j] = next_fade_amount;
			}
		
	
		if (samples_from[i*number_of_channels + j] >= 0) {
				sign[0][j] = 1;
			} else {
				sign[0][j] = 0;
			}
		

			if (samples_to[i*number_of_channels + j] >= 0) {
				sign[1][j] = 1;
			} else {
				sign[1][j] = 0;
			}

	for (n = 0; n < 2; n++) {	
			
			if(lastsign[n][j] == -1)
				lastsign[n][j] = sign[n][j];
			
			if (sign[n][j] != lastsign[n][j]) {
			 if(n == 1) {
			 	//XMMS_DBG("fade amount in is: %f ", next_in_fade_amount);
				current_fade_amount[n][j] = next_in_fade_amount;
			} else {
				current_fade_amount[n][j] = next_fade_amount;
			}
				//XMMS_DBG ("Zero Crossing at %d", current_sample_number);
				if(current_fade_amount[n][j] < 0)
					current_fade_amount[n][j] = 0;
				if(current_fade_amount[n][j] > 100)
					current_fade_amount[n][j] = 100;
			}

		}

			faded_samples[i*number_of_channels + j] = ((((samples_from[i*number_of_channels + j] * current_fade_amount[0][j]) ) + ((samples_to[i*number_of_channels + j] * current_fade_amount[1][j]) )) );

			lastsign[0][j] = sign[0][j];
			lastsign[1][j] = sign[1][j];

		}
		
	}

	return len;

}

