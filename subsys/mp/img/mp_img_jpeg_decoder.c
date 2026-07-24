/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/mp_buffer.h>
#include <zephyr/mp/mp_caps.h>
#include <zephyr/mp/mp_dispatch.h>
#include <zephyr/mp/mp_structure.h>
#include <zephyr/mp/mp_value.h>

#include <zephyr/mp/img/mp_img_jpeg_decoder.h>

LOG_MODULE_REGISTER(mp_img_jpeg_decoder, CONFIG_MP_LOG_LEVEL);

/* Internal output pool (used when downstream doesn't propose a pool) */
NET_BUF_POOL_FIXED_DEFINE(mp_img_dec_pool, CONFIG_MP_IMG_JPEG_DECODER_POOL_NUM,
			  CONFIG_MP_IMG_JPEG_DECODER_MAX_OUT_FRAME_SIZE, sizeof(struct mp_buffer_meta),
			  mp_buffer_destroy);

static int mp_img_jpeg_decoder_outpool_acquire(struct mp_buffer_pool *pool, struct net_buf **buf)
{
	struct net_buf *out;
	struct mp_buffer_meta *m;

	if (pool == NULL || buf == NULL) {
		return -EINVAL;
	}

	out = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
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

static int mp_img_jpeg_decoder_outpool_release(struct mp_buffer_pool *pool, struct net_buf *buf)
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

static void mp_img_jpeg_decoder_outpool_init(struct mp_img_jpeg_decoder *dec)
{
	mp_buffer_pool_init(&dec->out_pool);
	dec->out_pool.nb_pool = &mp_img_dec_pool;
	dec->out_pool.config.size = CONFIG_MP_IMG_JPEG_DECODER_MAX_OUT_FRAME_SIZE;
	dec->out_pool.config.align = 1;
	dec->out_pool.config.min_buffers = CONFIG_MP_IMG_JPEG_DECODER_POOL_NUM;
	dec->out_pool.config.max_buffers = CONFIG_MP_IMG_JPEG_DECODER_POOL_NUM;
	dec->out_pool.acquire_buffer = mp_img_jpeg_decoder_outpool_acquire;
	dec->out_pool.release_buffer = mp_img_jpeg_decoder_outpool_release;
	dec->out_pool.started = true; /* net_buf pool is static; no explicit start */
}

static int mp_img_jpeg_decoder_decode_one(struct mp_img_jpeg_decoder *dec, struct net_buf *in_buf,
				       struct net_buf *out_buf)
{
	JPEGIMAGE *jpg = &dec->jpg;
	uint32_t in_sz = mp_buffer_get_meta(in_buf)->bytes_used;
	uint32_t out_sz;

	if (in_sz == 0) {
		mp_buffer_get_meta(out_buf)->bytes_used = 0;
		out_buf->len = 0;
		return -ENODATA;
	}

	if (JPEG_openRAM(jpg, in_buf->data, (int)in_sz, NULL) == 0) {
		LOG_ERR("JPEG open failed");
		return -EINVAL;
	}

	JPEG_setFramebuffer(jpg, (void *)out_buf->data);

	switch (dec->out_pixfmt) {
	case VIDEO_PIX_FMT_RGB565:
		JPEG_setPixelType(jpg, RGB565_LITTLE_ENDIAN);
		break;
	case VIDEO_PIX_FMT_RGB565X:
		JPEG_setPixelType(jpg, RGB565_BIG_ENDIAN);
		break;
	default:
		LOG_ERR("Unsupported output pixfmt %u", dec->out_pixfmt);
		JPEG_close(jpg);
		return -ENOTSUP;
	}

	if (JPEG_decode(jpg, 0, 0, 0) == 0) {
		LOG_ERR("JPEG decode failed");
		JPEG_close(jpg);
		return -EIO;
	}

	/*
	 * Do not use JPEG_getBpp() as it reports the JPEG stream bpp (e.g.
	 * 24 bits for 3 components YCbCr), not the decoded output format bpp.
	 */
	out_sz = (uint32_t)JPEG_getWidth(jpg) * (uint32_t)JPEG_getHeight(jpg) *
		 video_bits_per_pixel(dec->out_pixfmt) / BITS_PER_BYTE;
	mp_buffer_get_meta(out_buf)->bytes_used = out_sz;
	out_buf->len = out_sz;

	JPEG_close(jpg);

	return 0;
}

static int mp_img_jpeg_decoder_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				    struct net_buf **out_buf)
{
	struct mp_transform *transform = (struct mp_transform *)pad->object.container;
	struct mp_img_jpeg_decoder *dec = (struct mp_img_jpeg_decoder *)transform;
	struct mp_buffer_pool *outpool = transform->outpool;
	struct net_buf *cur;
	struct net_buf *next;

	*out_buf = NULL;

	if (outpool == NULL) {
		outpool = &dec->out_pool;
	}

	if (in_buf == NULL || outpool->acquire_buffer == NULL) {
		return -EINVAL;
	}

	cur = in_buf;
	while (cur != NULL) {
		struct net_buf *out = NULL;
		int ret;

		/* Acquire first; on failure drop remaining input chain */
		ret = outpool->acquire_buffer(outpool, &out);
		if (ret != 0 || out == NULL) {
			LOG_ERR("Failed to acquire output buffer (%d)", ret);
			net_buf_unref(cur);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			return -ENOMEM;
		}

		next = cur->frags;
		cur->frags = NULL;

		if (mp_img_jpeg_decoder_decode_one(dec, cur, out) < 0) {
			net_buf_unref(out);
			net_buf_unref(cur);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			/* remaining input chain (next) will be freed by net_buf_unref(cur) only if
			 * we didn't detach; we detached, so free it explicitly.
			 */
			while (next != NULL) {
				struct net_buf *tmp = next->frags;

				next->frags = NULL;
				net_buf_unref(next);
				next = tmp;
			}
			return -EIO;
		}

		if (*out_buf == NULL) {
			*out_buf = out;
		} else {
			net_buf_frag_add(*out_buf, out);
		}

		net_buf_unref(cur);
		cur = next;
	}

	return 0;
}

static struct mp_caps *mp_img_jpeg_decoder_supported_caps(struct mp_transform *transform,
						       enum mp_pad_direction direction)
{
	ARG_UNUSED(transform);

	if (direction == MP_PAD_SINK) {
		return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
				   VIDEO_PIX_FMT_JPEG, MP_CAPS_END);
	}

	if (direction == MP_PAD_SRC) {
		struct mp_value *fmts = mp_value_new(MP_TYPE_LIST, NULL);

		if (fmts == NULL) {
			return NULL;
		}
		mp_value_list_append(fmts, mp_value_new(MP_TYPE_UINT, VIDEO_PIX_FMT_RGB565));
		mp_value_list_append(fmts, mp_value_new(MP_TYPE_UINT, VIDEO_PIX_FMT_RGB565X));

		return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST, fmts,
				   MP_CAPS_END);
	}

	return NULL;
}

static struct mp_caps *mp_img_jpeg_decoder_transform_caps(struct mp_transform *transform,
						       enum mp_pad_direction direction,
						       struct mp_caps *incaps)
{
	ARG_UNUSED(transform);

	if (incaps == NULL) {
		return NULL;
	}

	if (mp_caps_is_any(incaps)) {
		return mp_caps_new_any();
	}

	struct mp_caps *out = mp_caps_new(MP_MEDIA_END);
	struct mp_cap_structure *cs;

	SYS_SLIST_FOR_EACH_CONTAINER(&incaps->caps_structures, cs, node) {
		struct mp_structure *s = cs->structure;
		struct mp_value *w = mp_structure_get_value(s, MP_CAPS_IMAGE_WIDTH);
		struct mp_value *h = mp_structure_get_value(s, MP_CAPS_IMAGE_HEIGHT);
		struct mp_value *fr = mp_structure_get_value(s, MP_CAPS_FRAME_RATE);

		if (direction == MP_PAD_SRC) {
			/* JPEG -> RGB565{,X} */
			struct mp_value *fmts = mp_value_new(MP_TYPE_LIST, NULL);

			if (fmts == NULL) {
				continue;
			}
			mp_value_list_append(fmts,
					     mp_value_new(MP_TYPE_UINT, VIDEO_PIX_FMT_RGB565));
			mp_value_list_append(fmts,
					     mp_value_new(MP_TYPE_UINT, VIDEO_PIX_FMT_RGB565X));

			struct mp_structure *ns =
				mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_LIST,
						 fmts, MP_CAPS_END);
			if (w != NULL) {
				mp_structure_append(ns, MP_CAPS_IMAGE_WIDTH, mp_value_duplicate(w));
			}
			if (h != NULL) {
				mp_structure_append(ns, MP_CAPS_IMAGE_HEIGHT,
						    mp_value_duplicate(h));
			}
			if (fr != NULL) {
				mp_structure_append(ns, MP_CAPS_FRAME_RATE, mp_value_duplicate(fr));
			}
			mp_caps_append(out, ns);
		} else if (direction == MP_PAD_SINK) {
			/* RGB565{,X} -> JPEG */
			struct mp_structure *ns =
				mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
						 VIDEO_PIX_FMT_JPEG, MP_CAPS_END);
			if (w != NULL) {
				mp_structure_append(ns, MP_CAPS_IMAGE_WIDTH, mp_value_duplicate(w));
			}
			if (h != NULL) {
				mp_structure_append(ns, MP_CAPS_IMAGE_HEIGHT,
						    mp_value_duplicate(h));
			}
			if (fr != NULL) {
				mp_structure_append(ns, MP_CAPS_FRAME_RATE, mp_value_duplicate(fr));
			}
			mp_caps_append(out, ns);
		} else {
			/* Unknown direction: skip this structure */
		}
	}

	return out;
}

static int mp_img_jpeg_decoder_set_caps(struct mp_transform *transform,
				     enum mp_pad_direction direction, struct mp_caps *caps)
{
	struct mp_img_jpeg_decoder *dec = (struct mp_img_jpeg_decoder *)transform;
	struct mp_structure *s;
	struct mp_value *v;

	if (caps == NULL) {
		return -EINVAL;
	}

	if (direction == MP_PAD_SINK) {
		mp_caps_replace(&transform->sinkpad.caps, caps);

		return 0;
	}

	if (direction == MP_PAD_SRC) {
		s = mp_caps_get_structure(caps, 0);
		v = mp_structure_get_value(s, MP_CAPS_PIXEL_FORMAT);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			dec->out_pixfmt = mp_value_get_uint(v);
		}
		mp_caps_replace(&transform->srcpad.caps, caps);

		return 0;
	}

	return -EINVAL;
}

static int mp_img_jpeg_decoder_decide_allocation(struct mp_transform *self, struct mp_dispatch *query)
{
	struct mp_img_jpeg_decoder *dec = (struct mp_img_jpeg_decoder *)self;
	struct mp_buffer_pool *down_pool = mp_dispatch_get_pool(query);

	/* Default to our internal pool */
	self->outpool = &dec->out_pool;

	if (down_pool != NULL) {
		self->outpool = down_pool;
	}

	return 0;
}

void mp_img_jpeg_decoder_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_img_jpeg_decoder *dec = (struct mp_img_jpeg_decoder *)transform;
	struct mp_caps *sink_caps;
	struct mp_caps *src_caps;

	mp_transform_init(self);

	dec->out_pixfmt = VIDEO_PIX_FMT_RGB565;

	transform->mode = MP_MODE_NORMAL;
	transform->outpool = &dec->out_pool;

	/* Get supported caps */
	sink_caps = mp_img_jpeg_decoder_supported_caps(transform, MP_PAD_SINK);
	src_caps = mp_img_jpeg_decoder_supported_caps(transform, MP_PAD_SRC);
	mp_transform_update_caps(transform, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);

	mp_img_jpeg_decoder_outpool_init(dec);

	transform->set_caps = mp_img_jpeg_decoder_set_caps;
	transform->transform_caps = mp_img_jpeg_decoder_transform_caps;
	transform->sinkpad.chainfn = mp_img_jpeg_decoder_chainfn;
	transform->decide_allocation = mp_img_jpeg_decoder_decide_allocation;
}
