/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_gain.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

LOG_MODULE_REGISTER(mp_zaud_gain, CONFIG_MP_LOG_LEVEL);

/* Gain percentage definitions */
#define GAIN_PERCENT_MIN   0    /* 0% = mute */
#define GAIN_PERCENT_MAX   1000 /* 1000% = 10x amplification */
#define GAIN_PERCENT_UNITY 100  /* 100% = unity gain (no change) */

/* Q16.16 signed fixed-point definitions */
#define GAIN_UNITY_FIXED            (1 << 16) /* 65536 = 1.0 in Q16.16 */
#define GAIN_MULTIPLY(sample, gain) (((int32_t)(sample) * (gain)) >> 16)

/* Convert percentage to Q16.16 fixed-point gain */
static int32_t percent_to_fixed_gain(int gain_percent)
{
	/* Clamp percentage to valid range */
	if (gain_percent < GAIN_PERCENT_MIN) {
		gain_percent = GAIN_PERCENT_MIN;
	} else if (gain_percent > GAIN_PERCENT_MAX) {
		gain_percent = GAIN_PERCENT_MAX;
	} else {
		/* gain_percent is within valid range, no clamping needed */
	}

	/* Convert: gain_percent% → Q16.16 fixed point */
	return (gain_percent * GAIN_UNITY_FIXED) / 100;
}

static int mp_zaud_gain_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zaud_gain *self = (struct mp_zaud_gain *)obj;

	switch (key) {
	case PROP_GAIN:
		/* Expect percentage value (0-1000) */
		self->gain_percent = *(const int *)val;
		self->gain_fixed = percent_to_fixed_gain(self->gain_percent);

		/* Update mute flag for optimization */
		self->mute = (self->gain_percent == 0);

		LOG_DBG("Zaud gain set to %d%% (fixed: %d)\n", self->gain_percent,
			self->gain_fixed);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mp_zaud_gain_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zaud_gain *self = (struct mp_zaud_gain *)obj;

	switch (key) {
	case PROP_GAIN:
		/* Return percentage value */
		*(int *)val = self->gain_percent;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static void apply_gain_16bit(struct net_buf *buffer, int32_t gain_fixed)
{
	int16_t *samples = (int16_t *)buffer->data;
	size_t num_samples = mp_buffer_get_meta(buffer)->bytes_used / sizeof(int16_t);

	for (size_t i = 0; i < num_samples; i++) {
		/* Apply gain using fixed-point arithmetic */
		int32_t temp = GAIN_MULTIPLY(samples[i], gain_fixed);

		/* Clamp to 16-bit range */
		if (temp > INT16_MAX) {
			samples[i] = INT16_MAX;
		} else if (temp < INT16_MIN) {
			samples[i] = INT16_MIN;
		} else {
			samples[i] = (int16_t)temp;
		}
	}
}

static void apply_gain_32bit(struct net_buf *buffer, int32_t gain_fixed)
{
	int32_t *samples = (int32_t *)buffer->data;
	size_t num_samples = mp_buffer_get_meta(buffer)->bytes_used / sizeof(int32_t);

	for (size_t i = 0; i < num_samples; i++) {
		/* Apply gain using fixed-point arithmetic with 64-bit intermediate */
		int64_t temp = ((int64_t)samples[i] * gain_fixed) >> 16;

		/* Clamp to 32-bit range */
		if (temp > INT32_MAX) {
			samples[i] = INT32_MAX;
		} else if (temp < INT32_MIN) {
			samples[i] = INT32_MIN;
		} else {
			samples[i] = (int32_t)temp;
		}
	}
}

/*
 * 24-bit PCM handling
 *
 * In this plugin chain, buffers are typically sized using (bit_width / 8).
 * For 24-bit that implies 3 bytes/sample (packed little-endian).
 *
 * Some backends may still deliver 24-bit samples in a 32-bit container.
 * We support both:
 * - packed 24-bit LE:  [b0, b1, b2] per sample
 * - 32-bit container: signed 24-bit in bits [31:8] (left-justified)
 */
static void apply_gain_24bit(struct net_buf *buffer, int32_t gain_fixed)
{
	const size_t sz = mp_buffer_get_meta(buffer)->bytes_used;

	/* Prefer packed 3-byte samples when buffer size is divisible by 3 */
	if ((sz % 3U) == 0U) {
		uint8_t *p = (uint8_t *)buffer->data;
		size_t num_samples = sz / 3U;

		for (size_t i = 0; i < num_samples; i++) {
			uint32_t u =
				(uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
			/* sign-extend 24-bit */
			int32_t s24 = (u & 0x800000U) ? (int32_t)(u | 0xFF000000U) : (int32_t)u;
			int64_t temp = ((int64_t)s24 * gain_fixed) >> 16;

			if (temp > 0x7FFFFF) {
				temp = 0x7FFFFF;
			} else if (temp < -0x800000) {
				temp = -0x800000;
			} else {
				/* value within 24-bit range */
			}

			uint32_t out = (uint32_t)((int32_t)temp) & 0xFFFFFFU;

			p[0] = (uint8_t)(out & 0xFFU);
			p[1] = (uint8_t)((out >> 8) & 0xFFU);
			p[2] = (uint8_t)((out >> 16) & 0xFFU);
			p += 3;
		}
		return;
	}

	/* Fallback: treat as 32-bit container */
	if ((sz % sizeof(int32_t)) == 0U) {
		int32_t *samples = (int32_t *)buffer->data;
		size_t num_samples = sz / sizeof(int32_t);

		for (size_t i = 0; i < num_samples; i++) {
			int32_t s24 = samples[i] >> 8;
			int64_t temp = ((int64_t)s24 * gain_fixed) >> 16;

			if (temp > 0x7FFFFF) {
				temp = 0x7FFFFF;
			} else if (temp < -0x800000) {
				temp = -0x800000;
			} else {
				/* value within 24-bit range */
			}
			samples[i] = ((int32_t)temp) << 8;
		}
		return;
	}

	LOG_ERR("24-bit buffer size not aligned (size=%u)", (unsigned int)sz);
}

static void apply_audio_gain(struct net_buf *buffer, int32_t gain_fixed, uint8_t bit_width)
{
	switch (bit_width) {
	case 16:
		apply_gain_16bit(buffer, gain_fixed);
		break;
	case 24:
		apply_gain_24bit(buffer, gain_fixed);
		break;
	case 32:
		apply_gain_32bit(buffer, gain_fixed);
		break;
	default:
		LOG_ERR("Unsupported bit width: %d", bit_width);
		break;
	}
}

static int mp_zaud_gain_chainfn(struct mp_pad *pad, struct net_buf *in_buf,
				struct net_buf **out_buf)
{
	struct mp_zaud_gain *zaud_gain =
		CONTAINER_OF(pad->object.container, struct mp_zaud_gain, transform.element.object);
	uint32_t bytes_used = 0U;

	ARG_UNUSED(pad);

	/* Validate buffer */
	if (in_buf != NULL && in_buf->data != NULL) {
		bytes_used = mp_buffer_get_meta(in_buf)->bytes_used;
	}

	if (in_buf == NULL || in_buf->data == NULL || bytes_used == 0U) {
		LOG_ERR("Invalid buffer received");
		*out_buf = NULL;
		return -EINVAL;
	}

	/* Apply mute if enabled or gain is 0% */
	if (zaud_gain->mute == true || zaud_gain->gain_percent == 0) {
		memset(in_buf->data, 0, bytes_used);
	} else if (zaud_gain->gain_percent != GAIN_PERCENT_UNITY) {
		/* Apply gain only if not unity (100%) */
		/* TODO: bitWidth hardcoded */
		apply_audio_gain(in_buf, zaud_gain->gain_fixed, zaud_gain->bit_width);
	} else {
		/* Gain is exactly 100%, pass through without modification */
	}

	/* In-place processing, return same buffer */
	*out_buf = in_buf;

	return 0;
}

static struct mp_caps *mp_zaud_gain_supported_caps(struct mp_transform *transform,
						   enum mp_pad_direction direction)
{
	struct mp_value *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	mp_value_list_append(supported_bit_width, mp_value_new(MP_TYPE_UINT, MP_ZAUD_BIT_WIDTH_32));
	mp_value_list_append(supported_bit_width, mp_value_new(MP_TYPE_UINT, MP_ZAUD_BIT_WIDTH_24));
	mp_value_list_append(supported_bit_width, mp_value_new(MP_TYPE_UINT, MP_ZAUD_BIT_WIDTH_16));

	if ((direction == MP_PAD_SRC) || (direction == MP_PAD_SINK)) {
		return mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_BITWIDTH, MP_TYPE_LIST,
				   supported_bit_width, MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN, true,
				   MP_CAPS_END);
	} else {
		return NULL;
	}
}

static int mp_zaud_gain_set_caps(struct mp_transform *transform, enum mp_pad_direction direction,
				 struct mp_caps *caps)
{
	struct mp_zaud_gain *zaud_gain = (struct mp_zaud_gain *)transform;
	/* Get the first structure from caps */
	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);
	/* Extract bit_width from the structure */
	uint32_t bit_width =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_BITWIDTH));
	/* Store bit_width in the zaud_gain structure */
	zaud_gain->bit_width = bit_width;
	LOG_DBG("Bit width set to %u", bit_width);

	return 0;
}

static void mp_zaud_gain_update_caps(struct mp_transform *transform)
{
	struct mp_caps *sink_caps = mp_zaud_gain_supported_caps(transform, MP_PAD_SINK);
	struct mp_caps *src_caps = mp_zaud_gain_supported_caps(transform, MP_PAD_SRC);

	mp_transform_update_caps(transform, sink_caps, src_caps);
	mp_caps_unref(sink_caps);
	mp_caps_unref(src_caps);
}

void mp_zaud_gain_init(struct mp_element *self)
{
	struct mp_transform *transform = (struct mp_transform *)self;
	struct mp_zaud_gain *zaud_gain = (struct mp_zaud_gain *)transform;

	/* Init base class */
	mp_transform_init(self);

	/* Initialize with 100% gain (unity) */
	zaud_gain->gain_percent = GAIN_PERCENT_UNITY;
	zaud_gain->gain_fixed = GAIN_UNITY_FIXED;
	zaud_gain->mute = false;  /* Default mute */
	zaud_gain->bit_width = 0; /* Default */

	self->object.set_property = mp_zaud_gain_set_property;
	self->object.get_property = mp_zaud_gain_get_property;

	transform->mode = MP_MODE_INPLACE;
	transform->sinkpad.chainfn = mp_zaud_gain_chainfn;
	transform->set_caps = mp_zaud_gain_set_caps;

	mp_zaud_gain_update_caps(transform);
}
