/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2007 XMMS2 Team
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
  * @file gme decoder.
  * @url http://wiki.xmms2.xmms.se/index.php/Notes_from_developing_an_xform_plugin
  */

#include "xmms/xmms_xformplugin.h"
#include "xmms/xmms_sample.h"
#include "xmms/xmms_log.h"
#include "xmms/xmms_medialib.h"

#include "gme/gme.h"

#include <glib.h>
#include <stdlib.h>

/* 
 * Persistent data
 * 
 */
typedef struct xmms_gme_data_St {
	Music_Emu *emu; /* An emulation instance for the GME library */
} xmms_gme_data_t;

/*
 * Function prototypes
 */

static gboolean xmms_gme_plugin_setup (xmms_xform_plugin_t *xform_plugin);
static gint xmms_gme_read (xmms_xform_t *xform, xmms_sample_t *buf, gint len, xmms_error_t *err);
static gboolean xmms_gme_init (xmms_xform_t *decoder);
static void xmms_gme_destroy (xmms_xform_t *decoder);
#if 0 /* Works in progress */
static gint64 xmms_gme_seek (xmms_xform_t *xform, gint64 samples, xmms_xform_seek_mode_t whence, xmms_error_t *err);
static gboolean xmms_gme_browse (xmms_xform_t *xform, const gchar *url, xmms_error_t *error);
#endif

/*
 * Plugin header
 */

XMMS_XFORM_PLUGIN ("gme",
                   "Game Music decoder", XMMS_VERSION,
                   "Game Music Emulator music decoder",
                   xmms_gme_plugin_setup);

static gboolean
xmms_gme_plugin_setup (xmms_xform_plugin_t *xform_plugin)
{
	xmms_xform_methods_t methods;

	XMMS_XFORM_METHODS_INIT (methods);
	methods.init = xmms_gme_init;
	methods.destroy = xmms_gme_destroy;
	methods.read = xmms_gme_read;
#if 0
	methods.seek = xmms_gme_seek;
	methods.browse = xmms_gme_browse;
#endif

	xmms_xform_plugin_methods_set (xform_plugin, &methods);

	xmms_xform_plugin_config_property_register (xform_plugin, "loops", "2", NULL, NULL);
	xmms_xform_plugin_config_property_register (xform_plugin, "maxlength", "300", NULL, NULL);

	/* todo: add other mime types */
	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "application/x-spc",
	                              NULL);

	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "application/x-nsf",
	                              NULL);

	xmms_xform_plugin_indata_add (xform_plugin,
	                              XMMS_STREAM_TYPE_MIMETYPE,
	                              "application/x-gbs",
	                              NULL);

	/* todo: add other magic */
	xmms_magic_add ("SPC700 save state",
	                "application/x-spc",
	                "0 string SNES-SPC700 Sound File Data",
	                NULL);

	xmms_magic_add ("NSF file",
	                "application/x-nsf",
	                "0 string NESM",
	                NULL);

	xmms_magic_add ("GBS file",
	                "application/x-gbs",
	                "0 string GBS",
	                NULL);

	/* todo: add other file extensions */
	xmms_magic_extension_add ("application/x-spc", "*.spc");
	xmms_magic_extension_add ("application/x-nsf", "*.nsf");
	xmms_magic_extension_add ("application/x-gbs", "*.gbs");

	return TRUE;
}

static gboolean
xmms_gme_init (xmms_xform_t *xform)
{
	xmms_gme_data_t *data;
	gme_err_t init_error;
	GString *file_contents; /* The raw data from the file. */
	track_info_t metadata;
	xmms_config_property_t *val;
	int loops;
	int maxlength;
	const char *subtune_str;
	int subtune = 0;
	long fadelen = -1;


	g_return_val_if_fail (xform, FALSE);

	data = g_new0 (xmms_gme_data_t, 1);

	xmms_xform_private_data_set (xform, data);

	xmms_xform_outdata_type_add (xform,
	                             XMMS_STREAM_TYPE_MIMETYPE,
	                             "audio/pcm", /* PCM samples */
	                             XMMS_STREAM_TYPE_FMT_FORMAT,
	                             XMMS_SAMPLE_FORMAT_S16, /* 16-bit signed */
	                             XMMS_STREAM_TYPE_FMT_CHANNELS,
	                             2, /* stereo */
	                             XMMS_STREAM_TYPE_FMT_SAMPLERATE,
	                             44100, /* 44 kHz */
	                             XMMS_STREAM_TYPE_END);

	file_contents = g_string_new ("");

	for (;;) {
		xmms_error_t error;
		gchar buf[4096];
		gint ret;

		ret = xmms_xform_read (xform, buf, sizeof (buf), &error);
		if (ret == -1) {
			XMMS_DBG ("Error reading emulated music data");
			return FALSE;
		}
		if (ret == 0) {
			break;
		}
		g_string_append_len (file_contents, buf, ret);
	}

	init_error = gme_open_data (file_contents->str, file_contents->len, &data->emu, 44100);

	if (init_error) {
		XMMS_DBG ("gme_open_data returned an error: %s", init_error);
		return FALSE;
	}

	if (xmms_xform_metadata_get_str (xform, "subtune", &subtune_str)) {
		subtune = strtol (subtune_str, NULL, 10);
		XMMS_DBG ("Setting subtune to %d", subtune);
		if ((subtune < 0 || subtune > gme_track_count (data->emu))) {
			XMMS_DBG ("Invalid subtune index");
			return FALSE;
		}
	}
	else {
		xmms_xform_metadata_set_int (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_SUBTUNES, gme_track_count (data->emu));
	}

	/*
	 *  Get metadata here
	 */
	if ((init_error = gme_track_info (data->emu, &metadata, subtune))) {
		XMMS_DBG ("Couldn't get GME track info: %s", init_error);
		init_error = "";
	}
	else {
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_TITLE, metadata.song);
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_ARTIST, metadata.author);
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_ALBUM, metadata.game);
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_COMMENT, metadata.comment);
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_YEAR, metadata.copyright);
		xmms_xform_metadata_set_str (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_GENRE, metadata.system);  /* I mapped genre to the system type */

		val = xmms_xform_config_lookup (xform, "loops");
		loops = xmms_config_property_get_int (val);

		XMMS_DBG ("intro_length = %ld, loops = %d, loop_length = %ld", metadata.intro_length, loops, metadata.loop_length);

		if (metadata.intro_length > 0) {
			if ((loops > 0) && (metadata.loop_length > 0)) {
				fadelen = metadata.intro_length + loops * metadata.loop_length;
				XMMS_DBG ("fadelen now = %ld", fadelen);
			}
			else {
				fadelen = metadata.length;
				XMMS_DBG ("fadelen now = %ld", fadelen);
			}
		}
	}

	val = xmms_xform_config_lookup (xform, "maxlength");
	maxlength = xmms_config_property_get_int (val);

	XMMS_DBG ("maxlength = %d seconds", maxlength);

	if (maxlength > 0 && (fadelen < 0 || (maxlength * 1000L < fadelen))) {
		fadelen = maxlength * 1000L;
		XMMS_DBG ("fadelen now = %ld", fadelen);
	}

	XMMS_DBG ("gme.fadelen = %ld", fadelen);

	if ((init_error = gme_start_track (data->emu, subtune))) {
		XMMS_DBG ("gme_start_track returned an error: %s", init_error);
		return FALSE;
	}

	if (fadelen > 0) {
		XMMS_DBG ("Setting song length and fade length...");
		xmms_xform_metadata_set_int (xform, XMMS_MEDIALIB_ENTRY_PROPERTY_DURATION, fadelen);
		gme_set_fade (data->emu, fadelen);
	}

	g_string_free (file_contents, TRUE);

	return TRUE;
}

/*
 * Don't be like Photoshop!
 * Give back what you borrowed!
 */
static void
xmms_gme_destroy (xmms_xform_t *xform)
{

	xmms_gme_data_t *data;

	g_return_if_fail (xform);

	data = xmms_xform_private_data_get (xform);
	g_return_if_fail (data);

	if (data->emu)
		gme_delete (data->emu);

	g_free (data);

}

/* Read some data */
static gint
xmms_gme_read (xmms_xform_t *xform, xmms_sample_t *buf, gint len, xmms_error_t *err)
{
	xmms_gme_data_t *data;
	gme_err_t play_error;

	g_return_val_if_fail (xform, -1);

	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, -1);

	if (gme_track_ended (data->emu))
		return 0;

	if ((play_error = gme_play (data->emu, len/2, buf))) {
		XMMS_DBG ("gme_play returned an error: %s", play_error);
		xmms_error_set (err, XMMS_ERROR_GENERIC, play_error);
		return -1;
	}

	return len;
}

/*
 * This function is going to be just a bit on the messy side, so I'll implement it later
 */
#if 0
static gint64
xmms_gme_seek (xmms_xform_t *xform, gint64 samples, xmms_xform_seek_mode_t whence, xmms_error_t *err)
{
	xmms_gme_data_t *data;

	g_return_val_if_fail (xform, FALSE);

	data = xmms_xform_private_data_get (xform);
	g_return_val_if_fail (data, FALSE);

	return -1;
}
#endif

/*
 * This function adds subtunes to the playlist
 */
#if 0 /* Will begin notes on this method on the wiki so I can get it figured out */
static gboolean
xmms_gme_browse (xmms_xform_t *xform, const gchar *url, xmms_error_t *error)
{
	int tracks;

	g_return_val_if_fail (xform, FALSE);

	tracks = gme_track_count (/* some emu from somewhere */);

	return TRUE;
}
#endif
