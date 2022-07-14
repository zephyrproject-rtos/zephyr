/*
 * Copyright (c) 2022 Andreas Sandberg
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

#define GD7965_PIXELS_PER_BYTE		8U

struct gd7965_dt_array {
	uint8_t *data;
	uint8_t len;
};

struct gd7965_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec dc_gpio;
	struct gpio_dt_spec busy_gpio;
	struct gpio_dt_spec reset_gpio;

	uint16_t height;
	uint16_t width;

	uint8_t cdi;
	bool override_cdi;
	uint8_t tcon;
	bool override_tcon;
	struct gd7965_dt_array softstart;
	struct gd7965_dt_array pwr;
};

struct gd7965_data {
	bool blanking_on;

	/* Border and data polarity settings */
	uint8_t bdd_polarity;
};

static inline int gd7965_write_cmd(const struct device *dev, uint8_t cmd,
				   const uint8_t *data, size_t len)
{
	const struct gd7965_config *config = dev->config;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
	int err;

	err = gpio_pin_set_dt(&config->dc_gpio, 1);
	if (err < 0) {
		return err;
	}

	err = spi_write_dt(&config->bus, &buf_set);
	if (err < 0) {
		goto spi_out;
	}

	if (data != NULL) {
		buf.buf = (void *)data;
		buf.len = len;

		err = gpio_pin_set_dt(&config->dc_gpio, 0);
		if (err < 0) {
			goto spi_out;
		}

		err = spi_write_dt(&config->bus, &buf_set);
		if (err < 0) {
			goto spi_out;
		}
	}

spi_out:
	spi_release_dt(&config->bus);
	return err;
}

static inline int gd7965_write_cmd_pattern(const struct device *dev,
					   uint8_t cmd,
					   uint8_t pattern, size_t len)
{
	const struct gd7965_config *config = dev->config;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
	int err;
	uint8_t data[64];

	err = gpio_pin_set_dt(&config->dc_gpio, 1);
	if (err < 0) {
		return err;
	}

	err = spi_write_dt(&config->bus, &buf_set);
	if (err < 0) {
		goto spi_out;
	}

	err = gpio_pin_set_dt(&config->dc_gpio, 0);
	if (err < 0) {
		goto spi_out;
	}

	memset(data, pattern, sizeof(data));
	while (len) {
		buf.buf = data;
		buf.len = MIN(len, sizeof(data));

		err = spi_write_dt(&config->bus, &buf_set);
		if (err < 0) {
			goto spi_out;
		}

		len -= buf.len;
	}

spi_out:
	spi_release_dt(&config->bus);
	return err;
}

static inline int gd7965_write_cmd_uint8(const struct device *dev, uint8_t cmd,
					 uint8_t data)
{
	return gd7965_write_cmd(dev, cmd, &data, 1);
}

static inline int gd7965_write_array_opt(const struct device *dev, uint8_t cmd,
					 const struct gd7965_dt_array *array)
{
	if (array->len && array->data) {
		return gd7965_write_cmd(dev, cmd, array->data, array->len);
	} else {
		return 0;
	}
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
	struct gd7965_data *data = dev->data;

	if (data->blanking_on) {
		/* Update EPD panel in normal mode */
		gd7965_busy_wait(dev);
		if (gd7965_update_display(dev)) {
			return -EIO;
		}
	}

	data->blanking_on = false;

	return 0;
}

static int gd7965_blanking_on(const struct device *dev)
{
	struct gd7965_data *data = dev->data;

	data->blanking_on = true;

	return 0;
}

static int gd7965_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	const struct gd7965_config *config = dev->config;
	struct gd7965_data *data = dev->data;

	uint16_t x_end_idx = x + desc->width - 1;
	uint16_t y_end_idx = y + desc->height - 1;
	const struct gd7965_ptl ptl = {
		.hrst = sys_cpu_to_be16(x),
		.hred = sys_cpu_to_be16(x_end_idx),
		.vrst = sys_cpu_to_be16(y),
		.vred = sys_cpu_to_be16(y_end_idx),
		.flags = GD7965_PTL_FLAG_PT_SCAN,
	};
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

	if ((y_end_idx > (config->height - 1)) ||
	    (x_end_idx > (config->width - 1))) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	/* Setup Partial Window and enable Partial Mode */
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "ptl");

	gd7965_busy_wait(dev);
	if (gd7965_write_cmd(dev, GD7965_CMD_PTIN, NULL, 0)) {
		return -EIO;
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_PTL,
			     (const void *)&ptl, sizeof(ptl))) {
		return -EIO;
	}

	if (config->override_cdi) {
		/* Disable boarder output */
		data->bdd_polarity |= GD7965_CDI_BDZ;
		if (gd7965_write_cmd_uint8(dev, GD7965_CMD_CDI,
					   data->bdd_polarity)) {
			return -EIO;
		}
	}

	if (gd7965_write_cmd(dev, GD7965_CMD_DTM2, (uint8_t *)buf, buf_len)) {
		return -EIO;
	}

	/* Update partial window and disable Partial Mode */
	if (data->blanking_on == false) {
		if (gd7965_update_display(dev)) {
			return -EIO;
		}
	}

	if (config->override_cdi) {
		/* Enable boarder output */
		data->bdd_polarity &= ~GD7965_CDI_BDZ;
		if (gd7965_write_cmd_uint8(dev, GD7965_CMD_CDI,
					   data->bdd_polarity)) {
			return -EIO;
		}
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
	const struct gd7965_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
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
	const struct gd7965_config *config = dev->config;
	const int size = config->width * config->height
		/ GD7965_PIXELS_PER_BYTE;

	if (gd7965_write_cmd_pattern(dev, GD7965_CMD_DTM1, pattern, size)) {
		return -EIO;
	}

	if (gd7965_write_cmd_pattern(dev, GD7965_CMD_DTM2, pattern, size)) {
		return -EIO;
	}

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
	struct gd7965_data *data = dev->data;
	const uint8_t psr_kw =
		GD7965_PSR_KW_R |
		GD7965_PSR_UD |
		GD7965_PSR_SHL |
		GD7965_PSR_SHD |
		GD7965_PSR_RST;
	const struct gd7965_tres tres = {
		.hres = sys_cpu_to_be16(config->width),
		.vres = sys_cpu_to_be16(config->height),
	};
	uint8_t cdi[GD7965_CDI_REG_LENGTH];

	data->blanking_on = true;

	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_sleep(K_MSEC(GD7965_RESET_DELAY));
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_sleep(K_MSEC(GD7965_RESET_DELAY));
	gd7965_busy_wait(dev);

	LOG_DBG("Initialize GD7965 controller");

	if (gd7965_write_array_opt(dev, GD7965_CMD_PWR, &config->pwr)) {
		return -EIO;
	}

	if (gd7965_write_array_opt(dev, GD7965_CMD_BTST, &config->softstart)) {
		return -EIO;
	}

	/* Turn on: booster, controller, regulators, and sensor. */
	if (gd7965_write_cmd(dev, GD7965_CMD_PON, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(GD7965_PON_DELAY));
	gd7965_busy_wait(dev);

	/* Panel settings, KW mode */
	if (gd7965_write_cmd_uint8(dev, GD7965_CMD_PSR, psr_kw)) {
		return -EIO;
	}

	/* Set panel resolution */
	LOG_HEXDUMP_DBG(&tres, sizeof(tres), "TRES");
	if (gd7965_write_cmd(dev, GD7965_CMD_TRES,
			     (const void *)&tres, sizeof(tres))) {
		return -EIO;
	}

	data->bdd_polarity = GD7965_CDI_BDV1 |
		       GD7965_CDI_N2OCP | GD7965_CDI_DDX0;
	if (config->override_cdi) {
		cdi[GD7965_CDI_BDZ_DDX_IDX] = data->bdd_polarity;
		cdi[GD7965_CDI_CDI_IDX] = config->cdi;
		LOG_HEXDUMP_DBG(cdi, sizeof(cdi), "CDI");
		if (gd7965_write_cmd(dev, GD7965_CMD_CDI,
				     cdi, sizeof(cdi))) {
			return -EIO;
		}
	}

	if (config->override_tcon) {
		if (gd7965_write_cmd_uint8(dev, GD7965_CMD_TCON,
					   config->tcon)) {
			return -EIO;
		}
	}

	/* Enable Auto Sequence */
	if (gd7965_write_cmd_uint8(dev, GD7965_CMD_AUTO,
				   GD7965_AUTO_PON_DRF_POF)) {
		return -EIO;
	}

	if (gd7965_clear_and_write_buffer(dev, 0xff, false)) {
		return -EIO;
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

#define GD7965_MAKE_INST_ARRAY_OPT(n, p)				\
	static uint8_t data_ ## n ## _ ## p[] = DT_INST_PROP_OR(n, p, {})

#define GD7965_MAKE_INST_ARRAY(n, p)					\
	static uint8_t data_ ## n ## _ ## p[] = DT_INST_PROP(n, p)

#define GD7965_ASSIGN_ARRAY(n, p)					\
	{								\
		.data = data_ ## n ## _ ## p,				\
		.len = sizeof(data_ ## n ## _ ## p),			\
	}

#define GD7965_DEFINE(n)						\
	GD7965_MAKE_INST_ARRAY_OPT(n, softstart);			\
	GD7965_MAKE_INST_ARRAY_OPT(n, pwr);				\
									\
	static const struct gd7965_config gd7965_cfg_##n = {		\
		.bus = SPI_DT_SPEC_INST_GET(n,				\
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |		\
			SPI_LOCK_ON,					\
			0),						\
		.reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),	\
		.dc_gpio = GPIO_DT_SPEC_INST_GET(n, dc_gpios),		\
		.busy_gpio = GPIO_DT_SPEC_INST_GET(n, busy_gpios),	\
									\
		.height = DT_INST_PROP(n, height),			\
		.width = DT_INST_PROP(n, width),			\
									\
		.cdi = DT_INST_PROP_OR(n, cdi, 0),			\
		.override_cdi = DT_INST_NODE_HAS_PROP(n, cdi),		\
		.tcon = DT_INST_PROP_OR(n, tcon, 0),			\
		.override_tcon = DT_INST_NODE_HAS_PROP(n, tcon),	\
		.softstart = GD7965_ASSIGN_ARRAY(n, softstart),		\
		.pwr = GD7965_ASSIGN_ARRAY(n, pwr),			\
	};								\
									\
	static struct gd7965_data gd7965_data_##n = {};			\
									\
	DEVICE_DT_INST_DEFINE(n, gd7965_init, NULL,			\
			      &gd7965_data_##n,				\
			      &gd7965_cfg_##n,				\
			      POST_KERNEL,				\
			      CONFIG_DISPLAY_INIT_PRIORITY,		\
			      &gd7965_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GD7965_DEFINE)
