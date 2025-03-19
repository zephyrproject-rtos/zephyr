/*
 * Copyright (c) 2025 ZARM, University of Bremen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM42688_DEV_CFG_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM42688_DEV_CFG_H_

#define DT_DRV_COMPAT invensense_icm42688

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
#include <zephyr/drivers/i2c.h>
#endif
#include <zephyr/drivers/gpio.h>

/**
 * @brief Bus configuration
 */
union icm42688_bus_cfg {
#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
	struct spi_dt_spec spi;
#endif

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(i2c)
	struct i2c_dt_spec i2c;
#endif
};

/**
 * @brief I/O interface for bus communication  
 */
struct icm42688_io_ops {
	int (*read)(const struct device *dev, uint16_t reg, uint8_t *data, size_t length);
    int (*update_register)(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t data);
    int (*single_write)(const struct device *dev, uint16_t reg, uint8_t data);
};

/**
 * @brief Device config (struct device)
 */
struct icm42688_dev_cfg {
	union icm42688_bus_cfg bus;
	struct icm42688_io_ops *io_ops;
	struct gpio_dt_spec gpio_int1;
	struct gpio_dt_spec gpio_int2;
	uint8_t inst_on_bus;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM42688_DEV_CFG_H_ */