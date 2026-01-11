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
#include "ft8xx_dev_data.h"
#include "ft8xx_drv.h"

#define FT800_RAM_CMD_SIZE 4096UL

enum {
	CMD_DLSTART   = 0xffffff00,
	CMD_SWAP      = 0xffffff01,
	CMD_TEXT      = 0xffffff0c,
	CMD_NUMBER    = 0xffffff2e,
	CMD_CALIBRATE = 0xffffff15,
} ft8xx_cmd;

static uint16_t ram_cmd_fullness(const struct device *dev)
{
	const struct ft8xx_data *data = dev->data;

	return (data->reg_cmd_write - data->reg_cmd_read) % 4096UL;
}

static uint16_t ram_cmd_freespace(const struct device *dev)
{
	return (FT800_RAM_CMD_SIZE - 4UL) - ram_cmd_fullness(dev);
}

static void refresh_reg_cmd_read(const struct device *dev)
{
	struct ft8xx_data *data = dev->data;

	data->reg_cmd_read = ft8xx_rd32(dev, FT800_REG_CMD_READ);
}

static void flush_reg_cmd_write(const struct device *dev)
{
	struct ft8xx_data *data = dev->data;

	ft8xx_wr32(dev, FT800_REG_CMD_WRITE, data->reg_cmd_write);
}

static void increase_reg_cmd_write(const struct device *dev, uint16_t value)
{
	struct ft8xx_data *data = dev->data;

	data->reg_cmd_write = (data->reg_cmd_write + value) % FT800_RAM_CMD_SIZE;
}

static uint32_t ram_cmd_wr_address(const struct device *dev)
{
	struct ft8xx_data *data = dev->data;

	return FT800_RAM_CMD + data->reg_cmd_write;
}

static size_t ram_cmd_wr16(const struct device *dev, uint16_t data)
{
	ft8xx_wr16(dev, ram_cmd_wr_address(dev), data);
	increase_reg_cmd_write(dev, sizeof(data));

	return sizeof(data);
}

static size_t ram_cmd_wr32(const struct device *dev, uint32_t data)
{
	ft8xx_wr32(dev, ram_cmd_wr_address(dev), data);
	increase_reg_cmd_write(dev, sizeof(data));

	return sizeof(data);
}

static size_t ram_cmd_wr_var(const struct device *dev, const uint8_t *data, size_t data_size,
			   size_t padding_size)
{
	(void)ft8xx_drv_write(dev, ram_cmd_wr_address(dev), data, data_size);
	increase_reg_cmd_write(dev, data_size + padding_size);

	return data_size + padding_size;
}

static void cmd_beginning(const struct device *dev, size_t cmd_size)
{
	while (ram_cmd_freespace(dev) < cmd_size) {
		refresh_reg_cmd_read(dev);
	}
}

static void cmd_ending(const struct device *dev, size_t cmd_size, size_t written_bytes)
{
	__ASSERT(written_bytes == cmd_size, "Written %zu bytes, expected %zu", written_bytes,
		 cmd_size);
	(void)written_bytes;
	(void)cmd_size;
	flush_reg_cmd_write(dev);
}

void ft8xx_copro_cmd(const struct device *dev, uint32_t cmd)
{
	const size_t cmd_size = sizeof(cmd);
	size_t written_bytes = 0;

	cmd_beginning(dev, cmd_size);
	written_bytes += ram_cmd_wr32(dev, cmd);
	cmd_ending(dev, cmd_size, written_bytes);
}

void ft8xx_copro_cmd_dlstart(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_DLSTART);
}

void ft8xx_copro_cmd_swap(const struct device *dev)
{
	ft8xx_copro_cmd(dev, CMD_SWAP);
}

void ft8xx_copro_cmd_text(const struct device *dev,
			   int16_t x,
			   int16_t y,
			   int16_t font,
			   uint16_t options,
			   const char *string)
{
	const uint16_t str_bytes = strlen(string) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const size_t cmd_size = sizeof(CMD_TEXT) +
				sizeof(x) +
				sizeof(y) +
				sizeof(font) +
				sizeof(options) +
				str_bytes +
				padding_bytes;
	size_t written_bytes = 0;

	cmd_beginning(dev, cmd_size);
	written_bytes += ram_cmd_wr32(dev, CMD_TEXT);
	written_bytes += ram_cmd_wr16(dev, x);
	written_bytes += ram_cmd_wr16(dev, y);
	written_bytes += ram_cmd_wr16(dev, font);
	written_bytes += ram_cmd_wr16(dev, options);
	written_bytes += ram_cmd_wr_var(dev, (const uint8_t *)string, str_bytes, padding_bytes);
	cmd_ending(dev, cmd_size, written_bytes);
}

void ft8xx_copro_cmd_number(const struct device *dev,
			     int16_t x,
			     int16_t y,
			     int16_t font,
			     uint16_t options,
			     int32_t number)
{
	const size_t cmd_size = sizeof(CMD_NUMBER) +
				sizeof(x) +
				sizeof(y) +
				sizeof(font) +
				sizeof(options) +
				sizeof(number);
	size_t written_bytes = 0;

	cmd_beginning(dev, cmd_size);
	written_bytes += ram_cmd_wr32(dev, CMD_NUMBER);
	written_bytes += ram_cmd_wr16(dev, x);
	written_bytes += ram_cmd_wr16(dev, y);
	written_bytes += ram_cmd_wr16(dev, font);
	written_bytes += ram_cmd_wr16(dev, options);
	written_bytes += ram_cmd_wr32(dev, number);
	cmd_ending(dev, cmd_size, written_bytes);
}

void ft8xx_copro_cmd_calibrate(const struct device *dev, uint32_t *result)
{
	uint32_t result_address;

	const size_t cmd_size = sizeof(CMD_CALIBRATE) +
				sizeof(uint32_t);
	size_t written_bytes = 0;

	cmd_beginning(dev, cmd_size);
	written_bytes += ram_cmd_wr32(dev, CMD_CALIBRATE);
	result_address = ram_cmd_wr_address(dev);
	written_bytes += ram_cmd_wr32(dev, 1UL);
	cmd_ending(dev, cmd_size, written_bytes);

	/* Wait until calibration is finished. */
	while (ram_cmd_fullness(dev) > 0) {
		refresh_reg_cmd_read(dev);
	}

	*result = ft8xx_rd32(dev, result_address);
}
