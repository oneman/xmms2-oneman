/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003	Peter Alm, Tobias Rundstr�m, Anders Gustafsson
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




#include "xmmspriv/xmms_ringbuf.h"
#include <string.h>

/** @defgroup Ringbuffer Ringbuffer
  * @ingroup XMMSServer
  * @brief Ringbuffer primitive.
  * @{
  */

/** 
 * A ringbuffer
 */
struct xmms_ringbuf_St {
	/** The actual bufferdata */
	guint8 *buffer;
	/** Number of bytes in #buffer */
	gint buffer_size;
	/** Read and write index */
	gint rd_index, wr_index;
	gboolean eos;

	gint hotspot_pos;
	void (*hotspot_callback) (void *);
	void *hotspot_arg;

	GCond *free_cond, *used_cond, *eos_cond;
};


/**
 * The total size of the ringbuffer.
 */
gint
xmms_ringbuf_size (xmms_ringbuf_t *ringbuf)
{
	g_return_val_if_fail (ringbuf, 0);

	return ringbuf->buffer_size;
}

/**
 * Allocate a new ringbuffer
 *
 * @param size The total size of the new ringbuffer
 * @returns a new #xmms_ringbuf_t
 */
xmms_ringbuf_t *
xmms_ringbuf_new (guint size)
{
	xmms_ringbuf_t *ringbuf = g_new0 (xmms_ringbuf_t, 1);

	g_return_val_if_fail (size > 0, NULL);

	ringbuf->buffer_size = size + 1;
	ringbuf->buffer = g_malloc0 (ringbuf->buffer_size);

	ringbuf->free_cond = g_cond_new ();
	ringbuf->used_cond = g_cond_new ();
	ringbuf->eos_cond = g_cond_new ();

	return ringbuf;
}

/**
 * Free all memory used by the ringbuffer
 */
void
xmms_ringbuf_destroy (xmms_ringbuf_t *ringbuf)
{
	g_return_if_fail (ringbuf);

	g_cond_free (ringbuf->eos_cond);
	g_cond_free (ringbuf->used_cond);
	g_cond_free (ringbuf->free_cond);
	
	if (ringbuf->buffer)
		g_free (ringbuf->buffer);

	g_free (ringbuf);
}

/**
 * Clear the ringbuffers data
 */
void
xmms_ringbuf_clear (xmms_ringbuf_t *ringbuf)
{
	g_return_if_fail (ringbuf);

	ringbuf->rd_index = 0;
	ringbuf->wr_index = 0;

	g_cond_signal (ringbuf->free_cond);
}

/**
 * Number of bytes free in the ringbuffer
 */
guint
xmms_ringbuf_bytes_free (const xmms_ringbuf_t *ringbuf)
{
	g_return_val_if_fail (ringbuf, 0);

	if (ringbuf->rd_index > ringbuf->wr_index)
		return (ringbuf->rd_index - ringbuf->wr_index) - 1;
	return (ringbuf->buffer_size - (ringbuf->wr_index - ringbuf->rd_index)) - 1;
}

/**
 * Number of bytes used in the buffer
 */
guint
xmms_ringbuf_bytes_used (const xmms_ringbuf_t *ringbuf)
{
	g_return_val_if_fail (ringbuf, 0);
     
	if (ringbuf->wr_index >= ringbuf->rd_index)
		return ringbuf->wr_index - ringbuf->rd_index;
	return ringbuf->buffer_size - (ringbuf->rd_index - ringbuf->wr_index);     
}

/**
 * Reads data from the ringbuffer. This is a non-blocking call and can
 * return less data than you wanted. Use #xmms_ringbuf_wait_used to
 * ensure that you get as much data as you want.
 *
 * @param ringbuf Buffer to read from
 * @param data Allocated buffer where the readed data will end up
 * @param length number of bytes to read
 * @returns number of bytes that acutally was read.
 */
guint
xmms_ringbuf_read (xmms_ringbuf_t *ringbuf, gpointer data, guint length)
{
	guint to_read, r = 0, cnt;
	guint8 *data_ptr = data;

	g_return_val_if_fail (ringbuf, 0);

	to_read = MIN (length, xmms_ringbuf_bytes_used (ringbuf));

	if (ringbuf->hotspot_callback) {
		if (ringbuf->hotspot_pos != ringbuf->rd_index) {
			/* make sure we don't cross a hotspot */
			to_read = MIN (to_read,
			               (ringbuf->hotspot_pos - ringbuf->rd_index)
			               % ringbuf->buffer_size);
		} else {
			ringbuf->hotspot_callback (ringbuf->hotspot_arg);
			ringbuf->hotspot_callback = NULL;
		}
	}

	while (to_read > 0) {
		cnt = MIN (to_read, ringbuf->buffer_size - ringbuf->rd_index);
		memcpy (data_ptr, ringbuf->buffer + ringbuf->rd_index, cnt);
		ringbuf->rd_index = (ringbuf->rd_index + cnt) % ringbuf->buffer_size;
		to_read -= cnt;
		r += cnt;
		data_ptr += cnt;
	}

	g_cond_broadcast (ringbuf->free_cond);

	return r;
}

/**
 * Same as #xmms_ringbuf_read but does not advance in the buffer after
 * the data has been read.
 *
 * @sa xmms_ringbuf_read
 */
guint
xmms_ringbuf_peek (xmms_ringbuf_t *ringbuf, gpointer data, guint length)
{
	guint ret, tmp;

	g_return_val_if_fail (ringbuf, 0);

	tmp = ringbuf->rd_index;
	ret = xmms_ringbuf_read (ringbuf, data, length);
	ringbuf->rd_index = tmp;

	return ret;
}

/**
 * Same as #xmms_ringbuf_read but blocks until you have all the data you want.
 *
 * @sa xmms_ringbuf_read
 */
guint
xmms_ringbuf_read_wait (xmms_ringbuf_t *ringbuf, gpointer data,
                        guint length, GMutex *mtx)
{
	g_return_val_if_fail (ringbuf, 0);
	g_return_val_if_fail (length > 0, 0);
	g_return_val_if_fail (length <= ringbuf->buffer_size, 0);
	g_return_val_if_fail (mtx, 0);

	xmms_ringbuf_wait_used (ringbuf, length, mtx);

	return xmms_ringbuf_read (ringbuf, data, length);
}

/**
 * Same as #xmms_ringbuf_peek but blocks until you have all the data you want.
 *
 * @sa xmms_ringbuf_peek
 */
guint
xmms_ringbuf_peek_wait (xmms_ringbuf_t *ringbuf, gpointer data,
                        guint length, GMutex *mtx)
{
	g_return_val_if_fail (ringbuf, 0);
	g_return_val_if_fail (length > 0, 0);
	g_return_val_if_fail (length <= ringbuf->buffer_size, 0);
	g_return_val_if_fail (mtx, 0);

	xmms_ringbuf_wait_used (ringbuf, length, mtx);

	return xmms_ringbuf_peek (ringbuf, data, length);
}

/**
 * Write data to the ringbuffer. If not all data can be written 
 * to the buffer the function will not block.
 *
 * @sa xmms_ringbuf_write_wait
 * 
 * @param ringbuf Ringbuffer to put data in.
 * @param data Data to put in ringbuffer
 * @param length Length of #data
 * @returns Number of bytes that was written
 */
guint
xmms_ringbuf_write (xmms_ringbuf_t *ringbuf, gconstpointer data, guint length)
{
	guint to_write, w = 0, cnt;
	const guint8 *data_ptr = data;

	g_return_val_if_fail (ringbuf, 0);

	to_write = MIN (length, xmms_ringbuf_bytes_free(ringbuf));
	while (to_write > 0) {
		cnt = MIN (to_write, ringbuf->buffer_size - ringbuf->wr_index);
		memcpy (&ringbuf->buffer[ringbuf->wr_index], &data_ptr[w], cnt);
		ringbuf->wr_index = (ringbuf->wr_index + cnt) % ringbuf->buffer_size;
		to_write -= cnt;
		w += cnt;
	}

	g_cond_broadcast (ringbuf->used_cond);
	
	return w;
}

/**
 * Same as #xmms_ringbuf_write but blocks until there is enough free space.
 */

guint
xmms_ringbuf_write_wait (xmms_ringbuf_t *ringbuf, gconstpointer data, guint length, GMutex *mtx)
{
	guint written = 0;
	const guint8 *src = data;

	g_return_val_if_fail (ringbuf, 0);
	g_return_val_if_fail (length > 0, 0);
	g_return_val_if_fail (mtx, 0);

	while (written < length) {
		written += xmms_ringbuf_write (ringbuf, src + written, length - written);

		if (written == length || ringbuf->eos) {
			break;
		}

		g_cond_wait (ringbuf->free_cond, mtx);
	}

	return written;
}

/**
 * Block until we have free space in the ringbuffer.
 */
void
xmms_ringbuf_wait_free (const xmms_ringbuf_t *ringbuf, guint len, GMutex *mtx)
{
	g_return_if_fail (ringbuf);
	g_return_if_fail (len > 0);
	g_return_if_fail (mtx);
	g_return_if_fail (ringbuf->free_cond != NULL);
	
	while ((xmms_ringbuf_bytes_free (ringbuf) < len) && !ringbuf->eos)
		g_cond_wait (ringbuf->free_cond, mtx);
}

/**
 * Block until we have used space in the buffer
 */

void
xmms_ringbuf_wait_used (const xmms_ringbuf_t *ringbuf, guint len, GMutex *mtx)
{
	g_return_if_fail (ringbuf);
	g_return_if_fail (len > 0);
	g_return_if_fail (len <= ringbuf->buffer_size);
	g_return_if_fail (mtx);
	g_return_if_fail (ringbuf->used_cond != NULL);

	while ((xmms_ringbuf_bytes_used (ringbuf) < len) && !ringbuf->eos)
		g_cond_wait (ringbuf->used_cond, mtx);
}

/**
 * Tell if the ringbuffer is EOS
 *
 * @returns TRUE if the ringbuffer is EOSed.
 */

gboolean
xmms_ringbuf_iseos (const xmms_ringbuf_t *ringbuf)
{
	g_return_val_if_fail (ringbuf, TRUE);

	if (xmms_ringbuf_bytes_used (ringbuf) > 0)
		return FALSE;
	
	return ringbuf->eos;
}

/**
 * Set EOS flag on ringbuffer.
 */
void
xmms_ringbuf_set_eos (xmms_ringbuf_t *ringbuf, gboolean eos)
{
	g_return_if_fail (ringbuf);

	ringbuf->eos = eos;
	if (eos) {
		g_cond_broadcast (ringbuf->eos_cond);
		g_cond_broadcast (ringbuf->used_cond);
		g_cond_broadcast (ringbuf->free_cond);
	}
}


/**
 * Block until we are EOSed
 */
void
xmms_ringbuf_wait_eos (const xmms_ringbuf_t *ringbuf, GMutex *mtx)
{
	g_return_if_fail (ringbuf);
	g_return_if_fail (mtx);
	g_return_if_fail (ringbuf->eos_cond != NULL);

	while (!xmms_ringbuf_iseos (ringbuf))
		g_cond_wait (ringbuf->eos_cond, mtx);

}
/** @} */

/**
 * @internal 
 * Unused
 */
void
xmms_ringbuf_hotspot_set (xmms_ringbuf_t *ringbuf, void (*cb) (void *), void *arg)
{
	g_return_if_fail (ringbuf);

	g_return_if_fail (!ringbuf->hotspot_callback);

	ringbuf->hotspot_pos = ringbuf->wr_index;
	ringbuf->hotspot_callback = cb;
	ringbuf->hotspot_arg = arg;

}


