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
#include <drivers/spi.h>

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
	const struct device *reset;
	const struct device *bus;
#if DT_INST_ON_BUS(0, spi)
	struct spi_cs_control cs_ctrl;
	struct spi_config spi_config;
	const struct device *data_cmd;
#endif
	uint8_t contrast;
	uint8_t scan_mode;
};

#if DT_INST_ON_BUS(0, i2c)
static inline int ssd1306_write_bus(const struct device *dev,
				    uint8_t *buf, size_t len, bool command)
{
	struct ssd1306_data *driver = dev->data;

	return i2c_burst_write(driver->bus, DT_INST_REG_ADDR(0),
			       command ? SSD1306_CONTROL_ALL_BYTES_CMD :
					 SSD1306_CONTROL_ALL_BYTES_DATA,
			       buf, len);
}

#elif DT_INST_ON_BUS(0, spi)

static inline int ssd1306_write_bus(const struct device *dev,
				    uint8_t *buf, size_t len, bool command)
{
	struct ssd1306_data *driver = dev->data;
	int errno;

	gpio_pin_set(driver->data_cmd, DT_INST_GPIO_PIN(0, data_cmd_gpios),
		     command ? 0 : 1);
	struct spi_buf tx_buf = {
		.buf = buf,
		.len = len
	};

	struct spi_buf_set tx_bufs = {
		.buffers = &tx_buf,
		.count = 1
	};

	errno = spi_write(driver->bus, &driver->spi_config, &tx_bufs);

	return errno;
}
#endif

static inline int ssd1306_set_panel_orientation(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		(SSD1306_PANEL_SEGMENT_REMAP ?
		 SSD1306_SET_SEGMENT_MAP_REMAPED :
		 SSD1306_SET_SEGMENT_MAP_NORMAL),
		(SSD1306_PANEL_COM_INVDIR ?
		 SSD1306_SET_COM_OUTPUT_SCAN_FLIPPED :
		 SSD1306_SET_COM_OUTPUT_SCAN_NORMAL)
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_timing_setting(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		SSD1306_SET_CLOCK_DIV_RATIO,
		(SSD1306_CLOCK_FREQUENCY << 4) | SSD1306_CLOCK_DIV_RATIO,
		SSD1306_SET_CHARGE_PERIOD,
		DT_INST_PROP(0, prechargep),
		SSD1306_SET_VCOM_DESELECT_LEVEL,
		SSD1306_PANEL_VCOM_DESEL_LEVEL
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_hardware_config(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		SSD1306_SET_START_LINE,
		SSD1306_SET_DISPLAY_OFFSET,
		DT_INST_PROP(0, display_offset),
		SSD1306_SET_PADS_HW_CONFIG,
		SSD1306_COM_PINS_HW_CONFIG,
		SSD1306_SET_MULTIPLEX_RATIO,
		DT_INST_PROP(0, multiplex_ratio)
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static inline int ssd1306_set_charge_pump(const struct device *dev)
{
	uint8_t cmd_buf[] = {
#if defined(CONFIG_SSD1306_DEFAULT)
		SSD1306_SET_CHARGE_PUMP_ON,
		SSD1306_SET_CHARGE_PUMP_ON_ENABLED,
#endif
#if defined(CONFIG_SSD1306_SH1106_COMPATIBLE)
		SH1106_SET_DCDC_MODE,
		SH1106_SET_DCDC_ENABLED,
#endif
		SSD1306_PANEL_PUMP_VOLTAGE,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static int ssd1306_resume(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		SSD1306_DISPLAY_ON,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static int ssd1306_suspend(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		SSD1306_DISPLAY_OFF,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
}

static int ssd1306_write(const struct device *dev, const uint16_t x, const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
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
		SSD1306_SET_MEM_ADDRESSING_MODE,
		SSD1306_ADDRESSING_MODE,
		SSD1306_SET_COLUMN_ADDRESS,
		x,
		(x + desc->width - 1),
		SSD1306_SET_PAGE_ADDRESS,
		y/8,
		((y + desc->height)/8 - 1)
	};

	if (ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true)) {
		LOG_ERR("Failed to write command");
		return -1;
	}

	return ssd1306_write_bus(dev, (uint8_t *)buf, buf_len, false);

#elif defined(CONFIG_SSD1306_SH1106_COMPATIBLE)
	uint8_t x_offset = x + DT_INST_PROP(0, segment_offset);
	uint8_t cmd_buf[] = {
		SSD1306_SET_LOWER_COL_ADDRESS |
			(x_offset & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_SET_HIGHER_COL_ADDRESS |
			((x_offset >> 4) & SSD1306_SET_LOWER_COL_ADDRESS_MASK),
		SSD1306_SET_PAGE_START_ADDRESS | (y / 8)
	};
	uint8_t *buf_ptr = (uint8_t *)buf;

	for (uint8_t n = 0; n < desc->height / 8; n++) {
		cmd_buf[sizeof(cmd_buf) - 1] =
			SSD1306_SET_PAGE_START_ADDRESS | (n + (y / 8));
		LOG_HEXDUMP_DBG(cmd_buf, sizeof(cmd_buf), "cmd_buf");

		if (ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true)) {
			return -1;
		}

		if (ssd1306_write_bus(dev, buf_ptr, desc->width, false)) {
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
	uint8_t cmd_buf[] = {
		SSD1306_SET_CONTRAST_CTRL,
		contrast,
	};

	return ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true);
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

static int ssd1306_init_device(const struct device *dev)
{
	uint8_t cmd_buf[] = {
		SSD1306_SET_ENTIRE_DISPLAY_OFF,
#ifdef CONFIG_SSD1306_REVERSE_MODE
		SSD1306_SET_REVERSE_DISPLAY,
#else
		SSD1306_SET_NORMAL_DISPLAY,
#endif
	};

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct ssd1306_data *driver = dev->data;

	k_sleep(K_MSEC(SSD1306_RESET_DELAY));
	gpio_pin_set(driver->reset,
		     DT_INST_GPIO_PIN(0, reset_gpios), 1);
	k_sleep(K_MSEC(SSD1306_RESET_DELAY));
	gpio_pin_set(driver->reset,
		     DT_INST_GPIO_PIN(0, reset_gpios), 0);
#endif

	/* Turn display off */
	if (ssd1306_suspend(dev)) {
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

	if (ssd1306_write_bus(dev, cmd_buf, sizeof(cmd_buf), true)) {
		return -EIO;
	}

	if (ssd1306_set_contrast(dev, CONFIG_SSD1306_DEFAULT_CONTRAST)) {
		return -EIO;
	}

	ssd1306_resume(dev);

	return 0;
}

static int ssd1306_init(const struct device *dev)
{
	struct ssd1306_data *driver = dev->data;

	LOG_DBG("");

	driver->bus = device_get_binding(DT_INST_BUS_LABEL(0));
	if (driver->bus == NULL) {
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

#if DT_INST_ON_BUS(0, spi)
	driver->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	driver->spi_config.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB |
				       SPI_WORD_SET(8) | SPI_LINES_SINGLE;
	driver->spi_config.slave = DT_INST_REG_ADDR(0);
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	driver->cs_ctrl.gpio_dev = device_get_binding(
				   DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	driver->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	driver->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	driver->cs_ctrl.delay = 0U;
	driver->spi_config.cs = &driver->cs_ctrl;
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	driver->data_cmd = device_get_binding(
			   DT_INST_GPIO_LABEL(0, data_cmd_gpios));
	if (driver->data_cmd == NULL) {
		LOG_ERR("Failed to get pointer to %s device!",
				DT_INST_GPIO_LABEL(0, data_cmd_gpios));
		return -EINVAL;
	}

	gpio_pin_configure(driver->data_cmd,
			   DT_INST_GPIO_PIN(0, data_cmd_gpios),
			   GPIO_OUTPUT_INACTIVE |
			   DT_INST_GPIO_FLAGS(0, data_cmd_gpios));
#endif /* DT_INST_ON_BUS(0, spi) */

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

DEVICE_DT_INST_DEFINE(0, ssd1306_init, device_pm_control_nop,
		    &ssd1306_driver, NULL,
		    POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY,
		    &ssd1306_driver_api);
