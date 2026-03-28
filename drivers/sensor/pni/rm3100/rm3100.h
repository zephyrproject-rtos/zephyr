/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_
#define ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/regmap.h>
#include "rm3100_reg.h"

/* RM3100 produces 3 bytes (24-bit) of data per axis */
#define RM3100_BYTES_PER_AXIS 3
#define RM3100_TOTAL_BYTES (RM3100_BYTES_PER_AXIS * 3)

struct rm3100_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t channels : 3;
		uint16_t cycle_count;
		uint8_t status;
		struct {
			bool drdy : 1;
		} events;
	} header;
	union {
		uint8_t payload[RM3100_TOTAL_BYTES];
		struct {
			uint32_t x : 24;
			uint32_t y : 24;
			uint32_t z : 24;
		} __attribute__((__packed__)) magn;
	};
};

struct rm3100_config {
	struct gpio_dt_spec int_gpio;
};

struct rm3100_stream {
	struct gpio_callback cb;
	const struct device *dev;
	struct rtio_iodev_sqe *iodev_sqe;
	struct {
		struct {
			bool drdy : 1;
		} enabled;
		struct {
			enum sensor_stream_data_opt drdy;
		} opt;
	} settings;
};

struct rm3100_data {
	/* RTIO context */
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
		rtio_bus_type type;
	} rtio;
	struct {
		uint8_t odr;
	} settings;
#if defined(CONFIG_RM3100_STREAM)
	struct rm3100_stream stream;
#endif /* CONFIG_RM3100_STREAM */
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_ */
