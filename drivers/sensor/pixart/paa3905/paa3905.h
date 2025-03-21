/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAA3905_H_
#define ZEPHYR_DRIVERS_SENSOR_PAA3905_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

struct paa3905_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t channels : 3;
		struct {
			bool drdy : 1;
			bool motion : 1;
		} events;
	} header;
	union {
		uint8_t buf[14];
		struct {
			uint8_t motion;
			uint8_t observation;
			struct {
				int16_t x;
				int16_t y;
			} delta;
			uint8_t challenging_conditions;
			uint8_t squal;
			uint8_t raw_sum;
			uint8_t raw_max;
			uint8_t raw_min;
			uint32_t shutter : 24;
		} __attribute__((__packed__));
	};
};

struct paa3905_stream {
	struct gpio_callback cb;
	const struct device *dev;
	struct rtio_iodev_sqe *iodev_sqe;
	struct k_timer timer;
	struct {
		struct {
			bool drdy : 1;
			bool motion : 1;
		} enabled;
		struct {
			enum sensor_stream_data_opt drdy;
			enum sensor_stream_data_opt motion;
		} opt;
	} settings;
};

struct paa3905_data {
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
#if defined(CONFIG_PAA3905_STREAM)
	struct paa3905_stream stream;
#endif /* CONFIG_PAA3905_STREAM */
};

struct paa3905_config {
	struct gpio_dt_spec int_gpio;
	int resolution;
	bool led_control;
	uint32_t backup_timer_period;
};

/** Made public in order to perform chip recovery if erratic behavior
 * is detected.
 */
int paa3905_recover(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_PAA3905_H_ */
