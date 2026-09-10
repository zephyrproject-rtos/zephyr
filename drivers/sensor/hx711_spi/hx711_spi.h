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

#if defined(CONFIG_HX711_SPI_TRIGGER)
#include <zephyr/drivers/pinctrl.h>

#define PINCTRL_STATE_TRIGGER PINCTRL_STATE_PRIV_START
#endif /* defined(CONFIG_HX711_SPI_TRIGGER) */

#define HX711_SAMPLE_MAX 0x7FFFFF

enum hx711_ch_gain {
	HX711_CHB_GAIN_32 = 0xA0,
	HX711_CHA_GAIN_64 = 0xA8,
	HX711_CHA_GAIN_128 = 0x80
};

#if defined(CONFIG_HX711_SPI_TRIGGER)
struct hx711_trigger_config {
	const struct pinctrl_dev_config *pcfg;
	struct gpio_dt_spec int_gpio;
};

struct hx711_trigger_data {
	bool trigger_armed;
	struct gpio_callback gpio_cb;
	const struct device *dev;

	sensor_trigger_handler_t data_ready_handler;
	const struct sensor_trigger *data_ready_trigger;
	struct k_work work;
#if defined(CONFIG_HX711_SPI_TRIGGER_OWN_THREAD)
	struct k_work_q work_q;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_HX711_SPI_THREAD_STACK_SIZE);
#endif
};
#endif /* defined(CONFIG_HX711_SPI_TRIGGER) */

struct hx711_config {
	struct spi_dt_spec spi;
	int32_t avdd_uv;

#if defined(CONFIG_HX711_SPI_TRIGGER)
	struct hx711_trigger_config *trig_cfg;
#endif
};

struct hx711_data {
	int32_t sample_uv;

	uint8_t gain;
	int32_t conv_factor_uv_to_g;
	int32_t offset;

#if defined(CONFIG_HX711_SPI_TRIGGER)
	struct hx711_trigger_data *trig_data;
#endif
};

#endif /* ZEPHYR_DRIVERS_SENSOR_HX711_SPI_HX711_SPI_H_ */
