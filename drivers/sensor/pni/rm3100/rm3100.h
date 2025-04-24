/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_
#define ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include "rm3100_reg.h"

/* RM3100 produces 3 bytes (24-bit) of data per axis */
#define RM3100_BYTES_PER_AXIS 3
#define RM3100_TOTAL_BYTES (RM3100_BYTES_PER_AXIS * 3)

struct rm3100_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t channels : 3;
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
	uint32_t unused; /* Will be expanded with stremaing-mode to hold int-gpios */
};

struct rm3100_data {
	/* RTIO context */
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PNI_RM3100_H_ */
