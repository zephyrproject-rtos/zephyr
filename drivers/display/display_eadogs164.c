/*
 * Copyright (c) 2020
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ea_eadogs164

#include <device.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <sys/__assert.h>
#include <logging/log.h>
#include <string.h>
#include <drivers/display.h>

#include "display_eadogs164.h"

struct eadogs164_display_data eadogs0_display_driver;
static const struct eadogs164_display_config eadogs0_display_cfg = {
	.bus_name = DT_INST_BUS_LABEL(0),
	.base_address = DT_INST_REG_ADDR(0),
};

uint8_t disp_orientation;

/* global variable for holding orientation information
 * only top and bottom view are considered in this implementation
 */

LOG_MODULE_REGISTER(DISPLAY_EADOGS164, CONFIG_DISPLAY_LOG_LEVEL);

static int eadogs164_display_init(const struct device *dev)
{
	struct eadogs164_display_data *data = dev->data;
	const struct eadogs164_display_config *cfg = dev->config;
	const struct device *i2c = device_get_binding(cfg->bus_name);

	if (i2c == NULL) {
		LOG_DBG("Failed to get pointer to %s device!",
			cfg->bus_name);
		return -EINVAL;
	}
	data->bus = i2c;

	if (!cfg->base_address) {
		LOG_DBG("No I2C address");
		return -EINVAL;
	}
	data->dev = dev;

	/* Start Up Time for LCD */
	k_sleep(K_MSEC(10));

	uint8_t cmd[2];

	cmd[0] = CONTROL_BYTE;

	/* 8 bit data length extension Bit RE=1; REV=0 */
	cmd[1] = EALCD_CMD_FUNCTION_SET;
	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
					eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}

	/* 4 Line Display */
	cmd[1] = EALCD_CMD_EXTENDED_FUNCTION_SET;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	/* Bottom View */
	cmd[1] = EALCD_CMD_ENTRY_MODE_SET_BOTTOM_VIEW;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	disp_orientation = DISPLAY_ORIENTATION_ROTATED_180;

	/* BS1 = 1 */
	cmd[1] = EALCD_CMD_BIAS_SETTING;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	/* 8 bit data length extension bit RE=0;IS=1 */
	cmd[1] = EALCD_CMD_FUNCTION_SET_2;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* BS0=1, Bias = 1/6 */
	cmd[1] = EALCD_CMD_INTERNAL_OSC;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Divider on and Set Value */

	cmd[1] = EALCD_CMD_FOLLOWER_CONTROL;
#ifdef CONFIG_EADOGS164_DISPLAY_RAB2_0
	cmd[1] = cmd[1] & 0xF8;
	/* clears RAB2-0 */
	cmd[1] = cmd[1] | (CONFIG_EADOGS164_DISPLAY_RAB2_0 & 0x07);
	/* copy lower 3 bits into RAB2-0 */
#endif

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Booster on and set contrast */
	cmd[1] = EALCD_CMD_POWER_CONTROL;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Set Contrast */
	cmd[1] = EALCD_CMD_CONTRAST_SET;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* 8 bit data length extension bit RE=0;IS=0 */
	cmd[1] = EALCD_CMD_FUNCTION_SET_3;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Display ON, Cursor on, Blink on */
	cmd[1] = EALCD_CMD_DISPLAY_ON;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Clear the Display */
	cmd[1] = EALCD_CMD_SCREEN_CLEAR;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
#ifdef CONFIG_EADOGS164_DISPLAY_ROM
	/* Function Set 8-bit, RE=1 */
	cmd[0] = 0;
	cmd[1] = EALCD_CMD_FUNCTION_SET;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* ROM Selection Command */
	cmd[0] = 0;
	cmd[1] = EALCD_CMD_FUNCTION_SET_ROM_SEL;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	cmd[1] = 0;
	/* clears the variable */
	cmd[1] = CONFIG_EADOGS164_DISPLAY_ROM << 2;
	/* shift ROM1,ROM2 bits to the command */
	cmd[0] = CONTROL_BYTE | DATA_MASK;
	/* it is Data(1) rather Command(0) */
	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* Function Set 8-bit, RE=0 */
	cmd[0] = CONTROL_BYTE;
	/* it is Command(0) rather data(1) */
	cmd[1] = EALCD_CMD_FUNCTION_SET_3;

	if (i2c_write(eadogs_i2c_device(dev), cmd, sizeof(cmd),
				eadogs_i2c_address(dev)) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
#endif

	return 0;
}
static uint8_t get_address(const uint16_t x,
			       const uint16_t y)
{
	uint8_t address = 0;

	switch (y) {
	case 0:
		address = x + 4;
		break;
	case 1:
		address = x + 0x24;
		break;
	case 2:
		address = x + 0x44;
		break;
	case 3:
		address = x + 0x64;
		break;
	default:
		break;
	}
	if (disp_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
	/*	It is Bottom view for EADOGS164 Display  */
		address = address - 4;
	}
	return address;
}

/* At the moment this function sends specified characters at x,y up to
 * single line more advanced implementation is left for future as
 * the display supports various line support. Only bottom, top view
 * are implemented at the moment
 */

static int eadogs164_display_write(const struct device *dev, const uint16_t x,
			       const uint16_t y,
			       const struct display_buffer_descriptor *desc,
			       const void *buf)
{
	uint8_t address;

	address = get_address(x, y);

	address |= 0x80;
	uint8_t cmd[2];
	/* Set DDRAM address */
	cmd[0] = CONTROL_BYTE;
	cmd[1] = address;

	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	/* write ASCII data into DDRAM */
	cmd[0] = CONTROL_BYTE | DATA_MASK;
	/* it is Data (1) rather than Command (0) */
	for (uint8_t i = 0; i < desc->buf_size; i++) {
		cmd[1] = ((uint8_t *) (buf))[i];
		if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
				eadogs0_display_cfg.base_address) != 0) {
			LOG_DBG("Not able to send command: %d", cmd[1]);
			return -EINVAL;
		}
	}
	return 0;
}


/* At the moment this function reads from x,y up to single line
 * more advanced implementation is left for future as
 * the display supports various line support. Only bottom, top view
 * are implemented at the moment
 */

static int eadogs164_display_read(const struct device *dev, const uint16_t x,
			      const uint16_t y,
			      const struct display_buffer_descriptor *desc,
			      void *buf)
{
	uint8_t address = 0;

	address = get_address(x, y);

	address |= 0x80;
	uint8_t cmd[2];
	/* Set DDRAM address */
	cmd[0] = CONTROL_BYTE;
	cmd[1] = address;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* write ASCII data into DDRAM */

	cmd[0] = CONTROL_BYTE | DATA_MASK;
	/* it is Data(1) rather than Command(0) */
	if (i2c_write(eadogs0_display_driver.bus, cmd, 1,
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send data: %d", cmd[0]);
		return -EINVAL;
	}	/* start reading data, first byte is dummy */

	if (i2c_read(eadogs0_display_driver.bus, (uint8_t *)buf,
	desc->buf_size+1, eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to read data from LCD");
		return -EINVAL;
	}	/* readjust the cursor */
	cmd[0] = CONTROL_BYTE;
	cmd[1] = address + desc->buf_size;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	return 0;
}

static void *eadogs164_display_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int eadogs164_display_blanking_off(const struct device *dev)
{

	uint8_t cmd[2];

	cmd[0] = CONTROL_BYTE;

	/* clears blinking bit */

	cmd[1] = EALCD_CMD_DISPLAY_ON & 0xFE;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	return 0;
}

static int eadogs164_display_blanking_on(const struct device *dev)
{
	uint8_t cmd[2];

	cmd[0] = CONTROL_BYTE;
	/* Set Blinking bit */
	cmd[1] = EALCD_CMD_DISPLAY_ON | 0x01;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	return 0;
}

static int eadogs164_display_set_brightness(const struct device *dev,
					const uint8_t brightness)
{
	/* No Implementation */
	return 0;
}

static int eadogs164_display_set_contrast(const struct device *dev,
				      const uint8_t contrast)
{
	uint8_t cmd[2];

	cmd[0] = CONTROL_BYTE;

	/* Set POWER CONTROL */
	cmd[1] = EALCD_CMD_POWER_CONTROL;
	cmd[1] = cmd[1] & (~(0x03));
	/* clear DB0 and DB1 bits where 0x03 is the mask for C4 and D5 */
	cmd[1] = cmd[1] | ((contrast & 0x30) >> 4);
	/* set DB0 and DB1 in destination */
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}	/* now write DB0-DB3 */

	/* Set Contrast */
	cmd[1] = EALCD_CMD_CONTRAST_SET;
	cmd[1] = cmd[1] & (~(0x0F));
	/* clear DB0 and DB3 bits where 0x30 is the mask for DB4 and DB5 */
	cmd[1] = cmd[1] | (contrast & 0x0F);
	/* set DB4 and DB5 in destination */
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	return 0;
}

static void eadogs164_display_get_capabilities(const struct device *dev,
		struct display_capabilities *capabilities)
{
	/* No Implementation */
}

static int eadogs164_display_set_pixel_format(const struct device *dev,
		const enum display_pixel_format pixel_format)
{
	/* No Implementation */
	return 0;
}

static int eadogs164_display_set_orientation(const struct device *dev,
					   const enum display_orientation
					   orientation)
{
	disp_orientation = orientation;

	uint8_t cmd[2];

	cmd[0] = CONTROL_BYTE;

	cmd[1] = EALCD_CMD_FUNCTION_SET;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	if (disp_orientation == DISPLAY_ORIENTATION_NORMAL) {
		/* it is top view */
		cmd[1] = EALCD_CMD_ENTRY_MODE_SET_TOP_VIEW;
	} else if (disp_orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		/* it is bottom view */
		cmd[1] = EALCD_CMD_ENTRY_MODE_SET_BOTTOM_VIEW;
	} else {
		LOG_ERR("orientation not supported");
	}
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}		/* 8 bit data length extension bit, RE=0;IS=1 */

	cmd[1] = EALCD_CMD_FUNCTION_SET_3;
	if (i2c_write(eadogs0_display_driver.bus, cmd, sizeof(cmd),
			eadogs0_display_cfg.base_address) != 0) {
		LOG_DBG("Not able to send command: %d", cmd[1]);
		return -EINVAL;
	}
	return 0;
}

static const struct display_driver_api eadogs164_display_api = {
	.blanking_on = eadogs164_display_blanking_on,
	.blanking_off = eadogs164_display_blanking_off,
	.write = eadogs164_display_write,
	.read = eadogs164_display_read,
	.get_framebuffer = eadogs164_display_get_framebuffer,
	.set_brightness = eadogs164_display_set_brightness,
	.set_contrast = eadogs164_display_set_contrast,
	.get_capabilities = eadogs164_display_get_capabilities,
	.set_pixel_format = eadogs164_display_set_pixel_format,
	.set_orientation = eadogs164_display_set_orientation,
};
DEVICE_AND_API_INIT(eadogs0, DT_INST_LABEL(0),
		eadogs164_display_init, &eadogs0_display_driver,
		&eadogs0_display_cfg, POST_KERNEL,
		CONFIG_APPLICATION_INIT_PRIORITY,
		&eadogs164_display_api);
