/*
 * Copyright (c) 2020
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_EADOGS164_H_
#define ZEPHYR_DRIVERS_DISPLAY_EADOGS164_H_

#include <device.h>
#include <kernel.h>
#include <drivers/gpio.h>


/********************************************
 *  LCD FUNCTIONS
 *******************************************/

#define DATA_MASK 0x40
#define CONTROL_BYTE 0x00

/* LCD Display Commands */

#define EALCD_CMD_SCREEN_CLEAR 0x01


#define EALCD_CMD_FUNCTION_SET 0x3A
#define EALCD_CMD_EXTENDED_FUNCTION_SET 0x09
#define EALCD_CMD_ENTRY_MODE_SET_TOP_VIEW 0x05
#define EALCD_CMD_ENTRY_MODE_SET_BOTTOM_VIEW 0x06

#define EALCD_CMD_BIAS_SETTING 0x1E
#define EALCD_CMD_FUNCTION_SET_2 0x39
#define EALCD_CMD_INTERNAL_OSC 0x1B
#define EALCD_CMD_FOLLOWER_CONTROL 0x6B
#define EALCD_CMD_POWER_CONTROL 0x56
#define EALCD_CMD_CONTRAST_SET 0x7A
#define EALCD_CMD_FUNCTION_SET_3 0x38
#define EALCD_CMD_DISPLAY_ON 0x0F

/* Change Character Table */

#define EALCD_CMD_FUNCTION_SET_ROM_SEL 0x72

struct eadogs164_display_data {
	const struct device *dev;
	const struct device *bus;
};

struct eadogs164_display_config {
	char *bus_name;
	uint8_t base_address;
};

static inline uint8_t eadogs_i2c_address(const struct device *dev)
{
	const struct eadogs164_display_config *dcp = dev->config;

	return dcp->base_address;
}

static inline const struct device *eadogs_i2c_device(const struct device *dev)
{
	const struct eadogs164_display_data *ddp = dev->data;

	return ddp->bus;
}

#endif /* ZEPHYR_DRIVERS_DISPLAY_EADOGS164_H_ */
