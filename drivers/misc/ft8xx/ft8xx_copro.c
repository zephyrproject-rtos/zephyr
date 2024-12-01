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
	CMD_BGCOLOR   = 0xffffff09,
	CMD_FGCOLOR   = 0xffffff0a,
	CMD_TEXT      = 0xffffff0c,
	CMD_SLIDER    = 0xffffff10,
	CMD_TOGGLE    = 0xffffff12,
	CMD_TRACK     = 0xffffff2c,
	CMD_NUMBER    = 0xffffff2e,
	CMD_CALIBRATE = 0xffffff15,
} ft8xx_cmd;

static uint16_t reg_cmd_read;
static uint16_t reg_cmd_write;

static uint16_t ram_cmd_fullness(void)
{
	return (reg_cmd_write - reg_cmd_read) % 4096UL;
}

static uint16_t ram_cmd_freespace(void)
{
	return (FT800_RAM_CMD_SIZE - 4UL) - ram_cmd_fullness();
}

static void refresh_reg_cmd_read(void)
{
	reg_cmd_read = ft8xx_rd32(FT800_REG_CMD_READ);
}

static void flush_reg_cmd_write(void)
{
	ft8xx_wr32(FT800_REG_CMD_WRITE, reg_cmd_write);
}

static void increase_reg_cmd_write(uint16_t value)
{
	reg_cmd_write = (reg_cmd_write + value) % FT800_RAM_CMD_SIZE;
}

void ft8xx_copro_cmd(uint32_t cmd)
{
	while (ram_cmd_freespace() < sizeof(cmd)) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, cmd);
	increase_reg_cmd_write(sizeof(cmd));

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_dlstart(void)
{
	ft8xx_copro_cmd(CMD_DLSTART);
}

void ft8xx_copro_cmd_swap(void)
{
	ft8xx_copro_cmd(CMD_SWAP);
}

void ft8xx_copro_cmd_fgcolor(uint32_t c)
{
	const uint16_t cmd_size = sizeof(CMD_FGCOLOR) +
				  sizeof(c);

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_FGCOLOR);
	increase_reg_cmd_write(sizeof(CMD_FGCOLOR));

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, c);
	increase_reg_cmd_write(sizeof(c));

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_bgcolor(uint32_t c)
{
	const uint16_t cmd_size = sizeof(CMD_BGCOLOR) +
				  sizeof(c);

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_BGCOLOR);
	increase_reg_cmd_write(sizeof(CMD_BGCOLOR));

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, c);
	increase_reg_cmd_write(sizeof(c));

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_slider(int16_t x,
			    int16_t y,
			    int16_t w,
			    int16_t h,
			    uint16_t options,
			    uint16_t val,
			    uint16_t range)
{
	size_t padding_bytes = 2;
	const uint16_t cmd_size = sizeof(CMD_SLIDER) +
				  sizeof(x) +
				  sizeof(y) +
				  sizeof(w) +
				  sizeof(h) +
				  sizeof(options) +
				  sizeof(val) +
				  sizeof(range) +
				  padding_bytes;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_SLIDER);
	increase_reg_cmd_write(sizeof(CMD_SLIDER));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, w);
	increase_reg_cmd_write(sizeof(w));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, h);
	increase_reg_cmd_write(sizeof(h));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, val);
	increase_reg_cmd_write(sizeof(val));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, range);
	increase_reg_cmd_write(sizeof(range) + padding_bytes);

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_toggle(int16_t x,
			    int16_t y,
			    int16_t w,
			    int16_t font,
			    uint16_t options,
			    uint16_t state,
			    const char *s)
{
	const uint16_t str_bytes = strlen(s) + 1;
	const uint16_t padding_bytes = (4 - (str_bytes % 4)) % 4;
	const uint16_t cmd_size = sizeof(CMD_TOGGLE) +
				  sizeof(x) +
				  sizeof(y) +
				  sizeof(w) +
				  sizeof(font) +
				  sizeof(options) +
				  sizeof(state) +
				  str_bytes +
				  padding_bytes;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_TOGGLE);
	increase_reg_cmd_write(sizeof(CMD_TOGGLE));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, w);
	increase_reg_cmd_write(sizeof(w));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, state);
	increase_reg_cmd_write(sizeof(state));

	(void)ft8xx_drv_write(FT800_RAM_CMD + reg_cmd_write, s, str_bytes);
	increase_reg_cmd_write(str_bytes + padding_bytes);

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_track(int16_t x,
			   int16_t y,
			   int16_t w,
			   int16_t h,
			   int16_t tag)
{
	size_t padding_bytes = 2;
	const uint16_t cmd_size = sizeof(CMD_TRACK) +
				  sizeof(x) +
				  sizeof(y) +
				  sizeof(w) +
				  sizeof(h) +
				  sizeof(tag) +
				  padding_bytes;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_TRACK);
	increase_reg_cmd_write(sizeof(CMD_TRACK));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, w);
	increase_reg_cmd_write(sizeof(w));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, h);
	increase_reg_cmd_write(sizeof(h));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, tag);
	increase_reg_cmd_write(sizeof(tag) + padding_bytes);

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_text(int16_t x,
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

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_TEXT);
	increase_reg_cmd_write(sizeof(CMD_TEXT));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	(void)ft8xx_drv_write(FT800_RAM_CMD + reg_cmd_write, s, str_bytes);
	increase_reg_cmd_write(str_bytes + padding_bytes);

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_number(int16_t x,
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

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_NUMBER);
	increase_reg_cmd_write(sizeof(CMD_NUMBER));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, x);
	increase_reg_cmd_write(sizeof(x));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, y);
	increase_reg_cmd_write(sizeof(y));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, font);
	increase_reg_cmd_write(sizeof(font));

	ft8xx_wr16(FT800_RAM_CMD + reg_cmd_write, options);
	increase_reg_cmd_write(sizeof(options));

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, n);
	increase_reg_cmd_write(sizeof(n));

	flush_reg_cmd_write();
}

void ft8xx_copro_cmd_calibrate(uint32_t *result)
{
	const uint16_t cmd_size = sizeof(CMD_CALIBRATE) + sizeof(uint32_t);
	uint32_t result_address;

	while (ram_cmd_freespace() < cmd_size) {
		refresh_reg_cmd_read();
	}

	ft8xx_wr32(FT800_RAM_CMD + reg_cmd_write, CMD_CALIBRATE);
	increase_reg_cmd_write(sizeof(CMD_CALIBRATE));

	result_address = FT800_RAM_CMD + reg_cmd_write;
	ft8xx_wr32(result_address, 1UL);
	increase_reg_cmd_write(sizeof(uint32_t));

	flush_reg_cmd_write();

	/* Wait until calibration is finished. */
	while (ram_cmd_fullness() > 0) {
		refresh_reg_cmd_read();
	}

	*result = ft8xx_rd32(result_address);
}
