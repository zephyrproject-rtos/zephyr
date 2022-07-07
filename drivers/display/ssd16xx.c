/*
 * Copyright (c) 2018-2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT solomon_ssd16xxfb

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ssd16xx);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>

#include "ssd16xx_regs.h"

/**
 * SSD1673, SSD1608, SSD1681, ILI3897 compatible EPD controller driver.
 */

#define EPD_PANEL_NUMOF_ROWS_PER_PAGE	8
#define SSD16XX_PANEL_FIRST_PAGE	0
#define SSD16XX_PANEL_FIRST_GATE	0
#define SSD16XX_PIXELS_PER_BYTE		8
#define SSD16XX_DEFAULT_TR_VALUE	25U
#define SSD16XX_TR_SCALE_FACTOR		256U

struct ssd16xx_data {
	uint8_t scan_mode;
	uint8_t update_cmd;
	bool blanking_on;
};

struct ssd16xx_dt_array {
	uint8_t *data;
	uint8_t len;
};

struct ssd16xx_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec dc_gpio;
	struct gpio_dt_spec busy_gpio;
	struct gpio_dt_spec reset_gpio;
	struct ssd16xx_dt_array lut_initial;
	struct ssd16xx_dt_array lut_default;
	struct ssd16xx_dt_array softstart;
	struct ssd16xx_dt_array gdv;
	struct ssd16xx_dt_array sdv;
	bool orientation;
	uint16_t height;
	uint16_t width;
	uint8_t vcom;
	uint8_t b_waveform;
	uint8_t tssv;
	uint8_t pp_width_bits;
	uint8_t pp_height_bits;
};

static inline void ssd16xx_busy_wait(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	int pin = gpio_pin_get_dt(&config->busy_gpio);

	while (pin > 0) {
		__ASSERT(pin >= 0, "Failed to get pin level");
		k_msleep(SSD16XX_BUSY_DELAY);
		pin = gpio_pin_get_dt(&config->busy_gpio);
	}
}

static inline int ssd16xx_write_cmd(const struct device *dev, uint8_t cmd,
				    uint8_t *data, size_t len)
{
	const struct ssd16xx_config *config = dev->config;
	struct spi_buf buf = {.buf = &cmd, .len = sizeof(cmd)};
	struct spi_buf_set buf_set = {.buffers = &buf, .count = 1};
	int err = 0;

	ssd16xx_busy_wait(dev);

	err = gpio_pin_set_dt(&config->dc_gpio, 1);
	if (err < 0) {
		return err;
	}

	err = spi_write_dt(&config->bus, &buf_set);
	if (err < 0) {
		goto spi_out;
	}

	if (data != NULL) {
		buf.buf = data;
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

static inline size_t push_x_param(const struct device *dev,
				  uint8_t *data, uint16_t x)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->pp_width_bits == 8) {
		data[0] = (uint8_t)x;
		return 1;
	}

	if (config->pp_width_bits == 16) {
		sys_put_le16(sys_cpu_to_le16(x), data);
		return 2;
	}

	LOG_ERR("Unsupported pp_width_bits %u", config->pp_width_bits);
	return 0;
}

static inline size_t push_y_param(const struct device *dev,
				  uint8_t *data, uint16_t y)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->pp_height_bits == 8) {
		data[0] = (uint8_t)y;
		return 1;
	}

	if (config->pp_height_bits == 16) {
		sys_put_le16(sys_cpu_to_le16(y), data);
		return 2;
	}

	LOG_ERR("Unsupported pp_height_bitsa %u", config->pp_height_bits);
	return 0;
}



static inline int ssd16xx_set_ram_param(const struct device *dev,
					uint16_t sx, uint16_t ex,
					uint16_t sy, uint16_t ey)
{
	int err;
	uint8_t tmp[4];
	size_t len;

	len  = push_x_param(dev, tmp, sx);
	len += push_x_param(dev, tmp + len, ex);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_XPOS_CTRL, tmp, len);
	if (err < 0) {
		return err;
	}

	len  = push_y_param(dev, tmp, sy);
	len += push_y_param(dev, tmp + len, ey);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_YPOS_CTRL, tmp,	len);
	if (err < 0) {
		return err;
	}

	return 0;
}

static inline int ssd16xx_set_ram_ptr(const struct device *dev, uint16_t x,
				      uint16_t y)
{
	int err;
	uint8_t tmp[2];
	size_t len;

	len = push_x_param(dev, tmp, x);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_XPOS_CNTR, tmp, len);
	if (err < 0) {
		return err;
	}

	len = push_y_param(dev, tmp, y);
	return ssd16xx_write_cmd(dev, SSD16XX_CMD_RAM_YPOS_CNTR, tmp, len);
}

static int ssd16xx_update_display(const struct device *dev)
{
	struct ssd16xx_data *data = dev->data;
	int err;

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_CTRL2,
				&data->update_cmd, 1);
	if (err < 0) {
		return err;
	}

	return ssd16xx_write_cmd(dev, SSD16XX_CMD_MASTER_ACTIVATION, NULL, 0);
}

static int ssd16xx_blanking_off(const struct device *dev)
{
	struct ssd16xx_data *data = dev->data;

	if (data->blanking_on) {
		data->blanking_on = false;
		return ssd16xx_update_display(dev);
	}

	return 0;
}

static int ssd16xx_blanking_on(const struct device *dev)
{
	struct ssd16xx_data *data = dev->data;

	data->blanking_on = true;

	return 0;
}

static int ssd16xx_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	int err;
	size_t buf_len;
	uint16_t x_start;
	uint16_t x_end;
	uint16_t y_start;
	uint16_t y_end;
	uint16_t panel_h = config->height -
			   config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if ((y + desc->height) > panel_h) {
		LOG_ERR("Buffer out of bounds (height)");
		return -EINVAL;
	}

	if ((x + desc->width) > config->width) {
		LOG_ERR("Buffer out of bounds (width)");
		return -EINVAL;
	}

	if ((desc->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
		LOG_ERR("Buffer height not multiple of %d",
				EPD_PANEL_NUMOF_ROWS_PER_PAGE);
		return -EINVAL;
	}

	if ((y % EPD_PANEL_NUMOF_ROWS_PER_PAGE) != 0U) {
		LOG_ERR("Y coordinate not multiple of %d",
				EPD_PANEL_NUMOF_ROWS_PER_PAGE);
		return -EINVAL;
	}

	switch (data->scan_mode) {
	case SSD16XX_DATA_ENTRY_XIYDY:
		x_start = y / SSD16XX_PIXELS_PER_BYTE;
		x_end = (y + desc->height - 1) / SSD16XX_PIXELS_PER_BYTE;
		y_start = (x + desc->width - 1);
		y_end = x;
		break;

	case SSD16XX_DATA_ENTRY_XDYIY:
		x_start = (panel_h - 1 - y) / SSD16XX_PIXELS_PER_BYTE;
		x_end = (panel_h - 1 - (y + desc->height - 1)) /
			SSD16XX_PIXELS_PER_BYTE;
		y_start = x;
		y_end = (x + desc->width - 1);
		break;
	default:
		return -EINVAL;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_ENTRY_MODE,
				&data->scan_mode, sizeof(data->scan_mode));
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_param(dev, x_start, x_end, y_start, y_end);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_ptr(dev, x_start, y_start);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_WRITE_RAM, (uint8_t *)buf,
				buf_len);
	if (err < 0) {
		return err;
	}

	if (!data->blanking_on) {
		err = ssd16xx_update_display(dev);
		if (err < 0) {
			return err;
		}
	}

	return 0;
}

static int ssd16xx_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_ERR("not supported");
	return -ENOTSUP;
}

static void *ssd16xx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("not supported");
	return NULL;
}

static int ssd16xx_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static int ssd16xx_set_contrast(const struct device *dev, uint8_t contrast)
{
	LOG_WRN("not supported");
	return -ENOTSUP;
}

static void ssd16xx_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	const struct ssd16xx_config *config = dev->config;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height -
			     config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_VTILED |
			    SCREEN_INFO_MONO_MSB_FIRST |
			    SCREEN_INFO_EPD |
			    SCREEN_INFO_DOUBLE_BUFFER;
}

static int ssd16xx_set_orientation(const struct device *dev,
				   const enum display_orientation
				   orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int ssd16xx_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}

	LOG_ERR("not supported");
	return -ENOTSUP;
}

static int ssd16xx_clear_cntlr_mem(const struct device *dev, uint8_t ram_cmd,
				   bool update)
{
	const struct ssd16xx_config *config = dev->config;
	uint16_t panel_h = config->height / EPD_PANEL_NUMOF_ROWS_PER_PAGE;
	uint16_t last_gate = config->width - 1;
	uint8_t scan_mode = SSD16XX_DATA_ENTRY_XIYDY;
	uint8_t clear_page[64];
	int err;

	/*
	 * Clear unusable memory area when the resolution of the panel is not
	 * multiple of an octet.
	 */
	if (config->height % EPD_PANEL_NUMOF_ROWS_PER_PAGE) {
		panel_h += 1;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_ENTRY_MODE, &scan_mode, 1);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_param(dev, SSD16XX_PANEL_FIRST_PAGE,
				    panel_h - 1, last_gate,
				    SSD16XX_PANEL_FIRST_GATE);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_set_ram_ptr(dev, SSD16XX_PANEL_FIRST_PAGE, last_gate);
	if (err < 0) {
		return err;
	}

	memset(clear_page, 0xff, sizeof(clear_page));
	for (int h = 0; h < panel_h; h++) {
		size_t x = config->width;

		while (x) {
			size_t l = MIN(x, sizeof(clear_page));

			x -= l;
			err = ssd16xx_write_cmd(dev, ram_cmd, clear_page, l);
			if (err < 0) {
				return err;
			}
		}
	}

	if (update) {
		return ssd16xx_update_display(dev);
	}

	return 0;
}

static inline int ssd16xx_load_ws_from_otp_tssv(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	uint8_t tssv = config->tssv;
	int err;

	/*
	 * Controller has an integrated temperature sensor or external
	 * temperature sensor is connected to the controller.
	 */
	LOG_INF("Select and load WS from OTP");
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_TSENSOR_SELECTION, &tssv, 1);
	if (err == 0) {
		data->update_cmd |= SSD16XX_CTRL2_LOAD_LUT |
				    SSD16XX_CTRL2_LOAD_TEMPERATURE;
	}

	return err;
}

static inline int ssd16xx_load_ws_from_otp(const struct device *dev)
{
	int16_t t = (SSD16XX_DEFAULT_TR_VALUE * SSD16XX_TR_SCALE_FACTOR);
	struct ssd16xx_data *data = dev->data;
	uint8_t tmp[2];
	int err;

	LOG_INF("Load default WS (25 degrees Celsius) from OTP");

	tmp[0] = SSD16XX_CTRL2_ENABLE_CLK;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_CTRL2, tmp, 1);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_MASTER_ACTIVATION, NULL, 0);
	if (err < 0) {
		return err;
	}

	/* Load temperature value */
	sys_put_be16(t, tmp);
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_TSENS_CTRL, tmp, 2);
	if (err < 0) {
		return err;
	}

	tmp[0] = SSD16XX_CTRL2_DISABLE_CLK;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_CTRL2, tmp, 1);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_MASTER_ACTIVATION, NULL, 0);
	if (err < 0) {
		return err;
	}

	data->update_cmd |= SSD16XX_CTRL2_LOAD_LUT;

	return 0;
}

static int ssd16xx_load_ws_initial(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->lut_initial.len) {
		return ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_LUT,
					 config->lut_initial.data,
					 config->lut_initial.len);
	} else {
		if (config->tssv) {
			return ssd16xx_load_ws_from_otp_tssv(dev);
		} else {
			return ssd16xx_load_ws_from_otp(dev);
		}
	}
}

static int ssd16xx_load_ws_default(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;

	if (config->lut_default.len) {
		return ssd16xx_write_cmd(dev, SSD16XX_CMD_UPDATE_LUT,
					 config->lut_default.data,
					 config->lut_default.len);
	}

	return 0;
}


static int ssd16xx_controller_init(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	struct ssd16xx_data *data = dev->data;
	uint16_t last_gate = config->width - 1;
	int err;
	uint8_t tmp[3];
	size_t len;

	LOG_DBG("");

	data->blanking_on = false;

	err = gpio_pin_set_dt(&config->reset_gpio, 1);
	if (err < 0) {
		return err;
	}

	k_msleep(SSD16XX_RESET_DELAY);
	err = gpio_pin_set_dt(&config->reset_gpio, 0);
	if (err < 0) {
		return err;
	}

	k_msleep(SSD16XX_RESET_DELAY);

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SW_RESET, NULL, 0);
	if (err < 0) {
		return err;
	}

	len = push_y_param(dev, tmp, last_gate);
	tmp[len++] = 0U;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_GDO_CTRL, tmp, len);
	if (err < 0) {
		return err;
	}

	if (config->softstart.len) {
		err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SOFTSTART,
					config->softstart.data,
					config->softstart.len);
		if (err < 0) {
			return err;
		}
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_GDV_CTRL, config->gdv.data,
				config->gdv.len);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_SDV_CTRL, config->sdv.data,
				config->sdv.len);
	if (err < 0) {
		return err;
	}

	tmp[0] = config->vcom;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_VCOM_VOLTAGE, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = SSD16XX_VAL_DUMMY_LINE;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_DUMMY_LINE, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = SSD16XX_VAL_GATE_LWIDTH;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_GATE_LINE_WIDTH, tmp, 1);
	if (err < 0) {
		return err;
	}

	tmp[0] = config->b_waveform;
	err = ssd16xx_write_cmd(dev, SSD16XX_CMD_BWF_CTRL, tmp, 1);
	if (err < 0) {
		return err;
	}

	if (config->orientation == 1) {
		data->scan_mode = SSD16XX_DATA_ENTRY_XIYDY;
	} else {
		data->scan_mode = SSD16XX_DATA_ENTRY_XDYIY;
	}

	data->update_cmd = (SSD16XX_CTRL2_ENABLE_CLK |
			      SSD16XX_CTRL2_ENABLE_ANALOG |
			      SSD16XX_CTRL2_TO_PATTERN |
			      SSD16XX_CTRL2_DISABLE_ANALOG |
			      SSD16XX_CTRL2_DISABLE_CLK);

	err = ssd16xx_load_ws_initial(dev);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RAM, true);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RED_RAM,
					     false);
	if (err < 0) {
		return err;
	}

	err = ssd16xx_load_ws_default(dev);
	if (err < 0) {
		return err;
	}

	return ssd16xx_clear_cntlr_mem(dev, SSD16XX_CMD_WRITE_RAM, true);
}

static int ssd16xx_init(const struct device *dev)
{
	const struct ssd16xx_config *config = dev->config;
	int err;

	LOG_DBG("");

	if (!spi_is_ready(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->reset_gpio.port)) {
		LOG_ERR("Reset GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure reset GPIO");
		return err;
	}

	if (!device_is_ready(config->dc_gpio.port)) {
		LOG_ERR("DC GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);
	if (err < 0) {
		LOG_ERR("Failed to configure DC GPIO");
		return err;
	}

	if (!device_is_ready(config->busy_gpio.port)) {
		LOG_ERR("Busy GPIO device not ready");
		return -ENODEV;
	}

	err = gpio_pin_configure_dt(&config->busy_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure busy GPIO");
		return err;
	}

	return ssd16xx_controller_init(dev);
}

static struct display_driver_api ssd16xx_driver_api = {
	.blanking_on = ssd16xx_blanking_on,
	.blanking_off = ssd16xx_blanking_off,
	.write = ssd16xx_write,
	.read = ssd16xx_read,
	.get_framebuffer = ssd16xx_get_framebuffer,
	.set_brightness = ssd16xx_set_brightness,
	.set_contrast = ssd16xx_set_contrast,
	.get_capabilities = ssd16xx_get_capabilities,
	.set_pixel_format = ssd16xx_set_pixel_format,
	.set_orientation = ssd16xx_set_orientation,
};

#define LUT_INITIAL_DEFINE(n)						\
	static uint8_t lut_initial_##n[] = DT_INST_PROP(n, lut_initial);
#define LUT_DEFAULT_DEFINE(n)						\
	static uint8_t lut_default_##n[] = DT_INST_PROP(n, lut_default);
#define SOFTSTART_DEFINE(n)						\
	static uint8_t softstart_##n[] = DT_INST_PROP(n, softstart);

#define LUT_INITIAL_ASSIGN(n)						\
		.lut_initial = {					\
			.data = lut_initial_##n,			\
			.len = sizeof(lut_initial_##n),			\
		},

#define LUT_DEFAULT_ASSIGN(n)						\
		.lut_default = {					\
			.data = lut_default_##n,			\
			.len = sizeof(lut_default_##n),			\
		},

#define SOFTSTART_ASSIGN(n)						\
		.softstart = {						\
			.data = softstart_##n,				\
			.len = sizeof(softstart_##n),			\
		},

#define CMD_ARRAY_DEFINE(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_initial),		\
		    (LUT_INITIAL_DEFINE(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_default),		\
		    (LUT_DEFAULT_DEFINE(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, softstart),		\
		    (SOFTSTART_DEFINE(n)),				\
		    ())

#define CMD_ARRAY_COND(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_initial),		\
		    (LUT_INITIAL_ASSIGN(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, lut_default),		\
		    (LUT_DEFAULT_ASSIGN(n)),				\
		    ())							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, softstart),		\
		    (SOFTSTART_ASSIGN(n)),				\
		    ())

#define SSD16XX_DEFINE(n)						\
	static uint8_t gdv_##n[] = DT_INST_PROP(n, gdv);		\
	static uint8_t sdv_##n[] = DT_INST_PROP(n, sdv);		\
									\
	CMD_ARRAY_DEFINE(n)						\
									\
	static const struct ssd16xx_config ssd16xx_cfg_##n = {		\
		.bus = SPI_DT_SPEC_INST_GET(n,				\
			SPI_OP_MODE_MASTER | SPI_WORD_SET(8) |		\
			SPI_HOLD_ON_CS | SPI_LOCK_ON,			\
			0),						\
		.reset_gpio = GPIO_DT_SPEC_INST_GET(n, reset_gpios),	\
		.dc_gpio = GPIO_DT_SPEC_INST_GET(n, dc_gpios),		\
		.busy_gpio = GPIO_DT_SPEC_INST_GET(n, busy_gpios),	\
		.height = DT_INST_PROP(n, height),			\
		.width = DT_INST_PROP(n, width),			\
		.orientation = DT_INST_PROP(n, orientation_flipped),	\
		.vcom = DT_INST_PROP(n, vcom),				\
		.pp_width_bits = DT_INST_PROP(n, pp_width_bits),	\
		.pp_height_bits = DT_INST_PROP(n, pp_height_bits),	\
		.b_waveform = DT_INST_PROP(n, border_waveform),		\
		.gdv = {						\
			.data = gdv_##n,				\
			.len = sizeof(gdv_##n),				\
		},							\
		.sdv = {						\
			.data = sdv_##n,				\
			.len = sizeof(sdv_##n),				\
		},							\
		.tssv = DT_INST_PROP_OR(n, tssv, 0),			\
		CMD_ARRAY_COND(n)					\
	};								\
									\
	static struct ssd16xx_data ssd16xx_data_##n;			\
									\
	DEVICE_DT_INST_DEFINE(n, ssd16xx_init, NULL,			\
			      &ssd16xx_data_##n,			\
			      &ssd16xx_cfg_##n,				\
			      POST_KERNEL,				\
			      CONFIG_DISPLAY_INIT_PRIORITY,		\
			      &ssd16xx_driver_api);			\


DT_INST_FOREACH_STATUS_OKAY(SSD16XX_DEFINE)
