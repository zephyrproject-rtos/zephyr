/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header file for the PCAL9535A driver.
 */

#ifndef _GPIO_PCAL9535A_H_
#define _GPIO_PCAL9535A_H_

#include <kernel.h>

#include <gpio.h>
#include <i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Configuration data */
struct gpio_pcal9535a_config {
	/** The master I2C device's name */
	const char * const i2c_master_dev_name;

	/** The slave address of the chip */
	uint16_t i2c_slave_addr;

	uint8_t stride[2];
};

/** Store the port 0/1 data for each register pair. */
union gpio_pcal9535a_port_data {
	uint16_t all;
	uint8_t port[2];
	uint8_t byte[2];
};

/** Runtime driver data */
struct gpio_pcal9535a_drv_data {
	/** Master I2C device */
	struct device *i2c_master;

	/**
	 * Specify polarity inversion of pin. This is used for ouput as
	 * the polarity inversion registers on chip affects inputs only.
	 */
	uint32_t out_pol_inv;

	struct {
		union gpio_pcal9535a_port_data output;
		union gpio_pcal9535a_port_data pol_inv;
		union gpio_pcal9535a_port_data dir;
		union gpio_pcal9535a_port_data pud_en;
		union gpio_pcal9535a_port_data pud_sel;
	} reg_cache;

	uint8_t stride[2];
};

#ifdef __cplusplus
}
#endif

#endif /* _GPIO_PCAL9535A_H_ */
