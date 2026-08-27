/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM4268X_ICM4268X_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM4268X_ICM4268X_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>

struct icm4268x_bus {
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
	} rtio;
};

int icm4268x_prep_reg_read_rtio_async(const struct icm4268x_bus *bus, uint8_t reg, uint8_t *buf,
				      size_t size, struct rtio_sqe **out);

int icm4268x_prep_reg_write_rtio_async(const struct icm4268x_bus *bus, uint8_t reg,
				       const uint8_t *buf, size_t size, struct rtio_sqe **out);

int icm4268x_reg_read_rtio(const struct icm4268x_bus *bus, uint8_t start, uint8_t *buf, int size);

int icm4268x_reg_write_rtio(const struct icm4268x_bus *bus, uint8_t reg, const uint8_t *buf,
			    int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM4268X_ICM4268X_BUS_H_ */
