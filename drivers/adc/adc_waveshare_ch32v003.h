/*
 * Copyright (c) 2026 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ADC_WAVESHARE_CH32V003_H_
#define ZEPHYR_DRIVERS_ADC_WAVESHARE_CH32V003_H_

#include <stdint.h>
#include <zephyr/device.h>

int ch32v003_get_adc_value(const struct device *dev, uint16_t *value);

#endif /* ZEPHYR_DRIVERS_ADC_WAVESHARE_CH32V003_H_ */
