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
#include <zephyr/drivers/gpio.h>
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

#define BMA4XX_BUS_I2C 0
#define BMA4XX_BUS_SPI 1

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
	uint8_t bus_type;

	const struct gpio_dt_spec gpio_interrupt;
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

struct bma4xx_runtime_config {
	bool fifo_en;
	int32_t batch_ticks;

	bool interrupt1_fifo_wm;
	bool interrupt1_fifo_full;

	uint8_t accel_pwr_mode;
	uint8_t aux_pwr_mode;

	/** Current full-scale range setting as a register value */
	uint8_t accel_fs_range;
	/** Current bandwidth parameter (BWP) as a register value */
	uint8_t accel_bwp;
	/** Current output data rate as a register value */
	uint8_t accel_odr;
};

struct bma4xx_data {
	struct bma4xx_runtime_config cfg;
	/** Pointer to bus-specific I/O API */
	const struct bma4xx_hw_operations *hw_ops;
	/** Chip ID value stored in BMA4XX_REG_CHIP_ID */
	uint8_t chip_id;
	struct rtio *r;
	struct rtio_iodev *iodev;
#ifdef CONFIG_BMA4XX_STREAM
	struct rtio_iodev_sqe *streaming_sqe;
	uint8_t int_status;
	uint16_t fifo_count;
	uint64_t timestamp;
	const struct device *dev;
	struct gpio_callback gpio_cb;
#endif /* CONFIG_BMA4XX_STREAM */
};

static inline int bma4xx_accel_reg_to_hz(uint8_t odr, struct sensor_value *out)
{
	static const struct sensor_value odr_values[] = {
		{0, 0},    {0, 781250}, {1, 562500}, {3, 125000}, {6, 250000}, {12, 500000},
		{25, 0},   {50, 0},     {100, 0},    {200, 0},    {400, 0},    {800, 0},
		{1600, 0}, {3200, 0},   {6400, 0},   {12800, 0},
	};

	if (odr >= ARRAY_SIZE(odr_values)) {
		return -EINVAL;
	}

	*out = odr_values[odr];

	return 0;
}

/**
 * @brief Initialize the SPI bus
 *
 * @param dev bma4xx device pointer
 *
 * @retval 0 success
 * @retval -errdev Error
 */
int bma4xx_spi_init(const struct device *dev);

/**
 * @brief Initialize the I2C bus
 *
 * @param dev bma4xx device pointer
 *
 * @retval 0 success
 * @retval -errdev Error
 */
int bma4xx_i2c_init(const struct device *dev);

/**
 * @brief (Re)Configure the sensor with the given configuration
 *
 * @param dev bma4xx device pointer
 * @param cfg bma4xx_runtime_config pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int bma4xx_configure(const struct device *dev, struct bma4xx_runtime_config *cfg);

/**
 * @brief Safely (re)Configure the sensor with the given configuration
 *
 * Will rollback to prior configuration if new configuration is invalid
 *
 * @param dev bma4xx device pointer
 * @param cfg bma4xx_runtime_config pointer
 *
 * @retval 0 success
 * @retval -errno Error
 */
int bma4xx_safely_configure(const struct device *dev, struct bma4xx_runtime_config *cfg);

#endif /* ZEPHYR_DRIVERS_SENSOR_BMA4XX_BMA4XX_H_ */
