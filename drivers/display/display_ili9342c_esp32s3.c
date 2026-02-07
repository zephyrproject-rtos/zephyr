/*
 * Copyright (c) 2026 NotioNext Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_lcd

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_ili9342c_esp32s3, CONFIG_DISPLAY_LOG_LEVEL);

/* ILI9342C Commands */
#define ILI9342C_CMD_SWRESET     0x01
#define ILI9342C_CMD_SLPOUT      0x11
#define ILI9342C_CMD_INVOFF      0x20
#define ILI9342C_CMD_INVON       0x21
#define ILI9342C_CMD_DISPOFF     0x28
#define ILI9342C_CMD_DISPON      0x29
#define ILI9342C_CMD_CASET       0x2A
#define ILI9342C_CMD_RASET       0x2B
#define ILI9342C_CMD_RAMWR       0x2C
#define ILI9342C_CMD_MADCTL      0x36
#define ILI9342C_CMD_COLMOD      0x3A
#define ILI9342C_CMD_SETEXTC     0xC8
#define ILI9342C_CMD_PWCTRL1     0xC0
#define ILI9342C_CMD_PWCTRL2     0xC1
#define ILI9342C_CMD_VMCTRL1     0xC5
#define ILI9342C_CMD_VMCTRL2     0xC7
#define ILI9342C_CMD_PGAMCTRL    0xE0
#define ILI9342C_CMD_NGAMCTRL    0xE1
#define ILI9342C_CMD_IFMODE      0xB0
#define ILI9342C_CMD_FRMCTR1     0xB1
#define ILI9342C_CMD_DISCTRL     0xB6

/* MADCTL bits */
#define ILI9342C_MADCTL_MY  0x80
#define ILI9342C_MADCTL_MX  0x40
#define ILI9342C_MADCTL_MV  0x20
#define ILI9342C_MADCTL_ML  0x10
#define ILI9342C_MADCTL_BGR 0x08
#define ILI9342C_MADCTL_MH  0x04

struct esp32_lcd_config {
	uint16_t width;
	uint16_t height;
	uint8_t pixel_format;
	uint16_t rotation;

	struct gpio_dt_spec dc_gpio;
	struct gpio_dt_spec reset_gpio;

	struct spi_dt_spec spi;
};

struct esp32_lcd_data {
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

static int esp32_lcd_transmit_cmd(const struct device *dev, uint8_t cmd,
				   const uint8_t *data, size_t len)
{
	const struct esp32_lcd_config *config = dev->config;
	int ret;

	/* Set DC low for command */
	gpio_pin_set_dt(&config->dc_gpio, 0);

	struct spi_buf tx_buf = {
		.buf = &cmd,
		.len = 1,
	};
	struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		LOG_ERR("Failed to send command 0x%02x: %d", cmd, ret);
		return ret;
	}

	/* Send data if any */
	if (data && len > 0) {
		/* Set DC high for data */
		gpio_pin_set_dt(&config->dc_gpio, 1);

		tx_buf.buf = (void *)data;
		tx_buf.len = len;

		ret = spi_write_dt(&config->spi, &tx);
		if (ret < 0) {
			LOG_ERR("Failed to send data for command 0x%02x: %d", cmd, ret);
			return ret;
		}
	}

	return 0;
}

static int esp32_lcd_transmit_data(const struct device *dev,
				    const void *data, size_t len)
{
	const struct esp32_lcd_config *config = dev->config;

	/* Set DC high for data */
	gpio_pin_set_dt(&config->dc_gpio, 1);

	struct spi_buf tx_buf = {
		.buf = (void *)data,
		.len = len,
	};
	struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};

	return spi_write_dt(&config->spi, &tx);
}

static int esp32_lcd_write(const struct device *dev, const uint16_t x,
			    const uint16_t y,
			    const struct display_buffer_descriptor *desc,
			    const void *buf)
{
	int ret;

	LOG_DBG("Writing to display: x=%d, y=%d, w=%d, h=%d",
		x, y, desc->width, desc->height);

	uint16_t x_end = x + desc->width - 1;
	uint16_t y_end = y + desc->height - 1;

	/* Set column address */
	uint8_t col_data[4] = {
		(x >> 8) & 0xFF,
		x & 0xFF,
		(x_end >> 8) & 0xFF,
		x_end & 0xFF,
	};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_CASET, col_data, 4);
	if (ret < 0) {
		return ret;
	}

	/* Set row address */
	uint8_t row_data[4] = {
		(y >> 8) & 0xFF,
		y & 0xFF,
		(y_end >> 8) & 0xFF,
		y_end & 0xFF,
	};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_RASET, row_data, 4);
	if (ret < 0) {
		return ret;
	}

	/* Write memory */
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_RAMWR, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Send pixel data */
	size_t data_len = desc->width * desc->height * 2; /* RGB565 = 2 bytes per pixel */
	ret = esp32_lcd_transmit_data(dev, buf, data_len);
	if (ret < 0) {
		LOG_ERR("Failed to write pixel data: %d", ret);
		return ret;
	}

	return 0;
}

static int esp32_lcd_blanking_on(const struct device *dev)
{
	return esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_DISPOFF, NULL, 0);
}

static int esp32_lcd_blanking_off(const struct device *dev)
{
	return esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_DISPON, NULL, 0);
}

static void esp32_lcd_get_capabilities(const struct device *dev,
					struct display_capabilities *caps)
{
	const struct esp32_lcd_config *config = dev->config;
	struct esp32_lcd_data *data = dev->data;

	memset(caps, 0, sizeof(struct display_capabilities));

	caps->x_resolution = config->width;
	caps->y_resolution = config->height;
	caps->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	caps->current_pixel_format = PIXEL_FORMAT_RGB_565;
	caps->current_orientation = data->orientation;
}

static int esp32_lcd_set_pixel_format(const struct device *dev,
				       const enum display_pixel_format pf)
{
	if (pf != PIXEL_FORMAT_RGB_565) {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	return 0;
}

static int esp32_lcd_init(const struct device *dev)
{
	const struct esp32_lcd_config *config = dev->config;
	int ret;

	LOG_INF("Initializing ESP32 LCD display");

	/* Check SPI device */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	/* Configure DC GPIO */
	if (!gpio_is_ready_dt(&config->dc_gpio)) {
		LOG_ERR("DC GPIO not ready");
		return -ENODEV;
	}
	ret = gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure DC GPIO: %d", ret);
		return ret;
	}

	/* Configure and toggle reset GPIO */
	if (gpio_is_ready_dt(&config->reset_gpio)) {
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Failed to configure reset GPIO: %d", ret);
			return ret;
		}

		gpio_pin_set_dt(&config->reset_gpio, 0);
		k_msleep(10);
		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_msleep(120);
		LOG_INF("Display reset complete");
	}

	/* Software reset */
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_SWRESET, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(150);

	/* Sleep out */
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_SLPOUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(120);

	/* Power control 1 */
	uint8_t pwctrl1[] = {0x23};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_PWCTRL1, pwctrl1, sizeof(pwctrl1));
	if (ret < 0) {
		return ret;
	}

	/* Power control 2 */
	uint8_t pwctrl2[] = {0x10};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_PWCTRL2, pwctrl2, sizeof(pwctrl2));
	if (ret < 0) {
		return ret;
	}

	/* VCOM control 1 */
	uint8_t vmctrl1[] = {0x3e, 0x28};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_VMCTRL1, vmctrl1, sizeof(vmctrl1));
	if (ret < 0) {
		return ret;
	}

	/* VCOM control 2 */
	uint8_t vmctrl2[] = {0x86};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_VMCTRL2, vmctrl2, sizeof(vmctrl2));
	if (ret < 0) {
		return ret;
	}

	/* Memory access control (rotation) - ESP32-S3 Box3 uses BGR order */
	uint8_t madctl = ILI9342C_MADCTL_BGR;
	switch (config->rotation) {
	case 0:
		/* Use both MX and MY flags for correct orientation */
		madctl |= ILI9342C_MADCTL_MX | ILI9342C_MADCTL_MY;
		break;
	case 90:
		madctl |= ILI9342C_MADCTL_MV | ILI9342C_MADCTL_MX;
		break;
	case 180:
		/* No additional flags for 180° */
		break;
	case 270:
		madctl |= ILI9342C_MADCTL_MV | ILI9342C_MADCTL_MY;
		break;
	default:
		/* Use both MX and MY flags for correct orientation */
		madctl |= ILI9342C_MADCTL_MX | ILI9342C_MADCTL_MY;
		break;
	}
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_MADCTL, &madctl, 1);
	if (ret < 0) {
		return ret;
	}

	/* Pixel format - RGB565 */
	uint8_t colmod[] = {0x55};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_COLMOD, colmod, sizeof(colmod));
	if (ret < 0) {
		return ret;
	}

	/* Frame rate control */
	uint8_t frmctr1[] = {0x00, 0x18};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_FRMCTR1, frmctr1, sizeof(frmctr1));
	if (ret < 0) {
		return ret;
	}

	/* Display function control */
	uint8_t disctrl[] = {0x08, 0x82, 0x27};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_DISCTRL, disctrl, sizeof(disctrl));
	if (ret < 0) {
		return ret;
	}

	/* Positive gamma correction */
	uint8_t pgamctrl[] = {
		0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
		0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00
	};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_PGAMCTRL, pgamctrl, sizeof(pgamctrl));
	if (ret < 0) {
		return ret;
	}

	/* Negative gamma correction */
	uint8_t ngamctrl[] = {
		0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
		0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F
	};
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_NGAMCTRL, ngamctrl, sizeof(ngamctrl));
	if (ret < 0) {
		return ret;
	}

	/* Inversion off */
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_INVOFF, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	/* Display on */
	ret = esp32_lcd_transmit_cmd(dev, ILI9342C_CMD_DISPON, NULL, 0);
	if (ret < 0) {
		return ret;
	}
	k_msleep(100);

	LOG_INF("ESP32 LCD display initialized successfully");

	return 0;
}

static const struct display_driver_api esp32_lcd_api = {
	.blanking_on = esp32_lcd_blanking_on,
	.blanking_off = esp32_lcd_blanking_off,
	.write = esp32_lcd_write,
	.get_capabilities = esp32_lcd_get_capabilities,
	.set_pixel_format = esp32_lcd_set_pixel_format,
};

#define ESP32_LCD_INIT(inst)							\
	static const struct esp32_lcd_config esp32_lcd_config_##inst = {	\
		.width = DT_INST_PROP(inst, width),				\
		.height = DT_INST_PROP(inst, height),				\
		.pixel_format = DT_INST_PROP(inst, pixel_format),		\
		.rotation = DT_INST_PROP(inst, rotation),			\
		.dc_gpio = GPIO_DT_SPEC_INST_GET(inst, dc_gpios),		\
		.reset_gpio = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),	\
		.spi = SPI_DT_SPEC_INST_GET(inst,				\
					    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | \
					    SPI_TRANSFER_MSB),			\
	};									\
										\
	static struct esp32_lcd_data esp32_lcd_data_##inst = {			\
		.pixel_format = PIXEL_FORMAT_RGB_565,				\
		.orientation = DISPLAY_ORIENTATION_NORMAL,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(inst, esp32_lcd_init, NULL,			\
			      &esp32_lcd_data_##inst,				\
			      &esp32_lcd_config_##inst,				\
			      POST_KERNEL, CONFIG_DISPLAY_INIT_PRIORITY,	\
			      &esp32_lcd_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_LCD_INIT)
