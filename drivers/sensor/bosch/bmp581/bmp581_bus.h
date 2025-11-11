/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

enum bmp581_bus_type {
	BMP581_BUS_TYPE_I2C,
};

struct bmp581_bus {
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
		enum bmp581_bus_type type;
	} rtio;
};

int bmp581_prep_reg_read_rtio_async(const struct bmp581_bus *bus,
				    uint8_t reg, uint8_t *buf, size_t size,
				    struct rtio_sqe **out);

int bmp581_prep_reg_write_rtio_async(const struct bmp581_bus *bus,
				     uint8_t reg, const uint8_t *buf, size_t size,
				     struct rtio_sqe **out);

int bmp581_reg_read_rtio(const struct bmp581_bus *bus, uint8_t start, uint8_t *buf, int size);

int bmp581_reg_write_rtio(const struct bmp581_bus *bus, uint8_t reg, const uint8_t *buf, int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMP581_BMP581_BUS_H_ */
