/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>

#include <zephyr/mp/mp_caps.h>
#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_dispatch.h>
#include <zephyr/mp/mp_element.h>
#include <zephyr/mp/mp_value.h>

#include <zephyr/mp/img/mp_img_jpeg_parser.h>

LOG_MODULE_REGISTER(mp_img_jpeg_parser, CONFIG_MP_LOG_LEVEL);

/*
 * Internal output pool which is static and simple. For specific requirements, e.g. alignment,
 * the proposed downstream pool will be used
 */
NET_BUF_POOL_FIXED_DEFINE(mp_img_jpeg_parser_pool, CONFIG_MP_IMG_JPEG_PARSER_POOL_NUM,
			  CONFIG_MP_IMG_JPEG_PARSER_MAX_FRAME_SIZE, sizeof(struct mp_buffer_meta),
			  mp_buffer_destroy);

#define JPEG_EOI_BYTE0 0xFFU
#define JPEG_EOI_BYTE1 0xD9U

static int mp_img_jpeg_parser_decide_allocation(struct mp_parser *parser, struct mp_dispatch *query)
{
	struct mp_img_jpeg_parser *jpeg_parser = (struct mp_img_jpeg_parser *)parser;
	struct mp_buffer_pool *query_pool = mp_dispatch_get_pool(query);

	/* Use the internal pool by default */
	if (CONFIG_MP_IMG_JPEG_PARSER_MAX_FRAME_SIZE > 0) {
		parser->outpool = &jpeg_parser->out_pool;
	}

	/* Use the proposed pool from downstream when available */
	if (query_pool != NULL) {
		/* Add one extra buffer to hold the parser's partial frame */
		query_pool->config.min_buffers += 1;
		parser->outpool = query_pool;
	}

	if (parser->outpool == NULL) {
		return -ENOMEM;
	}

	/* TODO: Do negotiation when downstream only propose pool config */

	return 0;
}

static uint8_t *find_jpeg_eoi(const uint8_t *data, size_t len)
{
	if (data == NULL || len < 2) {
		return NULL;
	}

	for (size_t i = 0; i < len - 1; i++) {
		if (data[i] == JPEG_EOI_BYTE0 && data[i + 1] == JPEG_EOI_BYTE1) {
			return (uint8_t *)&data[i];
		}
	}

	return NULL;
}

static inline void set_bytes_used(struct net_buf *buf, uint32_t used)
{
	struct mp_buffer_meta *bm = mp_buffer_get_meta(buf);

	bm->bytes_used = used;
	buf->len = used;

	/* Keep Zephyr video_buffer (if any) in sync for vid_transform/video_enqueue */
	/* TODO: Ensure driver_buf is a video_buffer */
	if (bm->driver_buf != NULL) {
		((struct video_buffer *)bm->driver_buf)->bytesused = used;
	}
}

static int copy_jpeg_frame(struct net_buf *dst, const uint8_t *src, size_t len)
{
	struct mp_buffer_meta *bm;
	uint32_t cap;

	if (dst == NULL || src == NULL) {
		return -EINVAL;
	}

	bm = mp_buffer_get_meta(dst);
	cap = bm && bm->pool ? bm->pool->config.size : 0;
	if (cap < len) {
		return -ENOBUFS;
	}

	memcpy(dst->data, src, len);

	set_bytes_used(dst, (uint32_t)len);

	return 0;
}

static int append_to_partial(struct net_buf *partial, const uint8_t *src, size_t len)
{
	struct mp_buffer_meta *m;
	uint32_t used;
	uint32_t cap;

	if (partial == NULL || src == NULL) {
		return -EINVAL;
	}

	m = mp_buffer_get_meta(partial);
	used = m->bytes_used;
	cap = m->pool ? m->pool->config.size : 0;

	if (cap < used || (cap - used) < len) {
		return -ENOBUFS;
	}

	memcpy(partial->data + used, src, len);

	set_bytes_used(partial, used + (uint32_t)len);

	return 0;
}

static int mp_img_jpeg_parser_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *out;
	struct mp_buffer_meta *m;

	if (pool == NULL || buf == NULL || pool->nb_pool == NULL) {
		return -EINVAL;
	}

	out = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_FOREVER);
	if (out == NULL) {
		return -ENOBUFS;
	}

	m = mp_buffer_get_meta(out);
	m->pool = pool;
	m->bytes_used = 0;
	m->timestamp = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	out->len = 0;

	*buf = out;
	return 0;
}

static int mp_img_jpeg_parser_release_buffer(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	if (buf != NULL) {
		struct mp_buffer_meta *m = mp_buffer_get_meta(buf);

		if (m != NULL) {
			m->bytes_used = 0;
			m->timestamp = 0;
			m->driver_buf = NULL;
			m->priv = NULL;
		}
		buf->len = 0;
	}

	return 0;
}

static int mp_img_jpeg_parser_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				   struct net_buf **out_buf)
{
	struct mp_parser *parser = (struct mp_parser *)pad->object.container;
	struct mp_img_jpeg_parser *jpeg_parser = (struct mp_img_jpeg_parser *)parser;
	struct mp_buffer_pool *outpool = parser->outpool;
	const uint8_t *data = in_buf->data;
	uint32_t in_used = mp_buffer_get_meta(in_buf)->bytes_used;
	size_t parse_offset = 0;
	const uint8_t *eoi_ptr;

	*out_buf = NULL;

	/* If we have a partial buffer, try to complete it with this input buffer */
	if (jpeg_parser->partial_frame != NULL) {
		struct net_buf *partial = jpeg_parser->partial_frame;
		uint32_t partial_used = mp_buffer_get_meta(partial)->bytes_used;
		uint8_t *partial_eoi;
		bool boundary_eoi = false;

		/* Search EOI across [partial_frame + in_buf] by concatenating temporarily */
		/* First, look for EOI within partial itself */
		partial_eoi = find_jpeg_eoi(partial->data, partial_used);
		if (partial_eoi == NULL) {
			/*
			 * Look for EOI in the in_buf, but account for possible 0xFF at the end
			 * of partial
			 */
			bool boundary_ff = (partial_used > 0 &&
					    partial->data[partial_used - 1] == JPEG_EOI_BYTE0);
			boundary_eoi = (boundary_ff && in_used > 0 && data[0] == JPEG_EOI_BYTE1);

			eoi_ptr = find_jpeg_eoi(data, in_used);
			if (boundary_eoi) {
				/*
				 * EOI spans boundary: partial ends with 0xFF, in_buf starts with
				 * 0xD9
				 */
				eoi_ptr = data;
			}
		} else {
			eoi_ptr = partial_eoi;
		}

		if (eoi_ptr == NULL) {
			/* Still partial: append everything */
			if (append_to_partial(partial, data, in_used) < 0) {
				LOG_ERR("Partial buffer overflow");
				net_buf_unref(in_buf);
				return -ENOBUFS;
			}
			net_buf_unref(in_buf);
			return 0;
		}

		/* We have EOI; copy bytes up to and including EOI into partial */
		size_t to_copy;

		if (eoi_ptr == partial_eoi) {
			to_copy = 0;
		} else {
			/* eoi_ptr points into input buffer */
			to_copy = (size_t)(eoi_ptr - data) + 2U;
		}

		if (to_copy > 0) {
			if (append_to_partial(partial, data, to_copy) < 0) {
				LOG_ERR("Partial buffer overflow");
				net_buf_unref(in_buf);
				return -ENOBUFS;
			}
		}

		/* The partial buffer now holds a complete JPEG frame; output it */
		*out_buf = partial;
		jpeg_parser->partial_frame = NULL;
		parse_offset = boundary_eoi ? 1U : to_copy;
	}

	/* Parse remaining data in input buffer for complete frames */
	while (parse_offset < in_used &&
	       (eoi_ptr = find_jpeg_eoi(data + parse_offset, in_used - parse_offset)) != NULL) {
		struct net_buf *out = NULL;
		size_t up_to_eoi = (size_t)(eoi_ptr - data) + 2U;
		size_t len = up_to_eoi - parse_offset;

		if (outpool->acquire_buffer(outpool, &out) != 0 || out == NULL) {
			LOG_ERR("Failed to acquire output buffer");
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return -ENOMEM;
		}

		if (copy_jpeg_frame(out, data + parse_offset, len) < 0) {
			LOG_ERR("Failed to copy JPEG frame");
			net_buf_unref(out);
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return -ENOBUFS;
		}

		if (*out_buf == NULL) {
			*out_buf = out;
		} else {
			net_buf_frag_add(*out_buf, out);
		}

		parse_offset = up_to_eoi;
	}

	/* Any remaining bytes are a partial frame start: accumulate into partial_frame */
	if (parse_offset < in_used) {
		struct net_buf *partial = NULL;
		size_t remain = in_used - parse_offset;

		if (outpool->acquire_buffer(outpool, &partial) != 0 || partial == NULL) {
			LOG_ERR("Failed to acquire partial buffer");
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return -ENOMEM;
		}

		if (copy_jpeg_frame(partial, data + parse_offset, remain) < 0) {
			LOG_ERR("Failed to init partial buffer");
			net_buf_unref(partial);
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return -ENOBUFS;
		}

		jpeg_parser->partial_frame = partial;
	}

	net_buf_unref(in_buf);
	return 0;
}

static enum mp_state_change_return mp_img_jpeg_parser_change_state(struct mp_element *self,
								    enum mp_state_change transition)
{
	struct mp_img_jpeg_parser *jpeg_parser = (struct mp_img_jpeg_parser *)self;

	/*
	 * On teardown (PAUSED -> READY), drop any leftover bytes in partial frame.
	 * Otherwise, next stream's opening bytes get spliced onto these stale bytes,
	 * producing a corrupt JPEG between replays.
	 */
	if (transition == MP_STATE_CHANGE_PAUSED_TO_READY &&
	    jpeg_parser->partial_frame != NULL) {
		net_buf_unref(jpeg_parser->partial_frame);
		jpeg_parser->partial_frame = NULL;
	}

	/*
	 * Chain to the base parser change_state, which resets the negotiated pad
	 * caps back to the template caps on PAUSED_TO_READY so a subsequent
	 * re-negotiation starts fresh.
	 */
	return mp_parser_change_state(self, transition);
}

void mp_img_jpeg_parser_init(struct mp_element *self)
{
	struct mp_parser *parser = (struct mp_parser *)self;
	struct mp_img_jpeg_parser *jpeg_parser = (struct mp_img_jpeg_parser *)parser;

	mp_parser_init(self);

	self->change_state = mp_img_jpeg_parser_change_state;
	jpeg_parser->partial_frame = NULL;

	/* Get supported caps */
	struct mp_caps *sink_caps = mp_caps_new_any();

	struct mp_caps *src_caps = mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
					       VIDEO_PIX_FMT_JPEG, MP_CAPS_END);
	mp_parser_update_caps(parser, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);

	parser->sinkpad.chainfn = mp_img_jpeg_parser_chainfn;
	parser->decide_allocation = mp_img_jpeg_parser_decide_allocation;

	if (CONFIG_MP_IMG_JPEG_PARSER_MAX_FRAME_SIZE > 0) {
		mp_buffer_pool_init(&jpeg_parser->out_pool);
		jpeg_parser->out_pool.nb_pool = &mp_img_jpeg_parser_pool;
		jpeg_parser->out_pool.config.size = CONFIG_MP_IMG_JPEG_PARSER_MAX_FRAME_SIZE;
		jpeg_parser->out_pool.acquire_buffer = mp_img_jpeg_parser_acquire_buffer;
		jpeg_parser->out_pool.release_buffer = mp_img_jpeg_parser_release_buffer;
		/* net_buf pool is static; no explicit start */
		jpeg_parser->out_pool.started = true;
		parser->outpool = &jpeg_parser->out_pool;
	}
}
