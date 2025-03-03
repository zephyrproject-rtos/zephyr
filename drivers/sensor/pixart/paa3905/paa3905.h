/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_PAA3905_H_
#define ZEPHYR_DRIVERS_SENSOR_PAA3905_H_

#include <stdint.h>
#include <zephyr/rtio/rtio.h>

struct paa3905_encoded_data {
	struct {
		uint64_t timestamp;
		uint8_t channels : 3;
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

struct paa3905_data {
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
};

struct paa3905_config {
	int resolution;
	bool led_control;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_PAA3905_H_ */
