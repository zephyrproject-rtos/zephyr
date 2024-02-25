/*
 * Copyright (c) 2024 Ilia Kharin
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_H_
#define ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_H_

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include "pinnacle_reg.h"

/* Calidation time takes approximately 100 ms */
#define PINNACLE_CALIBRATION_TIME_MS	150

struct pinnacle_active_range {
	uint16_t x_min;
	uint16_t x_max;
	uint16_t y_min;
	uint16_t y_max;
};

struct pinnacle_resolution {
	uint16_t x;
	uint16_t y;
};

struct pinnacle_config {
	struct spi_dt_spec spi;
#ifdef CONFIG_PINNACLE_TRIGGER
	struct gpio_dt_spec dr_gpio;
#endif
	uint8_t idle_packets_count;
	bool clipping_enabled;
	bool scaling_enabled;
	struct pinnacle_active_range active_range;
	struct pinnacle_resolution resolution;
};

struct pinnacle_sample {
	uint16_t x;
	uint16_t y;
	uint8_t z;
};

struct pinnacle_data {
	struct pinnacle_sample sample;
#ifdef CONFIG_PINNACLE_TRIGGER
	const struct device *dev;

	struct gpio_callback dr_cb_data;

	sensor_trigger_handler_t th_handler;
	const struct sensor_trigger *th_trigger;

#if defined(CONFIG_PINNACLE_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(dr_thread_stack,
			      CONFIG_PINNACLE_THREAD_STACK_SIZE);
	struct k_sem dr_sem;
	struct k_thread dr_thread;
#elif defined(CONFIG_PINNACLE_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
#endif /* CONFIG_PINNACLE_TRIGGER */
};

#ifdef CONFIG_PINNACLE_TRIGGER
int pinnacle_init_interrupt(const struct device *dev);
int pinnacle_trigger_set(const struct device *dev,
			 const struct sensor_trigger *trig,
			 sensor_trigger_handler_t handler);
#endif

int pinnacle_check_spi(const struct spi_dt_spec *spi);
int pinnacle_write_spi(const struct spi_dt_spec *spi,
		       uint8_t address, uint8_t value);
int pinnacle_read_spi(const struct spi_dt_spec *spi,
		      uint8_t address, uint8_t *value);
int pinnacle_seq_read_spi(const struct spi_dt_spec *spi,
			  uint8_t address, uint8_t *data,
			  uint8_t count);

#endif /* ZEPHYR_DRIVERS_SENSOR_PINNACLE_PINNACLE_H_ */
