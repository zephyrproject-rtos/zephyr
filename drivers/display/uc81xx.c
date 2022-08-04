/*
 * Copyright (c) 2022 Andreas Sandberg
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "uc81xx_regs.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uc81xx, CONFIG_DISPLAY_LOG_LEVEL);

/**
 * UC81XX compatible EPD controller driver.
 *
 * Currently only the black/white panels are supported (KW mode),
 * also first gate/source should be 0.
 */

#define UC81XX_PIXELS_PER_BYTE		8U

struct uc81xx_quirks {
	uint16_t max_width;
	uint16_t max_height;

	int (*set_cdi)(const struct device *dev, bool border);
};

struct uc81xx_dt_array {
	uint8_t *data;
	uint8_t len;
};

struct uc81xx_config {
	const struct uc81xx_quirks *quirks;

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
	struct uc81xx_dt_array softstart;
	struct uc81xx_dt_array pwr;
};

struct uc81xx_data {
	bool blanking_on;
};

static inline int uc81xx_write_cmd(const struct device *dev, uint8_t cmd,
				   const uint8_t *data, size_t len)
{
	const struct uc81xx_config *config = dev->config;
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

static inline int uc81xx_write_cmd_pattern(const struct device *dev,
					   uint8_t cmd,
					   uint8_t pattern, size_t len)
{
	const struct uc81xx_config *config = dev->config;
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

static inline int uc81xx_write_cmd_uint8(const struct device *dev, uint8_t cmd,
					 uint8_t data)
{
	return uc81xx_write_cmd(dev, cmd, &data, 1);
}

static inline int uc81xx_write_array_opt(const struct device *dev, uint8_t cmd,
					 const struct uc81xx_dt_array *array)
{
	if (array->len && array->data) {
		return uc81xx_write_cmd(dev, cmd, array->data, array->len);
	} else {
		return 0;
	}
}

static inline void uc81xx_busy_wait(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy_gpio);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		LOG_DBG("wait %u", pin);
		k_sleep(K_MSEC(UC81XX_BUSY_DELAY));
		pin = gpio_pin_get_dt(&config->busy_gpio);
	}
}

static int uc81xx_update_display(const struct device *dev)
{
	LOG_DBG("Trigger update sequence");

	/* Turn on: booster, controller, regulators, and sensor. */
	if (uc81xx_write_cmd(dev, UC81XX_CMD_PON, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(UC81XX_PON_DELAY));
	uc81xx_busy_wait(dev);

	if (uc81xx_write_cmd(dev, UC81XX_CMD_DRF, NULL, 0)) {
		return -EIO;
	}

	k_sleep(K_MSEC(UC81XX_BUSY_DELAY));
	uc81xx_busy_wait(dev);

	/* Turn on: booster, controller, regulators, and sensor. */
	if (uc81xx_write_cmd(dev, UC81XX_CMD_POF, NULL, 0)) {
		return -EIO;
	}

	return 0;
}

static int uc81xx_blanking_off(const struct device *dev)
{
	struct uc81xx_data *data = dev->data;

	if (data->blanking_on) {
		/* Update EPD panel in normal mode */
		uc81xx_busy_wait(dev);
		if (uc81xx_update_display(dev)) {
			return -EIO;
		}
	}

	data->blanking_on = false;

	return 0;
}

static int uc81xx_blanking_on(const struct device *dev)
{
	struct uc81xx_data *data = dev->data;

	data->blanking_on = true;

	return 0;
}

static int uc81xx_write(const struct device *dev, const uint16_t x, const uint16_t y,
			const struct display_buffer_descriptor *desc,
			const void *buf)
{
	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;

	uint16_t x_end_idx = x + desc->width - 1;
	uint16_t y_end_idx = y + desc->height - 1;
	const struct uc81xx_ptl ptl = {
		.hrst = sys_cpu_to_be16(x),
		.hred = sys_cpu_to_be16(x_end_idx),
		.vrst = sys_cpu_to_be16(y),
		.vred = sys_cpu_to_be16(y_end_idx),
		.flags = UC81XX_PTL_FLAG_PT_SCAN,
	};
	size_t buf_len;

	LOG_DBG("x %u, y %u, height %u, width %u, pitch %u",
		x, y, desc->height, desc->width, desc->pitch);

	buf_len = MIN(desc->buf_size,
		      desc->height * desc->width / UC81XX_PIXELS_PER_BYTE);
	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT(buf != NULL, "Buffer is not available");
	__ASSERT(buf_len != 0U, "Buffer of length zero");
	__ASSERT(!(desc->width % UC81XX_PIXELS_PER_BYTE),
		 "Buffer width not multiple of %d", UC81XX_PIXELS_PER_BYTE);

	if ((y_end_idx > (config->height - 1)) ||
	    (x_end_idx > (config->width - 1))) {
		LOG_ERR("Position out of bounds");
		return -EINVAL;
	}

	/* Setup Partial Window and enable Partial Mode */
	LOG_HEXDUMP_DBG(&ptl, sizeof(ptl), "ptl");

	uc81xx_busy_wait(dev);
	if (uc81xx_write_cmd(dev, UC81XX_CMD_PTIN, NULL, 0)) {
		return -EIO;
	}

	if (uc81xx_write_cmd(dev, UC81XX_CMD_PTL,
			     (const void *)&ptl, sizeof(ptl))) {
		return -EIO;
	}

	/* Disable border output */
	if (config->quirks->set_cdi(dev, false)) {
		return -EIO;
	}

	if (uc81xx_write_cmd(dev, UC81XX_CMD_DTM2, (uint8_t *)buf, buf_len)) {
		return -EIO;
	}

	/* Update partial window and disable Partial Mode */
	if (data->blanking_on == false) {
		if (uc81xx_update_display(dev)) {
			return -EIO;
		}
	}

	/* Enable border output */
	if (config->quirks->set_cdi(dev, true)) {
		return -EIO;
	}

	if (uc81xx_write_cmd(dev, UC81XX_CMD_PTOUT, NULL, 0)) {
		return -EIO;
	}

	return 0;
}

static int uc81xx_read(const struct device *dev, const uint16_t x, const uint16_t y,
		       const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *uc81xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int uc81xx_set_brightness(const struct device *dev,
				 const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int uc81xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void uc81xx_get_capabilities(const struct device *dev,
				    struct display_capabilities *caps)
{
	const struct uc81xx_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_MSB_FIRST | SCREEN_INFO_EPD;
}

static int uc81xx_set_orientation(const struct device *dev,
				  const enum display_orientation
				  orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int uc81xx_set_pixel_format(const struct device *dev,
				   const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int uc81xx_clear_and_write_buffer(const struct device *dev,
					 uint8_t pattern, bool update)
{
	const struct uc81xx_config *config = dev->config;
	const int size = config->width * config->height
		/ UC81XX_PIXELS_PER_BYTE;

	if (uc81xx_write_cmd_pattern(dev, UC81XX_CMD_DTM1, pattern, size)) {
		return -EIO;
	}

	if (uc81xx_write_cmd_pattern(dev, UC81XX_CMD_DTM2, pattern, size)) {
		return -EIO;
	}

	if (update == true) {
		if (uc81xx_update_display(dev)) {
			return -EIO;
		}
	}

	return 0;
}

static int uc81xx_controller_init(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;
	struct uc81xx_data *data = dev->data;
	const uint8_t psr_kw =
		UC81XX_PSR_KW_R |
		UC81XX_PSR_UD |
		UC81XX_PSR_SHL |
		UC81XX_PSR_SHD |
		UC81XX_PSR_RST;
	const struct uc81xx_tres tres = {
		.hres = sys_cpu_to_be16(config->width),
		.vres = sys_cpu_to_be16(config->height),
	};

	data->blanking_on = true;

	gpio_pin_set_dt(&config->reset_gpio, 1);
	k_sleep(K_MSEC(UC81XX_RESET_DELAY));
	gpio_pin_set_dt(&config->reset_gpio, 0);
	k_sleep(K_MSEC(UC81XX_RESET_DELAY));
	uc81xx_busy_wait(dev);

	LOG_DBG("Initialize UC81XX controller");

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_PWR, &config->pwr)) {
		return -EIO;
	}

	if (uc81xx_write_array_opt(dev, UC81XX_CMD_BTST, &config->softstart)) {
		return -EIO;
	}

	/* Panel settings, KW mode */
	if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_PSR, psr_kw)) {
		return -EIO;
	}

	/* Set panel resolution */
	LOG_HEXDUMP_DBG(&tres, sizeof(tres), "TRES");
	if (uc81xx_write_cmd(dev, UC81XX_CMD_TRES,
			     (const void *)&tres, sizeof(tres))) {
		return -EIO;
	}

	/* Set CDI and enable border output */
	if (config->quirks->set_cdi(dev, true)) {
		return -EIO;
	}

	if (config->override_tcon) {
		if (uc81xx_write_cmd_uint8(dev, UC81XX_CMD_TCON,
					   config->tcon)) {
			return -EIO;
		}
	}

	if (uc81xx_clear_and_write_buffer(dev, 0xff, false)) {
		return -EIO;
	}

	return 0;
}

static int uc81xx_init(const struct device *dev)
{
	const struct uc81xx_config *config = dev->config;

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

	if (config->width > config->quirks->max_width ||
	    config->height > config->quirks->max_height) {
		LOG_ERR("Display size out of range.");
		return -EINVAL;
	}

	return uc81xx_controller_init(dev);
}

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8176)
static int uc8176_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	uint8_t cdi =
		UC8176_CDI_VBD1 | UC8176_CDI_DDX0 |
		(config->cdi & UC8176_CDI_CDI_MASK);

	if (!config->override_cdi) {
		return 0;
	}

	if (!border) {
		/* Floating border */
		cdi |= UC8176_CDI_VBD1 | UC8176_CDI_VBD0;
	}

	LOG_DBG("CDI: %#hhx", cdi);
	return uc81xx_write_cmd_uint8(dev, UC81XX_CMD_CDI, cdi);
}

static const struct uc81xx_quirks uc8176_quirks = {
	.max_width = 400,
	.max_height = 300,

	.set_cdi = uc8176_set_cdi,
};
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(ultrachip_uc8179)
static int uc8179_set_cdi(const struct device *dev, bool border)
{
	const struct uc81xx_config *config = dev->config;
	uint8_t cdi[UC8179_CDI_REG_LENGTH] = {
		UC8179_CDI_BDV1 | UC8179_CDI_N2OCP | UC8179_CDI_DDX0,
		config->cdi,
	};

	if (!config->override_cdi) {
		return 0;
	}

	cdi[UC8179_CDI_BDZ_DDX_IDX] |= border ? 0 : UC8179_CDI_BDZ;

	LOG_HEXDUMP_DBG(cdi, sizeof(cdi), "CDI");
	return uc81xx_write_cmd(dev, UC81XX_CMD_CDI, cdi, sizeof(cdi));
}

static const struct uc81xx_quirks uc8179_quirks = {
	.max_width = 800,
	.max_height = 600,

	.set_cdi = uc8179_set_cdi,
};
#endif

static struct display_driver_api uc81xx_driver_api = {
	.blanking_on = uc81xx_blanking_on,
	.blanking_off = uc81xx_blanking_off,
	.write = uc81xx_write,
	.read = uc81xx_read,
	.get_framebuffer = uc81xx_get_framebuffer,
	.set_brightness = uc81xx_set_brightness,
	.set_contrast = uc81xx_set_contrast,
	.get_capabilities = uc81xx_get_capabilities,
	.set_pixel_format = uc81xx_set_pixel_format,
	.set_orientation = uc81xx_set_orientation,
};

#define UC81XX_MAKE_ARRAY_OPT(n, p)					\
	static uint8_t data_ ## n ## _ ## p[] = DT_PROP_OR(n, p, {})

#define UC81XX_MAKE_ARRAY(n, p)						\
	static uint8_t data_ ## n ## _ ## p[] = DT_PROP(n, p)

#define UC81XX_ASSIGN_ARRAY(n, p)					\
	{								\
		.data = data_ ## n ## _ ## p,				\
		.len = sizeof(data_ ## n ## _ ## p),			\
	}

#define UC81XX_DEFINE(n, quirks_ptr)					\
	UC81XX_MAKE_ARRAY_OPT(n, softstart);				\
	UC81XX_MAKE_ARRAY_OPT(n, pwr);					\
									\
	static const struct uc81xx_config uc81xx_cfg_ ## n = {		\
		.quirks = quirks_ptr,					\
		.bus = SPI_DT_SPEC_GET(n,				\
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |		\
			SPI_LOCK_ON,					\
			0),						\
		.reset_gpio = GPIO_DT_SPEC_GET(n, reset_gpios),		\
		.dc_gpio = GPIO_DT_SPEC_GET(n, dc_gpios),		\
		.busy_gpio = GPIO_DT_SPEC_GET(n, busy_gpios),		\
									\
		.height = DT_PROP(n, height),				\
		.width = DT_PROP(n, width),				\
									\
		.cdi = DT_PROP_OR(n, cdi, 0),				\
		.override_cdi = DT_NODE_HAS_PROP(n, cdi),		\
		.tcon = DT_PROP_OR(n, tcon, 0),				\
		.override_tcon = DT_NODE_HAS_PROP(n, tcon),		\
		.softstart = UC81XX_ASSIGN_ARRAY(n, softstart),		\
		.pwr = UC81XX_ASSIGN_ARRAY(n, pwr),			\
	};								\
									\
	static struct uc81xx_data uc81xx_data_##n = {};			\
									\
	DEVICE_DT_DEFINE(n, uc81xx_init, NULL,				\
			 &uc81xx_data_ ## n,				\
			 &uc81xx_cfg_ ## n,				\
			 POST_KERNEL,					\
			 CONFIG_DISPLAY_INIT_PRIORITY,			\
			 &uc81xx_driver_api);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8176, UC81XX_DEFINE,
			     &uc8176_quirks);

DT_FOREACH_STATUS_OKAY_VARGS(ultrachip_uc8179, UC81XX_DEFINE,
			     &uc8179_quirks);
