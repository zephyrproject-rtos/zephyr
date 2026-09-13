/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_IN_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_IN_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tlv320aic3104_input {
	TLV320AIC3104_INPUT_LINE1 = 0,
	TLV320AIC3104_INPUT_LINE2 = 1,
};

int tlv320aic3104_in_set_line2_routing(const struct device *dev, bool is_mono);

int tlv320aic3104_in_route_input(const struct device *dev, audio_channel_t channel, uint32_t input);

int tlv320aic3104_in_set_pga_gain(const struct device *dev, audio_channel_t channel, int gain_didb);

int tlv320aic3104_in_start(const struct device *dev);

int tlv320aic3104_in_stop(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif
