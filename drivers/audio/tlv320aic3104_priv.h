/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_PRIV_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tlv320aic3104_channel_mode {
	TLV320AIC3104_CHANNEL_STEREO = 0,
	TLV320AIC3104_CHANNEL_MONO = 1,
};

struct tlv320aic3104_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec reset_gpio;

	uint8_t mic_bias_level;

	uint8_t adc_hpf;
};

struct tlv320aic3104_data {
	uint8_t page_cache;
	enum tlv320aic3104_channel_mode channel_mode;

	uint8_t last_dac_vol;
	uint8_t last_routing_vol;

	bool output_running;
	bool output_muted;

	uint8_t last_left_adc_pga;
	uint8_t last_right_adc_pga;

	audio_codec_error_callback_t fault_cb;
	uint32_t fault_sticky_errors;
};

#ifdef __cplusplus
}
#endif

#endif
