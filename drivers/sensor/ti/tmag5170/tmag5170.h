/*
 * Copyright (c) 2023 Michal Morsisko
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_
#define ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

struct tmag5170_dev_config {
	struct spi_dt_spec bus;
	uint8_t magnetic_channels;
	uint8_t x_range;
	uint8_t y_range;
	uint8_t z_range;
	uint8_t oversampling;
	bool temperature_measurement;
	uint8_t magnet_type;
	uint8_t angle_measurement;
	bool disable_temperature_oversampling;
	uint8_t sleep_time;
	uint8_t operating_mode;
#if defined(CONFIG_TMAG5170_TRIGGER)
	struct gpio_dt_spec int_gpio;
#endif
};

struct tmag5170_data {
	uint8_t chip_revision;
	uint16_t x;
	uint16_t y;
	uint16_t z;
	uint16_t temperature;
	uint16_t angle;
#if defined(CONFIG_TMAG5170_TRIGGER)
	struct gpio_callback gpio_cb;
	sensor_trigger_handler_t handler_drdy;
	const struct sensor_trigger *trigger_drdy;
	const struct device *dev;
#endif

#if defined(CONFIG_TMAG5170_TRIGGER_OWN_THREAD)
	struct k_sem sem;
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(thread_stack,
			      CONFIG_TMAG5170_THREAD_STACK_SIZE);
#elif defined(CONFIG_TMAG5170_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
};

#if defined(CONFIG_TMAG5170_TRIGGER)
int tmag5170_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);

int tmag5170_trigger_init(const struct device *dev);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_TMAG5170_TMAG5170_H_ */
