/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the MCP230xx driver.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MCP230XX_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MCP230XX_H_

#include <kernel.h>

#include <drivers/gpio.h>
#include <drivers/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register definitions */
#define REG_IODIR 0x00
#define REG_IPOL 0x01
#define REG_GPINTEN 0x02
#define REG_DEFVAL 0x03
#define REG_INTCON 0x04
#define REG_IOCON 0x05
#define REG_GPPU 0x06
#define REG_INTF 0x07
#define REG_INTCAP 0x08
#define REG_GPIO 0x09
#define REG_IKAT 0x0A

/** Configuration data */
struct mcp230xx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config config;

	/** I2C device */
	const struct i2c_dt_spec i2c;
	uint8_t ngpios;
};

/** Runtime driver data */
struct mcp230xx_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data data;

	struct k_sem lock;

	struct {
		uint16_t iodir;
		uint16_t ipol;
		uint16_t gpinten;
		uint16_t defval;
		uint16_t intcon;
		uint16_t iocon;
		uint16_t gppu;
		uint16_t intf;
		uint16_t intcap;
		uint16_t gpio;
		uint16_t olat;
	} reg_cache;
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_MCP230XX_H_ */
