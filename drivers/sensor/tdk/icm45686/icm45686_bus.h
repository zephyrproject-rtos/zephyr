/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_ICM45686_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_ICM45686_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i3c.h>

enum icm45686_bus_type {
	ICM45686_BUS_SPI,
	ICM45686_BUS_I2C,
	ICM45686_BUS_I3C,
};

struct icm45686_bus {
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
		enum icm45686_bus_type type;
/** Required to support In-band Interrupts */
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm45686, i3c)
		struct {
			struct i3c_device_desc *desc;
			const struct i3c_device_id id;
		} i3c;
#endif
	} rtio;
};

int icm45686_prep_reg_read_rtio_async(const struct icm45686_bus *bus, uint8_t reg, uint8_t *buf,
				      size_t size, struct rtio_sqe **out);

int icm45686_prep_reg_write_rtio_async(const struct icm45686_bus *bus, uint8_t reg,
				       const uint8_t *buf, size_t size, struct rtio_sqe **out);

int icm45686_reg_read_rtio(const struct icm45686_bus *bus, uint8_t start, uint8_t *buf, int size);

int icm45686_reg_write_rtio(const struct icm45686_bus *bus, uint8_t reg, const uint8_t *buf,
			    int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_ICM45686_BUS_H_ */
