/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 DevItWise
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_BUS_H_
#define ZEPHYR_DRIVERS_AUDIO_TLV320AIC3104_BUS_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

bool tlv320aic3104_bus_is_ready(const struct device *dev);

int tlv320aic3104_bus_write_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t val);

int tlv320aic3104_bus_read_reg(const struct device *dev, uint8_t page, uint8_t addr, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif
