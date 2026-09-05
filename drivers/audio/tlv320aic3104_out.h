/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_OUT_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_OUT_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#include "tlv320aic3104_priv.h"

#ifdef __cplusplus
extern "C" {
#endif

enum tlv320aic3104_output {
	TLV320AIC3104_OUTPUT_HP = 0,
	TLV320AIC3104_OUTPUT_LOP = 1,
};

#define CODEC_VOLUME_ATTEN_MAX 117

void tlv320aic3104_out_start(const struct device *dev);

void tlv320aic3104_out_stop(const struct device *dev);

int tlv320aic3104_out_set_volume(const struct device *dev, int vol);

void tlv320aic3104_out_set_mute(const struct device *dev, bool mute);

int tlv320aic3104_out_route_output(const struct device *dev, audio_channel_t channel,
				   uint32_t output);

int tlv320aic3104_out_set_channel_mode(const struct device *dev, bool is_mono);

#ifdef __cplusplus
}
#endif

#endif
