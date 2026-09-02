/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int tlv320aic3104_channel_to_lr(audio_channel_t channel, bool *do_left,
					      bool *do_right)
{
	switch (channel) {
	case AUDIO_CHANNEL_FRONT_LEFT:
		*do_left = true;
		*do_right = false;
		break;
	case AUDIO_CHANNEL_FRONT_RIGHT:
		*do_left = false;
		*do_right = true;
		break;
	case AUDIO_CHANNEL_ALL:
		*do_left = true;
		*do_right = true;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

enum tlv320aic3104_channel_mode {
	TLV320AIC3104_CHANNEL_STEREO = 0,
	TLV320AIC3104_CHANNEL_MONO = 1,
};

struct tlv320aic3104_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;

	uint8_t mic_bias_level;

	uint8_t adc_hpf;

	bool hprcom_vcm_output;
};

struct tlv320aic3104_data {
	uint8_t page_cache;
	uint8_t channel_mode;

	uint8_t last_dac_vol;
	uint8_t last_routing_vol;

	bool output_running;
	bool output_muted;
	uint8_t output_left;
	uint8_t output_right;

	uint8_t last_left_adc_pga;
	uint8_t last_right_adc_pga;

	audio_codec_error_callback_t fault_cb;
	uint32_t fault_sticky_errors;
};

#ifdef __cplusplus
}
#endif

#endif
