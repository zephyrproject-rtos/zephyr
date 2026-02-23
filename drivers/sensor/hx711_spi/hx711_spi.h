/*
 * Copyright (c) 2026 Zephyr Project Developers
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_HX711_SPI_HX711_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_HX711_SPI_HX711_SPI_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>

#define HX711_SAMPLE_MAX 0x7FFFFF

enum hx711_ch_gain {
	HX711_CHB_GAIN_32 = 0xA0,
	HX711_CHA_GAIN_64 = 0xA8,
	HX711_CHA_GAIN_128 = 0x80
};

struct hx711_config {
	struct spi_dt_spec spi;
	uint8_t gain;
	int32_t avdd_uv;
};

struct hx711_data {
	int32_t sample_uv;

	int32_t conv_factor_uv_to_g;
	int32_t offset;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_HX711_SPI_HX711_SPI_H_ */
