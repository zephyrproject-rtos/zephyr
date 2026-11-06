/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/drivers/video.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/img/mpipe_img_jpeg_decoder.h>

LOG_MODULE_REGISTER(mpipe_img_jpeg_decoder, CONFIG_MPIPE_LOG_LEVEL);

/* Internal output pool (used when downstream doesn't propose a pool) */
NET_BUF_POOL_FIXED_DEFINE(mpipe_img_dec_pool, CONFIG_MPIPE_IMG_JPEG_DECODER_POOL_NUM,
			  CONFIG_MPIPE_IMG_JPEG_DECODER_MAX_OUT_FRAME_SIZE,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

static int mpipe_img_jpeg_decoder_out_pool_acquire(struct mpipe_buffer_pool *pool,
						   struct net_buf **buf)
{
	struct net_buf *out;
	struct mpipe_buffer_meta *m;

	__ASSERT_NO_MSG(pool != NULL);
	__ASSERT_NO_MSG(buf != NULL);

	out = net_buf_alloc_len(pool->nb_pool, pool->config.size, K_NO_WAIT);
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

static int mpipe_img_jpeg_decoder_out_pool_release(struct mpipe_buffer_pool *pool,
						   struct net_buf *buf)
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

static void mpipe_img_jpeg_decoder_out_pool_init(struct mpipe_img_jpeg_decoder *dec)
{
	const struct mpipe_buffer_pool_config pool_req = {
		.size = CONFIG_MPIPE_IMG_JPEG_DECODER_MAX_OUT_FRAME_SIZE,
		.align = 1,
		.min_buffers = CONFIG_MPIPE_IMG_JPEG_DECODER_POOL_NUM,
		.max_buffers = CONFIG_MPIPE_IMG_JPEG_DECODER_POOL_NUM,
	};

	mpipe_buffer_pool_init(&dec->out_pool);
	dec->out_pool.nb_pool = &mpipe_img_dec_pool;
	(void)mpipe_buffer_pool_set_req_config(&dec->out_pool, &pool_req);
	dec->out_pool.acquire_buffer = mpipe_img_jpeg_decoder_out_pool_acquire;
	dec->out_pool.release_buffer = mpipe_img_jpeg_decoder_out_pool_release;
	dec->out_pool.started = true; /* net_buf pool is static; no explicit start */
}

static int mpipe_img_jpeg_decoder_decode_one(struct mpipe_img_jpeg_decoder *dec,
					     struct net_buf *in_buf, struct net_buf *out_buf)
{
	JPEGIMAGE *jpg = &dec->jpg;
	uint32_t in_sz = mpipe_buffer_get_meta(in_buf)->bytes_used;
	uint32_t out_sz;

	if (in_sz == 0) {
		mpipe_buffer_get_meta(out_buf)->bytes_used = 0;
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
	mpipe_buffer_get_meta(out_buf)->bytes_used = out_sz;
	out_buf->len = out_sz;

	JPEG_close(jpg);

	return 0;
}

static int mpipe_img_jpeg_decoder_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
					   struct net_buf **out_buf)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_img_jpeg_decoder *dec = (struct mpipe_img_jpeg_decoder *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	struct net_buf *cur;
	struct net_buf *next;

	*out_buf = NULL;

	if (out_pool == NULL) {
		out_pool = &dec->out_pool;
	}

	if (in_buf == NULL || out_pool->acquire_buffer == NULL) {
		return -EINVAL;
	}

	cur = in_buf;
	while (cur != NULL) {
		struct net_buf *out = NULL;
		int ret;

		/* Acquire first; on failure drop remaining input chain */
		ret = out_pool->acquire_buffer(out_pool, &out);
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

		if (mpipe_img_jpeg_decoder_decode_one(dec, cur, out) < 0) {
			net_buf_unref(out);
			net_buf_unref(cur);
			if (*out_buf != NULL) {
				net_buf_unref(*out_buf);
				*out_buf = NULL;
			}
			if (next != NULL) {
				net_buf_unref(next);
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

/* Output formats the decoder can produce, one enumeration index each */
static const uint32_t jpeg_decoder_out_pixfmts[] = {
	VIDEO_PIX_FMT_RGB565,
	VIDEO_PIX_FMT_RGB565X,
};

#define JPEG_DECODER_SINK_FIELDS(X) X(MPIPE_CAPS_PIXEL_FORMAT, MPIPE_VALUE_UINT(VIDEO_PIX_FMT_JPEG))

MPIPE_STRUCTURE_DEFINE(jpeg_decoder_sink_caps, MPIPE_MEDIA_VIDEO, JPEG_DECODER_SINK_FIELDS);

static int mpipe_img_jpeg_decoder_enum_caps(struct mpipe_pad *pad, uint32_t index,
					    const struct mpipe_structure *filter,
					    struct mpipe_structure *out)
{
	struct mpipe_structure candidate;
	struct mpipe_value fmt;
	int ret;

	if (pad->direction == MPIPE_PAD_SINK) {
		if (index > 0) {
			return -ENOENT;
		}

		return mpipe_pad_enum_filter(&jpeg_decoder_sink_caps, filter, out);
	}

	if (index >= ARRAY_SIZE(jpeg_decoder_out_pixfmts)) {
		return -ENOENT;
	}

	ret = mpipe_structure_init(&candidate, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	fmt.type = MPIPE_TYPE_UINT;
	fmt.v_uint = jpeg_decoder_out_pixfmts[index];
	ret = mpipe_structure_append_value(&candidate, MPIPE_CAPS_PIXEL_FORMAT, &fmt);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int mpipe_img_jpeg_decoder_transform_caps(struct mpipe_transform *transform,
						 enum mpipe_pad_direction direction,
						 const struct mpipe_structure *in, uint32_t index,
						 struct mpipe_structure *out)
{
	struct mpipe_value fmt;
	int ret;

	ARG_UNUSED(transform);

	if (in == NULL) {
		return -EINVAL;
	}

	if (mpipe_structure_is_any(in)) {
		/* Constrains nothing, so neither does the transformation of it */
		return (index > 0U) ? -ENOENT : mpipe_structure_init_any(out);
	}

	fmt.type = MPIPE_TYPE_UINT;

	if (direction == MPIPE_PAD_SRC) {
		/* JPEG -> one transformation per output format */
		if (index >= ARRAY_SIZE(jpeg_decoder_out_pixfmts)) {
			return -ENOENT;
		}

		fmt.v_uint = jpeg_decoder_out_pixfmts[index];
	} else if (direction == MPIPE_PAD_SINK) {
		/* RGB565{,X} -> JPEG */
		if (index > 0U) {
			return -ENOENT;
		}

		fmt.v_uint = VIDEO_PIX_FMT_JPEG;
	} else {
		return -EINVAL;
	}

	ret = mpipe_structure_init(out, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	ret = mpipe_structure_append_value(out, MPIPE_CAPS_PIXEL_FORMAT, &fmt);
	if (ret != 0) {
		mpipe_structure_clear(out);
		return ret;
	}

	/*
	 * Decoding changes the format, not the geometry or the timing. An input
	 * that does not constrain one of them is not an error - a field travels
	 * only when one side constrains it - but running out of room for one is.
	 */
	static const uint8_t passthrough[] = {MPIPE_CAPS_IMAGE_WIDTH, MPIPE_CAPS_IMAGE_HEIGHT,
					      MPIPE_CAPS_FRAME_INTERVAL};

	for (uint8_t i = 0; i < ARRAY_SIZE(passthrough); i++) {
		ret = mpipe_structure_copy_field(in, out, passthrough[i]);
		if (ret != 0 && ret != -ENOENT) {
			mpipe_structure_clear(out);
			return ret;
		}
	}

	return 0;
}

static int mpipe_img_jpeg_decoder_set_caps(struct mpipe_transform *transform,
					   enum mpipe_pad_direction direction,
					   const struct mpipe_structure *caps)
{
	struct mpipe_img_jpeg_decoder *dec = (struct mpipe_img_jpeg_decoder *)transform;
	const struct mpipe_value *v;

	if (caps == NULL) {
		return -EINVAL;
	}

	if (direction == MPIPE_PAD_SINK) {
		return mpipe_pad_set_caps(&transform->sink_pad, caps);
	}

	if (direction == MPIPE_PAD_SRC) {
		v = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			dec->out_pixfmt = mpipe_value_get_uint(v);
		}
		return mpipe_pad_set_caps(&transform->src_pad, caps);
	}

	return -EINVAL;
}

static int mpipe_img_jpeg_decoder_decide_buffer_pool(struct mpipe_transform *self,
						     struct mpipe_dispatch *query)
{
	struct mpipe_img_jpeg_decoder *dec = (struct mpipe_img_jpeg_decoder *)self;
	struct mpipe_buffer_pool *down_pool = query->pool;

	/* Default to our internal pool */
	self->out_pool = &dec->out_pool;

	if (down_pool != NULL) {
		self->out_pool = down_pool;
	}

	return 0;
}

int mpipe_img_jpeg_decoder_init(struct mpipe_img_jpeg_decoder *dec, uint8_t id)
{
	__ASSERT_NO_MSG(dec != NULL);

	struct mpipe_element *self = &dec->transform.element;
	struct mpipe_transform *transform = &dec->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "img_jpeg_decoder");

	dec->out_pixfmt = VIDEO_PIX_FMT_RGB565;

	transform->mode = MPIPE_MODE_NORMAL;
	transform->out_pool = &dec->out_pool;

	transform->sink_pad.enum_caps_fn = mpipe_img_jpeg_decoder_enum_caps;
	transform->src_pad.enum_caps_fn = mpipe_img_jpeg_decoder_enum_caps;

	mpipe_img_jpeg_decoder_out_pool_init(dec);

	transform->set_caps = mpipe_img_jpeg_decoder_set_caps;
	transform->transform_caps = mpipe_img_jpeg_decoder_transform_caps;
	transform->sink_pad.chain_fn = mpipe_img_jpeg_decoder_chain_fn;
	transform->decide_buffer_pool = mpipe_img_jpeg_decoder_decide_buffer_pool;

	return 0;
}
