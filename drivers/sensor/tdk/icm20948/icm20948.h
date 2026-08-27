/*
 * Copyright (c) 2026 T3 Gemstone
 *	T3 Gemstone Developer Team <support@t3gemstone.org>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM20948_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM20948_H_

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/* No bank has been selected yet, or the last attempt to select one failed */
#define ICM20948_BANK_UNKNOWN 0xFF

struct icm20948_config {
	struct spi_dt_spec spi;
	uint8_t gyro_fs;
	uint8_t accel_fs;
	uint8_t gyro_dlpf;
	uint8_t accel_dlpf;
};

struct icm20948_data {
	struct k_sem lock;
	uint8_t bank;
	int16_t accel[3];
	int16_t gyro[3];
	int16_t temp;
};

/*
 * Register accessors. The user bank encoded in the high byte of reg is
 * selected before the transfer if it is not selected already, so callers
 * never deal with banks themselves.
 *
 * The caller is expected to hold the device lock.
 */
int icm20948_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len);
int icm20948_write(const struct device *dev, uint16_t reg, uint8_t val);
int icm20948_update(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t val);

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM20948_H_ */
