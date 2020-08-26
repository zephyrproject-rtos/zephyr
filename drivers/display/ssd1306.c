/*
 * Copyright (c) 2018 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT solomon_ssd1306fb

#include <logging/log.h>
LOG_MODULE_REGISTER(ssd1306, CONFIG_DISPLAY_LOG_LEVEL);

#include <string.h>
#include <device.h>
#include <init.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>

#include "ssd1306_regs.h"
#include <display/cfb.h>

#if DT_INST_PROP(0, segment_remap) == 1
#define SSD1306_PANEL_SEGMENT_REMAP	true
#else
#define SSD1306_PANEL_SEGMENT_REMAP	false
#endif

#if DT_INST_PROP(0, com_invdir) == 1
#define SSD1306_PANEL_COM_INVDIR	true
#else
#define SSD1306_PANEL_COM_INVDIR	false
#endif

#if DT_INST_PROP(0, com_sequential) == 1
#define SSD1306_COM_PINS_HW_CONFIG	SSD1306_SET_PADS_HW_SEQUENTIAL
#else
#define SSD1306_COM_PINS_HW_CONFIG	SSD1306_SET_PADS_HW_ALTERNATIVE
#endif

#define SSD1306_PANEL_NUMOF_PAGES	(DT_INST_PROP(0, height) / 8)
#define SSD1306_CLOCK_DIV_RATIO		0x0
#define SSD1306_CLOCK_FREQUENCY		0x8
#define SSD1306_PANEL_VCOM_DESEL_LEVEL	0x20
#define SSD1306_PANEL_PUMP_VOLTAGE	SSD1306_SET_PUMP_VOLTAGE_90

#ifndef SSD1306_ADDRESSING_MODE
#define SSD1306_ADDRESSING_MODE		(SSD1306_SET_MEM_ADDRESSING_HORIZONTAL)
#endif

struct ssd1306_data {
	struct device *reset;
	struct device *i2c;
	uint8_t contrast;
	uint8_t scan_mode;
};

static inline int ssd1306_reg_read(struct ssd1306_data *driver,
				   uint8_t reg, uint8_t * const val)
{
	return i2c_reg_read_byte(driver->i2c,
				 DT_INST_REG_ADDR(0),
				 reg, val);
}

static inline int ssd1306_reg_write(struct ssd1306_data *driver,
				    uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte(driver->i2c,
				  DT_INST_REG_ADDR(0),
				  reg, val);
}

static inline int ssd1306_reg_update(struct ssd1306_data *driver, uint8_t reg,
				     uint8_t mask, uint8_t val)
{
	return i2c_reg_update_byte(driver->i2c,
				   DT_INST_REG_ADDR(0),
				   reg, mask, val);
}

static inline int ssd1306_set_panel_orientation(struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		(SSD1306_PANEL_SEGMENT_REMAP ?
		 SSD1306_SET_SEGMENT_MAP_REMAPED :
		 SSD1306_SET_SEGMENT_MAP_NORMAL),
		SSD1306_CONTROL_LAST_BYTE_CMD,
		(SSD1306_PANEL_COM_INVDIR ?
		 SSD1306_SET_COM_OUTPUT_SCAN_FLIPPED :
		 SSD1306_SET_COM_OUTPUT_SCAN_NORMAL)
	};

	return i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			 DT_INST_REG_ADDR(0));
}

static inline int ssd1306_set_timing_setting(struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_CLOCK_DIV_RATIO,
		SSD1306_CONTROL_BYTE_CMD,
		(SSD1306_CLOCK_FREQUENCY << 4) | SSD1306_CLOCK_DIV_RATIO,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_CHARGE_PERIOD,
		SSD1306_CONTROL_BYTE_CMD,
		DT_INST_PROP(0, prechargep),
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_VCOM_DESELECT_LEVEL,
		SSD1306_CONTROL_LAST_BYTE_CMD,
		SSD1306_PANEL_VCOM_DESEL_LEVEL
	};

	return i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			 DT_INST_REG_ADDR(0));
}

static inline int ssd1306_set_hardware_config(struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_START_LINE,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_DISPLAY_OFFSET,
		SSD1306_CONTROL_BYTE_CMD,
		DT_INST_PROP(0, display_offset),
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_PADS_HW_CONFIG,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_COM_PINS_HW_CONFIG,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_MULTIPLEX_RATIO,
		SSD1306_CONTROL_LAST_BYTE_CMD,
		DT_INST_PROP(0, multiplex_ratio)
	};

	return i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			 DT_INST_REG_ADDR(0));
}

static inline int ssd1306_set_charge_pump(const struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	uint8_t cmd_buf[] = {
#if defined(CONFIG_SSD1306_DEFAULT)
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_CHARGE_PUMP_ON,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_CHARGE_PUMP_ON_ENABLED,
#endif
#if defined(CONFIG_SSD1306_SH1106_COMPATIBLE)
		SSD1306_CONTROL_BYTE_CMD,
		SH1106_SET_DCDC_MODE,
		SSD1306_CONTROL_BYTE_CMD,
		SH1106_SET_DCDC_ENABLED,
#endif
		SSD1306_CONTROL_LAST_BYTE_CMD,
		SSD1306_PANEL_PUMP_VOLTAGE,
	};

	return i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			 DT_INST_REG_ADDR(0));
}

static int ssd1306_resume(const struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	/* set display on */
	return ssd1306_reg_write(driver, SSD1306_CONTROL_LAST_BYTE_CMD,
				 SSD1306_DISPLAY_ON);
}

static int ssd1306_suspend(const struct device *dev)
{
	struct ssd1306_data *driver = dev->data;
	/* set display on */
	return ssd1306_reg_write(driver, SSD1306_CONTROL_LAST_BYTE_CMD,
				 SSD1306_DISPLAY_OFF);
}

static int ssd1306_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ssd1306_data *driver = dev->data;
	size_t buf_len;

	if (desc->pitch < desc->width) {
		LOG_ERR("Pitch is smaller then width");
		return -1;
	}

	buf_len = MIN(desc->buf_size, desc->height * desc->width / 8);
	if (buf == NULL || buf_len == 0U) {
		LOG_ERR("Display buffer is not available");
		return -1;
	}

	if (desc->pitch > desc->width) {
		LOG_ERR("Unsupported mode");
		return -1;
	}

	if ((y & 0x7) != 0U) {
		LOG_ERR("Unsupported origin");
		return -1;
	}

	LOG_DBG("x %u, y %u, pitch %u, width %u, height %u, buf_len %u",
		x, y, desc->pitch, desc->width, desc->height, buf_len);

#if defined(CONFIG_SSD1306_DEFAULT)
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_MEM_ADDRESSING_MODE,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_ADDRESSING_MODE,
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_COLUMN_ADDRESS,
		SSD1306_CONTROL_BYTE_CMD,
		x,
		SSD1306_CONTROL_BYTE_CMD,
		(x + desc->width - 1),
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_PAGE_ADDRESS,
		SSD1306_CONTROL_BYTE_CMD,
		y/8,
		SSD1306_CONTROL_LAST_BYTE_CMD,
		((y + desc->height)/8 - 1)
	};

	if (i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
		      DT_INST_REG_ADDR(0))) {
		LOG_ERR("Failed to write command");
		return -1;
	}

	return i2c_burst_write(driver->i2c,
			       DT_INST_REG_ADDR(0),
			       SSD1306_CONTROL_LAST_BYTE_DATA,
			       (uint8_t *)buf, buf_len);

#elif defined(CONFIG_SSD1306_SH1106_COMPATIBLE)
	uint8_t x_offset = x + DT_INST_PROP(0, segment_offset);
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_LOWER_COL_ADDRESS |
			(x_offset & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_HIGHER_COL_ADDRESS |
			((x_offset >> 4) & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_CONTROL_LAST_BYTE_CMD,
		SSD1306_SET_PAGE_START_ADDRESS | (y / 8)
	};
	uint8_t *buf_ptr = (uint8_t *)buf;

	for (uint8_t n = 0; n < desc->height / 8; n++) {
		cmd_buf[sizeof(cmd_buf) - 1] =
			SSD1306_SET_PAGE_START_ADDRESS | (n + (y / 8));
		LOG_HEXDUMP_DBG(cmd_buf, sizeof(cmd_buf), "cmd_buf");

		if (i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			      DT_INST_REG_ADDR(0))) {
			return -1;
		}

		if (i2c_burst_write(driver->i2c,
				    DT_INST_REG_ADDR(0),
				    SSD1306_CONTROL_LAST_BYTE_DATA,
				    buf_ptr, desc->width)) {
			return -1;
		}

		buf_ptr = buf_ptr + desc->width;
		if (buf_ptr > ((uint8_t *)buf + buf_len)) {
			LOG_ERR("Exceeded buffer length");
			return -1;
		}
	}
#endif

	return 0;
}

static int ssd1306_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static void *ssd1306_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Unsupported");
	return NULL;
}

static int ssd1306_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_WRN("Unsupported");
	return -ENOTSUP;
}

static int ssd1306_set_contrast(const struct device *dev, const uint8_t contrast)
{
	struct ssd1306_data *driver = dev->data;
	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_CONTRAST_CTRL,
		SSD1306_CONTROL_LAST_BYTE_CMD,
		contrast,
	};

	return i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
			 DT_INST_REG_ADDR(0));
}

static void ssd1306_get_capabilities(const struct device *dev,
				     struct display_capabilities *caps)
{
	memset(caps, 0, sizeof(struct display_capabilities));
	caps->x_resolution = DT_INST_PROP(0, width);
	caps->y_resolution = DT_INST_PROP(0, height);
	caps->supported_pixel_formats = PIXEL_FORMAT_MONO10;
	caps->current_pixel_format = PIXEL_FORMAT_MONO10;
	caps->screen_info = SCREEN_INFO_MONO_VTILED;
}

static int ssd1306_set_orientation(const struct device *dev,
				   const enum display_orientation
				   orientation)
{
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int ssd1306_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pf)
{
	if (pf == PIXEL_FORMAT_MONO10) {
		return 0;
	}
	LOG_ERR("Unsupported");
	return -ENOTSUP;
}

static int ssd1306_init_device(struct device *dev)
{
	struct ssd1306_data *driver = dev->data;

	uint8_t cmd_buf[] = {
		SSD1306_CONTROL_BYTE_CMD,
		SSD1306_SET_ENTIRE_DISPLAY_OFF,
		SSD1306_CONTROL_LAST_BYTE_CMD,
#ifdef CONFIG_SSD1306_REVERSE_MODE
		SSD1306_SET_REVERSE_DISPLAY,
#else
		SSD1306_SET_NORMAL_DISPLAY,
#endif
	};

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	k_sleep(SSD1306_RESET_DELAY);
	gpio_pin_set(driver->reset,
		     DT_INST_GPIO_PIN(0, reset_gpios), 1);
	k_sleep(SSD1306_RESET_DELAY);
	gpio_pin_set(driver->reset,
		     DT_INST_GPIO_PIN(0, reset_gpios), 0);
#endif

	/* Turn display off */
	if (ssd1306_reg_write(driver, SSD1306_CONTROL_LAST_BYTE_CMD,
			      SSD1306_DISPLAY_OFF)) {
		return -EIO;
	}

	if (ssd1306_set_timing_setting(dev)) {
		return -EIO;
	}

	if (ssd1306_set_hardware_config(dev)) {
		return -EIO;
	}

	if (ssd1306_set_panel_orientation(dev)) {
		return -EIO;
	}

	if (ssd1306_set_charge_pump(dev)) {
		return -EIO;
	}

	if (i2c_write(driver->i2c, cmd_buf, sizeof(cmd_buf),
		      DT_INST_REG_ADDR(0))) {
		return -EIO;
	}

	if (ssd1306_set_contrast(dev, CONFIG_SSD1306_DEFAULT_CONTRAST)) {
		return -EIO;
	}

	ssd1306_resume(dev);

	return 0;
}

static int ssd1306_init(struct device *dev)
{
	struct ssd1306_data *driver = dev->data;

	LOG_DBG("");

	driver->i2c = device_get_binding(DT_INST_BUS_LABEL(0));
	if (driver->i2c == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	driver->reset = device_get_binding(
			DT_INST_GPIO_LABEL(0, reset_gpios));
	if (driver->reset == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
			    DT_INST_GPIO_LABEL(0, reset_gpios));
		return -EINVAL;
	}

	gpio_pin_configure(driver->reset,
			   DT_INST_GPIO_PIN(0, reset_gpios),
			   GPIO_OUTPUT_INACTIVE |
			   DT_INST_GPIO_FLAGS(0, reset_gpios));
#endif

	if (ssd1306_init_device(dev)) {
		LOG_ERR("Failed to initialize device!");
		return -EIO;
	}

	return 0;
}

static struct ssd1306_data ssd1306_driver;

static struct display_driver_api ssd1306_driver_api = {
	.blanking_on = ssd1306_suspend,
	.blanking_off = ssd1306_resume,
	.write = ssd1306_write,
	.read = ssd1306_read,
	.get_framebuffer = ssd1306_get_framebuffer,
	.set_brightness = ssd1306_set_brightness,
	.set_contrast = ssd1306_set_contrast,
	.get_capabilities = ssd1306_get_capabilities,
	.set_pixel_format = ssd1306_set_pixel_format,
	.set_orientation = ssd1306_set_orientation,
};

DEVICE_AND_API_INIT(ssd1306, DT_INST_LABEL(0), ssd1306_init,
		    &ssd1306_driver, NULL,
		    POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
		    &ssd1306_driver_api);
