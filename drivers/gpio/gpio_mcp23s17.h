/*
 * Copyright (c) 2020 Geanix ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the MCP23S17 driver.
 */

#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_MCP23S17_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_MCP23S17_H_

#include <zephyr/kernel.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Register definitions */
#define REG_IODIR_PORTA                 0x00
#define REG_IODIR_PORTB                 0x01
#define REG_IPOL_PORTA                  0x02
#define REG_IPOL_PORTB                  0x03
#define REG_GPINTEN_PORTA               0x04
#define REG_GPINTEN_PORTB               0x05
#define REG_DEFVAL_PORTA                0x06
#define REG_DEFVAL_PORTB                0x07
#define REG_INTCON_PORTA                0x08
#define REG_INTCON_PORTB                0x09
#define REG_GPPU_PORTA                  0x0C
#define REG_GPPU_PORTB                  0x0D
#define REG_INTF_PORTA                  0x0E
#define REG_INTF_PORTB                  0x0F
#define REG_INTCAP_PORTA                0x10
#define REG_INTCAP_PORTB                0x11
#define REG_GPIO_PORTA                  0x12
#define REG_GPIO_PORTB                  0x13
#define REG_OLAT_PORTA                  0x14
#define REG_OLAT_PORTB                  0x15

#define MCP23S17_ADDR                   0x40
#define MCP23S17_READBIT                0x01

/** Configuration data */
struct mcp23s17_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	struct spi_dt_spec bus;
};

/** Runtime driver data */
struct mcp23s17_drv_data {
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

#endif  /* ZEPHYR_DRIVERS_GPIO_GPIO_MCP23S17_H_ */
