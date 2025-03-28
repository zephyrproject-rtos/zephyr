/*
 * Copyright (c) 2023 Google LLC
 * Copyright (c) 2024 Cienet
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_H_
#define ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include "bma4xx_defs.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

/*
 * Types
 */

union bma4xx_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c) */

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
};

struct bma4xx_config {
	int (*bus_init)(const struct device *dev);
	const union bma4xx_bus_cfg bus_cfg;
};

/** Used to implement bus-specific R/W operations. See bma4xx_i2c.c and
 *  bma4xx_spi.c.
 */
struct bma4xx_hw_operations {
	int (*read_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*write_data)(const struct device *dev, uint8_t reg_addr, uint8_t *value, uint8_t len);
	int (*read_reg)(const struct device *dev, uint8_t reg_addr, uint8_t *value);
	int (*write_reg)(const struct device *dev, uint8_t reg_addr, uint8_t value);
	int (*update_reg)(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value);
};

struct bma4xx_data {
	/** Current full-scale range setting as a register value */
	uint8_t accel_fs_range;
	/** Current bandwidth parameter (BWP) as a register value */
	uint8_t accel_bwp;
	/** Current output data rate as a register value */
	uint8_t accel_odr;
	/** Pointer to bus-specific I/O API */
	const struct bma4xx_hw_operations *hw_ops;
	/** Chip ID value stored in BMA4XX_REG_CHIP_ID */
	uint8_t chip_id;
};

/*
 * RTIO types
 */

struct bma4xx_decoder_header {
	uint64_t timestamp;
	uint8_t is_fifo: 1;
	uint8_t accel_fs: 2;
	uint8_t reserved: 5;
} __attribute__((__packed__));

struct bma4xx_encoded_data {
	struct bma4xx_decoder_header header;
	struct {
		/** Set if `accel_xyz` has data */
		uint8_t has_accel: 1;
		/** Set if `temp` has data */
		uint8_t has_temp: 1;
		uint8_t reserved: 6;
	} __attribute__((__packed__));
	int16_t accel_xyz[3];
#ifdef CONFIG_BMA4XX_TEMPERATURE
	int8_t temp;
#endif /* CONFIG_BMA4XX_TEMPERATURE */
};

int bma4xx_spi_init(const struct device *dev);
int bma4xx_i2c_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_H_ */
