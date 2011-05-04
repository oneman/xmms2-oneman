/** @file reverse.c
 *  reverse playback effect plugin
 *
 *  Copyright (C) 2006-2011 XMMS2 Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */




#include "xmms/xmms_xformplugin.h"
#include "xmms/xmms_sample.h"
#include "xmms/xmms_log.h"
#include "xmms/xmms_streamtype.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>


typedef struct {

	gboolean enabled;
	gint frame_size;
	guint8 *buffer;
	guint8 *waste_buffer;
	gint offset;
	
} xmms_reverse_data_t;


static gboolean xmms_reverse_plugin_setup (xmms_xform_plugin_t *xform_plugin);
static gboolean xmms_reverse_init (xmms_xform_t *xform);
static void xmms_reverse_destroy (xmms_xform_t *xform);
static void xmms_reverse_config_changed (xmms_object_t *object, xmmsv_t *_data, gpointer userdata);
static gint xmms_reverse_read (xmms_xform_t *xform, xmms_sample_t *buf, gint len,
                               xmms_error_t *err);
static gint64 xmms_reverse_seek (xmms_xform_t *xform, gint64 offset,
                                 xmms_xform_seek_mode_t whence,
                                 xmms_error_t *err);

/*
 * Plugin header
 */

XMMS_XFORM_PLUGIN ("reverse",
                   "reverse effect", XMMS_VERSION,
                   "Reverse effect plugin",
                   xmms_reverse_plugin_setup);

static gboolean
xmms_reverse_plugin_setup (xmms_xform_plugin_t *xform_plugin)
{
	xmms_xform_methods_t methods;

	XMMS_XFORM_METHODS_INIT (methods);
	methods.init = xmms_reverse_init;
	methods.destroy = xmms_reverse_destroy;
	methods.read = xmms_reverse_read;
	methods.seek = xmms_reverse_seek;

	xmms_xform_plugin_methods_set (xform_plugin, &methods);

	xmms_xform_plugin_config_property_register (xform_plugin, "enabled", "1",
	                                            NULL, NULL);

	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "audio/pcm",
	                              XMMS_STREAM_TYPE_FMT_FORMAT,
	                              XMMS_SAMPLE_FORMAT_FLOAT,
	                              XMMS_STREAM_TYPE_END);
	                              
	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "audio/pcm",
	                              XMMS_STREAM_TYPE_FMT_FORMAT,
	                              XMMS_SAMPLE_FORMAT_S16,
	                              XMMS_STREAM_TYPE_END);
	                              
	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "audio/pcm",
	                              XMMS_STREAM_TYPE_FMT_FORMAT,
	                              XMMS_SAMPLE_FORMAT_S32,
	                              XMMS_STREAM_TYPE_END);

	return TRUE;
}

static gboolean
xmms_reverse_init (xmms_xform_t *xform)
{
	xmms_reverse_data_t *data;
	xmms_config_property_t *config;

	g_return_val_if_fail (xform, FALSE);

	data = g_new0 (xmms_reverse_data_t, 1);

	xmms_xform_private_data_set (xform, data);

	data->buffer = g_malloc (40000);
	data->waste_buffer = g_malloc (40000);
	data->frame_size = xmms_sample_size_get(xmms_xform_indata_get_int (xform, XMMS_STREAM_TYPE_FMT_FORMAT)) * xmms_xform_indata_get_int (xform, XMMS_STREAM_TYPE_FMT_CHANNELS);

	config = xmms_xform_config_lookup (xform, "enabled");
	g_return_val_if_fail (config, FALSE);
	xmms_config_property_callback_set (config, xmms_reverse_config_changed, data);
	data->enabled = !!xmms_config_property_get_int (config);

	xmms_xform_outdata_type_copy (xform);

	return TRUE;
}

static void
xmms_reverse_destroy (xmms_xform_t *xform)
{
	xmms_reverse_data_t *data;
	xmms_config_property_t *config;

	g_return_if_fail (xform);

	data = xmms_xform_private_data_get (xform);
	g_return_if_fail (data);

	g_free (data->buffer);
	g_free (data->waste_buffer);

	config = xmms_xform_config_lookup (xform, "enabled");
	xmms_config_property_callback_remove (config, xmms_reverse_config_changed, data);

}

static void
xmms_reverse_config_changed (xmms_object_t *object, xmmsv_t *_data, gpointer userdata)
{
	xmms_config_property_t *val;
	xmms_reverse_data_t *data;
	const gchar *name;
	gint value;

	g_return_if_fail (object);
	g_return_if_fail (userdata);

	val = (xmms_config_property_t *) object;
	data = (xmms_reverse_data_t *) userdata;

	name = xmms_config_property_get_name (val);
	value = xmms_config_property_get_int (val);

	XMMS_DBG ("config value changed! %s => %d", name, value);
	/* we are passed the full config key, not just the last token,
	 * which makes this code kinda ugly.
	 * fix when bug 97 has been resolved
	 */
	name = strrchr (name, '.') + 1;

	if (!strcmp (name, "enabled")) {
		data->enabled = !!value; }
	
}

static gint
xmms_reverse_read (xmms_xform_t *xform, void *buffer, gint len,
                   xmms_error_t *error)
{
	
	xmms_reverse_data_t *data;
	
	guint ret, skip;
	gint16 i, j, f;
	guint8 *output_buffer;
		
	g_return_val_if_fail (xform, -1);

	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, -1);

	if (!data->enabled) {
		
		ret = xmms_xform_read (xform, buffer, len, error);
		data->offset = data->offset + ret / data->frame_size;
		return ret;
			
	} else {
		
		skip = 0;

		ret = xmms_xform_read (xform, data->buffer, len, error);

		output_buffer = buffer;
  				
		for(i=0, j = len - data->frame_size; i < len; i += data->frame_size, j -= data->frame_size)
		{ 
 			for (f=0; f < data->frame_size; f++) {
 				output_buffer[j + f] = data->buffer[i + f];
			}
		}


		data->offset = data->offset - ret / data->frame_size;

		skip = xmms_xform_seek (xform, data->offset, XMMS_XFORM_SEEK_SET, error);

		skip = data->offset - skip;

		/* Skip kludge */
		if (skip > 0) {
			xmms_xform_read (xform, data->waste_buffer, skip, error);
		}

		return ret;

			
	}

}

static gint64
xmms_reverse_seek (xmms_xform_t *xform, gint64 offset,
                   xmms_xform_seek_mode_t whence, xmms_error_t *err)
{
	xmms_reverse_data_t *data;
	data = xmms_xform_private_data_get (xform);
	data->offset = offset;

	return xmms_xform_seek (xform, offset, whence, err);
}
