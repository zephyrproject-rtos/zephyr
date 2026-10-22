/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_RTIO_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_RTIO_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

enum bmi08x_rtio_bus_type {
	BMI08X_RTIO_BUS_TYPE_I2C,
	BMI08X_RTIO_BUS_TYPE_SPI,
};

struct bmi08x_rtio_bus {
	struct rtio *ctx;
	struct rtio_iodev *iodev;
	enum bmi08x_rtio_bus_type type;
};

int bmi08x_prep_reg_read_rtio_async(const struct bmi08x_rtio_bus *bus,
				    uint8_t reg, uint8_t *buf, size_t size,
				    struct rtio_sqe **out, bool dummy_byte);

int bmi08x_prep_reg_write_rtio_async(const struct bmi08x_rtio_bus *bus,
				     uint8_t reg, const uint8_t *buf, size_t size,
				     struct rtio_sqe **out);

int bmi08x_reg_read_rtio(const struct bmi08x_rtio_bus *bus, uint8_t start, uint8_t *buf, int size,
			 bool dummy_byte);

int bmi08x_reg_write_rtio(const struct bmi08x_rtio_bus *bus, uint8_t reg, const uint8_t *buf,
			  int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI08X_BMI08X_RTIO_BUS_H_ */
