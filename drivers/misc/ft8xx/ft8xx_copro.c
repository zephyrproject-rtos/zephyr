/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_copro.h>

#include <stdint.h>
#include <string.h>

#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>
#include <zephyr/drivers/misc/ft8xx/ft8xx_memory.h>
#include "ft8xx_drv.h"

#define FT800_RAM_CMD_SIZE 4096UL

enum {
	CMD_DLSTART   = 0xffffff00,
	CMD_SWAP      = 0xffffff01,
	CMD_TEXT      = 0xffffff0c,
	CMD_NUMBER    = 0xffffff2e,
	CMD_CALIBRATE = 0xffffff15,
} ft8xx_cmd;

static uint16_t reg_cmd_read;
static uint16_t reg_cmd_write;

static uint16_t ram_cmd_fullness(const struct device *dev)
{
	return (reg_cmd_write - reg_cmd_read) % 4096UL;
}

static uint16_t ram_cmd_freespace(const struct device *dev)
{
	return (FT800_RAM_CMD_SIZE - 4UL) - ram_cmd_fullness(dev);
}

static void refresh_reg_cmd_read(const struct device *dev)
{
	reg_cmd_read = ft8xx_rd32(dev, FT800_REG_CMD_READ);
}

static void flush_reg_cmd_write(const struct device *dev)
{
	ft8xx_wr32(dev, FT800_REG_CMD_WRITE, reg_cmd_write);
}

static void increase_reg_cmd_write(const struct device *dev, uint16_t value)
{
	reg_cmd_write = (reg_cmd_write + value) % FT800_RAM_CMD_SIZE;
}

void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd)
{
	while (ram_cmd_freespace(dev) < sizeof(cmd)) {
		refresh_reg_cmd_read(dev);
	}

	ft8xx_wr32(dev, FT800_RAM_CMD + reg_cmd_write, cmd);
	increase_reg_cmd_write(dev, sizeof(cmd));

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_dlstart(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_DLSTART);
}

void ft8xx_copro_cmd_swap(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_SWAP);
}

void ft8xx_copro_cmd_text(const struct device *dev, int16_t x,
			   int16_t y,
			   int16_t font,
			   uint16_t options,
			   const char *s)
{
	const uint16_t str_bytes = strlen(s) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const uint16_t cmd_size = sizeof(CMD_TEXT) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options) +
				   str_bytes +
				   padding_bytes;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(dev, FT800_RAM_CMD + reg_cmd_write, CMD_TEXT);
	increase_reg_cmd_write(sizeof(CMD_TEXT));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(dev, sizeof(options));

	(void)ft8xx_drv_write(dev, FT800_RAM_CMD + reg_cmd_write, s, str_bytes);
	increase_reg_cmd_write(dev, str_bytes + padding_bytes);

	flush_reg_cmd_write(dev, );
}

void ft8xx_copro_cmd_number(const struct device *dev, int16_t x,
			     int16_t y,
			     int16_t font,
			     uint16_t options,
			     int32_t n)
{
	const uint16_t cmd_size = sizeof(CMD_NUMBER) +
				   sizeof(x) +
				   sizeof(y) +
				   sizeof(font) +
				   sizeof(options) +
				   sizeof(n);

	while (ram_cmd_freespace(dev) < cmd_size) {
		refresh_reg_cmd_read(dev);
	}

	ft8xx_wr32(dev, FT800_RAM_CMD + reg_cmd_write, CMD_NUMBER);
	increase_reg_cmd_write(sizeof(CMD_NUMBER));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(dev, FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	ft8xx_wr32(dev, FT800_RAM_CMD + reg_cmd_write, n);
	increase_reg_cmd_write(sizeof(n));

	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd_calibrate(const struct device *dev, uint32_t *result)
{
	const uint16_t cmd_size = sizeof(CMD_CALIBRATE) + sizeof(uint32_t);
	uint32_t result_address;

	while (ram_cmd_freespace(dev) < cmd_size) {
		refresh_reg_cmd_read(dev);
	}

	ft8xx_wr32(dev, FT800_RAM_CMD + reg_cmd_write, CMD_CALIBRATE);
	increase_reg_cmd_write(sizeof(CMD_CALIBRATE));

	result_address = FT800_RAM_CMD + reg_cmd_write;
	ft8xx_wr32(dev, result_address, 1UL);
	increase_reg_cmd_write(dev, sizeof(uint32_t));

	flush_reg_cmd_write(dev);

	/* Wait until calibration is finished. */
	while (ram_cmd_fullness(dev) > 0) {
		refresh_reg_cmd_read(dev);
	}

	*result = ft8xx_rd32(dev, result_address);
}
