/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Silicon Signals Pvt. Ltd.
 * Author: Bhavin Sharma <bhavin.sharma@siliconsignals.io>
 * Author: Elgin Perumbilly <elgin.perumbilly@siliconsignals.io>
 */

#define DT_DRV_COMPAT jdi_lpm013m126

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lpm013m126, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

/* Panel properties */
#define LPM_BPP         3

/* Command bytes */
#define LPM_WRITELINE_CMD   0x80
#define LPM_ALLCLEAR_CMD    0x20

struct lpm013m126_data {
	struct k_timer vcom_timer;
	int vcom_state;
};

struct lpm013m126_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec disp_gpio;
	struct gpio_dt_spec extcomin_gpio;
	uint32_t extcomin_freq;
	uint8_t width;
	uint8_t height;
};

/* bit-reversal of row address */
static inline uint8_t bitrev8(uint8_t x)
{
	x = ((x & 0xF0) >> 4) | ((x & 0x0F) << 4);
	x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
	x = ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
	return x;
}

/* The native format (1 bit per channel) is rather unusual. LVGL and
 * other libraries don't support it. In addition, the format is not
 * very convenient for the application. So, we prefer to advertise
 * a well known format and convert it under the hood.
 * A native implementation of this format would allow to save memory
 * for the frame buffer (11kB instead of 30kB).
 */
static inline uint8_t rgb565_to_rgb3(uint16_t color)
{
	uint8_t r = FIELD_GET(0xF800, color);
	uint8_t g = FIELD_GET(0x07E0, color);
	uint8_t b = FIELD_GET(0x001F, color);

	r >>= 4;
	g >>= 5;
	b >>= 4;
	return (r << 2) | (g << 1) | b;
}

/* Pack one row of RGB565 pixels into panel format */
static void lpm_pack_row(const struct device *dev, uint8_t *dst, const uint16_t *src)
{
	const struct lpm013m126_config *cfg = dev->config;

	int bitpos = 0;
	uint8_t byte = 0;

	for (int x = 0; x < cfg->width; x++) {
		uint8_t pix = rgb565_to_rgb3(src[x]);

		for (int b = 2; b >= 0; b--) {
			byte |= ((pix >> b) & 0x1) << (7 - bitpos);
			bitpos++;

			if (bitpos == 8) {
				*dst++ = byte;
				bitpos = 0;
				byte = 0;
			}
		}
	}
	/* flush final partial byte if needed */
	if (bitpos) {
		*dst++ = byte;
	}
}

/* VCOM toggle callback */
static void lpm_vcom_toggle(struct k_timer *timer)
{
	const struct device *dev = k_timer_user_data_get(timer);
	const struct lpm013m126_config *cfg = dev->config;
	struct lpm013m126_data *data = dev->data;

	data->vcom_state = !data->vcom_state;
	gpio_pin_set_dt(&cfg->extcomin_gpio, data->vcom_state);
}

/* Send a line to the display */
static int lpm_send_line(const struct device *dev, uint8_t line, uint8_t *buf, size_t len)
{
	const struct lpm013m126_config *cfg = dev->config;

	uint8_t cmd = LPM_WRITELINE_CMD;
	uint8_t addr = bitrev8(line);

	struct spi_buf tx_bufs[] = {
		{ .buf = &cmd,  .len = 1 },
		{ .buf = &addr, .len = 1 },
		{ .buf = buf, .len = len },
	};

	struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	return spi_write_dt(&cfg->bus, &tx);
}

/* Send all-clear command */
static int lpm_all_clear(const struct device *dev)
{
	const struct lpm013m126_config *cfg = dev->config;

	uint8_t cmd = LPM_ALLCLEAR_CMD;
	uint8_t dummy = 0x00;

	struct spi_buf tx_bufs[] = {
		{ .buf = &cmd, .len = 1 },
		{ .buf = &dummy, .len = 1 },
	};

	struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs)
	};

	return spi_write_dt(&cfg->bus, &tx);
}

/* Write buffer to panel */
static int lpm_write(const struct device *dev, const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct lpm013m126_config *cfg = dev->config;

	if (x != 0 || desc->width != cfg->width) {
		LOG_ERR("Only full-width writes supported");
		return -ENOTSUP;
	}
	if ((y + desc->height) > cfg->height) {
		LOG_ERR("Buffer out of bounds");
		return -EINVAL;
	}

	const uint16_t *src = buf;
	size_t packed_len = DIV_ROUND_UP(cfg->width * LPM_BPP, 8);
	uint8_t linebuf[packed_len];

	for (int row = 0; row < desc->height; row++) {
		lpm_pack_row(dev, linebuf, src);
		lpm_send_line(dev, y + row + 1, linebuf, packed_len);
		src += cfg->width;
	}

	return 0;
}

static void lpm_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct lpm013m126_config *cfg = dev->config;

	memset(caps, 0, sizeof(*caps));
	caps->x_resolution = cfg->width;
	caps->y_resolution = cfg->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
	caps->screen_info = SCREEN_INFO_X_ALIGNMENT_WIDTH;
}

static int lpm_set_pixel_format(const struct device *dev, enum display_pixel_format pf)
{
	return (pf == PIXEL_FORMAT_RGB_565) ? 0 : -ENOTSUP;
}

static int lpm_blanking_off(const struct device *dev)
{
	const struct lpm013m126_config *cfg = dev->config;

	return gpio_pin_set_dt(&cfg->disp_gpio, 1);
}

static int lpm_blanking_on(const struct device *dev)
{
	const struct lpm013m126_config *cfg = dev->config;

	return gpio_pin_set_dt(&cfg->disp_gpio, 0);
}

static int lpm_init(const struct device *dev)
{
	const struct lpm013m126_config *cfg = dev->config;

	struct lpm013m126_data *data = dev->data;

	if (!spi_is_ready_dt(&cfg->bus)) {
		LOG_ERR("SPI not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->disp_gpio)) {
		LOG_ERR("DISP pin not ready");
		return -ENODEV;
	}
	if (!gpio_is_ready_dt(&cfg->extcomin_gpio)) {
		LOG_ERR("EXTCOMIN pin not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&cfg->disp_gpio, GPIO_OUTPUT_HIGH);
	gpio_pin_configure_dt(&cfg->extcomin_gpio, GPIO_OUTPUT_LOW);

	lpm_all_clear(dev);

	data->vcom_state = 0;
	k_timer_init(&data->vcom_timer, lpm_vcom_toggle, NULL);
	k_timer_user_data_set(&data->vcom_timer, (void *)dev);
	k_timer_start(&data->vcom_timer, K_MSEC(1000 / cfg->extcomin_freq / 2),
		      K_MSEC(1000 / cfg->extcomin_freq / 2));

	return 0;
}

static const struct display_driver_api lpm_api = {
	.blanking_on = lpm_blanking_on,
	.blanking_off = lpm_blanking_off,
	.write = lpm_write,
	.get_capabilities = lpm_get_capabilities,
	.set_pixel_format = lpm_set_pixel_format,
};

#define LPM013M126_INIT(inst)							\
	static const struct lpm013m126_config lpm_cfg_##inst = {		\
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_OP_MODE_MASTER |		\
					    SPI_WORD_SET(8) |			\
					    SPI_TRANSFER_MSB, 0),		\
		.disp_gpio = GPIO_DT_SPEC_INST_GET(inst, disp_gpios),		\
		.extcomin_gpio = GPIO_DT_SPEC_INST_GET(inst, extcomin_gpios),	\
		.extcomin_freq = DT_INST_PROP(inst, extcomin_frequency),	\
		.width = DT_INST_PROP(inst, width),				\
		.height = DT_INST_PROP(inst, height),				\
	};									\
	static struct lpm013m126_data lpm_data_##inst;				\
	DEVICE_DT_INST_DEFINE(inst, lpm_init, NULL, &lpm_data_##inst,		\
			      &lpm_cfg_##inst, POST_KERNEL,			\
			      CONFIG_DISPLAY_INIT_PRIORITY, &lpm_api);

DT_INST_FOREACH_STATUS_OKAY(LPM013M126_INIT);
