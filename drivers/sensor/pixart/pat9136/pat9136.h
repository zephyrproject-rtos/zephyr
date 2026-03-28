/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAT9136_H_
#define ZEPHYR_DRIVERS_SENSOR_PAT9136_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>

struct pat9136_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t channels : 6;
		struct {
			bool drdy : 1;
			bool motion : 1;
		} events;
		union {
			uint8_t buf[4];
			struct {
				uint16_t x;
				uint16_t y;
			} __attribute__((__packed__));
		} resolution;
	} header;
	union {
		uint8_t buf[12];
		struct {
			uint8_t motion;
			uint8_t observation;
			struct {
				int16_t x;
				int16_t y;
			} delta;
			uint8_t squal;
			uint8_t raw_sum;
			uint8_t raw_max;
			uint8_t raw_min;
			uint16_t shutter;
		} __attribute__((__packed__));
	};
};

struct pat9136_stream {
	struct gpio_callback cb;
	const struct device *dev;
	struct rtio_iodev_sqe *iodev_sqe;
	struct {
		struct k_timer backup;
		struct {
			atomic_t armed;
			struct k_timer timer;
		} cooldown;
	} timer;
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

struct pat9136_data {
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
#if defined(CONFIG_PAT9136_STREAM)
	struct pat9136_stream stream;
#endif /* CONFIG_PAT9136_STREAM */
};

struct pat9136_config {
	struct gpio_dt_spec int_gpio;
	uint16_t resolution;
	uint32_t backup_timer_period;
	uint32_t cooldown_timer_period;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PAT9136_H_ */
