/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/audio_caps.h>

#include <zephyr/mp/aud/mp_aud.h>

struct mp_aud_desc {
	uint32_t value;
	uint32_t mask;
};

static const struct mp_aud_desc mp_aud_sample_rates[] = {
	{MP_AUD_SAMPLE_RATE_8000, AUDIO_SAMPLE_RATE_8000},
	{MP_AUD_SAMPLE_RATE_16000, AUDIO_SAMPLE_RATE_16000},
	{MP_AUD_SAMPLE_RATE_32000, AUDIO_SAMPLE_RATE_32000},
	{MP_AUD_SAMPLE_RATE_44100, AUDIO_SAMPLE_RATE_44100},
	{MP_AUD_SAMPLE_RATE_48000, AUDIO_SAMPLE_RATE_48000},
	{MP_AUD_SAMPLE_RATE_96000, AUDIO_SAMPLE_RATE_96000},
};

static const struct mp_aud_desc mp_aud_bit_widths[] = {
	{MP_AUD_BIT_WIDTH_16, AUDIO_BIT_WIDTH_16},
	{MP_AUD_BIT_WIDTH_24, AUDIO_BIT_WIDTH_24},
	{MP_AUD_BIT_WIDTH_32, AUDIO_BIT_WIDTH_32},
};

const uint32_t audio2mp_sample_rate(uint32_t sample_rate_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_aud_sample_rates); i++) {
		if (mp_aud_sample_rates[i].mask == sample_rate_mask) {
			return mp_aud_sample_rates[i].value;
		}
	}

	return 0;
}

const uint32_t audio2mp_bit_width(uint32_t bit_width_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_aud_bit_widths); i++) {
		if (mp_aud_bit_widths[i].mask == bit_width_mask) {
			return mp_aud_bit_widths[i].value;
		}
	}

	return 0;
}
