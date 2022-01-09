/*
 * Copyright (c) 2022 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_hx8347d.h"
#include <drivers/display.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_hx8347, CONFIG_DISPLAY_LOG_LEVEL);

#define DT_DRV_COMPAT himax_hx8347d

#define X_RESOLUTION 240
#define Y_RESOLUTION 320
#define BYTES_PER_PIXEL 2U

struct hx8347d_data {
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

struct gamma_correction {
	uint8_t offset_positive[6];
	uint8_t offset_negative[6];
	uint8_t center_positive[2];
	uint8_t center_negative[2];
	uint8_t macro_positive[5];
	uint8_t macro_negative[5];
};

struct vcom_values {
	uint8_t high;
	uint8_t low;
	uint8_t offset;
};

struct hx8347d_config {
	struct spi_dt_spec bus;
	uint8_t pixel_format;
	uint16_t rotation;
	bool inversion;
	struct gamma_correction gamma;
	struct vcom_values vcom;
};

static int hx8347d_write_index(const struct spi_dt_spec *bus, uint8_t reg)
{
	const uint8_t cmd_buf[] = { HX8347D_SET_INDEX, reg };

	const struct spi_buf tx_buf[] = {
		{ .buf = (void *)cmd_buf, .len = sizeof(cmd_buf) },
	};

	const struct spi_buf_set tx = { .buffers = tx_buf, .count = ARRAY_SIZE(tx_buf) };

	return spi_write_dt(bus, &tx);
}

static int hx8347d_write_data(const struct spi_dt_spec *bus, const void *data, size_t len)
{
	const uint8_t cmd_buf[] = { HX8347D_WRITE };

	const struct spi_buf tx_buf[] = { { .buf = (void *)cmd_buf, .len = sizeof(cmd_buf) },
					  { .buf = (void *)data, .len = len } };

	const struct spi_buf_set tx = { .buffers = tx_buf, .count = ARRAY_SIZE(tx_buf) };

	return spi_write_dt(bus, &tx);
}

static int hx8347d_write(const struct spi_dt_spec *bus, uint8_t reg, const void *data, size_t len)
{
	int ret = hx8347d_write_index(bus, reg);
	if (ret != 0) {
		return ret;
	}

	return hx8347d_write_data(bus, data, len);
}

static inline int hx8347d_write_byte(const struct spi_dt_spec *bus, uint8_t reg, uint8_t data)
{
	const uint8_t byte = data;
	return hx8347d_write(bus, reg, &byte, 1);
}

struct gram_area {
	uint16_t x_start;
	uint16_t x_end;
	uint16_t y_start;
	uint16_t y_end;
};

static void hx8347d_set_ramwr(const struct spi_dt_spec *bus, const struct gram_area *area)
{
	hx8347d_write_byte(bus, HX8347D_COLUMN_ADDRESS_START2, area->x_start >> 8);
	hx8347d_write_byte(bus, HX8347D_COLUMN_ADDRESS_START1, area->x_start & 0xFFU);
	hx8347d_write_byte(bus, HX8347D_COLUMN_ADDRESS_END2, area->x_end >> 8);
	hx8347d_write_byte(bus, HX8347D_COLUMN_ADDRESS_END1, area->x_end & 0xFFU);

	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_START2, area->y_start >> 8);
	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_START1, area->y_start & 0xFFU);
	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_END2, area->y_end >> 8);
	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_END1, area->y_end & 0xFFU);
}

static void hx8347d_set_row(const struct spi_dt_spec *bus, const uint16_t row)
{
	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_START2, row >> 8);
	hx8347d_write_byte(bus, HX8347D_ROW_ADDRESS_START1, row & 0xFFU);
}

static int hx8347d_write_gram(const struct device *dev, const uint16_t x, const uint16_t y,
			      const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct hx8347d_config *cfg = dev->config;
	const struct spi_dt_spec *bus = &cfg->bus;
	const uint16_t *data = buf;
	const struct gram_area area = { .x_start = x,
					.x_end = x + desc->width - 1U,
					.y_start = y,
					.y_end = y + desc->height - 1U };

	__ASSERT((BYTES_PER_PIXEL * desc->pitch * desc->height) <= desc->buf_size,
		 "Input buffer too small");

	LOG_DBG("Write gram X: %d, Y: %d, W: %d, H: %d, P: %d", x, y, desc->width, desc->height,
		desc->pitch);

	hx8347d_set_ramwr(bus, &area);

	if (desc->width == desc->pitch) {
		hx8347d_write(bus, HX8347D_READ_DATA, buf,
			      desc->width * desc->height * BYTES_PER_PIXEL);
		return 0;
	}

	for (uint16_t row = y; row < desc->height + y; ++row) {
		hx8347d_set_row(bus, row);
		hx8347d_write(bus, HX8347D_READ_DATA, data, desc->width * BYTES_PER_PIXEL);
		data += desc->pitch;
	}

	return 0;
}

static int hx8347d_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	if (pixel_format != PIXEL_FORMAT_RGB_565) {
		LOG_ERR("Only RGB565 supported");
		return -EINVAL;
	}

	return 0;
}

static int hx8347d_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	const struct hx8347d_config *cfg = dev->config;
	const struct spi_dt_spec *bus = &cfg->bus;

	LOG_INF("Set orientation %d", orientation);
	hx8347d_write_byte(bus, HX8347D_MEMORY_ACCESS_CTRL, 0x80);

	return 0;
}

static void hx8347d_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	const struct display_capabilities capabilities_const = {
		.x_resolution = X_RESOLUTION,
		.y_resolution = Y_RESOLUTION,
		.supported_pixel_formats = PIXEL_FORMAT_RGB_565,
		.screen_info = 0,
		.current_pixel_format = PIXEL_FORMAT_RGB_565,
		.current_orientation = DISPLAY_ORIENTATION_NORMAL
	};

	*capabilities = capabilities_const;
}

static void fill_black(const struct spi_dt_spec *bus)
{
	const uint16_t color_row[X_RESOLUTION] = { 0 };

	for (uint16_t row = 0; row < Y_RESOLUTION; ++row) {
		hx8347d_set_row(bus, row);
		hx8347d_write(bus, HX8347D_READ_DATA, color_row, sizeof(color_row));
	}
}

static void hx8347d_write_gamma_values(const struct spi_dt_spec *bus, uint8_t start, uint8_t len,
				       const uint8_t *values)
{
	for (uint8_t i = 0; i < len; ++i) {
		hx8347d_write_byte(bus, start + i, values[i]);
	}
}

static void hx8347d_adjust_gamma_curve(const struct device *dev)
{
	const struct hx8347d_config *cfg = dev->config;
	const struct gamma_correction *gamma = &cfg->gamma;
	const struct spi_dt_spec *bus = &cfg->bus;

	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL1, sizeof(gamma->offset_positive),
				   gamma->offset_positive);
	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL7, sizeof(gamma->center_positive),
				   gamma->center_positive);
	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL9, sizeof(gamma->macro_positive),
				   gamma->macro_positive);
	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL14, sizeof(gamma->offset_negative),
				   gamma->offset_negative);
	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL20, sizeof(gamma->center_negative),
				   gamma->center_negative);
	hx8347d_write_gamma_values(bus, HX8347D_GAMMA_CTRL22, sizeof(gamma->macro_negative),
				   gamma->macro_negative);
	hx8347d_write_byte(bus, HX8347D_GAMMA_CTRL27, 0xCC);
}

static void hx8347d_set_vcom(const struct device *dev)
{
	const struct hx8347d_config *cfg = dev->config;
	const struct vcom_values *vcom = &cfg->vcom;
	const struct spi_dt_spec *bus = &cfg->bus;

	hx8347d_write_byte(bus, HX8347D_VCOM_CTRL1, vcom->offset);
	hx8347d_write_byte(bus, HX8347D_VCOM_CTRL2, vcom->high);
	hx8347d_write_byte(bus, HX8347D_VCOM_CTRL3, vcom->low);
}

static int hx8347d_init(const struct device *dev)
{
	const struct hx8347d_config *cfg = dev->config;
	const struct spi_dt_spec *bus = &cfg->bus;

	if (!spi_is_ready(&cfg->bus)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}

	hx8347d_write_byte(bus, HX8347D_SOURCE_OP_CTRL1, 0x40);
	hx8347d_write_byte(bus, HX8347D_SOURCE_OP_CTRL2, 0x38);
	hx8347d_write_byte(bus, HX8347D_DISPLAY_CTRL2, 0xa3);

	hx8347d_set_vcom(dev);

	/* Power voltage setting */
	hx8347d_write_byte(bus, HX8347D_POWER_CTRL2, 0x1B);
	hx8347d_write_byte(bus, HX8347D_POWER_CTRL1, 0x01);

	hx8347d_adjust_gamma_curve(dev);

	/* Power on setting up flow */
	/* Display frame rate = 70Hz RADJ = '0110' */
	hx8347d_write_byte(bus, HX8347D_OSC_CTRL1, 0x36);
	/* OSC_EN = 1 */
	hx8347d_write_byte(bus, HX8347D_OSC_CTRL2, 0x01);
	/* AP[2:0] = 111 */
	hx8347d_write_byte(bus, HX8347D_POWER_CTRL3, 0x06);
	/* AP[2:0] = 111 */
	hx8347d_write_byte(bus, HX8347D_POWER_CTRL4, 0x06);
	/* GAS=1, VOMG=00, PON=1, DK=0, XDK=0, DVDH_TRI=0, STB=0*/
	hx8347d_write_byte(bus, HX8347D_POWER_CTRL6, 0x90);
	/* REF = 1 */
	hx8347d_write_byte(bus, HX8347D_DISPLAY_CTRL1, 0x01);

	k_msleep(10);
	/* 262k/65k color selection */
	/* default 0x06 262k color,  0x05 65k color */
	hx8347d_write_byte(bus, HX8347D_COLMOD_CTRL, 0x05);
	/* SET PANEL */
	/* SS_PANEL = 1, GS_PANEL = 0,REV_PANEL = 0, BGR_PANEL = 1 */
	hx8347d_write_byte(bus, HX8347D_PANEL_CTRL, 0x09);

	/* Set GRAM Area - Partial Display Control */
	/* DP_STB = 0, DP_STB_S = 0, SCROLL = 0, */
	hx8347d_write_byte(bus, HX8347D_DISPLAY_MODE_CTRL, 0x00U);
	hx8347d_set_orientation(dev, DISPLAY_ORIENTATION_NORMAL);

	hx8347d_write_byte(bus, HX8347D_READ_DATA, 0x00);

	fill_black(bus);

	/* Display ON */
	hx8347d_write_byte(bus, HX8347D_DISPLAY_CTRL3, 0x38U);
	k_msleep(100);
	hx8347d_write_byte(bus, HX8347D_DISPLAY_CTRL3, 0x3CU);
	k_msleep(100);

	return 0;
}

static int blanking_dummy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return -ENOSYS;
}

static int read_dummy(const struct device *dev, const uint16_t x, const uint16_t y,
		      const struct display_buffer_descriptor *desc, void *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(x);
	ARG_UNUSED(y);
	ARG_UNUSED(desc);
	ARG_UNUSED(buf);
	return -ENOSYS;
}

static void *get_framebuffer_dummy(const struct device *dev)
{
	ARG_UNUSED(dev);
	return NULL;
}

static int set_brightness_dummy(const struct device *dev, const uint8_t brightness)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(brightness);
	return -ENOTSUP;
}

static int set_contrast_dummy(const struct device *dev, const uint8_t contrast)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(contrast);
	return -ENOTSUP;
}

static const struct display_driver_api hx8347d_api = {
	.blanking_on = blanking_dummy,
	.blanking_off = blanking_dummy,
	.write = hx8347d_write_gram,
	.read = read_dummy,
	.get_framebuffer = get_framebuffer_dummy,
	.set_brightness = set_brightness_dummy,
	.set_contrast = set_contrast_dummy,
	.get_capabilities = hx8347d_get_capabilities,
	.set_pixel_format = hx8347d_set_pixel_format,
	.set_orientation = hx8347d_set_orientation,
};

// clang-format off
#define HX8347D_INST(inst)                                                                         \
static const struct hx8347d_config hx8347d_config_##inst = {                                       \
	.bus = SPI_DT_SPEC_INST_GET(                                                               \
		inst,                                                                              \
		SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),          \
	.pixel_format = DT_INST_PROP(inst, pixel_format),                                          \
	.rotation = DT_INST_PROP(inst, rotation),                                                  \
	.inversion = DT_INST_PROP(inst, display_inversion),                                        \
	.gamma = {                                                                                 \
		.offset_positive = DT_INST_PROP(inst, gamma_offset_positive),                      \
		.offset_negative = DT_INST_PROP(inst, gamma_offset_negative),                      \
		.center_positive = DT_INST_PROP(inst, gamma_center_positive),                      \
		.center_negative = DT_INST_PROP(inst, gamma_center_negative),                      \
		.macro_positive =  DT_INST_PROP(inst, gamma_macro_positive),                       \
		.macro_negative =  DT_INST_PROP(inst, gamma_macro_negative),                       \
	},                                                                                         \
	.vcom = {                                                                                  \
		.high = DT_INST_PROP(inst, vcom_high),                                             \
		.low = DT_INST_PROP(inst, vcom_low),                                               \
		.offset = DT_INST_PROP(inst, vcom_offset)                                          \
	}                                                                                          \
};                                                                                                 \
static struct hx8347d_data hx8347d_data_##inst;                                                    \
DEVICE_DT_INST_DEFINE(inst, hx8347d_init, NULL, &hx8347d_data_##inst, &hx8347d_config_##inst,      \
		      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &hx8347d_api);
// clang-format on
DT_INST_FOREACH_STATUS_OKAY(HX8347D_INST)
