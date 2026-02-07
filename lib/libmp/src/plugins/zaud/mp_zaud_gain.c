/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_zaud.h"
#include "mp_zaud_gain.h"
#include "mp_zaud_property.h"

LOG_MODULE_REGISTER(mp_zaud_gain, CONFIG_LIBMP_LOG_LEVEL);

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
	}

	/* Convert: gain_percent% → Q16.16 fixed point */
	return (gain_percent * GAIN_UNITY_FIXED) / 100;
}

static int mp_zaud_gain_set_property(struct mp_object *obj, uint32_t key, const void *val)
{
	struct mp_zaud_gain *self = MP_ZAUD_GAIN(obj);

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
		return mp_transform_set_property(obj, key, val);
	}

	return 0;
}

static int mp_zaud_gain_get_property(struct mp_object *obj, uint32_t key, void *val)
{
	struct mp_zaud_gain *self = MP_ZAUD_GAIN(obj);

	switch (key) {
	case PROP_GAIN:
		/* Return percentage value */
		*(int *)val = self->gain_percent;
		return 0;
	default:
		return mp_transform_get_property(obj, key, val);
	}

	return 0;
}

static void apply_gain_16bit(struct mp_buffer *buffer, int32_t gain_fixed)
{
	int16_t *samples = (int16_t *)buffer->data;
	size_t num_samples = buffer->pool->config.size / sizeof(int16_t);

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

static void apply_audio_gain(struct mp_buffer *buffer, int32_t gain_fixed, uint8_t bit_width)
{
	/* Handle other bit widths */
	switch (bit_width) {
	case 16:
		apply_gain_16bit(buffer, gain_fixed);
		break;
	default:
		LOG_ERR("Unsupported bit width: %d", bit_width);
		break;
	}
}

static bool mp_zaud_gain_chainfn(struct mp_pad *pad, struct mp_buffer *buffer)
{
	struct mp_transform *transform = MP_TRANSFORM(pad->object.container);
	struct mp_zaud_gain *zaud_gain = MP_ZAUD_GAIN(pad->object.container);

	/* Validate buffer */
	if (!buffer || !buffer->data || buffer->pool->config.size == 0) {
		LOG_ERR("Invalid buffer received");
		return false;
	}

	/* Apply mute if enabled or gain is 0% */
	if (zaud_gain->mute == true || zaud_gain->gain_percent == 0) {
		memset(buffer->data, 0, buffer->pool->config.size);
	} else if (zaud_gain->gain_percent != GAIN_PERCENT_UNITY) {
		/* Apply gain only if not unity (100%) */
		/* TODO: bitWidth hardcoded */
		apply_audio_gain(buffer, zaud_gain->gain_fixed, 16);
	}
	/* If gain is exactly 100%, pass through without modification */

	/* Push buffer to src pad */
	mp_pad_push(&transform->srcpad, buffer);

	return true;
}

static struct mp_caps *mp_zaud_gain_get_caps(struct mp_transform *transform,
					     enum mp_pad_direction direction)
{
	struct mp_value *supported_bit_width = mp_value_new(MP_TYPE_LIST, NULL);

	mp_value_list_append(supported_bit_width, mp_value_new(MP_TYPE_UINT, MP_ZAUD_BIT_WIDTH_16));

	if ((direction == MP_PAD_SRC) || (direction == MP_PAD_SINK)) {
		return mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_BITWIDTH, MP_TYPE_LIST,
				   supported_bit_width, MP_CAPS_INTERLEAVED, MP_TYPE_BOOLEAN, true,
				   MP_CAPS_END);
	} else {
		return NULL;
	}
}

void mp_zaud_gain_init(struct mp_element *self)
{
	struct mp_transform *transform = MP_TRANSFORM(self);
	struct mp_zaud_gain *zaud_gain = MP_ZAUD_GAIN(self);

	/* Init base class */
	mp_transform_init(self);

	/* Initialize with 100% gain (unity) */
	zaud_gain->gain_percent = GAIN_PERCENT_UNITY;
	zaud_gain->gain_fixed = GAIN_UNITY_FIXED;
	zaud_gain->mute = false; /* Default mute */

	self->object.set_property = mp_zaud_gain_set_property;
	self->object.get_property = mp_zaud_gain_get_property;

	transform->mode = MP_MODE_INPLACE;
	transform->sinkpad.chainfn = mp_zaud_gain_chainfn;
	transform->sinkpad.caps = mp_zaud_gain_get_caps(transform, MP_PAD_SINK);
	transform->srcpad.caps = mp_zaud_gain_get_caps(transform, MP_PAD_SRC);
	transform->get_caps = mp_zaud_gain_get_caps;
}
