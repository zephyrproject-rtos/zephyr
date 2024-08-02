/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2022-2023 Jamie McCrae
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ptc_pt6314

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/auxdisplay.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

/* Defines for the PT6314_INST_DISPLAY_ON_OFF */
#define PT6314_DO_BLINKING_ON (1 << 0)
#define PT6314_DO_CURSOR_ON   (1 << 1)
#define PT6314_DO_DISPLAY_ON  (1 << 2)

/* Defines for the PT6314_INST_FUNCTION_SET */
#define PT6314_FS_BRIGHTNESS(BR) (4 - (BR & BIT_MASK(2)))
#define PT6314_FS_ROWS_1         (0 << 3)
#define PT6314_FS_ROWS_2         (1 << 3)
#define PT6314_FS_8BIT_MODE      (1 << 4)

#define PT6314_BRIGHTNESS_MIN 1
#define PT6314_BRIGHTNESS_MAX 4

/* Defines for the PT6314_INST_DDRAM_ADDRESS_SET */
#define PT6314_DA_BASE_ROW_1 (0x00)
#define PT6314_DA_BASE_ROW_2 (0x40)

/* Display Commands */
#define PT6314_INST_CLEAR_DISPLAY           BIT(0)
#define PT6314_INST_CURSOR_HOME             BIT(1)
#define PT6314_INST_ENTRY_MODE_SET          BIT(2)
#define PT6314_INST_DISPLAY_ON_OFF          BIT(3)
#define PT6314_INST_CURSOR_OR_DISPLAY_SHIFT BIT(4)
#define PT6314_INST_FUNCTION_SET            BIT(5)
#define PT6314_INST_CGRAM_ADDRESS_SET       BIT(6)
#define PT6314_INST_DDRAM_ADDRESS_SET       BIT(7)

/* Start Byte */
#define PT6314_SB_RS_INST   (0 << 1)
#define PT6314_SB_RS_DATA   (1 << 1)
#define PT6314_SB_RW_WRITE  (0 << 2)
#define PT6314_SB_RW_READ   (1 << 2)
#define PT6314_SB_SYNC_BITS (BIT_MASK(5) << 3)

struct auxdisplay_pt6314_data {
	bool power;
	bool cursor;
	bool blinking;
	uint8_t brightness;
	uint16_t cursor_x;
	uint16_t cursor_y;
};

struct auxdisplay_pt6314_config {
	struct auxdisplay_capabilities capabilities;
	struct spi_dt_spec bus;
};

static int auxdisplay_pt6314_spi_write(const struct device *dev, uint8_t flags, uint8_t val)
{
	const struct auxdisplay_pt6314_config *config = dev->config;

	uint8_t buf[2] = {PT6314_SB_SYNC_BITS | PT6314_SB_RW_WRITE | flags, val};

	struct spi_buf tx_buf[] = {{.buf = buf, .len = sizeof(buf)}};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = 1};

	return spi_write_dt(&config->bus, &tx);
}

static inline int auxdisplay_pt6314_inst(const struct device *dev, uint8_t inst)
{
	return auxdisplay_pt6314_spi_write(dev, PT6314_SB_RS_INST, inst);
}

static inline int auxdisplay_pt6314_data(const struct device *dev, uint8_t data)
{
	return auxdisplay_pt6314_spi_write(dev, PT6314_SB_RS_DATA, data);
}

static int auxdisplay_pt6314_display_on_off(const struct device *dev)
{
	struct auxdisplay_pt6314_data *data = dev->data;
	uint8_t inst;

	inst = (data->power ? PT6314_DO_DISPLAY_ON : 0) | (data->cursor ? PT6314_DO_CURSOR_ON : 0) |
	       (data->blinking ? PT6314_DO_BLINKING_ON : 0);

	return auxdisplay_pt6314_inst(dev, PT6314_INST_DISPLAY_ON_OFF | inst);
}

static int auxdisplay_pt6314_function_set(const struct device *dev)
{
	const struct auxdisplay_pt6314_config *config = dev->config;
	struct auxdisplay_pt6314_data *data = dev->data;
	uint8_t inst;

	inst = PT6314_FS_8BIT_MODE |
	       (config->capabilities.rows == 2 ? PT6314_FS_ROWS_2 : PT6314_FS_ROWS_1) |
	       PT6314_FS_BRIGHTNESS(data->brightness);

	return auxdisplay_pt6314_inst(dev, PT6314_INST_FUNCTION_SET | inst);
}

static int auxdisplay_pt6314_ddram_address_set(const struct device *dev)
{
	struct auxdisplay_pt6314_data *data = dev->data;
	uint8_t inst;

	inst = (data->cursor_y == 0 ? PT6314_DA_BASE_ROW_1 : PT6314_DA_BASE_ROW_2) + data->cursor_x;

	return auxdisplay_pt6314_inst(dev, PT6314_INST_DDRAM_ADDRESS_SET | inst);
}

static int auxdisplay_pt6314_display_on(const struct device *dev)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	data->power = true;

	return auxdisplay_pt6314_display_on_off(dev);
}

static int auxdisplay_pt6314_display_off(const struct device *dev)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	data->power = false;

	return auxdisplay_pt6314_display_on_off(dev);
}

static int auxdisplay_pt6314_cursor_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	data->cursor = enable;

	return auxdisplay_pt6314_display_on_off(dev);
}

static int auxdisplay_pt6314_position_blinking_set_enabled(const struct device *dev, bool enable)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	data->blinking = enable;

	return auxdisplay_pt6314_display_on_off(dev);
}

static int auxdisplay_pt6314_cursor_position_set(const struct device *dev,
						 enum auxdisplay_position type, int16_t x,
						 int16_t y)
{
	const struct auxdisplay_pt6314_config *config = dev->config;
	struct auxdisplay_pt6314_data *data = dev->data;
	uint8_t inst;

	if (type == AUXDISPLAY_POSITION_RELATIVE) {
		x += data->cursor_x;
		y += data->cursor_y;
	} else if (type == AUXDISPLAY_POSITION_RELATIVE_DIRECTION) {
		return -EINVAL;
	}

	if (x < 0 || y < 0) {
		return -EINVAL;
	} else if (x >= config->capabilities.columns || y >= config->capabilities.rows) {
		return -EINVAL;
	}

	data->cursor_x = (uint16_t)x;
	data->cursor_y = (uint16_t)y;

	return auxdisplay_pt6314_ddram_address_set(dev);
}

static int auxdisplay_pt6314_cursor_position_get(const struct device *dev, int16_t *x, int16_t *y)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	*x = (int16_t)data->cursor_x;
	*y = (int16_t)data->cursor_y;

	return 0;
}

static int auxdisplay_pt6314_capabilities_get(const struct device *dev,
					      struct auxdisplay_capabilities *capabilities)
{
	const struct auxdisplay_pt6314_config *config = dev->config;

	memcpy(capabilities, &config->capabilities, sizeof(struct auxdisplay_capabilities));

	return 0;
}

static int auxdisplay_pt6314_clear(const struct device *dev)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	data->cursor_x = 0;
	data->cursor_y = 0;

	return auxdisplay_pt6314_inst(dev, PT6314_INST_CLEAR_DISPLAY);
}

static int auxdisplay_pt6314_brightness_set(const struct device *dev, uint8_t brightness)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	if (brightness < PT6314_BRIGHTNESS_MIN || brightness > PT6314_BRIGHTNESS_MAX) {
		return -EINVAL;
	}

	data->brightness = brightness;

	return auxdisplay_pt6314_function_set(dev);
}

static int auxdisplay_pt6314_brightness_get(const struct device *dev, uint8_t *brightness)
{
	struct auxdisplay_pt6314_data *data = dev->data;

	*brightness = data->brightness;

	return 0;
}

static int auxdisplay_pt6314_write(const struct device *dev, const uint8_t *text, uint16_t len)
{
	const struct auxdisplay_pt6314_config *config = dev->config;
	struct auxdisplay_pt6314_data *data = dev->data;
	int ret;
	int16_t i;

	for (i = 0; i < len; i++) {
		ret = auxdisplay_pt6314_data(dev, text[i]);
		if (ret) {
			return ret;
		}

		data->cursor_x++;

		if (data->cursor_x == config->capabilities.columns) {
			data->cursor_x = 0;
			data->cursor_y++;

			if (data->cursor_y == config->capabilities.rows) {
				data->cursor_y = 0;
			}

			ret = auxdisplay_pt6314_ddram_address_set(dev);
			if (ret) {
				return ret;
			}
		}
	}

	return 0;
}

static int auxdisplay_pt6314_init(const struct device *dev)
{
	const struct auxdisplay_pt6314_config *config = dev->config;
	struct auxdisplay_pt6314_data *data = dev->data;
	uint8_t inst;

	if (!device_is_ready(config->bus.bus)) {
		return -ENODEV;
	}

	auxdisplay_pt6314_function_set(dev);
	auxdisplay_pt6314_display_on_off(dev);
	auxdisplay_pt6314_clear(dev);

	return 0;
}

static const struct auxdisplay_driver_api auxdisplay_pt6314_auxdisplay_api = {
	.display_on = auxdisplay_pt6314_display_on,
	.display_off = auxdisplay_pt6314_display_off,
	.cursor_set_enabled = auxdisplay_pt6314_cursor_set_enabled,
	.position_blinking_set_enabled = auxdisplay_pt6314_position_blinking_set_enabled,
	.cursor_position_set = auxdisplay_pt6314_cursor_position_set,
	.cursor_position_get = auxdisplay_pt6314_cursor_position_get,
	.capabilities_get = auxdisplay_pt6314_capabilities_get,
	.clear = auxdisplay_pt6314_clear,
	.brightness_get = auxdisplay_pt6314_brightness_get,
	.brightness_set = auxdisplay_pt6314_brightness_set,
	.write = auxdisplay_pt6314_write,
};

#define AUXDISPLAY_PT6314_INST(n)                                                                  \
	static const struct auxdisplay_pt6314_config auxdisplay_pt6314_config_##n = {              \
		.capabilities =                                                                    \
			{                                                                          \
				.columns = DT_INST_PROP(n, columns),                               \
				.rows = DT_INST_PROP(n, rows),                                     \
				.mode = 0,                                                         \
				.brightness.minimum = PT6314_BRIGHTNESS_MIN,                       \
				.brightness.maximum = PT6314_BRIGHTNESS_MAX,                       \
				.backlight.minimum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.backlight.maximum = AUXDISPLAY_LIGHT_NOT_SUPPORTED,               \
				.custom_characters = 0,                                            \
			},                                                                         \
		.bus = SPI_DT_SPEC_INST_GET(n,                                                     \
					    SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |   \
						    SPI_TRANSFER_MSB | SPI_WORD_SET(8),            \
					    0),                                                    \
	};                                                                                         \
                                                                                                   \
	static struct auxdisplay_pt6314_data auxdisplay_pt6314_data_##n = {                        \
		.power = true,                                                                     \
		.cursor = false,                                                                   \
		.blinking = false,                                                                 \
		.brightness = PT6314_BRIGHTNESS_MAX,                                               \
		.cursor_x = 0,                                                                     \
		.cursor_y = 0,                                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &auxdisplay_pt6314_init, NULL, &auxdisplay_pt6314_data_##n,       \
			      &auxdisplay_pt6314_config_##n, POST_KERNEL,                          \
			      CONFIG_AUXDISPLAY_INIT_PRIORITY, &auxdisplay_pt6314_auxdisplay_api);

DT_INST_FOREACH_STATUS_OKAY(AUXDISPLAY_PT6314_INST)
