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

	/*
	 * The pool proposed upstream carries the config of the original pool,
	 * which may have been a downstream one on the previous run.
	 */
	jpeg_parser->out_pool.config = jpeg_parser->out_pool.req_config;

	/* Use the internal pool by default */
	parser->out_pool = NULL;
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

/* Present a freshly allocated buffer as an empty buffer of @p pool */
static void mpipe_img_jpeg_parser_init_meta(struct net_buf *buf, struct mpipe_buffer_pool *pool)
{
	struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);

	m->pool = pool;
	m->bytes_used = 0;
	m->timestamp = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	buf->len = 0;
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

static int mpipe_img_jpeg_parser_alloc(struct mpipe_img_jpeg_parser *jpeg_parser,
				       struct net_buf **buf)
{
	/* The parser's own pool */
	struct mpipe_buffer_pool *internal = &jpeg_parser->out_pool;
	/* The pool chosen during buffer negotiation */
	struct mpipe_buffer_pool *selected = jpeg_parser->base.out_pool;
	struct net_buf *out = NULL;
	int ret;

	/* Neither an internal pool nor a downstream proposal to output into */
	if (selected == NULL) {
		return -ENOBUFS;
	}

	if (selected == internal) {
		/** Get a buffer from the internal buffer pool */
		out = net_buf_alloc_len(internal->nb_pool, internal->config.size, K_FOREVER);
		if (out == NULL) {
			return -ENOBUFS;
		}

		mpipe_img_jpeg_parser_init_meta(out, internal);
	} else {
		/** Get a buffer from the downstream pool */
		ret = selected->acquire_buffer(selected, &out);
		if (ret != 0 || out == NULL) {
			return (ret != 0) ? ret : -ENOBUFS;
		}

		mpipe_buffer_get_meta(out)->timestamp = 0;
		set_bytes_used(out, 0);
	}

	*buf = out;

	return 0;
}

static int mpipe_img_jpeg_parser_acquire_buffer(struct mpipe_buffer_pool *pool,
						struct net_buf **buf)
{
	struct mpipe_img_jpeg_parser *jpeg_parser;
	struct net_buf *out;
	int ret;

	if (pool == NULL || buf == NULL) {
		return -EINVAL;
	}

	jpeg_parser = CONTAINER_OF(pool, struct mpipe_img_jpeg_parser, out_pool);

	/* The buffer being filled is the one to append to */
	if (jpeg_parser->partial_frame != NULL) {
		*buf = net_buf_ref(jpeg_parser->partial_frame);
		return 0;
	}

	/* No buffer is filling, get a new buffer from the pool */
	ret = mpipe_img_jpeg_parser_alloc(jpeg_parser, &out);
	if (ret != 0) {
		LOG_ERR("Failed to get a buffer to fill (%d)", ret);
		return ret;
	}

	/*
	 * The parser keeps a reference and hands out a second one, so the
	 * producer cannot recycle the buffer while the parser still holds it.
	 */
	jpeg_parser->partial_frame = net_buf_ref(out);
	jpeg_parser->scan_offset = 0;

	LOG_DBG("Accumulating into empty buffer %p", (void *)out->data);

	*buf = out;

	return 0;
}

/*
 * Offer the accumulation pool to the upstream element, so the producer
 * writes straight into the buffer the parser is assembling, avoiding a
 * copy of every byte.
 *
 * The offered pool is always the parser's own, since only its acquire hook
 * returns the buffer being assembled; adopting it means the producer fills
 * the buffers the next element will consume.
 *
 * Whether the producer accepts the offer is known only once it calls the
 * acquire hook; until then the parser copies from the producer's buffers.
 */
static int mpipe_img_jpeg_parser_propose_buffer_pool(struct mpipe_parser *parser,
						     struct mpipe_dispatch *query)
{
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)parser;

	/* The buffer being filled and the head of the next frame are held at once */
	if (parser->out_pool == &jpeg_parser->out_pool &&
	    CONFIG_MPIPE_IMG_JPEG_PARSER_POOL_NUM < 2) {
		LOG_ERR("No pool proposed upstream: the internal pool needs two buffers, "
			"raise CONFIG_MPIPE_IMG_JPEG_PARSER_POOL_NUM");
		return -ENOMEM;
	}

	/* If using a downstream pool, mirror its config */
	if (parser->out_pool != &jpeg_parser->out_pool) {
		jpeg_parser->out_pool.config = parser->out_pool->config;
	}

	query->pool = &jpeg_parser->out_pool;
	query->pool_cfg = jpeg_parser->out_pool.config;

	LOG_DBG("Proposing upstream to fill buffers of %u bytes from the %s pool",
		jpeg_parser->out_pool.config.size,
		(parser->out_pool == &jpeg_parser->out_pool) ? "internal" : "downstream");

	return 0;
}

/*
 * Frames are concatenated back-to-back with no gap, so only the EOI marker
 * needs to be matched to find frame boundaries: the buffer always starts on
 * a frame start, and the byte right after an EOI is the start of the next
 * frame.
 *
 * Scanning resumes where the previous round left off, not from the start of
 * the buffer.
 *
 * The first frame is sent downstream in the original buffer, uncopied.
 * Later complete frames are copied into their own buffer, chained after it.
 * An unterminated frame's head is copied into a fresh buffer, which becomes
 * the one the producer keeps appending to.
 */
static int mpipe_img_jpeg_parser_parse_in_place(struct mpipe_img_jpeg_parser *jpeg_parser,
						struct net_buf *in_buf, struct net_buf **out_buf)
{
	const uint8_t *data = in_buf->data;
	uint32_t in_used = mpipe_buffer_get_meta(in_buf)->bytes_used;
	/* End of buffer, regardless of which pool it came from */
	uint32_t cap = in_buf->size;
	uint32_t scan = jpeg_parser->scan_offset;
	uint32_t frame_start = 0;
	uint32_t first_len = 0;
	struct net_buf *copies = NULL;
	struct net_buf *next_accum = NULL;
	int ret;

	*out_buf = NULL;

	if (scan > in_used) {
		scan = in_used;
	}

	while (true) {
		const uint8_t *marker;
		struct net_buf *out;
		uint32_t len;

		marker = find_jpeg_eoi(data + scan, in_used - scan);
		if (marker == NULL) {
			break;
		}

		len = (uint32_t)(marker - data) + 2U - frame_start;
		scan = (uint32_t)(marker - data) + 2U;

		/* The first frame stays where it is, in the buffer forwarded below */
		if (frame_start == 0U) {
			first_len = len;
			frame_start = scan;

			continue;
		}

		/* Every frame after the first is copied into a new buffer */
		ret = mpipe_img_jpeg_parser_alloc(jpeg_parser, &out);
		if (ret != 0) {
			LOG_ERR("Failed to acquire an output buffer (%d)", ret);
			goto err;
		}

		if (copy_jpeg_frame(out, data + frame_start, len) < 0) {
			LOG_ERR("Failed to copy a JPEG frame");
			net_buf_unref(out);
			ret = -ENOBUFS;
			goto err;
		}

		LOG_DBG("Whole frame of %u bytes copied from %p to %p", len,
			(void *)(data + frame_start), (void *)out->data);

		if (copies == NULL) {
			copies = out;
		} else {
			net_buf_frag_add(copies, out);
		}

		frame_start = scan;
	}

	if (first_len == 0U) {
		/* No complete frame yet: drop the travelling ref, keep the parser's own */
		jpeg_parser->scan_offset = (in_used > 0U) ? in_used - 1U : 0U;

		if (in_used >= cap) {
			/* Frame too large for the buffer: drop it, or the producer sees no room
			 * left */
			LOG_WRN("Dropping a JPEG frame larger than %u bytes", cap);
			set_bytes_used(in_buf, 0);
			jpeg_parser->scan_offset = 0;
		} else {
			LOG_DBG("No EOI in %p yet, %u bytes held: keep filling", (void *)data,
				in_used);
		}

		net_buf_unref(in_buf);

		return 0;
	}

	/* Copy the unterminated frame's head into a fresh accumulation buffer */
	if (in_used > frame_start) {
		ret = mpipe_img_jpeg_parser_alloc(jpeg_parser, &next_accum);
		if (ret != 0) {
			LOG_ERR("Failed to acquire an accumulation buffer (%d)", ret);
			goto err;
		}

		if (copy_jpeg_frame(next_accum, data + frame_start, in_used - frame_start) < 0) {
			LOG_ERR("Failed to copy a partial JPEG frame");
			net_buf_unref(next_accum);
			ret = -ENOBUFS;
			goto err;
		}
	}

	/* Drop the parser's ref; the travelling buffer carries the frames downstream */
	LOG_DBG("Frame of %u bytes forwarded in place from %p", first_len, (void *)data);
	if (next_accum != NULL) {
		LOG_DBG("%u excess bytes copied from %p to %p", in_used - frame_start,
			(void *)(data + frame_start), (void *)next_accum->data);
	}

	net_buf_unref(jpeg_parser->partial_frame);
	jpeg_parser->partial_frame = next_accum;
	jpeg_parser->scan_offset = (next_accum != NULL) ? in_used - frame_start - 1U : 0U;

	set_bytes_used(in_buf, first_len);
	if (copies != NULL) {
		net_buf_frag_add(in_buf, copies);
	}
	*out_buf = in_buf;

	return 0;

err:
	if (copies != NULL) {
		net_buf_unref(copies);
	}
	net_buf_unref(in_buf);

	return ret;
}

static int mpipe_img_jpeg_parser_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
					  struct net_buf **out_buf)
{
	struct mpipe_parser *parser = (struct mpipe_parser *)pad->object.container;
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)parser;
	const uint8_t *data = in_buf->data;
	uint32_t in_used = mpipe_buffer_get_meta(in_buf)->bytes_used;
	size_t parse_offset = 0;
	const uint8_t *eoi_ptr;
	int ret;

	*out_buf = NULL;

	/*
	 * The buffer the producer filled is the one the parser is assembling into,
	 * so the frames are cut out of it where they already sit.
	 */
	if (in_buf == jpeg_parser->partial_frame) {
		return mpipe_img_jpeg_parser_parse_in_place(jpeg_parser, in_buf, out_buf);
	}

	/* If a frame is being assembled, try to complete it with this input buffer */
	if (jpeg_parser->partial_frame != NULL) {
		struct net_buf *partial = jpeg_parser->partial_frame;
		uint32_t partial_used = mpipe_buffer_get_meta(partial)->bytes_used;
		size_t to_copy;

		/*
		 * The partial frame never holds an EOI: its bytes were all scanned
		 * before being stored. Only this buffer is scanned, plus an EOI split
		 * across the two, so each byte is looked at once.
		 */
		bool boundary_eoi = (partial_used > 0U &&
				     partial->data[partial_used - 1U] == JPEG_EOI_BYTE0 &&
				     in_used > 0U && data[0] == JPEG_EOI_BYTE1);

		eoi_ptr = boundary_eoi ? data : find_jpeg_eoi(data, in_used);
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

		/* Copy the bytes up to and including the EOI; a split one only lacks 0xD9 */
		to_copy = boundary_eoi ? 1U : (size_t)(eoi_ptr - data) + 2U;
		if (append_to_partial(partial, data, to_copy) < 0) {
			LOG_ERR("Partial buffer overflow");
			net_buf_unref(in_buf);
			return -ENOBUFS;
		}

		/* The partial buffer now holds a complete JPEG frame; output it */
		*out_buf = partial;
		jpeg_parser->partial_frame = NULL;
		parse_offset = to_copy;
	}

	/* Parse remaining data in input buffer for complete frames */
	while (parse_offset < in_used &&
	       (eoi_ptr = find_jpeg_eoi(data + parse_offset, in_used - parse_offset)) != NULL) {
		struct net_buf *out = NULL;
		size_t up_to_eoi = (size_t)(eoi_ptr - data) + 2U;
		size_t len = up_to_eoi - parse_offset;

		ret = mpipe_img_jpeg_parser_alloc(jpeg_parser, &out);
		if (ret != 0) {
			LOG_ERR("Failed to acquire an output buffer (%d)", ret);
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return ret;
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

		ret = mpipe_img_jpeg_parser_alloc(jpeg_parser, &partial);
		if (ret != 0) {
			LOG_ERR("Failed to acquire a partial frame buffer (%d)", ret);
			net_buf_unref(in_buf);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return ret;
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
		jpeg_parser->scan_offset = 0;
	}

	net_buf_unref(in_buf);

	return 0;
}

static int mpipe_img_jpeg_parser_change_state(struct mpipe_element *self,
					      enum mpipe_state_change transition)
{
	struct mpipe_img_jpeg_parser *jpeg_parser = (struct mpipe_img_jpeg_parser *)self;

	/*
	 * On teardown (PAUSED -> READY), drop any leftover bytes in partial frame.
	 * Otherwise, next stream's opening bytes get spliced onto these stale bytes,
	 * producing a corrupt JPEG between replays.
	 */
	if (transition == MPIPE_STATE_CHANGE_PAUSED_TO_READY) {
		if (jpeg_parser->partial_frame != NULL) {
			net_buf_unref(jpeg_parser->partial_frame);
			jpeg_parser->partial_frame = NULL;
		}

		jpeg_parser->scan_offset = 0;
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
	jpeg_parser->scan_offset = 0;

	/*
	 * The capabilities are enumerated from .rodata, so the ANY caps that
	 * mpipe_parser_init() leaves on both sides are never consulted.
	 */
	parser->sink_pad.enum_caps_fn = mpipe_img_jpeg_parser_enum_caps;
	parser->src_pad.enum_caps_fn = mpipe_img_jpeg_parser_enum_caps;

	parser->sink_pad.chain_fn = mpipe_img_jpeg_parser_chain_fn;
	parser->decide_buffer_pool = mpipe_img_jpeg_parser_decide_buffer_pool;
	parser->propose_buffer_pool = mpipe_img_jpeg_parser_propose_buffer_pool;

	/*
	 * The pool is set up even without internal storage: it is also the one
	 * proposed upstream when the output goes into an adopted downstream pool.
	 */
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

	if (CONFIG_MPIPE_IMG_JPEG_PARSER_MAX_FRAME_SIZE > 0) {
		parser->out_pool = &jpeg_parser->out_pool;
	}

	return 0;
}
