/*
 * Copyright (c) 2025 Giyora Haroon
 *
 * This controller supports 8-bit parallel and 4-line SPI interfaces.
 * This implementation covers the SPI interface.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(st7565r, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>

#include "display_st7565r.h"

/* Default contrast (midrange) */
#define ST7565R_DEFAULT_CONTRAST 0x1F

/**
 * Default power control: Booster, Regulator, Follower ON.
 * In this setting, only the internal power supply is used. Refer to Table 8 in
 * the 'The Power Supply Circuits' section.
 */
#define ST7565R_DEFAULT_POWER_CONTROL_VAL                                                          \
	(ST7565R_POWER_CONTROL_VB_MASK | ST7565R_POWER_CONTROL_VR_MASK |                           \
	 ST7565R_POWER_CONTROL_VF_MASK)

/* Default resistor ratio: 4 (adjust based on display characteristics) */
#define ST7565R_DEFAULT_RESISTOR_RATIO 4

/* Default booster ratio */
#define ST7565R_DEFAULT_BOOSTER_RATIO ST7565R_SET_BOOSTER_RATIO_2X_3X_4X

/**
 *  Default Time in milliseconds to wait before initializing the
 *  controller after power-on.
 */
#define ST7565R_DEFAULT_READY_TIME_MS 10

/* Function pointer types */
typedef bool (*st7565r_bus_ready_fn)(const struct device *dev);
typedef int (*st7565r_write_bus_fn)(const struct device *dev, uint8_t *buf, size_t len,
				    bool command);
typedef const char *(*st7565r_bus_name_fn)(const struct device *dev);

/* Configuration struct */
struct st7565r_config {
	struct spi_dt_spec bus;
	struct gpio_dt_spec data_cmd;
	struct gpio_dt_spec reset;
	st7565r_bus_ready_fn bus_ready;
	st7565r_write_bus_fn write_bus;
	st7565r_bus_name_fn bus_name;
	uint16_t height;
	uint16_t width;
	uint8_t segment_offset;
	uint8_t lcd_bias;          /* 7 or 9 */
	uint8_t power_control_val; /* 3-bit mask for power circuits */
	uint8_t resistor_ratio;    /* 0-7 */
	uint8_t booster_ratio;     /* 0x00, 0x01, or 0x03 */
	bool segment_remap;
	bool com_invdir;
	bool color_inversion;
	int ready_time_ms;
};

/* Runtime data struct */
struct st7565r_data {
	enum display_pixel_format pf;
};

/* SPI Bus implementation */
static bool st7565r_bus_ready_spi(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;

	/* Check data_cmd pin only if it's specified in DT */
	if (config->data_cmd.port && !gpio_is_ready_dt(&config->data_cmd)) {
		LOG_ERR("Data/Command GPIO %s not ready!", config->data_cmd.port->name);
		return false;
	}

	/* Configure data_cmd pin if specified */
	if (config->data_cmd.port &&
	    gpio_pin_configure_dt(&config->data_cmd, GPIO_OUTPUT_INACTIVE) < 0) {
		LOG_ERR("Could not configure Data/Command GPIO!");
		return false;
	}

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("SPI bus %s not ready", config->bus.bus->name);
		return false;
	}

	return true;
}

static int st7565r_write_bus_spi(const struct device *dev, uint8_t *buf, size_t len, bool command)
{
	const struct st7565r_config *config = dev->config;
	int ret;

	/* Set D/C pin based on command/data */
	if (config->data_cmd.port) {
		gpio_pin_set_dt(&config->data_cmd, command ? 0 : 1);
	} else {
		/* Needs D/C pin for standard 4-wire SPI */
		LOG_ERR("Data/Command GPIO not specified for SPI!");
		return -ENODEV;
	}

	const struct spi_buf tx_buf = {.buf = buf, .len = len};

	const struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	ret = spi_write_dt(&config->bus, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("SPI write failed: %d", ret);
	}

	return ret;
}

static const char *st7565r_bus_name_spi(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;

	return config->bus.bus->name;
}

/* Generic bus functions */
static inline bool st7565r_bus_ready(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;

	return config->bus_ready(dev);
}

static inline int st7565r_write_bus(const struct device *dev, uint8_t *buf, size_t len,
				    bool command)
{
	const struct st7565r_config *config = dev->config;

	return config->write_bus(dev, buf, len, command);
}

/* Helper to send a single command byte */
static int st7565r_send_cmd(const struct device *dev, uint8_t cmd)
{
	return st7565r_write_bus(dev, &cmd, 1, true);
}

/* Helper to send a command byte followed by a data byte */
static int st7565r_send_cmd_data(const struct device *dev, uint8_t cmd, uint8_t data)
{
	uint8_t buf[2] = {cmd, data};
	return st7565r_write_bus(dev, buf, 2, true);
}

/* Helper to clear the display's RAM */
static int st7565r_clear_ram(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;
	uint8_t zero_buf[config->width];
	memset(zero_buf, 0, sizeof(zero_buf));

	for (uint8_t page = 0; page < (config->height / 8); page++) {
		uint8_t page_cmd = ST7565R_SET_PAGE_START_ADDRESS_CMD |
				   (page & ST7565R_SET_PAGE_START_ADDRESS_VAL_MASK);
		if (st7565r_send_cmd(dev, page_cmd) != 0) {
			LOG_ERR("Failed to set page address");
			return -EIO;
		}

		uint8_t col_cmds[2] = {ST7565R_SET_HIGHER_COL_ADDRESS_CMD,
				       ST7565R_SET_LOWER_COL_ADDRESS_CMD};
		if (st7565r_write_bus(dev, col_cmds, sizeof(col_cmds), true) != 0) {
			LOG_ERR("Failed to set column address");
			return -EIO;
		}

		if (st7565r_write_bus(dev, zero_buf, config->width, false) != 0) {
			LOG_ERR("Failed to clear page %d", page);
			return -EIO;
		}
	}

	return 0;
}

/******************************************************************************/
/*                      Display API implementations                           */
/******************************************************************************/

/* Turn LCD on (restore the frame buffer content to the display) */
static int st7565r_resume(const struct device *dev)
{
	return st7565r_send_cmd(dev, ST7565R_DISPLAY_ON);
}

/**
 * Turn LCD off.
 * This function blanks the complete display. The content of the frame
 * buffer will be retained while blanking is enabled.
 */
static int st7565r_suspend(const struct device *dev)
{
	return st7565r_send_cmd(dev, ST7565R_DISPLAY_OFF);
}

static int st7565r_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc, const void *buf)
{
	const struct st7565r_config *config = dev->config;
	size_t buf_len;
	uint8_t x_offset = x + config->segment_offset;
	uint8_t *buf_ptr = (uint8_t *)buf;
	uint8_t cmd_buf[2];

	if (desc == NULL || buf == NULL) {
		LOG_ERR("Display buffer is not available");
		return -EINVAL;
	}

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller than width");
		return -EINVAL;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf_len == 0U) {
		LOG_ERR("Display buffer is empty");
		return -EINVAL;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode: pitch > width");
		return -ENOTSUP;
	}

	if (x + desc->width > config->width) {
		LOG_ERR("Buffer width out of bounds: %u + %u > %u", x, desc->width, config->width);
		return -EINVAL;
	}

	if (y + desc->height > config->height) {
		LOG_ERR("Buffer height out of bounds: %u + %u > %u", y, desc->height,
			config->height);
		return -EINVAL;
	}

	if ((y % 8) != 0U) {
		LOG_ERR("Y coordinate must be page-aligned (multiple of 8)");
		return -EINVAL;
	}
	if ((desc->height % 8) != 0U) {
		LOG_ERR("Buffer height must be page-aligned (multiple of 8)");
		return -EINVAL;
	}

	uint8_t start_page = y / 8;
	uint8_t end_page = (y + desc->height - 1) / 8;

	for (uint8_t page = start_page; page <= end_page; page++) {
		uint8_t current_col = x_offset;
		uint8_t page_cmd = ST7565R_SET_PAGE_START_ADDRESS_CMD |
				   (page & ST7565R_SET_PAGE_START_ADDRESS_VAL_MASK);

		/* Set the page address */
		if (st7565r_send_cmd(dev, page_cmd) != 0) {
			LOG_ERR("Failed to set page address");
			return -EIO;
		}

		/* Set column address high nibble */
		cmd_buf[0] = ST7565R_SET_HIGHER_COL_ADDRESS_CMD |
			     ((current_col >> 4) & ST7565R_SET_HIGHER_COL_ADDRESS_VAL_MASK);
		/* Set column address low nibble */
		cmd_buf[1] = ST7565R_SET_LOWER_COL_ADDRESS_CMD |
			     (current_col & ST7565R_SET_LOWER_COL_ADDRESS_VAL_MASK);

		if (st7565r_write_bus(dev, cmd_buf, 2, true) != 0) {
			LOG_ERR("Failed to set column address");
			return -EIO;
		}

		/* Write data for this page */
		if (st7565r_write_bus(dev, buf_ptr, desc->width, false) != 0) {
			LOG_ERR("Failed to write data");
			return -EIO;
		}

		/* Advance buffer pointer to the next page (assumes pitch == width) */
		buf_ptr += desc->width;
		if (buf_ptr > ((uint8_t *)buf + buf_len)) {
			LOG_ERR("Exceeded buffer length");
			return -EIO;
		}
	}

	return 0;
}

static int st7565r_set_contrast(const struct device *dev, const uint8_t contrast)
{
	uint8_t val = contrast & ST7565R_SET_CONTRAST_VALUE_MASK;
	return st7565r_send_cmd_data(dev, ST7565R_SET_CONTRAST_CTRL_CMD, val);
}

static void st7565r_get_capabilities(const struct device *dev, struct display_capabilities *caps)
{
	const struct st7565r_config *config = dev->config;
	struct st7565r_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10 | PIXEL_FORMAT_MONO01;
	caps->current_pixel_format = data->pf;
	/* Screen info: Vertical byte order (pages), Tiled */
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
	caps->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7565r_set_pixel_format(const struct device *dev, const enum display_pixel_format pf)
{
	struct st7565r_data *data = dev->data;
	uint8_t cmd;
	int ret;

	if (pf == data->pf) {
		return 0;
	}

	if (pf == PIXEL_FORMAT_MONO10) { /* 1=black, 0=white */
		cmd = ST7565R_SET_REVERSE_DISPLAY;
	} else if (pf == PIXEL_FORMAT_MONO01) { /* 0=black, 1=white */
		cmd = ST7565R_SET_NORMAL_DISPLAY;
	} else {
		LOG_WRN("Unsupported pixel format: 0x%x", pf);
		return -ENOTSUP;
	}

	ret = st7565r_send_cmd(dev, cmd);
	if (ret) {
		LOG_ERR("Failed to set pixel format: %d", ret);
		return ret;
	}

	data->pf = pf;

	return 0;
}

/* See datasheet - Instruction Setup: Reference  */
static int st7565r_init_device(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;
	struct st7565r_data *data = dev->data;
	int ret;

	/* Follow initialization sequence from datasheet (Page 51) */

	/* 1. Reset if pin connected */
	if (config->reset.port) {
		gpio_pin_set_dt(&config->reset, 1);
		k_sleep(K_MSEC(ST7565R_RESET_DELAY)); /* 1ms */

		gpio_pin_set_dt(&config->reset, 0);
		k_sleep(K_MSEC(ST7565R_RESET_DELAY)); /* 1ms */
	}

	/* 2. Set LCD Bias */
	ret = st7565r_send_cmd(dev, (config->lcd_bias == 9) ? ST7565R_SET_LCD_BIAS_9
							    : ST7565R_SET_LCD_BIAS_7);
	if (ret != 0) {
		LOG_ERR("Failed to set LCD Bias: %d", ret);
		return ret;
	}

	/* 3. Set ADC / Segment Remap */
	ret = st7565r_send_cmd(dev, config->segment_remap ? ST7565R_SET_SEGMENT_MAP_REVERSED
							  : ST7565R_SET_SEGMENT_MAP_NORMAL);
	if (ret != 0) {
		LOG_ERR("Failed to set segment map: %d", ret);
		return ret;
	}

	/* 4. Set COM Scan Direction */
	ret = st7565r_send_cmd(dev, config->com_invdir ? ST7565R_SET_COM_OUTPUT_SCAN_REVERSED
						       : ST7565R_SET_COM_OUTPUT_SCAN_NORMAL);
	if (ret != 0) {
		LOG_ERR("Failed to set COM scan direction: %d", ret);
		return ret;
	}

	/* 5. Set Resistor Ratio */
	uint8_t rr_cmd = ST7565R_SET_RESISTOR_RATIO_CMD |
			 (config->resistor_ratio & ST7565R_SET_RESISTOR_RATIO_VAL_MASK);
	ret = st7565r_send_cmd(dev, rr_cmd);
	if (ret != 0) {
		LOG_ERR("Failed to set resistor ratio: %d", ret);
		return ret;
	}

	/* 6. Set Electronic Volume (contrast; can be set externally using the API) */
	ret = st7565r_set_contrast(dev, ST7565R_DEFAULT_CONTRAST);
	if (ret != 0) {
		LOG_ERR("Failed to set default contrast: %d", ret);
		return ret;
	}

	/* 7. Set Power Control - enable circuits */
	uint8_t pc_cmd = ST7565R_POWER_CONTROL_CMD |
			 (config->power_control_val & ST7565R_POWER_CONTROL_ALL_ON_MASK);
	ret = st7565r_send_cmd(dev, pc_cmd);
	if (ret != 0) {
		LOG_ERR("Failed to set power control: %d", ret);
		return ret;
	}

	/* Set Booster Ratio */
	ret = st7565r_send_cmd_data(dev, ST7565R_SET_BOOSTER_RATIO_SET_CMD, config->booster_ratio);
	if (ret != 0) {
		LOG_ERR("Failed to set booster ratio: %d", ret);
		return ret;
	}

	/* Set Initial Pixel Format (can be set externally using the API) */
	data->pf = config->color_inversion ? PIXEL_FORMAT_MONO10 : PIXEL_FORMAT_MONO01;
	ret = st7565r_send_cmd(dev, config->color_inversion ? ST7565R_SET_REVERSE_DISPLAY
							    : ST7565R_SET_NORMAL_DISPLAY);
	if (ret != 0) {
		LOG_ERR("Failed to set initial display mode: %d", ret);
		return ret;
	}

	/* Initialize DDRAM (Pages 0 - 7) */
	ret = st7565r_clear_ram(dev);
	if (ret != 0) {
		LOG_ERR("Failed to clear display RAM: %d", ret);
		return ret;
	}

	return 0;
}

static int st7565r_init(const struct device *dev)
{
	const struct st7565r_config *config = dev->config;

	/* Specify timeout as an absolute time since system boot */
	k_sleep(K_TIMEOUT_ABS_MS(config->ready_time_ms));

	if (!st7565r_bus_ready(dev)) {
		LOG_ERR("Bus device %s not ready!", config->bus_name(dev));
		return -ENODEV;
	}

	if (config->reset.port) {
		if (!gpio_is_ready_dt(&config->reset)) {
			LOG_ERR("Reset GPIO %s not ready!", config->reset.port->name);
			return -ENODEV;
		}
		int ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO!");
			return ret;
		}
	}

	if (st7565r_init_device(dev) != 0) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}

	return 0;
}

static const struct display_driver_api st7565r_driver_api = {
	.blanking_on = st7565r_suspend,
	.blanking_off = st7565r_resume,
	.write = st7565r_write,
	.set_contrast = st7565r_set_contrast,
	.get_capabilities = st7565r_get_capabilities,
	.set_pixel_format = st7565r_set_pixel_format,
};

#define DT_DRV_COMPAT sitronix_st7565r

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "ST7565R display driver enabled without any devices"
#endif

/* Zephyr Device Instantiation Macro */
#define ST7565R_DEFINE(node_id)                                                                    \
	static struct st7565r_data data_##node_id;                                                 \
	static const struct st7565r_config config_##node_id = {                                    \
		.bus = SPI_DT_SPEC_GET(                                                            \
			node_id, SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0),      \
		.bus_ready = st7565r_bus_ready_spi,                                                \
		.write_bus = st7565r_write_bus_spi,                                                \
		.bus_name = st7565r_bus_name_spi,                                                  \
		.data_cmd = GPIO_DT_SPEC_GET(node_id, data_cmd_gpios),                             \
		.reset = GPIO_DT_SPEC_GET_OR(node_id, reset_gpios, {0}),                           \
		.height = DT_PROP(node_id, height),                                                \
		.width = DT_PROP(node_id, width),                                                  \
		.segment_offset = DT_PROP_OR(node_id, segment_offset, 0),                          \
		.lcd_bias = DT_PROP(node_id, lcd_bias),                                            \
		.power_control_val =                                                               \
			DT_PROP_OR(node_id, power_control_val, ST7565R_DEFAULT_POWER_CONTROL_VAL), \
		.resistor_ratio =                                                                  \
			DT_PROP_OR(node_id, resistor_ratio, ST7565R_DEFAULT_RESISTOR_RATIO),       \
		.booster_ratio =                                                                   \
			DT_PROP_OR(node_id, booster_ratio, ST7565R_DEFAULT_BOOSTER_RATIO),         \
		.segment_remap = DT_PROP(node_id, segment_remap),                                  \
		.com_invdir = DT_PROP(node_id, com_invdir),                                        \
		.color_inversion = DT_PROP_OR(node_id, inversion_on, false),                       \
		.ready_time_ms =                                                                   \
			DT_PROP_OR(node_id, ready_time_ms, ST7565R_DEFAULT_READY_TIME_MS),         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(node_id, st7565r_init, NULL, &data_##node_id, &config_##node_id,          \
			 POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY, &st7565r_driver_api);

DT_FOREACH_STATUS_OKAY(sitronix_st7565r, ST7565R_DEFINE)
