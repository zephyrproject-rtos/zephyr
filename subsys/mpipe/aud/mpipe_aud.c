/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/audio_caps.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_pad.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>

LOG_MODULE_REGISTER(mpipe_aud, CONFIG_MPIPE_LOG_LEVEL);

struct mpipe_aud_desc {
	uint32_t value;
	uint32_t mask;
};

static const struct mpipe_aud_desc mpipe_aud_sample_rates[] = {
	{MPIPE_AUD_SAMPLE_RATE_8000, AUDIO_SAMPLE_RATE_8000},
	{MPIPE_AUD_SAMPLE_RATE_16000, AUDIO_SAMPLE_RATE_16000},
	{MPIPE_AUD_SAMPLE_RATE_32000, AUDIO_SAMPLE_RATE_32000},
	{MPIPE_AUD_SAMPLE_RATE_44100, AUDIO_SAMPLE_RATE_44100},
	{MPIPE_AUD_SAMPLE_RATE_48000, AUDIO_SAMPLE_RATE_48000},
	{MPIPE_AUD_SAMPLE_RATE_96000, AUDIO_SAMPLE_RATE_96000},
};

static const struct mpipe_aud_desc mpipe_aud_bit_widths[] = {
	{MPIPE_AUD_BIT_WIDTH_16, AUDIO_BIT_WIDTH_16},
	{MPIPE_AUD_BIT_WIDTH_24, AUDIO_BIT_WIDTH_24},
	{MPIPE_AUD_BIT_WIDTH_32, AUDIO_BIT_WIDTH_32},
};

/* Sample rate a single mask bit stands for, or 0 if that bit is not a known one */
static uint32_t audio2mp_sample_rate(uint32_t sample_rate_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mpipe_aud_sample_rates); i++) {
		if (mpipe_aud_sample_rates[i].mask == sample_rate_mask) {
			return mpipe_aud_sample_rates[i].value;
		}
	}

	return 0;
}

/* Bit width a single mask bit stands for, or 0 if that bit is not a known one */
static uint32_t audio2mp_bit_width(uint32_t bit_width_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mpipe_aud_bit_widths); i++) {
		if (mpipe_aud_bit_widths[i].mask == bit_width_mask) {
			return mpipe_aud_bit_widths[i].value;
		}
	}

	return 0;
}

/* How many known bit widths a mask advertises, the divisor splitting the index */
static uint32_t mpipe_aud_count_bit_widths(uint32_t bit_width_mask)
{
	uint32_t count = 0;

	for (uint8_t i = 0; i < 32U; i++) {
		if ((bit_width_mask & BIT(i)) != 0U && audio2mp_bit_width(BIT(i)) > 0U) {
			count++;
		}
	}

	return count;
}

/* Sample rate at n among those a mask advertises, or 0 if it has fewer */
static uint32_t mpipe_aud_nth_sample_rate(uint32_t sample_rate_mask, uint32_t n)
{
	uint32_t matched = 0;

	for (uint8_t i = 0; i < 32U; i++) {
		uint32_t sr;

		if ((sample_rate_mask & BIT(i)) == 0U) {
			continue;
		}

		sr = audio2mp_sample_rate(BIT(i));
		if (sr == 0U) {
			continue;
		}

		if (matched == n) {
			return sr;
		}

		matched++;
	}

	return 0;
}

/* Bit width at n among those a mask advertises, or 0 if it has fewer */
static uint32_t mpipe_aud_nth_bit_width(uint32_t bit_width_mask, uint32_t n)
{
	uint32_t matched = 0;

	for (uint8_t i = 0; i < 32U; i++) {
		uint32_t bw;

		if ((bit_width_mask & BIT(i)) == 0U) {
			continue;
		}

		bw = audio2mp_bit_width(BIT(i));
		if (bw == 0U) {
			continue;
		}

		if (matched == n) {
			return bw;
		}

		matched++;
	}

	return 0;
}

int mpipe_aud_enum_caps(const struct audio_caps *caps, uint32_t index,
			const struct mpipe_structure *filter, struct mpipe_structure *out)
{
	struct mpipe_structure candidate;
	uint32_t num_widths;
	uint32_t sample_rate;
	uint32_t bit_width;
	int ret;

	if (caps == NULL) {
		return -EINVAL;
	}

	num_widths = mpipe_aud_count_bit_widths(caps->supported_bit_widths);
	if (num_widths == 0U) {
		return -ENOENT;
	}

	sample_rate = mpipe_aud_nth_sample_rate(caps->supported_sample_rates, index / num_widths);
	if (sample_rate == 0U) {
		return -ENOENT;
	}

	bit_width = mpipe_aud_nth_bit_width(caps->supported_bit_widths, index % num_widths);
	if (bit_width == 0U) {
		return -ENOENT;
	}

	ret = mpipe_structure_init_fields(
		&candidate, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_SAMPLE_RATE, MPIPE_TYPE_UINT,
		sample_rate, MPIPE_CAPS_BITWIDTH, MPIPE_TYPE_UINT, bit_width,
		MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT_RANGE, caps->min_total_channels,
		caps->max_total_channels, 1, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT_RANGE,
		caps->min_frame_interval, caps->max_frame_interval, 1, MPIPE_CAPS_INTERLEAVED,
		MPIPE_TYPE_BOOLEAN, caps->interleaved, MPIPE_CAPS_END);
	if (ret != 0) {
		return ret;
	}

	return mpipe_pad_enum_filter(&candidate, filter, out);
}

int mpipe_aud_caps_get_uint(const struct mpipe_structure *caps, uint8_t field_id, uint32_t *out)
{
	const struct mpipe_value *value = mpipe_structure_get_value(caps, field_id);

	if (out == NULL) {
		return -EINVAL;
	}

	if (value == NULL) {
		LOG_ERR("Capability carries no field %u", field_id);
		return -ENOENT;
	}

	if (value->type != MPIPE_TYPE_UINT) {
		LOG_ERR("Field %u is not a fixed unsigned value", field_id);
		return -EINVAL;
	}

	*out = mpipe_value_get_uint(value);

	return 0;
}
