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
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/mpipe/mpipe_buffer.h>
#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/nn/mpipe_nn_converter.h>

LOG_MODULE_REGISTER(mpipe_nn_converter, CONFIG_MPIPE_LOG_LEVEL);

/* Longest tensor row the dump prints: 96 pixels of 3 channels of float32 would be more */
#define MPIPE_NN_CONVERTER_DUMP_ROW_MAX 1024

/* Internal output pool (used when downstream doesn't propose a pool) */
NET_BUF_POOL_FIXED_DEFINE(mpipe_nn_converter_pool, CONFIG_MPIPE_NN_CONVERTER_POOL_NUM,
			  CONFIG_MPIPE_NN_CONVERTER_MAX_TENSOR_SIZE,
			  sizeof(struct mpipe_buffer_meta), mpipe_buffer_destroy);

/*
 * Quantize one 8-bit sample: the raw byte, or the byte shifted by
 * MPIPE_PROP_NN_CONVERTER_ZERO_POINT and clamped to the tensor type's range
 */
static uint8_t nn_converter_quantize(const struct mpipe_nn_converter *conv, uint32_t sample)
{
	int32_t v;

	if (!conv->zero_point_set) {
		return (uint8_t)sample;
	}

	v = (int32_t)sample + conv->zero_point;

	if (conv->out_type == MPIPE_NN_TENSOR_TYPE_INT8) {
		v = CLAMP(v, INT8_MIN, INT8_MAX);
		return (uint8_t)(int8_t)v;
	}

	v = CLAMP(v, 0, UINT8_MAX);

	return (uint8_t)v;
}

/*
 * The conversion functions run one output-pixel pass each: nearest-neighbor
 * subsampling, color conversion and quantization together, so no intermediate
 * frame ever exists.
 */
static int nn_converter_grey(const struct mpipe_nn_converter *conv, const struct net_buf *in,
			     struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			uint8_t sample = src[(uint32_t)iy * conv->in_w + ix];

			*dst++ = nn_converter_quantize(conv, sample);
		}
	}

	return 0;
}

static int nn_converter_rgb24(const struct mpipe_nn_converter *conv, const struct net_buf *in,
			      struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			const uint8_t *px = &src[((uint32_t)iy * conv->in_w + ix) * 3U];

			for (uint8_t c = 0; c < 3U; c++) {
				*dst++ = nn_converter_quantize(conv, px[c]);
			}
		}
	}

	return 0;
}

static int nn_converter_rgb565_luma(const struct mpipe_nn_converter *conv, const struct net_buf *in,
				    struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			uint32_t off = ((uint32_t)iy * conv->in_w + ix) * 2U;
			uint16_t px = (uint16_t)(src[off] | (src[off + 1U] << 8U));
			uint8_t r5 = (uint8_t)((px >> 11U) & 0x1FU);
			uint8_t g6 = (uint8_t)((px >> 5U) & 0x3FU);
			uint8_t b5 = (uint8_t)(px & 0x1FU);
			uint8_t r = (uint8_t)((r5 << 3U) | (r5 >> 2U));
			uint8_t g = (uint8_t)((g6 << 2U) | (g6 >> 4U));
			uint8_t b = (uint8_t)((b5 << 3U) | (b5 >> 2U));
			uint32_t luma = (77U * r + 150U * g + 29U * b) >> 8U;

			*dst++ = nn_converter_quantize(conv, luma);
		}
	}

	return 0;
}

static int nn_converter_bgrx32_luma(const struct mpipe_nn_converter *conv, const struct net_buf *in,
				    struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			const uint8_t *px = &src[((uint32_t)iy * conv->in_w + ix) * 4U];
			uint32_t luma = (77U * px[2] + 150U * px[1] + 29U * px[0]) >> 8U;

			*dst++ = nn_converter_quantize(conv, luma);
		}
	}

	return 0;
}

static int nn_converter_rgb565_rgb(const struct mpipe_nn_converter *conv, const struct net_buf *in,
				   struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			uint32_t off = ((uint32_t)iy * conv->in_w + ix) * 2U;
			uint16_t px = (uint16_t)(src[off] | (src[off + 1U] << 8U));
			uint8_t r5 = (uint8_t)((px >> 11U) & 0x1FU);
			uint8_t g6 = (uint8_t)((px >> 5U) & 0x3FU);
			uint8_t b5 = (uint8_t)(px & 0x1FU);

			*dst++ = nn_converter_quantize(conv, (uint8_t)((r5 << 3U) | (r5 >> 2U)));
			*dst++ = nn_converter_quantize(conv, (uint8_t)((g6 << 2U) | (g6 >> 4U)));
			*dst++ = nn_converter_quantize(conv, (uint8_t)((b5 << 3U) | (b5 >> 2U)));
		}
	}

	return 0;
}

static int nn_converter_bgrx32_rgb(const struct mpipe_nn_converter *conv, const struct net_buf *in,
				   struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			const uint8_t *px = &src[((uint32_t)iy * conv->in_w + ix) * 4U];

			*dst++ = nn_converter_quantize(conv, px[2]);
			*dst++ = nn_converter_quantize(conv, px[1]);
			*dst++ = nn_converter_quantize(conv, px[0]);
		}
	}

	return 0;
}

static int nn_converter_xyuv32_luma(const struct mpipe_nn_converter *conv, const struct net_buf *in,
				    struct net_buf *out)
{
	const uint8_t *src = in->data;
	uint8_t *dst = out->data;

	for (uint16_t oy = 0; oy < conv->out_h; oy++) {
		uint16_t iy = (uint16_t)((uint32_t)oy * conv->in_h / conv->out_h);

		for (uint16_t ox = 0; ox < conv->out_w; ox++) {
			uint16_t ix = (uint16_t)((uint32_t)ox * conv->in_w / conv->out_w);
			/* The luma is the Y byte of each XYUV pixel */
			uint8_t sample = src[((uint32_t)iy * conv->in_w + ix) * 4U + 1U];

			*dst++ = nn_converter_quantize(conv, sample);
		}
	}

	return 0;
}

static const struct mpipe_nn_converter_desc nn_converter_descs[] = {
	{VIDEO_PIX_FMT_GREY, 1, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_grey},
	{VIDEO_PIX_FMT_GREY, 1, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_grey},
	{VIDEO_PIX_FMT_RGB24, 3, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_rgb24},
	{VIDEO_PIX_FMT_RGB24, 3, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_rgb24},
	{VIDEO_PIX_FMT_RGB565, 1, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_rgb565_luma},
	{VIDEO_PIX_FMT_RGB565, 1, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_rgb565_luma},
	{VIDEO_PIX_FMT_RGB565, 3, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_rgb565_rgb},
	{VIDEO_PIX_FMT_RGB565, 3, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_rgb565_rgb},
	{VIDEO_PIX_FMT_BGRX32, 1, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_bgrx32_luma},
	{VIDEO_PIX_FMT_BGRX32, 1, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_bgrx32_luma},
	{VIDEO_PIX_FMT_BGRX32, 3, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_bgrx32_rgb},
	{VIDEO_PIX_FMT_BGRX32, 3, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_bgrx32_rgb},
	{VIDEO_PIX_FMT_XYUV32, 1, MPIPE_NN_TENSOR_TYPE_INT8, nn_converter_xyuv32_luma},
	{VIDEO_PIX_FMT_XYUV32, 1, MPIPE_NN_TENSOR_TYPE_UINT8, nn_converter_xyuv32_luma},
};

/*
 * The pixel format at @p index among the distinct ones the table handles.
 * Returns 0 past the last one.
 */
static uint32_t nn_converter_pixfmt_at(uint32_t index)
{
	uint32_t matched = 0;

	for (size_t i = 0; i < ARRAY_SIZE(nn_converter_descs); i++) {
		bool seen = false;

		for (size_t j = 0; j < i; j++) {
			if (nn_converter_descs[j].pixfmt == nn_converter_descs[i].pixfmt) {
				seen = true;
				break;
			}
		}

		if (seen) {
			continue;
		}

		if (matched == index) {
			return nn_converter_descs[i].pixfmt;
		}

		matched++;
	}

	return 0;
}

/*
 * The distinct (type, channels) pair at @p index on the tensor side.
 * Returns NULL past the last one.
 */
static const struct mpipe_nn_converter_desc *nn_converter_pair_at(uint32_t index)
{
	uint32_t matched = 0;

	for (size_t i = 0; i < ARRAY_SIZE(nn_converter_descs); i++) {
		bool seen = false;

		for (size_t j = 0; j < i; j++) {
			if (nn_converter_descs[j].type == nn_converter_descs[i].type &&
			    nn_converter_descs[j].channels == nn_converter_descs[i].channels) {
				seen = true;
				break;
			}
		}

		if (seen) {
			continue;
		}

		if (matched == index) {
			return &nn_converter_descs[i];
		}

		matched++;
	}

	return NULL;
}

static int nn_converter_video_candidate(uint32_t pixfmt, struct mpipe_structure *out)
{
	struct mpipe_value v;
	int ret;

	ret = mpipe_structure_init(out, MPIPE_MEDIA_VIDEO);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = pixfmt;

	ret = mpipe_structure_append_value(out, MPIPE_CAPS_PIXEL_FORMAT, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
	}

	return ret;
}

static int nn_converter_tensor_candidate(uint8_t type, uint8_t channels,
					 struct mpipe_structure *out)
{
	struct mpipe_value v;
	int ret;

	ret = mpipe_structure_init(out, MPIPE_MEDIA_TENSOR);
	if (ret != 0) {
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = type;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_TENSOR_TYPE, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
		return ret;
	}

	v.type = MPIPE_TYPE_UINT;
	v.v_uint = channels;
	ret = mpipe_structure_append_value(out, MPIPE_CAPS_NUM_CHANNELS, &v);
	if (ret != 0) {
		mpipe_structure_clear(out);
	}

	return ret;
}

static int nn_converter_enum_caps(struct mpipe_pad *pad, uint32_t index,
				  const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_structure candidate;
	int ret;

	if (pad->direction == MPIPE_PAD_SINK) {
		uint32_t pixfmt = nn_converter_pixfmt_at(index);

		if (pixfmt == 0U) {
			return -ENOENT;
		}

		ret = nn_converter_video_candidate(pixfmt, &candidate);
	} else {
		const struct mpipe_nn_converter_desc *pair = nn_converter_pair_at(index);

		if (pair == NULL) {
			return -ENOENT;
		}

		ret = nn_converter_tensor_candidate(pair->type, pair->channels, &candidate);
	}

	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

/*
 * The table entry at @p index whose video side matches @p pixfmt, or whose
 * tensor side matches @p in when transforming toward the video side.
 */
static const struct mpipe_nn_converter_desc *
nn_converter_match_at(enum mpipe_pad_direction direction, const struct mpipe_structure *in,
		      uint32_t index)
{
	const struct mpipe_value *v;
	uint32_t matched = 0;

	for (size_t i = 0; i < ARRAY_SIZE(nn_converter_descs); i++) {
		const struct mpipe_nn_converter_desc *desc = &nn_converter_descs[i];

		if (direction == MPIPE_PAD_SRC) {
			/* Video in, tensor out: match the input pixel format */
			v = mpipe_structure_get_value(in, MPIPE_CAPS_PIXEL_FORMAT);
			if (v == NULL || v->type != MPIPE_TYPE_UINT ||
			    mpipe_value_get_uint(v) != desc->pixfmt) {
				continue;
			}
		} else {
			/* Tensor in, video out: match type and, when present, channels */
			v = mpipe_structure_get_value(in, MPIPE_CAPS_TENSOR_TYPE);
			if (v == NULL || v->type != MPIPE_TYPE_UINT ||
			    mpipe_value_get_uint(v) != desc->type) {
				continue;
			}

			v = mpipe_structure_get_value(in, MPIPE_CAPS_NUM_CHANNELS);
			if (v != NULL && v->type == MPIPE_TYPE_UINT &&
			    mpipe_value_get_uint(v) != desc->channels) {
				continue;
			}
		}

		if (matched == index) {
			return desc;
		}

		matched++;
	}

	return NULL;
}

static int nn_converter_transform_caps(struct mpipe_transform *self,
				       enum mpipe_pad_direction direction,
				       const struct mpipe_structure *in, uint32_t index,
				       struct mpipe_structure *out)
{
	const struct mpipe_nn_converter_desc *desc;
	int ret;

	ARG_UNUSED(self);

	if (in == NULL) {
		return -EINVAL;
	}

	if (mpipe_structure_is_any(in)) {
		/* Constrains nothing: everything this side produces is reachable */
		if (direction == MPIPE_PAD_SRC) {
			desc = nn_converter_pair_at(index);
		} else {
			uint32_t pixfmt = nn_converter_pixfmt_at(index);

			if (pixfmt == 0U) {
				return -ENOENT;
			}

			return nn_converter_video_candidate(pixfmt, out);
		}
	} else {
		desc = nn_converter_match_at(direction, in, index);
	}

	if (desc == NULL) {
		return -ENOENT;
	}

	if (direction == MPIPE_PAD_SRC) {
		ret = nn_converter_tensor_candidate(desc->type, desc->channels, out);
	} else {
		ret = nn_converter_video_candidate(desc->pixfmt, out);
	}

	if (ret != 0) {
		return ret;
	}

	/*
	 * The element scales, so geometry does not cross the media boundary;
	 * only the frame interval does, and an input without one is not an error
	 */
	ret = mpipe_structure_copy_field(in, out, MPIPE_CAPS_FRAME_INTERVAL);
	if (ret != 0 && ret != -ENOENT) {
		mpipe_structure_clear(out);
		return ret;
	}

	return 0;
}

static int nn_converter_set_caps(struct mpipe_transform *transform,
				 enum mpipe_pad_direction direction,
				 const struct mpipe_structure *caps)
{
	struct mpipe_nn_converter *conv = (struct mpipe_nn_converter *)transform;
	const struct mpipe_value *v;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	ret = mpipe_transform_set_caps(transform, direction, caps);
	if (ret < 0) {
		return ret;
	}

	if (direction == MPIPE_PAD_SINK) {
		v = mpipe_structure_get_value(caps, MPIPE_CAPS_PIXEL_FORMAT);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->in_pixfmt = mpipe_value_get_uint(v);
		}

		v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_WIDTH);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->in_w = (uint16_t)mpipe_value_get_uint(v);
		}

		v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_HEIGHT);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->in_h = (uint16_t)mpipe_value_get_uint(v);
		}
	} else {
		v = mpipe_structure_get_value(caps, MPIPE_CAPS_TENSOR_TYPE);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->out_type = (uint8_t)mpipe_value_get_uint(v);
		}

		v = mpipe_structure_get_value(caps, MPIPE_CAPS_NUM_CHANNELS);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->out_channels = (uint8_t)mpipe_value_get_uint(v);
		}

		v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_WIDTH);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->out_w = (uint16_t)mpipe_value_get_uint(v);
		}

		v = mpipe_structure_get_value(caps, MPIPE_CAPS_IMAGE_HEIGHT);
		if (v != NULL && v->type == MPIPE_TYPE_UINT) {
			conv->out_h = (uint16_t)mpipe_value_get_uint(v);
		}
	}

	/* Once both sides are known, pick the conversion entry */
	if (conv->in_pixfmt != 0U && conv->out_type != 0U && conv->out_channels != 0U) {
		conv->desc = NULL;
		for (size_t i = 0; i < ARRAY_SIZE(nn_converter_descs); i++) {
			if (nn_converter_descs[i].pixfmt == conv->in_pixfmt &&
			    nn_converter_descs[i].type == conv->out_type &&
			    nn_converter_descs[i].channels == conv->out_channels) {
				conv->desc = &nn_converter_descs[i];
				break;
			}
		}
		if (conv->desc == NULL) {
			LOG_ERR("Unsupported conversion 0x%08x -> type %u ch %u", conv->in_pixfmt,
				conv->out_type, conv->out_channels);
			return -ENOTSUP;
		}
	}

	if (conv->out_w != 0U && conv->out_h != 0U && conv->out_channels != 0U &&
	    conv->out_type != 0U) {
		uint64_t size = (uint64_t)conv->out_w * conv->out_h * conv->out_channels *
				mpipe_nn_tensor_type_size(conv->out_type);

		if (size > CONFIG_MPIPE_NN_CONVERTER_MAX_TENSOR_SIZE) {
			LOG_ERR("Tensor size %llu exceeds "
				"CONFIG_MPIPE_NN_CONVERTER_MAX_TENSOR_SIZE=%u",
				size, CONFIG_MPIPE_NN_CONVERTER_MAX_TENSOR_SIZE);
			return -ENOMEM;
		}

		conv->out_pool.config.size = (uint32_t)size;
	}

	return 0;
}

static int nn_converter_decide_buffer_pool(struct mpipe_transform *self,
					   struct mpipe_dispatch *query)
{
	struct mpipe_nn_converter *conv = (struct mpipe_nn_converter *)self;
	struct mpipe_buffer_pool *down_pool = query->pool;

	/* Use the internal pool by default, a downstream proposal when available */
	self->out_pool = &conv->out_pool;

	if (down_pool != NULL) {
		self->out_pool = down_pool;
	}

	return 0;
}

#ifdef CONFIG_MPIPE_NN_CONVERTER_DUMP
/*
 * Print one tensor as hex rows, one row per line of the image, so a host
 * script can reassemble the picture the model is about to see.
 */
static void nn_converter_dump_tensor(const struct mpipe_nn_converter *conv,
				     const struct net_buf *out)
{
	static uint32_t frames;
	static char line[16 + 2 * MPIPE_NN_CONVERTER_DUMP_ROW_MAX];
	static const char hex[] = "0123456789abcdef";
	const uint8_t *data = out->data;
	uint32_t row_bytes = (uint32_t)conv->out_w * conv->out_channels *
			     mpipe_nn_tensor_type_size(conv->out_type);

	if ((frames++ % CONFIG_MPIPE_NN_CONVERTER_DUMP_PERIOD) != 0U) {
		return;
	}

	if (row_bytes > MPIPE_NN_CONVERTER_DUMP_ROW_MAX) {
		printk("nn_converter_dump: row of %u bytes too long to dump\n", row_bytes);
		return;
	}

	printk("nn_converter_dump: %ux%ux%u type %u shift %d from %ux%u 0x%08x\n", conv->out_w,
	       conv->out_h, conv->out_channels, conv->out_type,
	       conv->zero_point_set ? conv->zero_point : 0, conv->in_w, conv->in_h,
	       conv->in_pixfmt);

	/* One printk per row: a console shell repaints its prompt between two calls */
	for (uint16_t y = 0; y < conv->out_h; y++) {
		const uint8_t *row = &data[(uint32_t)y * row_bytes];
		size_t n = 0;

		for (uint32_t i = 0; i < row_bytes; i++) {
			line[n++] = hex[row[i] >> 4];
			line[n++] = hex[row[i] & 0x0fU];
		}
		line[n] = '\0';
		printk("nn_converter_row:%s\n", line);
	}

	printk("nn_converter_dump: end\n");
}
#endif /* CONFIG_MPIPE_NN_CONVERTER_DUMP */

static int nn_converter_process_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				   struct net_buf **out_buf)
{
	struct mpipe_transform *transform = (struct mpipe_transform *)pad->object.container;
	struct mpipe_nn_converter *conv = (struct mpipe_nn_converter *)transform;
	struct mpipe_buffer_pool *out_pool = transform->out_pool;
	uint32_t in_sz;
	uint32_t out_sz;
	struct net_buf *out = NULL;
	struct mpipe_buffer_meta *meta;

	*out_buf = NULL;

	if (conv->in_w == 0U || conv->in_h == 0U || conv->out_w == 0U || conv->out_h == 0U ||
	    conv->desc == NULL) {
		LOG_ERR("Missing negotiated caps / conversion");
		net_buf_unref(in_buf);
		return -EINVAL;
	}

	in_sz = (uint32_t)conv->in_w * conv->in_h * video_bits_per_pixel(conv->in_pixfmt) /
		BITS_PER_BYTE;
	out_sz = (uint32_t)conv->out_w * conv->out_h * conv->out_channels *
		 mpipe_nn_tensor_type_size(conv->out_type);

	if (mpipe_buffer_get_meta(in_buf)->bytes_used < in_sz) {
		LOG_ERR("Input frame too small: %u < %u", mpipe_buffer_get_meta(in_buf)->bytes_used,
			in_sz);
		goto err;
	}

	if (out_pool->acquire_buffer(out_pool, &out) != 0 || out == NULL) {
		LOG_DBG("No free tensor buffer, frame dropped");
		net_buf_unref(in_buf);
		return 0;
	}

	if (out->size < out_sz) {
		LOG_ERR("Output buffer too small: %u < %u", out->size, out_sz);
		net_buf_unref(out);
		goto err;
	}

	if (conv->desc->fn(conv, in_buf, out) != 0) {
		LOG_ERR("Failed to convert frame to tensor");
		net_buf_unref(out);
		goto err;
	}

#ifdef CONFIG_MPIPE_NN_CONVERTER_DUMP
	nn_converter_dump_tensor(conv, out);
#endif

	meta = mpipe_buffer_get_meta(out);
	meta->bytes_used = out_sz;
	meta->pts = mpipe_buffer_get_meta(in_buf)->pts;
	out->len = out_sz;

	net_buf_unref(in_buf);
	*out_buf = out;

	return 0;
err:
	net_buf_unref(in_buf);

	return -EIO;
}

static int nn_converter_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_nn_converter *conv = (struct mpipe_nn_converter *)obj;

	switch (key) {
	case MPIPE_PROP_NN_CONVERTER_ZERO_POINT:
		if (val == NULL) {
			return -EINVAL;
		}
		conv->zero_point = *(const int32_t *)val;
		conv->zero_point_set = true;

		return 0;
	default:
		LOG_ERR("Property %u is unknown", key);
		return -ENOTSUP;
	}
}

static int nn_converter_out_pool_acquire(struct mpipe_buffer_pool *pool, struct net_buf **buf)
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
	m->pts = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	out->len = 0;

	*buf = out;

	return 0;
}

static int nn_converter_out_pool_release(struct mpipe_buffer_pool *pool, struct net_buf *buf)
{
	ARG_UNUSED(pool);

	__ASSERT_NO_MSG(buf != NULL);

	struct mpipe_buffer_meta *m = mpipe_buffer_get_meta(buf);

	m->bytes_used = 0;
	m->pts = 0;
	m->driver_buf = NULL;
	m->priv = NULL;
	buf->len = 0;

	return 0;
}

int mpipe_nn_converter_init(struct mpipe_nn_converter *conv, uint8_t id)
{
	__ASSERT_NO_MSG(conv != NULL);

	struct mpipe_element *self = &conv->transform.element;
	struct mpipe_transform *transform = &conv->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "nn_converter");

	self->object.set_property = nn_converter_set_property;

	transform->sink_pad.enum_caps_fn = nn_converter_enum_caps;
	transform->src_pad.enum_caps_fn = nn_converter_enum_caps;

	conv->in_w = 0;
	conv->in_h = 0;
	conv->out_w = 0;
	conv->out_h = 0;
	conv->in_pixfmt = 0;
	conv->out_type = 0;
	conv->out_channels = 0;
	conv->zero_point = 0;
	conv->zero_point_set = false;
	conv->desc = NULL;

	/* The net_buf pool is static: no start hook, the size follows the caps */
	const struct mpipe_buffer_pool_config pool_req = {
		.size = 0,
		.align = 1,
		.min_buffers = CONFIG_MPIPE_NN_CONVERTER_POOL_NUM,
		.max_buffers = CONFIG_MPIPE_NN_CONVERTER_POOL_NUM,
	};

	mpipe_buffer_pool_init(&conv->out_pool);
	conv->out_pool.nb_pool = &mpipe_nn_converter_pool;
	(void)mpipe_buffer_pool_set_req_config(&conv->out_pool, &pool_req);
	conv->out_pool.acquire_buffer = nn_converter_out_pool_acquire;
	conv->out_pool.release_buffer = nn_converter_out_pool_release;

	transform->mode = MPIPE_TRANSFORM_MODE_NORMAL;
	transform->out_pool = &conv->out_pool;
	transform->set_caps = nn_converter_set_caps;
	transform->transform_caps = nn_converter_transform_caps;
	transform->propose_buffer_pool = NULL;
	transform->decide_buffer_pool = nn_converter_decide_buffer_pool;
	transform->sink_pad.process_fn = nn_converter_process_fn;

	return 0;
}
