/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/audio/audio_caps.h>

#include <src/core/mp_element_factory.h>
#include <src/core/mp_plugin.h>

#include "mp_zaud.h"
#include "mp_zaud_dmic_src.h"
#include "mp_zaud_gain.h"
#include "mp_zaud_i2s_codec_sink.h"

struct mp_zaud_desc {
	uint32_t value;
	uint32_t mask;
};

static const struct mp_zaud_desc mp_zaud_sample_rates[] = {
	{MP_ZAUD_SAMPLE_RATE_8000, AUDIO_SAMPLE_RATE_8000},
	{MP_ZAUD_SAMPLE_RATE_16000, AUDIO_SAMPLE_RATE_16000},
	{MP_ZAUD_SAMPLE_RATE_32000, AUDIO_SAMPLE_RATE_32000},
	{MP_ZAUD_SAMPLE_RATE_44100, AUDIO_SAMPLE_RATE_44100},
	{MP_ZAUD_SAMPLE_RATE_48000, AUDIO_SAMPLE_RATE_48000},
	{MP_ZAUD_SAMPLE_RATE_96000, AUDIO_SAMPLE_RATE_96000},
};

static const struct mp_zaud_desc mp_zaud_bit_widths[] = {
	{MP_ZAUD_BIT_WIDTH_16, AUDIO_BIT_WIDTH_16},
	{MP_ZAUD_BIT_WIDTH_24, AUDIO_BIT_WIDTH_24},
	{MP_ZAUD_BIT_WIDTH_32, AUDIO_BIT_WIDTH_32},
};

const uint32_t audio2mp_sample_rate(uint32_t sample_rate_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zaud_sample_rates); i++) {
		if (mp_zaud_sample_rates[i].mask == sample_rate_mask) {
			return mp_zaud_sample_rates[i].value;
		}
	}

	return 0;
}

const uint32_t audio2mp_bit_width(uint32_t bit_width_mask)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(mp_zaud_bit_widths); i++) {
		if (mp_zaud_bit_widths[i].mask == bit_width_mask) {
			return mp_zaud_bit_widths[i].value;
		}
	}

	return 0;
}

static void plugin_init(void)
{
	MP_ELEMENT_FACTORY_DEFINE(MP_ZAUD_DMIC_SRC_ELEM, sizeof(struct mp_zaud_dmic_src),
				  mp_zaud_dmic_src_init);
	MP_ELEMENT_FACTORY_DEFINE(MP_ZAUD_GAIN_ELEM, sizeof(struct mp_zaud_gain),
				  mp_zaud_gain_init);
	MP_ELEMENT_FACTORY_DEFINE(MP_ZAUD_I2S_CODEC_SINK_ELEM,
				  sizeof(struct mp_zaud_i2s_codec_sink),
				  mp_zaud_i2s_codec_sink_init);
}

MP_PLUGIN_DEFINE(MP_ZAUD_PLUGIN, plugin_init);
