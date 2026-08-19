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

#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/img/mpipe_img_jpeg_parser.h>

LOG_MODULE_REGISTER(mpipe_img_jpeg_parser, CONFIG_MPIPE_LOG_LEVEL);

/*
 * The parser only ever emits JPEG, so its source capability is known at build
 * time and lives in .rodata rather than being allocated at init.
 */
#define JPEG_PARSER_SRC_FIELDS(X) X(MPIPE_CAPS_PIXEL_FORMAT, MPIPE_VALUE_UINT(VIDEO_PIX_FMT_JPEG))

MPIPE_STRUCTURE_DEFINE(jpeg_parser_src_caps, MPIPE_MEDIA_VIDEO, JPEG_PARSER_SRC_FIELDS);

static int mpipe_img_jpeg_parser_enum_caps(struct mpipe_pad *pad, uint32_t index,
					   const struct mpipe_structure *filter,
					   struct mpipe_structure *out)
{
	/* The sink accepts anything, so the source is the only side to describe */
	if (pad->direction == MPIPE_PAD_SINK) {
		if (index > 0) {
			return -ENOENT;
		}

		if (filter != NULL) {
			*out = *filter;
			return 0;
		}

		return mpipe_structure_init_any(out);
	}

	if (index > 0) {
		return -ENOENT;
	}

	return mpipe_pad_enum_filter(&jpeg_parser_src_caps, filter, out);
}

/*
 * Internal output pool which is static and simple. For specific requirements, e.g. alignment,
 * the proposed downstream pool will be used
 */
NET_BUF_POOL_FIXED_DEFINE(mpipe_img_jpeg_parser_pool, CONFIG_MPIPE_IMG_JPEG_PARSER_POOL_NUM,
			  CONFIG_MPIPE_IMG_JPEG_PARSER_MAX_FRAME_SIZE,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

#define JPEG_EOI_BYTE0 0xFFU
#define JPEG_EOI_BYTE1 0xD9U

static int mpipe_img_jpeg_parser_decide_buffer_pool(struct mpipe_parser *parser,
						    struct mpipe_dispatch *query)
{
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)parser;
	struct mpipe_buffer_pool *query_pool = query->pool;

	/* Use the internal pool by default */
	if (CONFIG_MPIPE_IMG_JPEG_PARSER_MAX_FRAME_SIZE > 0) {
		parser->out_pool = &jpeg_parser->out_pool;
	}

	/* Use the proposed pool from downstream when available */
	if (query_pool != NULL) {
		struct mpipe_buffer_pool_config cfg = query_pool->config;
		int ret;

		/* Add one extra buffer to hold the parser's partial frame */
		cfg.min_buffers += 1;

		/*
		 * The demand reaches the pool only through the owner-validated
		 * setter, which may refuse it.
		 */
		ret = mpipe_buffer_pool_set_config(query_pool, &cfg);
		if (ret == 0) {
			parser->out_pool = query_pool;
		} else {
			LOG_WRN("Proposed pool refused the config (%d), keeping the internal pool",
				ret);
		}
	}

	if (parser->out_pool == NULL) {
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
	struct mpipe_buffer_meta *bm = mpipe_buffer_get_meta(buf);

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
	struct mpipe_buffer_meta *bm;
	uint32_t cap;

	__ASSERT_NO_MSG(dst != NULL);
	__ASSERT_NO_MSG(src != NULL);

	bm = mpipe_buffer_get_meta(dst);
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
	struct mpipe_buffer_meta *m;
	uint32_t used;
	uint32_t cap;

	__ASSERT_NO_MSG(partial != NULL);
	__ASSERT_NO_MSG(src != NULL);

	m = mpipe_buffer_get_meta(partial);
	used = m->bytes_used;
	cap = m->pool ? m->pool->config.size : 0;

	if (cap < used || (cap - used) < len) {
		return -ENOBUFS;
	}

	memcpy(partial->data + used, src, len);

	set_bytes_used(partial, used + (uint32_t)len);

	return 0;
}

static int mpipe_img_jpeg_parser_acquire_buffer(struct mpipe_buffer_pool *pool,
						struct net_buf **buf)
{
	struct net_buf *out;
	struct mpipe_buffer_meta *m;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	if (pool->nb_pool == NULL) {
		return -EINVAL;
	}

	out = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_FOREVER);
	if (out == NULL) {
		return -ENOBUFS;
	}

	m = mpipe_buffer_get_meta(out);
	m->pool = pool;
	m->bytes_used = 0;
	m->timestamp = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	out->len = 0;

	*buf = out;

	return 0;
}

static int mpipe_img_jpeg_parser_release_buffer(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	if (buf != NULL) {
		struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);

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

static int mpipe_img_jpeg_parser_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
					  struct net_buf **out_buf)
{
	struct mpipe_parser *parser = (struct mpipe_parser *)pad->object.container;
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)parser;
	struct mpipe_buffer_pool *out_pool = parser->out_pool;
	const uint8_t *data = in_buf->data;
	uint32_t in_used = mpipe_buffer_get_meta(in_buf)->bytes_used;
	size_t parse_offset = 0;
	const uint8_t *eoi_ptr;

	*out_buf = NULL;

	/* If we have a partial buffer, try to complete it with this input buffer */
	if (jpeg_parser->partial_frame != NULL) {
		struct net_buf *partial = jpeg_parser->partial_frame;
		uint32_t partial_used = mpipe_buffer_get_meta(partial)->bytes_used;
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

		if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
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

		if (out_pool->acquire_buffer(out_pool, &partial) != 0 || partial == NULL) {
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

static enum mpipe_state_change_return
mpipe_img_jpeg_parser_change_state(struct mpipe_element *self, enum mpipe_state_change transition)
{
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)self;

	/*
	 * On teardown (PAUSED -> READY), drop any leftover bytes in partial frame.
	 * Otherwise, next stream's opening bytes get spliced onto these stale bytes,
	 * producing a corrupt JPEG between replays.
	 */
	if (transition == MPIPE_STATE_CHANGE_PAUSED_TO_READY &&
	    jpeg_parser->partial_frame != NULL) {
		net_buf_unref(jpeg_parser->partial_frame);
		jpeg_parser->partial_frame = NULL;
	}

	/*
	 * Chain to the base parser change_state, which resets the negotiated pad
	 * caps back to the template caps on PAUSED_TO_READY so a subsequent
	 * re-negotiation starts fresh.
	 */
	return mpipe_parser_change_state(self, transition);
}

int mpipe_img_jpeg_parser_init(struct mpipe_img_jpeg_parser *jpeg_parser, uint8_t id)
{
	__ASSERT_NO_MSG(jpeg_parser != NULL);

	struct mpipe_element *self = &jpeg_parser->base.element;
	struct mpipe_parser *parser = &jpeg_parser->base;
	int ret = mpipe_parser_init(parser, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "img_jpeg_parser");

	self->change_state = mpipe_img_jpeg_parser_change_state;
	jpeg_parser->partial_frame = NULL;

	/*
	 * The capabilities are enumerated from .rodata, so the ANY caps that
	 * mpipe_parser_init() leaves on both sides are never consulted.
	 */
	parser->sink_pad.enum_caps_fn = mpipe_img_jpeg_parser_enum_caps;
	parser->src_pad.enum_caps_fn = mpipe_img_jpeg_parser_enum_caps;

	parser->sink_pad.chain_fn = mpipe_img_jpeg_parser_chain_fn;
	parser->decide_buffer_pool = mpipe_img_jpeg_parser_decide_buffer_pool;

	if (CONFIG_MPIPE_IMG_JPEG_PARSER_MAX_FRAME_SIZE > 0) {
		const struct mpipe_buffer_pool_config pool_req = {
			.size = CONFIG_MPIPE_IMG_JPEG_PARSER_MAX_FRAME_SIZE,
		};

		mpipe_buffer_pool_init(&jpeg_parser->out_pool);
		jpeg_parser->out_pool.nb_pool = &mpipe_img_jpeg_parser_pool;
		(void)mpipe_buffer_pool_set_req_config(&jpeg_parser->out_pool, &pool_req);
		jpeg_parser->out_pool.acquire_buffer = mpipe_img_jpeg_parser_acquire_buffer;
		jpeg_parser->out_pool.release_buffer = mpipe_img_jpeg_parser_release_buffer;
		/* net_buf pool is static; no explicit start */
		jpeg_parser->out_pool.started = true;
		parser->out_pool = &jpeg_parser->out_pool;
	}

	return 0;
}
