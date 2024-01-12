/*
 * Copyright (c) 2023 Alen Daniel
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT kyo_tn0xxx

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tn0xxx, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

/* Driver for Kyocera's 2.16" RESOLUTION MEMORY IN PIXEL (MIP) TFT (TN0216ANVNANN)
 * Note:
 * high/1 means white, low/0 means black
 * SPI interface expects LSB first
 * see more notes in boards/shields/tn0xxx/doc/index.rst
 */

#define TN0XXX_PANEL_WIDTH  DT_INST_PROP(0, width)
#define TN0XXX_PANEL_HEIGHT DT_INST_PROP(0, height)

#define TN0XXX_PIXELS_PER_BYTE 8

#define LCD_ADDRESS_LEN_BITS          8
#define LCD_DUMMY_SPI_CYCLES_LEN_BITS 32
#define DUMMY_BYTE                    0x00

/* Data packet format
 * +-------------------+-------------------+--------------------+
 * | line addr (8 bits) | data (8 WIDTH bits) | dummy (32 bits) |
 * +-------------------+-------------------+--------------------+
 */

struct tn0xxx_config_s {
	struct spi_dt_spec bus;
};

static int update_display(const struct device *dev, uint16_t start_line, uint16_t num_lines,
			  const uint8_t *data)
{
	const struct tn0xxx_config_s *config = dev->config;
	uint8_t ln = start_line;
	uint8_t dummy_bytes[4];

	struct spi_buf line_buf[3] = {
		{
			.len = sizeof(ln),
			.buf = &ln,
		},
		{
			.len = TN0XXX_PANEL_WIDTH / TN0XXX_PIXELS_PER_BYTE,
		},
		{
			.len = sizeof(dummy_bytes),
			.buf = &dummy_bytes,
		},
	};
	struct spi_buf_set line_set = {
		.buffers = line_buf,
		.count = ARRAY_SIZE(line_buf),
	};
	int err = 0;

	memset(dummy_bytes, 0, sizeof(dummy_bytes));

	LOG_DBG("Lines %d to %d", start_line, start_line + num_lines - 1);

	/* Send each line to the screen including
	 * the line address and dummy bits
	 */
	for (; ln <= start_line + num_lines - 1; ln++) {
		line_buf[1].buf = (uint8_t *)data;
		err |= spi_write_dt(&config->bus, &line_set);
		data += TN0XXX_PANEL_WIDTH / TN0XXX_PIXELS_PER_BYTE;
	}

	spi_release_dt(&config->bus);

	return err;
}

static int tn0xxx_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc, const void *buf)
{
	LOG_DBG("X: %d, Y: %d, W: %d, H: %d, pitch: %d, Buf size: %d", x, y, desc->width,
		desc->height, desc->pitch, desc->buf_size);

	if (buf == NULL) {
		LOG_WRN("Display buffer is not available");
		return -EINVAL;
	}

	if ((y + desc->height) > TN0XXX_PANEL_HEIGHT) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if (desc->width != TN0XXX_PANEL_WIDTH) {
		LOG_ERR("Width restricted to panel width %d.. user provided %d", TN0XXX_PANEL_WIDTH,
			desc->width);
		return -EINVAL;
	}

	if (x != 0) {
		LOG_ERR("x-coordinate has to be 0");
		return -EINVAL;
	};

	return update_display(dev, y, desc->height, buf);
}

static void tn0xxx_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = TN0XXX_PANEL_WIDTH;
	caps->y_resolution = TN0XXX_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = PIXEL_FORMAT_MONO01;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
	caps->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH;
}

static int tn0xxx_init(const struct device *dev)
{
	const struct tn0xxx_config_s *config = dev->config;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	return 0;
}

static struct display_driver_api tn0xxx_driver_api = {
	.write = tn0xxx_write,
	.get_capabilities = tn0xxx_get_capabilities,
};

static const struct tn0xxx_config_s tn0xxx_config = {
	.bus = SPI_DT_SPEC_INST_GET(0,
				    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_LSB |
					    SPI_CS_ACTIVE_HIGH | SPI_HOLD_ON_CS | SPI_LOCK_ON,
				    2),
};

DEVICE_DT_INST_DEFINE(0, tn0xxx_init, NULL, NULL, &tn0xxx_config, POST_KERNEL,
		      CONFIG_DISPLAY_INIT_PRIORITY, &tn0xxx_driver_api);
