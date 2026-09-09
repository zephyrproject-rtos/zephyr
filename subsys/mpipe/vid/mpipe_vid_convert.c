/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/vid/mpipe_vid_convert.h>
#include <zephyr/mpipe/vid/mpipe_vid_convert_table.h>

LOG_MODULE_REGISTER(mpipe_vid_convert, CONFIG_MPIPE_LOG_LEVEL);

static void nv12_to_rgb565_impl(uint16_t width, uint16_t height, const uint8_t *y_plane,
				const uint8_t *uv_plane, uint16_t *rgb)
{
	int16_t r, g, b;
	uint8_t R, G, B, y, u, v;

	for (uint16_t i = 0U; i < height; i++) {
		for (uint16_t j = 0U; j < width; j++) {
			y = *(y_plane + width * i + j);
			u = *(uv_plane + width * (i / 2U) + j - (j % 2U));
			v = *(uv_plane + width * (i / 2U) + j + 1U - (j % 2U));
			r = (int16_t)y + (int16_t)(1402 * ((int16_t)v - 128) / 1000);
			g = (int16_t)y -
			    (int16_t)((344 * ((int16_t)u - 128) + 714 * ((int16_t)v - 128)) / 1000);
			b = (int16_t)y + (int16_t)(1772 * ((int16_t)u - 128) / 1000);
			R = (uint8_t)CLAMP(r, 0, 255);
			G = (uint8_t)CLAMP(g, 0, 255);
			B = (uint8_t)CLAMP(b, 0, 255);
			*rgb++ = (uint16_t)((((uint16_t)R & 0xF8U) << 8U) |
					    (((uint16_t)G & 0xFCU) << 3U) |
					    (((uint16_t)B & 0xF8U) >> 3U));
		}
	}
}

static inline uint32_t vid_convert_frame_size(uint32_t pixfmt, uint16_t width, uint16_t height)
{
	return (uint32_t)width * (uint32_t)height * video_bits_per_pixel(pixfmt) / BITS_PER_BYTE;
}

static int vid_convert_pool_start(struct mpipe_buffer_pool *pool)
{
	struct mpipe_vid_convert *conv;

	__ASSERT_NO_MSG(pool != NULL);

	conv = CONTAINER_OF(pool, struct mpipe_vid_convert, out_pool);

	if (pool->config.min_buffers == 0U) {
		pool->config.min_buffers = 1U;
	}

	if (pool->config.min_buffers > CONFIG_VIDEO_BUFFER_POOL_NUM_MAX) {
		LOG_ERR("min_buffers=%u exceeds CONFIG_VIDEO_BUFFER_POOL_NUM_MAX=%u",
			pool->config.min_buffers, CONFIG_VIDEO_BUFFER_POOL_NUM_MAX);
		return -EINVAL;
	}

	conv->vbuf_count = pool->config.min_buffers;

	for (uint8_t i = 0; i < conv->vbuf_count; i++) {
		struct video_buffer *vbuf = video_buffer_alloc(pool->config.size, K_NO_WAIT);

		if (vbuf == NULL) {
			LOG_ERR("Failed to allocate video buffer %u", i);
			return -ENOBUFS;
		}
		conv->vbufs[i] = vbuf;
		k_fifo_put(&conv->free_fifo, vbuf);
	}

	return 0;
}

static int vid_convert_pool_stop(struct mpipe_buffer_pool *pool)
{
	struct mpipe_vid_convert *conv;

	__ASSERT_NO_MSG(pool != NULL);

	conv = CONTAINER_OF(pool, struct mpipe_vid_convert, out_pool);

	for (uint8_t i = 0; i < conv->vbuf_count; i++) {
		if (conv->vbufs[i] != NULL) {
			(void)video_buffer_release(conv->vbufs[i]);
			conv->vbufs[i] = NULL;
		}
	}

	conv->vbuf_count = 0;

	return 0;
}

static int vid_convert_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **out)
{
	struct video_buffer *vbuf;
	struct mpipe_buffer_meta *meta;
	struct mpipe_vid_convert *conv;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(out != NULL);

	conv = CONTAINER_OF(pool, struct mpipe_vid_convert, out_pool);

	vbuf = k_fifo_get(&conv->free_fifo, K_FOREVER);
	if (vbuf == NULL) {
		return -ENOBUFS;
	}

	*out = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (*out == NULL) {
		k_fifo_put(&conv->free_fifo, vbuf);
		return -ENOBUFS;
	}

	meta = mpipe_buffer_get_meta(*out);
	meta->pool = pool;
	meta->driver_buf = vbuf;
	meta->priv = NULL;
	meta->bytes_used = 0;
	meta->timestamp = 0;

	return 0;
}

static int vid_convert_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	struct video_buffer *vbuf;
	struct mpipe_vid_convert *conv;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	conv = CONTAINER_OF(pool, struct mpipe_vid_convert, out_pool);

	vbuf = (struct video_buffer *)mpipe_buffer_get_meta(buf)->driver_buf;
	if (vbuf != NULL) {
		k_fifo_put(&conv->free_fifo, vbuf);
	}

	return 0;
}

/*
 * Return the pixel format at @p index among the distinct ones the converter
 * handles on this side. The same format appears in several descriptors, so
 * anything already produced by a lower index is skipped.
 */
static uint32_t vid_convert_pixfmt_at(enum mpipe_pad_direction direction, uint32_t index)
{
	uint32_t matched = 0;

	for (size_t i = 0; i < mpipe_vid_convert_descs_len; i++) {
		uint32_t pf = (direction == MPIPE_PAD_SINK) ? mpipe_vid_convert_descs[i].in_pixfmt
							    : mpipe_vid_convert_descs[i].out_pixfmt;
		bool seen = false;

		for (size_t j = 0; j < i; j++) {
			uint32_t prev = (direction == MPIPE_PAD_SINK)
						? mpipe_vid_convert_descs[j].in_pixfmt
						: mpipe_vid_convert_descs[j].out_pixfmt;

			if (prev == pf) {
				seen = true;
				break;
			}
		}

		if (seen) {
			continue;
		}

		if (matched == index) {
			return pf;
		}

		matched++;
	}

	return 0;
}

static int vid_convert_enum_caps(struct mpipe_pad *pad, uint32_t index,
				 const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	uint32_t pixfmt = vid_convert_pixfmt_at(pad->direction, index);
	struct mpipe_structure candidate;
	struct mpipe_value fmt;
	int ret;

	if (pixfmt == 0) {
		return -ENOENT;
	}

	ret = mpipe_structure_init(&candidate, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	fmt.type = MPIPE_TYPE_UINT;
	fmt.v_uint = pixfmt;
	ret = mpipe_structure_append_value(&candidate, MPIPE_CAPS_PIXEL_FORMAT, &fmt);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int vid_convert_set_caps(struct mpipe_transform *transform,
				enum mpipe_pad_direction direction,
				const struct mpipe_structure *caps)
{
	struct mpipe_vid_convert *conv = (struct mpipe_vid_convert *)transform;
	const struct mpipe_value *v;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = mpipe_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_WIDTH);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		conv->width = (uint16_t)mpipe_value_get_uint(v);
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_HEIGHT);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		conv->height = (uint16_t)mpipe_value_get_uint(v);
	}

	v = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);
	if (v != NULL && v->type == MPIPE_TYPE_UINT) {
		if (direction == MPIPE_PAD_SINK) {
			conv->in_pixfmt = mpipe_value_get_uint(v);
		} else {
			conv->out_pixfmt = mpipe_value_get_uint(v);
		}
	}

	/* Once both sides are known, pick the conversion entry */
	if (conv->in_pixfmt != 0U && conv->out_pixfmt != 0U) {
		conv->desc = NULL;
		for (size_t i = 0; i < mpipe_vid_convert_descs_len; i++) {
			if (mpipe_vid_convert_descs[i].in_pixfmt == conv->in_pixfmt &&
			    mpipe_vid_convert_descs[i].out_pixfmt == conv->out_pixfmt) {
				conv->desc = &mpipe_vid_convert_descs[i];
				break;
			}
		}
		if (conv->desc == NULL) {
			LOG_ERR("Unsupported conversion 0x%08x -> 0x%08x", conv->in_pixfmt,
				conv->out_pixfmt);
			return -ENOTSUP;
		}
	}

	if (conv->width != 0U && conv->height != 0U && conv->out_pixfmt != 0U) {
		conv->out_pool.config.size =
			vid_convert_frame_size(conv->out_pixfmt, conv->width, conv->height);
	}

	return 0;
}

/*
 * The format reachable from @p in_pf at @p index, taken toward @p direction.
 * The descriptor table is the list of conversions the element implements, so a
 * transformation is one entry of it whose input side matches. Returns 0 past
 * the last reachable format.
 */
static uint32_t vid_convert_reachable_pixfmt_at(enum mpipe_pad_direction direction, uint32_t in_pf,
						uint32_t index)
{
	uint32_t matched = 0;

	for (size_t i = 0; i < mpipe_vid_convert_descs_len; i++) {
		uint32_t src_pf = (direction == MPIPE_PAD_SRC)
					  ? mpipe_vid_convert_descs[i].in_pixfmt
					  : mpipe_vid_convert_descs[i].out_pixfmt;
		uint32_t dst_pf = (direction == MPIPE_PAD_SRC)
					  ? mpipe_vid_convert_descs[i].out_pixfmt
					  : mpipe_vid_convert_descs[i].in_pixfmt;

		if (src_pf != in_pf) {
			continue;
		}

		if (matched == index) {
			return dst_pf;
		}

		matched++;
	}

	return 0;
}

static int vid_convert_transform_caps(struct mpipe_transform *self,
				      enum mpipe_pad_direction direction,
				      const struct mpipe_structure *in, uint32_t index,
				      struct mpipe_structure *out)
{
	struct mpipe_value fmt;
	uint32_t pixfmt;
	int ret;

	ARG_UNUSED(self);

	if (in == NULL) {
		return -EINVAL;
	}

	if (mpipe_structure_is_any(in)) {
		/* Constrains nothing, so every format this side handles is reachable */
		pixfmt = vid_convert_pixfmt_at(direction, index);
	} else {
		const struct mpipe_value *pix =
			mpipe_structure_get_value(in, MPIPE_CAPS_PIXEL_FORMAT);

		/* Alternatives live on the enumeration index, so a format is a fixed value */
		if (pix == NULL || pix->type != MPIPE_TYPE_UINT) {
			return -ENOENT;
		}

		pixfmt = vid_convert_reachable_pixfmt_at(direction, mpipe_value_get_uint(pix),
							 index);
	}

	if (pixfmt == 0U) {
		return -ENOENT;
	}

	ret = mpipe_structure_init(out, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	fmt.type = MPIPE_TYPE_UINT;
	fmt.v_uint = pixfmt;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_PIXEL_FORMAT, &fmt);
	if (ret != 0) {
		mpipe_structure_clear(out);
		return ret;
	}

	/*
	 * Converting changes the format, not the geometry. An input that does
	 * not constrain the geometry is not an error - a field travels only
	 * when one side constrains it - but running out of room for one is.
	 */
	static const uint8_t passthrough[] = {MPIPE_CAPS_IMAGE_WIDTH, MPIPE_CAPS_IMAGE_HEIGHT};

	for (uint8_t i = 0; i < ARRAY_SIZE(passthrough); i++) {
		ret = mpipe_structure_copy_field(in, out, passthrough[i]);
		if (ret != 0 && ret != -ENOENT) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

static int vid_convert_decide_buffer_pool(struct mpipe_transform *self,
					  struct mpipe_dispatch *query)
{
	struct mpipe_vid_convert *conv = (struct mpipe_vid_convert *)self;
	struct mpipe_buffer_pool *down_pool = query->pool;

	/* Use the internal pool by default */
	self->out_pool = &conv->out_pool;

	/* Use the proposed pool from downstream when available */
	if (down_pool != NULL) {
		self->out_pool = down_pool;
	}

	/* TODO: Do negotiation when downstream only propose pool config */

	return 0;
}

static int vid_convert_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	struct net_buf *cur;
	struct net_buf *next;
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_vid_convert *conv = (struct mpipe_vid_convert *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	uint32_t out_sz = vid_convert_frame_size(conv->out_pixfmt, conv->width, conv->height);

	if (conv->width == 0U || conv->height == 0U || conv->in_pixfmt == 0U ||
	    conv->out_pixfmt == 0U || conv->desc == NULL || conv->desc->fn == NULL) {
		LOG_ERR("Missing negotiated caps / conversion");
		net_buf_unref(in_buf);
		return -EINVAL;
	}

	/* Process all input net_buf fragments */
	*out_buf = NULL;
	cur = in_buf;
	while (cur != NULL) {
		struct net_buf *out = NULL;

		if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
			LOG_ERR("Failed to acquire output buffer");
			goto err;
		}

		if (conv->in_pixfmt == conv->out_pixfmt) {
			memcpy(out->data, cur->data, MIN(out_sz, (uint32_t)cur->len));
		} else {
			if (conv->desc->fn(conv, cur, out) != 0) {
				LOG_ERR("Failed to convert pixel format");
				net_buf_unref(out);
				goto err;
			}
		}

		mpipe_buffer_get_meta(out)->bytes_used = out_sz;
		out->len = out_sz;

		if (mpipe_buffer_get_meta(out)->driver_buf != NULL) {
			((struct video_buffer *)mpipe_buffer_get_meta(out)->driver_buf)->bytesused =
				out_sz;
		}

		if (*out_buf == NULL) {
			*out_buf = out;
		} else {
			net_buf_frag_add(*out_buf, out);
		}

		next = cur->frags;
		cur->frags = NULL;
		net_buf_unref(cur);
		cur = next;
	}

	return 0;
err:
	net_buf_unref(cur);
	if (*out_buf != NULL) {
		net_buf_unref(*out_buf);
		*out_buf = NULL;
	}

	return -EIO;
}

int mpipe_vid_convert_init(struct mpipe_vid_convert *conv, uint8_t id)
{
	__ASSERT_NO_MSG(conv != NULL);

	struct mpipe_element *self = &conv->transform.element;
	struct mpipe_transform *transform = &conv->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "vid_convert");

	transform->sink_pad.enum_caps_fn = vid_convert_enum_caps;
	transform->src_pad.enum_caps_fn = vid_convert_enum_caps;

	conv->width = 0;
	conv->height = 0;
	conv->in_pixfmt = 0;
	conv->out_pixfmt = 0;
	conv->desc = NULL;
	conv->impl.nv12_to_rgb565 = nv12_to_rgb565_impl;

	k_fifo_init(&conv->free_fifo);
	memset(conv->vbufs, 0, sizeof(conv->vbufs));
	conv->vbuf_count = 0;

	mpipe_buffer_pool_init(&conv->out_pool);

	conv->out_pool.start = vid_convert_pool_start;
	conv->out_pool.stop = vid_convert_pool_stop;
	conv->out_pool.acquire_buffer = vid_convert_pool_acquire;
	conv->out_pool.release_buffer = vid_convert_pool_release;

	const struct mpipe_buffer_pool_config pool_req = {
		.size = 0,
		.align = 1,
		.min_buffers = 1,
		.max_buffers = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX,
	};

	(void)mpipe_buffer_pool_set_req_config(&conv->out_pool, &pool_req);

	transform->mode = MPIPE_MODE_NORMAL;
	transform->out_pool = &conv->out_pool;
	transform->set_caps = vid_convert_set_caps;
	transform->transform_caps = vid_convert_transform_caps;
	transform->propose_buffer_pool = NULL;
	transform->decide_buffer_pool = vid_convert_decide_buffer_pool;
	transform->sink_pad.chain_fn = vid_convert_chain_fn;

	return 0;
}
