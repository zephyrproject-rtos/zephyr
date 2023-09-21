/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the MCP23Xxx driver.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MCP23XXX_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MCP23XXX_H_

#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_GPIO_MCP230XX
#include <zephyr/drivers/i2c.h>
#endif /* CONFIG_GPIO_MCP230XX */
#ifdef CONFIG_GPIO_MCP23SXX
#include <zephyr/drivers/spi.h>
#endif /* CONFIG_GPIO_MCP23SXX */

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
#define REG_OLAT 0x0A

#define REG_IOCON_MIRROR BIT(6)

#define MCP23SXX_ADDR 0x40
#define MCP23SXX_READBIT 0x01

typedef int (*mcp23xxx_read_port_regs)(const struct device *dev, uint8_t reg, uint16_t *buf);
typedef int (*mcp23xxx_write_port_regs)(const struct device *dev, uint8_t reg, uint16_t value);
typedef int (*mcp23xxx_bus_is_ready)(const struct device *dev);
/** Configuration data */
struct mcp23xxx_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config config;

	/** I2C device */
	union {
#ifdef CONFIG_GPIO_MCP230XX
		struct i2c_dt_spec i2c;
#endif /* CONFIG_GPIO_MCP230XX */
#ifdef CONFIG_GPIO_MCP23SXX
		struct spi_dt_spec spi;
#endif /* CONFIG_GPIO_MCP23SXX */
	} bus;

	struct gpio_dt_spec gpio_int;
	struct gpio_dt_spec gpio_reset;

	uint8_t ngpios;
	mcp23xxx_read_port_regs read_fn;
	mcp23xxx_write_port_regs write_fn;
	mcp23xxx_bus_is_ready bus_fn;
};

/** Runtime driver data */
struct mcp23xxx_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data data;

	struct k_sem lock;
	sys_slist_t callbacks;
	const struct device *dev;
	struct gpio_callback int_gpio_cb;
	struct k_work work;

	uint16_t rising_edge_ints;
	uint16_t falling_edge_ints;

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

extern const struct gpio_driver_api gpio_mcp23xxx_api_table;

int gpio_mcp23xxx_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_MCP23XXX_H_ */
