/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/mpipe/mpipe_dispatch.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_gain.h>

LOG_MODULE_REGISTER(mpipe_aud_gain, CONFIG_MPIPE_LOG_LEVEL);

/* Gain percentage definitions */
#define GAIN_PERCENT_MIN   0    /* 0% = mute */
#define GAIN_PERCENT_MAX   1000 /* 1000% = 10x amplification */
#define GAIN_PERCENT_UNITY 100  /* 100% = unity gain (no change) */

/* Q16.16 signed fixed-point definitions */
#define GAIN_UNITY_FIXED (1 << 16) /* 65536 = 1.0 in Q16.16 */

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

	/* Convert: gain_percent% -> Q16.16 fixed point */
	return (gain_percent * GAIN_UNITY_FIXED) / 100;
}

static int mpipe_aud_gain_set_property(struct mpipe_object *obj, uint32_t key, const void *val)
{
	struct mpipe_aud_gain *self = (struct mpipe_aud_gain *)obj;

	switch (key) {
	case MPIPE_PROP_AUD_TRANSFORM_GAIN:
		/* Expect percentage value (0-1000) */
		self->gain_percent = *(const int *)val;
		self->gain_fixed = percent_to_fixed_gain(self->gain_percent);

		/* Update mute flag for optimization */
		self->mute = (self->gain_percent == 0);

		LOG_DBG("Audio gain set to %d%% (fixed: %d)\n", self->gain_percent,
			self->gain_fixed);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mpipe_aud_gain_get_property(struct mpipe_object *obj, uint32_t key, void *val)
{
	struct mpipe_aud_gain *self = (struct mpipe_aud_gain *)obj;

	switch (key) {
	case MPIPE_PROP_AUD_TRANSFORM_GAIN:
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
	size_t num_samples = mpipe_buffer_get_meta(buffer)->bytes_used / sizeof(int16_t);

	for (size_t i = 0; i < num_samples; i++) {
		/* 64-bit intermediate: a 32-bit product overflows above unity gain. */
		int64_t temp = ((int64_t)samples[i] * gain_fixed) >> 16;

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
	size_t num_samples = mpipe_buffer_get_meta(buffer)->bytes_used / sizeof(int32_t);

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
	const size_t sz = mpipe_buffer_get_meta(buffer)->bytes_used;

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

static int mpipe_aud_gain_chain_fn(struct mpipe_pad *pad, struct net_buf *in_buf,
				   struct net_buf **out_buf)
{
	struct mpipe_aud_gain *aud_gain = CONTAINER_OF(pad->object.container, struct mpipe_aud_gain,
						       transform.element.object);
	uint32_t bytes_used = 0U;

	ARG_UNUSED(pad);

	/* Validate buffer */
	if (in_buf != NULL && in_buf->data != NULL) {
		bytes_used = mpipe_buffer_get_meta(in_buf)->bytes_used;
	}

	if (in_buf == NULL || in_buf->data == NULL || bytes_used == 0U) {
		LOG_ERR("Invalid buffer received");
		if (in_buf != NULL) {
			net_buf_unref(in_buf);
		}
		*out_buf = NULL;
		return -EINVAL;
	}

	/* Apply mute if enabled or gain is 0% */
	if (aud_gain->mute == true || aud_gain->gain_percent == 0) {
		memset(in_buf->data, 0, bytes_used);
	} else if (aud_gain->gain_percent != GAIN_PERCENT_UNITY) {
		/* Apply gain only if not unity (100%) */
		/* TODO: bitWidth hardcoded */
		apply_audio_gain(in_buf, aud_gain->gain_fixed, aud_gain->bit_width);
	} else {
		/* Gain is exactly 100%, pass through without modification */
	}

	/* In-place processing, return same buffer */
	*out_buf = in_buf;

	return 0;
}

/*
 * The gain element only manipulates sample values, so it constrains the bit
 * width and the channel layout it can process but is agnostic to the sample
 * rate, channel count and frame interval, which it passes through unchanged.
 * Those pass-through fields are advertised as open ranges so the pad caps share
 * fields with neighboring elements (e.g. a caps filter fixing frame interval
 * and channel count). Without them the structures would have no field in common
 * and caps negotiation would refuse to link the pads.
 *
 * The bit widths it does constrain are alternatives, so each gets its own
 * enumeration index rather than being collected into a list value.
 */
static const uint32_t mpipe_aud_gain_bit_widths[] = {
	MPIPE_AUD_BIT_WIDTH_32,
	MPIPE_AUD_BIT_WIDTH_24,
	MPIPE_AUD_BIT_WIDTH_16,
};

static int mpipe_aud_gain_enum_caps(struct mpipe_pad *pad, uint32_t index,
				    const struct mpipe_structure *filter,
				    struct mpipe_structure *out)
{
	struct mpipe_structure candidate;
	int ret;

	ARG_UNUSED(pad);

	if (index >= ARRAY_SIZE(mpipe_aud_gain_bit_widths)) {
		return -ENOENT;
	}

	ret = mpipe_structure_init_fields(
		&candidate, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE, MPIPE_TYPE_UINT_RANGE,
		1U, UINT32_MAX, 1U, MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT_RANGE, 1U,
		UINT32_MAX, 1U, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT_RANGE, 1U, UINT32_MAX,
		1U, MPIPE_CAPS_BITWIDTH, MPIPE_TYPE_UINT, mpipe_aud_gain_bit_widths[index],
		MPIPE_CAPS_INTERLEAVED, MPIPE_TYPE_BOOLEAN, true, MPIPE_CAPS_END);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

static int mpipe_aud_gain_set_caps(struct mpipe_transform *transform,
				   enum mpipe_pad_direction direction,
				   const struct mpipe_structure *caps)
{
	struct mpipe_aud_gain *aud_gain = (struct mpipe_aud_gain *)transform;
	uint32_t bit_width;
	int ret;

	ret = mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_BITWIDTH, &bit_width);
	if (ret != 0) {
		return ret;
	}

	aud_gain->bit_width = bit_width;
	LOG_DBG("Bit width set to %u", bit_width);

	/* Record the negotiated caps on the pad, as the base class would */
	return mpipe_transform_set_caps(transform, direction, caps);
}

/*
 * The gain is in-place, so the buffer flows through unchanged and downstream's
 * buffering requirement is remembered here to be handed to upstream when it
 * proposes its own buffer pool config.
 */
static int mpipe_aud_gain_decide_buffer_pool(struct mpipe_transform *self,
					     struct mpipe_dispatch *query)
{
	struct mpipe_aud_gain *aud_gain = (struct mpipe_aud_gain *)self;
	struct mpipe_buffer_pool *query_pool = query->pool;
	struct mpipe_buffer_pool_config *qpc =
		(query_pool != NULL) ? &query_pool->config : &query->pool_cfg;

	if (qpc != NULL) {
		aud_gain->alloc_cfg = *qpc;
	}

	return 0;
}

static int mpipe_aud_gain_propose_buffer_pool(struct mpipe_transform *self,
					      struct mpipe_dispatch *query)
{
	struct mpipe_aud_gain *aud_gain = (struct mpipe_aud_gain *)self;

	query->pool_cfg = aud_gain->alloc_cfg;

	return 0;
}

int mpipe_aud_gain_init(struct mpipe_aud_gain *aud_gain, uint8_t id)
{
	__ASSERT_NO_MSG(aud_gain != NULL);

	struct mpipe_element *self = &aud_gain->transform.element;
	struct mpipe_transform *transform = &aud_gain->transform;
	int ret = mpipe_transform_init(transform, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "aud_gain");

	/* Initialize with 100% gain (unity) */
	aud_gain->gain_percent = GAIN_PERCENT_UNITY;
	aud_gain->gain_fixed = GAIN_UNITY_FIXED;
	aud_gain->mute = false;  /* Default mute */
	aud_gain->bit_width = 0; /* Default */
	memset(&aud_gain->alloc_cfg, 0, sizeof(aud_gain->alloc_cfg));

	self->object.set_property = mpipe_aud_gain_set_property;
	self->object.get_property = mpipe_aud_gain_get_property;

	transform->mode = MPIPE_MODE_INPLACE;
	transform->sink_pad.chain_fn = mpipe_aud_gain_chain_fn;
	transform->set_caps = mpipe_aud_gain_set_caps;
	transform->decide_buffer_pool = mpipe_aud_gain_decide_buffer_pool;
	transform->propose_buffer_pool = mpipe_aud_gain_propose_buffer_pool;

	transform->sink_pad.enum_caps_fn = mpipe_aud_gain_enum_caps;
	transform->src_pad.enum_caps_fn = mpipe_aud_gain_enum_caps;

	return 0;
}
