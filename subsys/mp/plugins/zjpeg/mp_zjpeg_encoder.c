/*
 * Copyright 2026 Meta Platforms, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mp/core/mp_buffer.h>
#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_dispatch.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zjpeg/mp_zjpeg_encoder.h>

LOG_MODULE_REGISTER(mp_zjpeg_encoder, CONFIG_MP_LOG_LEVEL);

/* Internal output pool (used when downstream doesn't propose a pool) */
NET_BUF_POOL_FIXED_DEFINE(mp_zjpeg_enc_pool, CONFIG_MP_ZJPEG_ENCODER_POOL_NUM,
			  CONFIG_MP_ZJPEG_ENCODER_MAX_OUT_FRAME_SIZE, sizeof(struct mp_buffer_meta),
			  mp_buffer_destroy);

static int mp_zjpeg_encoder_outpool_acquire(struct mp_buffer_pool *pool, struct net_buf **buf)
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
	m->priv = NULL;
	out->len = 0;

	*buf = out;

	return 0;
}

static int mp_zjpeg_encoder_outpool_release(struct mp_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	if (buf != NULL) {
		struct mp_buffer_meta *m = mp_buffer_get_meta(buf);

		if (m != NULL) {
			m->bytes_used = 0;
			m->timestamp = 0;
			m->priv = NULL;
		}
		buf->len = 0;
	}

	return 0;
}

static void mp_zjpeg_encoder_outpool_init(struct mp_zjpeg_encoder *enc)
{
	mp_buffer_pool_init(&enc->out_pool);
	enc->out_pool.nb_pool = &mp_zjpeg_enc_pool;
	enc->out_pool.config.size = CONFIG_MP_ZJPEG_ENCODER_MAX_OUT_FRAME_SIZE;
	enc->out_pool.config.align = 1;
	enc->out_pool.config.min_buffers = CONFIG_MP_ZJPEG_ENCODER_POOL_NUM;
	enc->out_pool.config.max_buffers = CONFIG_MP_ZJPEG_ENCODER_POOL_NUM;
	enc->out_pool.acquire_buffer = mp_zjpeg_encoder_outpool_acquire;
	enc->out_pool.release_buffer = mp_zjpeg_encoder_outpool_release;
	enc->out_pool.started = true; /* net_buf pool is static; no explicit start */
}

static int mp_zjpeg_encoder_encode_one(struct mp_zjpeg_encoder *enc, struct net_buf *in_buf,
				       struct net_buf *out_buf)
{
	JPEGE_IMAGE *jpe = &enc->jpe;
	JPEGENCODE jpec;
	uint32_t in_sz = mp_buffer_get_meta(in_buf)->bytes_used;
	int pitch = (int)enc->width * 2; /* RGB565: 2 bytes per pixel */
	int out_sz;
	int rc;

	if (in_sz == 0) {
		mp_buffer_get_meta(out_buf)->bytes_used = 0;
		out_buf->len = 0;
		return -ENODATA;
	}

	if (enc->width == 0 || enc->height == 0) {
		LOG_ERR("Frame dimensions not negotiated (w=%u h=%u)", enc->width, enc->height);
		return -EINVAL;
	}

	if (in_sz < (uint32_t)pitch * enc->height) {
		LOG_ERR("Input frame too small (%u < %u)", in_sz,
			(uint32_t)pitch * enc->height);
		return -EINVAL;
	}

	rc = JPEGOpenRAM(jpe, out_buf->data, (int)out_buf->size);
	if (rc != JPEGE_SUCCESS) {
		LOG_ERR("JPEGOpenRAM failed (%d)", rc);
		return -EINVAL;
	}

	rc = JPEGEncodeBegin(jpe, &jpec, (int)enc->width, (int)enc->height, JPEGE_PIXEL_RGB565,
			     enc->subsample, enc->qfactor);
	if (rc != JPEGE_SUCCESS) {
		LOG_ERR("JPEGEncodeBegin failed (%d)", rc);
		return -EINVAL;
	}

	rc = JPEGEncodeFrame(jpe, &jpec, in_buf->data, pitch);
	if (rc != JPEGE_SUCCESS) {
		LOG_ERR("JPEGEncodeFrame failed (%d)", JPEGGetLastError(jpe));
		return -EIO;
	}

	out_sz = JPEGEncodeEnd(jpe);
	if (out_sz <= 0) {
		LOG_ERR("JPEGEncodeEnd failed (%d)", JPEGGetLastError(jpe));
		return -EIO;
	}

	mp_buffer_get_meta(out_buf)->bytes_used = (uint32_t)out_sz;
	out_buf->len = (uint32_t)out_sz;

	return 0;
}

static int mp_zjpeg_encoder_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				    struct net_buf **out_buf)
{
	struct mp_transform *transform = (struct mp_transform *)pad->object.container;
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(transform);
	struct mp_buffer_pool *outpool = transform->outpool;
	struct net_buf *cur;
	struct net_buf *next;

	*out_buf = NULL;

	if (outpool == NULL) {
		outpool = &enc->out_pool;
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

		if (mp_zjpeg_encoder_encode_one(enc, cur, out) < 0) {
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

static struct mp_caps *mp_zjpeg_encoder_supported_caps(struct mp_transform *transform,
						       enum mp_pad_direction direction)
{
	ARG_UNUSED(transform);

	if (direction == MP_PAD_SINK) {
		return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
				   VIDEO_PIX_FMT_RGB565, MP_CAPS_END);
	}

	if (direction == MP_PAD_SRC) {
		return mp_caps_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
				   VIDEO_PIX_FMT_JPEG, MP_CAPS_END);
	}

	return NULL;
}

static struct mp_caps *mp_zjpeg_encoder_transform_caps(struct mp_transform *transform,
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
		struct mp_structure *ns;

		if (direction == MP_PAD_SRC) {
			/* RGB565 -> JPEG */
			ns = mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
					      VIDEO_PIX_FMT_JPEG, MP_CAPS_END);
		} else if (direction == MP_PAD_SINK) {
			/* JPEG -> RGB565 */
			ns = mp_structure_new(MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT,
					      VIDEO_PIX_FMT_RGB565, MP_CAPS_END);
		} else {
			/* Unknown direction: skip this structure */
			continue;
		}

		if (ns == NULL) {
			continue;
		}

		if (w != NULL) {
			mp_structure_append(ns, MP_CAPS_IMAGE_WIDTH, mp_value_duplicate(w));
		}
		if (h != NULL) {
			mp_structure_append(ns, MP_CAPS_IMAGE_HEIGHT, mp_value_duplicate(h));
		}
		if (fr != NULL) {
			mp_structure_append(ns, MP_CAPS_FRAME_RATE, mp_value_duplicate(fr));
		}
		mp_caps_append(out, ns);
	}

	return out;
}

static int mp_zjpeg_encoder_set_caps(struct mp_transform *transform,
				     enum mp_pad_direction direction, struct mp_caps *caps)
{
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(transform);
	struct mp_structure *s;
	struct mp_value *v;

	if (caps == NULL) {
		return -EINVAL;
	}

	if (direction == MP_PAD_SINK) {
		/* Cache width/height; the encoder needs the dimensions before encoding. */
		s = mp_caps_get_structure(caps, 0);
		v = mp_structure_get_value(s, MP_CAPS_IMAGE_WIDTH);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			enc->width = mp_value_get_uint(v);
		}
		v = mp_structure_get_value(s, MP_CAPS_IMAGE_HEIGHT);
		if (v != NULL && v->type == MP_TYPE_UINT) {
			enc->height = mp_value_get_uint(v);
		}
		mp_caps_replace(&transform->sinkpad.caps, caps);

		return 0;
	}

	if (direction == MP_PAD_SRC) {
		mp_caps_replace(&transform->srcpad.caps, caps);

		return 0;
	}

	return -EINVAL;
}

static int mp_zjpeg_encoder_decide_allocation(struct mp_transform *self, struct mp_dispatch *query)
{
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(self);
	struct mp_buffer_pool *down_pool = mp_dispatch_get_pool(query);

	/* Default to our internal pool */
	self->outpool = &enc->out_pool;

	if (down_pool != NULL) {
		self->outpool = down_pool;
	}

	return 0;
}

static int mp_zjpeg_encoder_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(obj);

	switch (key) {
	case PROP_ZJPEG_ENC_QUALITY:
		enc->qfactor = (uint8_t)(uintptr_t)val;
		return 0;
	case PROP_ZJPEG_ENC_SUBSAMPLE:
		enc->subsample = (uint8_t)(uintptr_t)val;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_zjpeg_encoder_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(obj);

	switch (key) {
	case PROP_ZJPEG_ENC_QUALITY:
		*(uint8_t *)val = enc->qfactor;
		return 0;
	case PROP_ZJPEG_ENC_SUBSAMPLE:
		*(uint8_t *)val = enc->subsample;
		return 0;
	default:
		return -ENOTSUP;
	}
}

void mp_zjpeg_encoder_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_zjpeg_encoder *enc = MP_ZJPEG_ENCODER(self);
	struct mp_caps *sink_caps;
	struct mp_caps *src_caps;

	mp_transform_init(self);

	enc->subsample = JPEGE_SUBSAMPLE_420;
	enc->qfactor = JPEGE_Q_BEST;
	enc->width = 0;
	enc->height = 0;

	transform->mode = MP_MODE_NORMAL;
	transform->outpool = &enc->out_pool;

	/* Get supported caps */
	sink_caps = mp_zjpeg_encoder_supported_caps(transform, MP_PAD_SINK);
	src_caps = mp_zjpeg_encoder_supported_caps(transform, MP_PAD_SRC);
	mp_transform_update_caps(transform, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);

	mp_zjpeg_encoder_outpool_init(enc);

	self->object.set_property = mp_zjpeg_encoder_set_property;
	self->object.get_property = mp_zjpeg_encoder_get_property;

	transform->set_caps = mp_zjpeg_encoder_set_caps;
	transform->transform_caps = mp_zjpeg_encoder_transform_caps;
	transform->sinkpad.chainfn = mp_zjpeg_encoder_chainfn;
	transform->decide_allocation = mp_zjpeg_encoder_decide_allocation;
}
