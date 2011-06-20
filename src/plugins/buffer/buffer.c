/** @file buffer.c
 *  Double yin-yang playback effect plugin
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

#include "../../includepriv/xmmspriv/xmms_ringbuf.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>


typedef struct {

	gboolean enabled;
	gboolean last_was_buffer;
	gint frame_size;
	guint8 *buffer;
	guint8 *waste_buffer;
	gboolean first_read;
	guint32 offset;
	gint last_ret;
	
	int thread_started;
	GMutex *ringbuffer_mutex;
	xmms_ringbuf_t *ringbuffer;
	GThread *filler_thread;
	xmms_xform_t *xform;
	char read_buffer[4096];
} xmms_buffer_data_t;


static gboolean xmms_buffer_plugin_setup (xmms_xform_plugin_t *xform_plugin);
static gboolean xmms_buffer_init (xmms_xform_t *xform);
static void xmms_buffer_destroy (xmms_xform_t *xform);
static void xmms_buffer_config_changed (xmms_object_t *object, xmmsv_t *_data, gpointer userdata);
static gint xmms_buffer_read (xmms_xform_t *xform, xmms_sample_t *buf, gint len,
                               xmms_error_t *err);
static gint64 xmms_buffer_seek (xmms_xform_t *xform, gint64 offset,
                                 xmms_xform_seek_mode_t whence,
                                 xmms_error_t *err);

/*
 * Plugin header
 */

XMMS_XFORM_PLUGIN ("buffer",
                   "buffer effect", XMMS_VERSION,
                   "buffer effect plugin",
                   xmms_buffer_plugin_setup);

static gboolean
xmms_buffer_plugin_setup (xmms_xform_plugin_t *xform_plugin)
{
	xmms_xform_methods_t methods;

	XMMS_XFORM_METHODS_INIT (methods);
	methods.init = xmms_buffer_init;
	methods.destroy = xmms_buffer_destroy;
	methods.read = xmms_buffer_read;
	methods.seek = xmms_buffer_seek;

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
xmms_buffer_init (xmms_xform_t *xform)
{
	xmms_buffer_data_t *data;
	xmms_config_property_t *config;

	g_return_val_if_fail (xform, FALSE);

	data = g_new0 (xmms_buffer_data_t, 1);

	xmms_xform_private_data_set (xform, data);

	data->ringbuffer = xmms_ringbuf_new (6000000);
	data->ringbuffer_mutex = g_mutex_new ();
	data->frame_size = xmms_sample_size_get(xmms_xform_indata_get_int (xform, XMMS_STREAM_TYPE_FMT_FORMAT)) * xmms_xform_indata_get_int (xform, XMMS_STREAM_TYPE_FMT_CHANNELS);
	//data->first_read = TRUE;
	config = xmms_xform_config_lookup (xform, "enabled");
	g_return_val_if_fail (config, FALSE);
	xmms_config_property_callback_set (config, xmms_buffer_config_changed, data);
	data->enabled = !!xmms_config_property_get_int (config);

	xmms_xform_outdata_type_copy (xform);




	return TRUE;
}

static void
xmms_buffer_destroy (xmms_xform_t *xform)
{
	xmms_buffer_data_t *data;
	xmms_config_property_t *config;

	g_return_if_fail (xform);

	data = xmms_xform_private_data_get (xform);
	g_return_if_fail (data);
	
	xmms_ringbuf_set_eor (data->ringbuffer, true);
	g_thread_join (data->filler_thread);
	xmms_ringbuf_destroy (data->ringbuffer);

	config = xmms_xform_config_lookup (xform, "enabled");
	xmms_config_property_callback_remove (config, xmms_buffer_config_changed, data);

}

static void
xmms_buffer_config_changed (xmms_object_t *object, xmmsv_t *_data, gpointer userdata)
{
	xmms_config_property_t *val;
	xmms_buffer_data_t *data;
	const gchar *name;
	gint value;

	g_return_if_fail (object);
	g_return_if_fail (userdata);

	val = (xmms_config_property_t *) object;
	data = (xmms_buffer_data_t *) userdata;

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

static void *
xmms_buffer_filler (void *arg)
{

	xmms_buffer_data_t *data = (xmms_buffer_data_t *)arg;
	
	xmms_error_t error;
	
	int ret;
	
	char buf[4096];
	
	while (1) {
	
		//xmms_xform_read (data->xform, data->read_buffer, sizeof(data->read_buffer), &error);
		//xmms_ringbuf_write_wait(data->ringbuffer, data->read_buffer, 4096, data->ringbuffer_mutex);
	
		ret = xmms_xform_read (data->xform, buf, sizeof(buf), &error);
		if (ret == 0) {
			xmms_ringbuf_set_eos (data->ringbuffer, true);
			break;
		}
		xmms_ringbuf_write_wait(data->ringbuffer, buf, ret, data->ringbuffer_mutex);
	
		if (xmms_ringbuf_is_eor (data->ringbuffer)) {
			break;
		}
	
	}
	
	return 0;

}

static gint
xmms_buffer_read (xmms_xform_t *xform, void *buffer, gint len,
                   xmms_error_t *error)
{
	
		xmms_buffer_data_t *data;
		int ret;
		
		data = xmms_xform_private_data_get (xform);
	
		
		if (!data->thread_started) {
			data->xform = xform;
			data->filler_thread = g_thread_create (xmms_buffer_filler, data, TRUE, NULL);
			data->thread_started = 1;
		}

		ret = xmms_ringbuf_read_wait (data->ringbuffer, buffer, len, data->ringbuffer_mutex);

		return ret;

			


}

static gint64
xmms_buffer_seek (xmms_xform_t *xform, gint64 offset,
                   xmms_xform_seek_mode_t whence, xmms_error_t *err)
{
	xmms_buffer_data_t *data;
	gint64 ret;
	
	data = xmms_xform_private_data_get (xform);
	
	
	
	data->offset = offset;
	g_mutex_lock(data->ringbuffer_mutex);
	ret = xmms_xform_seek (xform, offset, whence, err);
	xmms_ringbuf_clear (data->ringbuffer);
	g_mutex_unlock(data->ringbuffer_mutex);
	return ret;
}
