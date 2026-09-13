/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_FAULT_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_FAULT_H_

#include <stdint.h>

#include <zephyr/audio/codec.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

int tlv320aic3104_fault_init(const struct device *dev);

int tlv320aic3104_fault_register_callback(const struct device *dev,
					  audio_codec_error_callback_t cb);

int tlv320aic3104_fault_clear(const struct device *dev);

int tlv320aic3104_fault_check(const struct device *dev);

int tlv320aic3104_fault_get_errors(const struct device *dev, uint32_t *out_errors);

#ifdef __cplusplus
}
#endif

#endif
