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

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zvid/mp_zvid_convert.h>
#include <zephyr/mp/zvid/mp_zvid_convert_table.h>

LOG_MODULE_REGISTER(mp_zvid_convert, CONFIG_MP_LOG_LEVEL);

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

static inline uint32_t zvid_convert_frame_size(uint32_t pixfmt, uint16_t width, uint16_t height)
{
	return (uint32_t)width * (uint32_t)height * video_bits_per_pixel(pixfmt) / BITS_PER_BYTE;
}

static int zvid_convert_pool_start(struct mp_buffer_pool *pool)
{
	struct mp_zvid_convert *conv = CONTAINER_OF(pool, struct mp_zvid_convert, out_pool);

	if (pool == NULL) {
		return -EINVAL;
	}

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

static int zvid_convert_pool_stop(struct mp_buffer_pool *pool)
{
	struct mp_zvid_convert *conv = CONTAINER_OF(pool, struct mp_zvid_convert, out_pool);

	if (pool == NULL) {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < conv->vbuf_count; i++) {
		if (conv->vbufs[i] != NULL) {
			(void)video_buffer_release(conv->vbufs[i]);
			conv->vbufs[i] = NULL;
		}
	}

	conv->vbuf_count = 0;

	return 0;
}

static int zvid_convert_pool_acquire(struct mp_buffer_pool *pool, struct net_buf **out)
{
	struct video_buffer *vbuf;
	struct mp_buffer_meta *meta;
	struct mp_zvid_convert *conv = CONTAINER_OF(pool, struct mp_zvid_convert, out_pool);

	if (pool == NULL || out == NULL) {
		return -EINVAL;
	}

	vbuf = k_fifo_get(&conv->free_fifo, K_FOREVER);
	if (vbuf == NULL) {
		return -ENOBUFS;
	}

	*out = net_buf_alloc_with_data(pool->nb_pool, vbuf->buffer, vbuf->size, K_NO_WAIT);
	if (*out == NULL) {
		k_fifo_put(&conv->free_fifo, vbuf);
		return -ENOBUFS;
	}

	meta = mp_buffer_get_meta(*out);
	meta->pool = pool;
	meta->priv = vbuf;
	meta->bytes_used = 0;
	meta->timestamp = 0;

	return 0;
}

static int zvid_convert_pool_release(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	struct video_buffer *vbuf;
	struct mp_zvid_convert *conv = CONTAINER_OF(pool, struct mp_zvid_convert, out_pool);

	if (pool == NULL || buf == NULL) {
		return -EINVAL;
	}

	vbuf = (struct video_buffer *)mp_buffer_get_meta(buf)->priv;
	if (vbuf != NULL) {
		k_fifo_put(&conv->free_fifo, vbuf);
	}

	return 0;
}

static struct mp_value *zvid_convert_pixfmt_list(enum mp_pad_direction direction)
{
	struct mp_value *list = mp_value_new(MP_TYPE_LIST, NULL);

	if (list == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < mp_zvid_convert_descs_len; i++) {
		uint32_t pf = (direction == MP_PAD_SINK) ? mp_zvid_convert_descs[i].in_pixfmt
							 : mp_zvid_convert_descs[i].out_pixfmt;

		/* avoid duplicates */
		bool found = false;

		for (size_t j = 0; j < mp_value_list_get_size(list); j++) {
			struct mp_value *v = mp_value_list_get(list, (int)j);

			if (v != NULL && v->type == MP_TYPE_UINT && mp_value_get_uint(v) == pf) {
				found = true;
				break;
			}
		}
		if (!found) {
			mp_value_list_append(list, mp_value_new(MP_TYPE_UINT, pf));
		}
	}

	return list;
}

static struct mp_caps *zvid_convert_supported_caps(struct mp_transform *transform,
						   enum mp_pad_direction direction)
{
	ARG_UNUSED(transform);

	struct mp_value *fmts = zvid_convert_pixfmt_list(direction);

	if (fmts == NULL) {
		return NULL;
	}

	return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST, fmts, MP_CAPS_END);
}

static void zvid_convert_update_caps(struct mp_transform *transform)
{
	struct mp_caps *sink_caps = zvid_convert_supported_caps(transform, MP_PAD_SINK);
	struct mp_caps *src_caps = zvid_convert_supported_caps(transform, MP_PAD_SRC);

	mp_transform_update_caps(transform, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);
}

static int zvid_convert_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
				 struct mp_caps *caps)
{
	struct mp_zvid_convert *conv = (struct mp_zvid_convert *)transform;
	struct mp_structure *s;
	struct mp_value *v;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = mp_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	s = mp_caps_get_structure(caps, 0);
	if (s != NULL) {
		v = mp_structure_get_value(s, MP_CAPS_IMAGE_WIDTH);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			conv->width = (uint16_t)mp_value_get_uint(v);
		}
		v = mp_structure_get_value(s, MP_CAPS_IMAGE_HEIGHT);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			conv->height = (uint16_t)mp_value_get_uint(v);
		}
		v = mp_structure_get_value(s, MP_CAPS_PIXEL_FORMAT);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			if (direction == MP_PAD_SINK) {
				conv->in_pixfmt = mp_value_get_uint(v);
			} else {
				conv->out_pixfmt = mp_value_get_uint(v);
			}
		}
	}

	/* Once both sides are known, pick the conversion entry */
	if (conv->in_pixfmt != 0U && conv->out_pixfmt != 0U) {
		conv->desc = NULL;
		for (size_t i = 0; i < mp_zvid_convert_descs_len; i++) {
			if (mp_zvid_convert_descs[i].in_pixfmt == conv->in_pixfmt &&
			    mp_zvid_convert_descs[i].out_pixfmt == conv->out_pixfmt) {
				conv->desc = &mp_zvid_convert_descs[i];
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
			zvid_convert_frame_size(conv->out_pixfmt, conv->width, conv->height);
	}

	return 0;
}

static bool out_fmts_contains(struct mp_value *out_fmts, uint32_t pixfmt)
{
	for (size_t k = 0; k < mp_value_list_get_size(out_fmts); k++) {
		struct mp_value *ov = mp_value_list_get(out_fmts, (int)k);

		if (ov != NULL && ov->type == MP_TYPE_UINT &&
		    mp_value_get_uint(ov) == pixfmt) {
			return true;
		}
	}
	return false;
}

static struct mp_caps *zvid_convert_transform_caps(struct mp_transform *self,
						   enum mp_pad_direction direction,
						   struct mp_caps *incaps)
{
	if (incaps == NULL || mp_caps_is_empty(incaps)) {
		return NULL;
	}

	if (mp_caps_is_any(incaps)) {
		return zvid_convert_supported_caps(self, direction);
	}

	struct mp_caps *out = mp_caps_new(MP_MEDIA_END);
	struct mp_cap_structure *cs;

	SYS_SLIST_FOR_EACH_CONTAINER(&incaps->caps_structures, cs, node) {
		struct mp_structure *s = cs->structure;
		struct mp_value *pix = mp_structure_get_value(s, MP_CAPS_PIXEL_FORMAT);
		struct mp_value *w = mp_structure_get_value(s, MP_CAPS_IMAGE_WIDTH);
		struct mp_value *h = mp_structure_get_value(s, MP_CAPS_IMAGE_HEIGHT);
		struct mp_value *out_fmts = mp_value_new(MP_TYPE_LIST, NULL);

		if (out_fmts == NULL) {
			continue;
		}

		/*
		 * Pixel format in caps may be a fixed UINT (after fixate) or a LIST
		 * (after intersection but before fixate).
		 */
		if (pix != NULL && pix->type == MP_TYPE_UINT) {
			uint32_t in_pf = mp_value_get_uint(pix);

			for (size_t i = 0; i < mp_zvid_convert_descs_len; i++) {
				uint32_t src_pf = (direction == MP_PAD_SRC)
							  ? mp_zvid_convert_descs[i].in_pixfmt
							  : mp_zvid_convert_descs[i].out_pixfmt;
				uint32_t dst_pf = (direction == MP_PAD_SRC)
							  ? mp_zvid_convert_descs[i].out_pixfmt
							  : mp_zvid_convert_descs[i].in_pixfmt;

				if (in_pf == src_pf) {
					mp_value_list_append(out_fmts,
							     mp_value_new(MP_TYPE_UINT, dst_pf));
				}
			}
		} else if (pix != NULL && pix->type == MP_TYPE_LIST) {
			for (size_t j = 0; j < mp_value_list_get_size(pix); j++) {
				struct mp_value *pv = mp_value_list_get(pix, (int)j);

				if (pv == NULL || pv->type != MP_TYPE_UINT) {
					continue;
				}
				uint32_t in_pf = mp_value_get_uint(pv);

				for (size_t i = 0; i < mp_zvid_convert_descs_len; i++) {
					uint32_t src_pf =
						(direction == MP_PAD_SRC)
							? mp_zvid_convert_descs[i].in_pixfmt
							: mp_zvid_convert_descs[i].out_pixfmt;
					uint32_t dst_pf =
						(direction == MP_PAD_SRC)
							? mp_zvid_convert_descs[i].out_pixfmt
							: mp_zvid_convert_descs[i].in_pixfmt;

					if (in_pf != src_pf) {
						continue;
					}

					if (!out_fmts_contains(out_fmts, dst_pf)) {
						mp_value_list_append(
							out_fmts,
							mp_value_new(MP_TYPE_UINT, dst_pf));
					}
				}
			}
		} else {
			/* pix is NULL or unsupported type: skip */
		}

		if (mp_value_list_is_empty(out_fmts)) {
			mp_value_destroy(out_fmts);
			continue;
		}

		struct mp_structure *ns = mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT,
							   MP_TYPE_LIST, out_fmts, MP_CAPS_END);

		if (w != NULL) {
			mp_structure_append(ns, MP_CAPS_IMAGE_WIDTH, mp_value_duplicate(w));
		}
		if (h != NULL) {
			mp_structure_append(ns, MP_CAPS_IMAGE_HEIGHT, mp_value_duplicate(h));
		}

		mp_caps_append(out, ns);
	}

	return out;
}

static int zvid_convert_decide_allocation(struct mp_transform *self, struct mp_dispatch *query)
{
	struct mp_zvid_convert *conv = (struct mp_zvid_convert *)self;
	struct mp_buffer_pool *down_pool = mp_dispatch_get_pool(query);

	/* Use the internal pool by default */
	self->outpool = &conv->out_pool;

	/* Use the proposed pool from downstream when available */
	if (down_pool != NULL) {
		self->outpool = down_pool;
	}

	/* TODO: Do negotiation when downstream only propose pool config */

	return 0;
}

static int zvid_convert_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	struct net_buf *cur;
	struct net_buf *next;
	struct mp_transform *transform = (struct mp_transform *)pad->object.container;
	struct mp_zvid_convert *conv = (struct mp_zvid_convert *)transform;
	struct mp_buffer_pool *outpool = transform->outpool;
	uint32_t out_sz = zvid_convert_frame_size(conv->out_pixfmt, conv->width, conv->height);

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

		if (outpool->acquire_buffer(outpool, &out) != 0 || out == NULL) {
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

		mp_buffer_get_meta(out)->bytes_used = out_sz;
		out->len = out_sz;

		if (mp_buffer_get_meta(out)->priv != NULL) {
			((struct video_buffer *)mp_buffer_get_meta(out)->priv)->bytesused = out_sz;
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

void mp_zvid_convert_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_zvid_convert *conv = (struct mp_zvid_convert *)transform;

	mp_transform_init(self);

	zvid_convert_update_caps(transform);

	conv->width = 0;
	conv->height = 0;
	conv->in_pixfmt = 0;
	conv->out_pixfmt = 0;
	conv->desc = NULL;
	conv->impl.nv12_to_rgb565 = nv12_to_rgb565_impl;

	k_fifo_init(&conv->free_fifo);
	memset(conv->vbufs, 0, sizeof(conv->vbufs));
	conv->vbuf_count = 0;

	mp_buffer_pool_init(&conv->out_pool);

	conv->out_pool.start = zvid_convert_pool_start;
	conv->out_pool.stop = zvid_convert_pool_stop;
	conv->out_pool.acquire_buffer = zvid_convert_pool_acquire;
	conv->out_pool.release_buffer = zvid_convert_pool_release;
	conv->out_pool.config.size = 0;
	conv->out_pool.config.align = 1;
	conv->out_pool.config.min_buffers = 1;
	conv->out_pool.config.max_buffers = CONFIG_VIDEO_BUFFER_POOL_NUM_MAX;

	transform->mode = MP_MODE_NORMAL;
	transform->outpool = &conv->out_pool;
	transform->set_caps = zvid_convert_set_caps;
	transform->transform_caps = zvid_convert_transform_caps;
	transform->propose_allocation = NULL;
	transform->decide_allocation = zvid_convert_decide_allocation;
	transform->sinkpad.chainfn = zvid_convert_chainfn;
}
