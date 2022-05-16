/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gooddisplay_gd7965

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "gd7965_regs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gd7965, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * GD7965 compatible EPD controller driver.
 *
 * Currently only the black/white panels are supported (KW mode),
 * also first gate/source should be 0.
 */

#define EPD_PANEL_WIDTH			DT_INST_PROP(0, width)
#define EPD_PANEL_HEIGHT		DT_INST_PROP(0, height)
#define GD7965_PIXELS_PER_BYTE		8U

/* Horizontally aligned page! */
#define GD7965_NUMOF_PAGES		(EPD_PANEL_WIDTH / \
					 GD7965_PIXELS_PER_BYTE)
#define GD7965_PANEL_FIRST_GATE		0U
#define GD7965_PANEL_LAST_GATE		(EPD_PANEL_HEIGHT - 1)
#define GD7965_PANEL_FIRST_PAGE		0U
#define GD7965_PANEL_LAST_PAGE		(GD7965_NUMOF_PAGES - 1)

struct gd7965_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec dc_gpio;
	struct gpio_dt_spec busy_gpio;
	struct gpio_dt_spec reset_gpio;
};

static uint8_t gd7965_softstart[] = DT_INST_PROP(0, softstart);
static uint8_t gd7965_pwr[] = DT_INST_PROP(0, pwr);

/* Border and data polarity settings */
static uint8_t bdd_polarity;

static bool blanking_on = true;

static inline int gd7965_write_cmd(const struct device *dev, uint8_t cmd,
				   uint8_t *data, size_t len)
{
	const struct gd7965_config *config = dev->config;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};

	gpio_pin_set_dt(&config->dc_gpio, 1);
	if (spi_write_dt(&config->bus, &buf_set)) {
		return -EIO;
	}

	if (data != NULL) {
		buf.buf = data;
		buf.len = len;
		gpio_pin_set_dt(&config->dc_gpio, 0);
		if (spi_write_dt(&config->bus, &buf_set)) {
			return -EIO;
		}
	}

	return 0;
}

static inline void gd7965_busy_wait(const struct device *dev)
{
	const struct gd7965_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy_gpio);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		LOG_DBG("wait %u", pin);
		k_sleep(K_MSEC(GD7965_BUSY_DELAY));
		pin = gpio_pin_get_dt(&config->busy_gpio);
	}
}

static int gd7965_update_display(const struct device *dev)
{
	LOG_DBG("Trigger update sequence");
	if (gd7965_write_cmd(dev, GD7965_CMD_DRF, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(GD7965_BUSY_DELAY));

	return 0;
}

static int gd7965_blanking_off(const struct device *dev)
{
	if (blanking_on) {
		/* Update EPD panel in normal mode */
		gd7965_busy_wait(dev);
		if (gd7965_update_display(dev)) {
			return -EIO;
		}
	}

	blanking_on = false;

	return 0;
}

static int gd7965_blanking_on(const struct device *dev)
{
	blanking_on = true;

	return 0;
}

static int gd7965_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	uint16_t x_end_idx = x + desc->width - 1;
	uint16_t y_end_idx = y + desc->height - 1;
	uint8_t ptl[GD7965_PTL_REG_LENGTH] = {0};
	size_t buf_len;

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u",
		x, y, desc->height, desc->width, desc->pitch);

	buf_len = MIN(desc->buf_size,
		      desc->height * desc->width / GD7965_PIXELS_PER_BYTE);
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(buf != NULL, "Buffer is not available");
	__ASSERT(buf_len != 0U, "Buffer of length zero");
	__ASSERT(!(desc->width % GD7965_PIXELS_PER_BYTE),
		 "Buffer width not multiple of %d", GD7965_PIXELS_PER_BYTE);

	if ((y_end_idx > (EPD_PANEL_HEIGHT - 1)) ||
	    (x_end_idx > (EPD_PANEL_WIDTH - 1))) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	/* Setup Partial Window and enable Partial Mode */
	sys_put_be16(x, &ptl[GD7965_PTL_HRST_IDX]);
	sys_put_be16(x_end_idx, &ptl[GD7965_PTL_HRED_IDX]);
	sys_put_be16(y, &ptl[GD7965_PTL_VRST_IDX]);
	sys_put_be16(y_end_idx, &ptl[GD7965_PTL_VRED_IDX]);
	ptl[sizeof(ptl) - 1] = GD7965_PTL_PT_SCAN;
	LOG_HEXDUMP_DBG(ptl, sizeof(ptl), "ptl");

	gd7965_busy_wait(dev);
	if (gd7965_write_cmd(dev, GD7965_CMD_PTIN, NULL, 0)) {
		return -EIO;
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_PTL, ptl, sizeof(ptl))) {
		return -EIO;
	}

	/* Disable boarder output */
	bdd_polarity |= GD7965_CDI_BDZ;
	if (gd7965_write_cmd(dev, GD7965_CMD_CDI,
			     &bdd_polarity, sizeof(bdd_polarity))) {
		return -EIO;
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_DTM2, (uint8_t *)buf, buf_len)) {
		return -EIO;
	}

	/* Update partial window and disable Partial Mode */
	if (blanking_on == false) {
		if (gd7965_update_display(dev)) {
			return -EIO;
		}
	}

	/* Enable boarder output */
	bdd_polarity &= ~GD7965_CDI_BDZ;
	if (gd7965_write_cmd(dev, GD7965_CMD_CDI,
			     &bdd_polarity, sizeof(bdd_polarity))) {
		return -EIO;
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_PTOUT, NULL, 0)) {
		return -EIO;
	}

	return 0;
}

static int gd7965_read(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *gd7965_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int gd7965_set_brightness(const struct device *dev,
				 const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int gd7965_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void gd7965_get_capabilities(const struct device *dev,
				    struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = EPD_PANEL_WIDTH;
	caps->y_resolution = EPD_PANEL_HEIGHT;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
}

static int gd7965_set_orientation(const struct device *dev,
				  const enum display_orientation
				  orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int gd7965_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int gd7965_clear_and_write_buffer(const struct device *dev,
					 uint8_t pattern, bool update)
{
	struct display_buffer_descriptor desc = {
		.buf_size = GD7965_NUMOF_PAGES,
		.width = EPD_PANEL_WIDTH,
		.height = 1,
		.pitch = EPD_PANEL_WIDTH,
	};
	uint8_t *line;

	line = k_malloc(GD7965_NUMOF_PAGES);
	if (line == NULL) {
		return -ENOMEM;
	}

	memset(line, pattern, GD7965_NUMOF_PAGES);
	for (int i = 0; i < EPD_PANEL_HEIGHT; i++) {
		gd7965_write(dev, 0, i, &desc, line);
	}

	k_free(line);

	if (update == true) {
		if (gd7965_update_display(dev)) {
			return -EIO;
		}
	}

	return 0;
}

static int gd7965_controller_init(const struct device *dev)
{
	const struct gd7965_config *config = dev->config;
	uint8_t tmp[GD7965_TRES_REG_LENGTH];

	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_sleep(K_MSEC(GD7965_RESET_DELAY));
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_sleep(K_MSEC(GD7965_RESET_DELAY));
	gd7965_busy_wait(dev);

	LOG_DBG("Initialize GD7965 controller");

	if (gd7965_write_cmd(dev, GD7965_CMD_PWR, gd7965_pwr,
			     sizeof(gd7965_pwr))) {
		return -EIO;
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_BTST,
			     gd7965_softstart, sizeof(gd7965_softstart))) {
		return -EIO;
	}

	/* Turn on: booster, controller, regulators, and sensor. */
	if (gd7965_write_cmd(dev, GD7965_CMD_PON, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(GD7965_PON_DELAY));
	gd7965_busy_wait(dev);

	/* Panel settings, KW mode */
	tmp[0] = GD7965_PSR_KW_R |
		 GD7965_PSR_UD |
		 GD7965_PSR_SHL |
		 GD7965_PSR_SHD |
		 GD7965_PSR_RST;
	if (gd7965_write_cmd(dev, GD7965_CMD_PSR, tmp, 1)) {
		return -EIO;
	}

	/* Set panel resolution */
	sys_put_be16(EPD_PANEL_WIDTH, &tmp[GD7965_TRES_HRES_IDX]);
	sys_put_be16(EPD_PANEL_HEIGHT, &tmp[GD7965_TRES_VRES_IDX]);
	LOG_HEXDUMP_DBG(tmp, sizeof(tmp), "TRES");
	if (gd7965_write_cmd(dev, GD7965_CMD_TRES,
			     tmp, GD7965_TRES_REG_LENGTH)) {
		return -EIO;
	}

	bdd_polarity = GD7965_CDI_BDV1 |
		       GD7965_CDI_N2OCP | GD7965_CDI_DDX0;
	tmp[GD7965_CDI_BDZ_DDX_IDX] = bdd_polarity;
	tmp[GD7965_CDI_CDI_IDX] = DT_INST_PROP(0, cdi);
	LOG_HEXDUMP_DBG(tmp, GD7965_CDI_REG_LENGTH, "CDI");
	if (gd7965_write_cmd(dev, GD7965_CMD_CDI, tmp,
			     GD7965_CDI_REG_LENGTH)) {
		return -EIO;
	}

	tmp[0] = DT_INST_PROP(0, tcon);
	if (gd7965_write_cmd(dev, GD7965_CMD_TCON, tmp, 1)) {
		return -EIO;
	}

	/* Enable Auto Sequence */
	tmp[0] = GD7965_AUTO_PON_DRF_POF;
	if (gd7965_write_cmd(dev, GD7965_CMD_AUTO, tmp, 1)) {
		return -EIO;
	}

	if (gd7965_clear_and_write_buffer(dev, 0xff, false)) {
		return -1;
	}

	return 0;
}

static int gd7965_init(const struct device *dev)
{
	const struct gd7965_config *config = dev->config;

	LOG_DBG("");

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->reset_gpio.port)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);

	if (!device_is_ready(config->dc_gpio.port)) {
		LOG_ERR("DC GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);


	if (!device_is_ready(config->busy_gpio.port)) {
		LOG_ERR("Busy GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);

	return gd7965_controller_init(dev);
}

static const struct gd7965_config gd7965_config = {
	.bus = SPI_DT_SPEC_INST_GET(
		0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),
	.reset_gpio = GPIO_DT_SPEC_INST_GET(0, reset_gpios),
	.dc_gpio = GPIO_DT_SPEC_INST_GET(0, dc_gpios),
	.busy_gpio = GPIO_DT_SPEC_INST_GET(0, busy_gpios),
};

static struct display_driver_api gd7965_driver_api = {
	.blanking_on = gd7965_blanking_on,
	.blanking_off = gd7965_blanking_off,
	.write = gd7965_write,
	.read = gd7965_read,
	.get_framebuffer = gd7965_get_framebuffer,
	.set_brightness = gd7965_set_brightness,
	.set_contrast = gd7965_set_contrast,
	.get_capabilities = gd7965_get_capabilities,
	.set_pixel_format = gd7965_set_pixel_format,
	.set_orientation = gd7965_set_orientation,
};


DEVICE_DT_INST_DEFINE(0, gd7965_init, NULL, NULL, &gd7965_config, POST_KERNEL,
		      CONFIG_DISPLAY_INIT_PRIORITY, &gd7965_driver_api);
