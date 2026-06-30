/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM566XX_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM566XX_BUS_H_

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i3c.h>

enum icm566xx_bus_type {
	ICM566XX_BUS_I2C,
	ICM566XX_BUS_SPI,
	ICM566XX_BUS_I3C,
};

struct icm566xx_bus {
	struct {
		struct rtio *ctx;
		struct rtio_iodev *iodev;
		enum icm566xx_bus_type type;
		struct {
			struct i3c_device_desc *desc;
			const struct i3c_device_id id;
		} i3c;
	} rtio;
};

int icm566xx_prep_reg_read_rtio_async(const struct icm566xx_bus *bus, uint8_t reg, uint8_t *buf,
				      size_t size, struct rtio_sqe **out);

int icm566xx_prep_reg_write_rtio_async(const struct icm566xx_bus *bus, uint8_t reg,
				       const uint8_t *buf, size_t size, struct rtio_sqe **out);

int icm566xx_reg_read_rtio(const struct icm566xx_bus *bus, uint8_t start, uint8_t *buf, int size);

int icm566xx_reg_write_rtio(const struct icm566xx_bus *bus, uint8_t reg, const uint8_t *buf,
			    int size);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM566XX_BUS_H_ */
