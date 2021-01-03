/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9xxx.h"

#include <dt-bindings/display/ili9xxx.h>
#include <drivers/display.h>
#include <drivers/spi.h>
#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9xxx, CONFIG_DISPLAY_LOG_LEVEL);

struct ili9xxx_data {
	const struct device *reset_gpio;
	const struct device *command_data_gpio;
	const struct device *spi_dev;
	struct spi_config spi_config;
	struct spi_cs_control cs_ctrl;
	uint8_t bytes_per_pixel;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
};

int ili9xxx_transmit(const struct device *dev, uint8_t cmd, const void *tx_data,
		     size_t tx_len)
{
	const struct ili9xxx_config *config =
		(struct ili9xxx_config *)dev->config;
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	int r;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1U };

	/* send command */
	tx_buf.buf = &cmd;
	tx_buf.len = 1U;

	gpio_pin_set(data->command_data_gpio, config->cmd_data_pin,
		     ILI9XXX_CMD);
	r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	if (r < 0) {
		return r;
	}

	/* send data (if any) */
	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_len;

		gpio_pin_set(data->command_data_gpio, config->cmd_data_pin,
			     ILI9XXX_DATA);
		r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}

static int ili9xxx_exit_sleep(const struct device *dev)
{
	int r;

	r = ili9xxx_transmit(dev, ILI9XXX_SLPOUT, NULL, 0);
	if (r < 0) {
		return r;
	}

	k_sleep(K_MSEC(ILI9XXX_SLEEP_OUT_TIME));

	return 0;
}

static void ili9xxx_hw_reset(const struct device *dev)
{
	const struct ili9xxx_config *config =
		(struct ili9xxx_config *)dev->config;
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	if (data->reset_gpio == NULL) {
		return;
	}

	gpio_pin_set(data->reset_gpio, config->reset_pin, 1);
	k_sleep(K_MSEC(ILI9XXX_RESET_PULSE_TIME));
	gpio_pin_set(data->reset_gpio, config->reset_pin, 0);

	k_sleep(K_MSEC(ILI9XXX_RESET_WAIT_TIME));
}

static int ili9xxx_set_mem_area(const struct device *dev, const uint16_t x,
				const uint16_t y, const uint16_t w,
				const uint16_t h)
{
	int r;
	uint16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1U);
	r = ili9xxx_transmit(dev, ILI9XXX_CASET, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1U);
	r = ili9xxx_transmit(dev, ILI9XXX_PASET, &spi_data[0], 4U);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9xxx_write(const struct device *dev, const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	int r;
	const uint8_t *write_data_start = (const uint8_t *)buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * data->bytes_per_pixel * desc->height) <=
			 desc->buf_size,
		 "Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
		x, y);
	r = ili9xxx_set_mem_area(dev, x, y, desc->width, desc->height);
	if (r < 0) {
		return r;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	r = ili9xxx_transmit(dev, ILI9XXX_RAMWR, write_data_start,
			     desc->width * data->bytes_per_pixel * write_h);
	if (r < 0) {
		return r;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += desc->pitch * data->bytes_per_pixel;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * data->bytes_per_pixel * write_h;

		r = spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		if (r < 0) {
			return r;
		}

		write_data_start += desc->pitch * data->bytes_per_pixel;
	}

	return 0;
}

static int ili9xxx_read(const struct device *dev, const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc, void *buf)
{
	LOG_ERR("Reading not supported");
	return -ENOTSUP;
}

static void *ili9xxx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Direct framebuffer access not supported");
	return NULL;
}

static int ili9xxx_display_blanking_off(const struct device *dev)
{
	LOG_DBG("Turning display blanking off");
	return ili9xxx_transmit(dev, ILI9XXX_DISPON, NULL, 0);
}

static int ili9xxx_display_blanking_on(const struct device *dev)
{
	LOG_DBG("Turning display blanking on");
	return ili9xxx_transmit(dev, ILI9XXX_DISPOFF, NULL, 0);
}

static int ili9xxx_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	LOG_ERR("Set brightness not implemented");
	return -ENOTSUP;
}

static int ili9xxx_set_contrast(const struct device *dev,
				const uint8_t contrast)
{
	LOG_ERR("Set contrast not supported");
	return -ENOTSUP;
}

static int
ili9xxx_set_pixel_format(const struct device *dev,
			 const enum display_pixel_format pixel_format)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	int r;
	uint8_t tx_data;
	uint8_t bytes_per_pixel;

	if (pixel_format == PIXEL_FORMAT_RGB_565) {
		bytes_per_pixel = 2U;
		tx_data = ILI9XXX_PIXSET_MCU_16_BIT | ILI9XXX_PIXSET_RGB_16_BIT;
	} else if (pixel_format == PIXEL_FORMAT_RGB_888) {
		bytes_per_pixel = 3U;
		tx_data = ILI9XXX_PIXSET_MCU_18_BIT | ILI9XXX_PIXSET_RGB_18_BIT;
	} else {
		LOG_ERR("Unsupported pixel format");
		return -ENOTSUP;
	}

	r = ili9xxx_transmit(dev, ILI9XXX_PIXSET, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->pixel_format = pixel_format;
	data->bytes_per_pixel = bytes_per_pixel;

	return 0;
}

static int ili9xxx_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	int r;
	uint8_t tx_data = ILI9XXX_MADCTL_BGR;

	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		tx_data |= ILI9XXX_MADCTL_MX;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_90) {
		tx_data |= ILI9XXX_MADCTL_MV;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		tx_data |= ILI9XXX_MADCTL_MY;
	} else if (orientation == DISPLAY_ORIENTATION_ROTATED_270) {
		tx_data |= ILI9XXX_MADCTL_MV | ILI9XXX_MADCTL_MX |
			   ILI9XXX_MADCTL_MY;
	}

	r = ili9xxx_transmit(dev, ILI9XXX_MADCTL, &tx_data, 1U);
	if (r < 0) {
		return r;
	}

	data->orientation = orientation;

	return 0;
}

static void ili9xxx_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;
	const struct ili9xxx_config *config =
		(struct ili9xxx_config *)dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));

	capabilities->supported_pixel_formats =
		PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = data->pixel_format;

	if (data->orientation == DISPLAY_ORIENTATION_NORMAL ||
	    data->orientation == DISPLAY_ORIENTATION_ROTATED_180) {
		capabilities->x_resolution = config->x_resolution;
		capabilities->y_resolution = config->y_resolution;
	} else {
		capabilities->x_resolution = config->y_resolution;
		capabilities->y_resolution = config->x_resolution;
	}

	capabilities->current_orientation = data->orientation;
}

static int ili9xxx_configure(const struct device *dev)
{
	const struct ili9xxx_config *config =
		(struct ili9xxx_config *)dev->config;

	int r;
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;

	/* pixel format */
	if (config->pixel_format == ILI9XXX_PIXEL_FORMAT_RGB565) {
		pixel_format = PIXEL_FORMAT_RGB_565;
	} else {
		pixel_format = PIXEL_FORMAT_RGB_888;
	}

	r = ili9xxx_set_pixel_format(dev, pixel_format);
	if (r < 0) {
		return r;
	}

	/* orientation */
	if (config->rotation == 0U) {
		orientation = DISPLAY_ORIENTATION_NORMAL;
	} else if (config->rotation == 90U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_90;
	} else if (config->rotation == 180U) {
		orientation = DISPLAY_ORIENTATION_ROTATED_180;
	} else {
		orientation = DISPLAY_ORIENTATION_ROTATED_270;
	}

	r = ili9xxx_set_orientation(dev, orientation);
	if (r < 0) {
		return r;
	}

	if (config->inversion) {
		r = ili9xxx_transmit(dev, ILI9XXX_DINVON, NULL, 0U);
		if (r < 0) {
			return r;
		}
	}

	r = config->regs_init_fn(dev);
	if (r < 0) {
		return r;
	}

	return 0;
}

static int ili9xxx_init(const struct device *dev)
{
	const struct ili9xxx_config *config =
		(struct ili9xxx_config *)dev->config;
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->data;

	int r;

	data->spi_dev = device_get_binding(config->spi_name);
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device %s", config->spi_name);
		return -ENODEV;
	}

	data->spi_config.frequency = config->spi_max_freq;
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8U);
	data->spi_config.slave = config->spi_addr;

	data->cs_ctrl.gpio_dev = device_get_binding(config->spi_cs_label);
	if (data->cs_ctrl.gpio_dev != NULL) {
		data->cs_ctrl.gpio_pin = config->spi_cs_pin;
		data->cs_ctrl.gpio_dt_flags = config->spi_cs_flags;
		data->cs_ctrl.delay = 0U;
		data->spi_config.cs = &data->cs_ctrl;
	}

	data->command_data_gpio = device_get_binding(config->cmd_data_label);
	if (data->command_data_gpio == NULL) {
		LOG_ERR("Could not get command/data GPIO port %s",
			config->cmd_data_label);
		return -ENODEV;
	}

	r = gpio_pin_configure(data->command_data_gpio, config->cmd_data_pin,
			       GPIO_OUTPUT | config->cmd_data_flags);
	if (r < 0) {
		LOG_ERR("Could not configure command/data GPIO (%d)", r);
		return r;
	}

	data->reset_gpio = device_get_binding(config->reset_label);
	if (data->reset_gpio != NULL) {
		r = gpio_pin_configure(data->reset_gpio, config->reset_pin,
				       GPIO_OUTPUT_INACTIVE |
					       config->reset_flags);
		if (r < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", r);
			return r;
		}
	}

	ili9xxx_hw_reset(dev);

	r = ili9xxx_configure(dev);
	if (r < 0) {
		LOG_ERR("Could not configure display (%d)", r);
		return r;
	}

	r = ili9xxx_exit_sleep(dev);
	if (r < 0) {
		LOG_ERR("Could not exit sleep mode (%d)", r);
		return r;
	}

	return 0;
}

static const struct display_driver_api ili9xxx_api = {
	.blanking_on = ili9xxx_display_blanking_on,
	.blanking_off = ili9xxx_display_blanking_off,
	.write = ili9xxx_write,
	.read = ili9xxx_read,
	.get_framebuffer = ili9xxx_get_framebuffer,
	.set_brightness = ili9xxx_set_brightness,
	.set_contrast = ili9xxx_set_contrast,
	.get_capabilities = ili9xxx_get_capabilities,
	.set_pixel_format = ili9xxx_set_pixel_format,
	.set_orientation = ili9xxx_set_orientation,
};

#define INST_DT_ILI9XXX(n, t) DT_INST(n, ilitek_ili##t)

#define ILI9XXX_INIT(n, t)                                                     \
	ILI##t##_REGS_INIT(n);                                                 \
									       \
	static const struct ili9xxx_config ili9xxx_config_##n = {              \
		.spi_name = DT_BUS_LABEL(INST_DT_ILI9XXX(n, t)),               \
		.spi_addr = DT_REG_ADDR(INST_DT_ILI9XXX(n, t)),                \
		.spi_max_freq = UTIL_AND(                                      \
			DT_HAS_PROP(INST_DT_ILI9XXX(n, t), spi_max_frequency), \
			DT_PROP(INST_DT_ILI9XXX(n, t), spi_max_frequency)),    \
		.spi_cs_label = UTIL_AND(                                      \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_ILI9XXX(n, t)),        \
			DT_SPI_DEV_CS_GPIOS_LABEL(INST_DT_ILI9XXX(n, t))),     \
		.spi_cs_pin = UTIL_AND(                                        \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_ILI9XXX(n, t)),        \
			DT_SPI_DEV_CS_GPIOS_PIN(INST_DT_ILI9XXX(n, t))),       \
		.spi_cs_flags = UTIL_AND(                                      \
			DT_SPI_DEV_HAS_CS_GPIOS(INST_DT_ILI9XXX(n, t)),        \
			DT_SPI_DEV_CS_GPIOS_FLAGS(INST_DT_ILI9XXX(n, t))),     \
		.cmd_data_label =                                              \
			DT_GPIO_LABEL(INST_DT_ILI9XXX(n, t), cmd_data_gpios),  \
		.cmd_data_pin =                                                \
			DT_GPIO_PIN(INST_DT_ILI9XXX(n, t), cmd_data_gpios),    \
		.cmd_data_flags =                                              \
			DT_GPIO_FLAGS(INST_DT_ILI9XXX(n, t), cmd_data_gpios),  \
		.reset_label = UTIL_AND(                                       \
			DT_NODE_HAS_PROP(INST_DT_ILI9XXX(n, t), reset_gpios),  \
			DT_GPIO_LABEL(INST_DT_ILI9XXX(n, t), reset_gpios)),    \
		.reset_pin = UTIL_AND(                                         \
			DT_NODE_HAS_PROP(INST_DT_ILI9XXX(n, t), reset_gpios),  \
			DT_GPIO_PIN(INST_DT_ILI9XXX(n, t), reset_gpios)),      \
		.reset_flags = UTIL_AND(                                       \
			DT_NODE_HAS_PROP(INST_DT_ILI9XXX(n, t), reset_gpios),  \
			DT_GPIO_FLAGS(INST_DT_ILI9XXX(n, t), reset_gpios)),    \
		.pixel_format = DT_PROP(INST_DT_ILI9XXX(n, t), pixel_format),  \
		.rotation = DT_PROP(INST_DT_ILI9XXX(n, t), rotation),          \
		.x_resolution = ILI##t##_X_RES,                                \
		.y_resolution = ILI##t##_Y_RES,                                \
		.inversion = DT_PROP(INST_DT_ILI9XXX(n, t), display_inversion),\
		.regs = &ili9xxx_regs_##n,                                     \
		.regs_init_fn = ili##t##_regs_init,                            \
	};                                                                     \
									       \
	static struct ili9xxx_data ili9xxx_data_##n;                           \
									       \
	DEVICE_DT_DEFINE(INST_DT_ILI9XXX(n, t), ili9xxx_init,                  \
			    device_pm_control_nop, &ili9xxx_data_##n,          \
			    &ili9xxx_config_##n, POST_KERNEL,                  \
			    CONFIG_APPLICATION_INIT_PRIORITY, &ili9xxx_api);

#define DT_INST_FOREACH_ILI9XXX_STATUS_OKAY(t)                                 \
	UTIL_LISTIFY(DT_NUM_INST_STATUS_OKAY(ilitek_ili##t), ILI9XXX_INIT, t)

#ifdef CONFIG_ILI9340
#include "display_ili9340.h"
DT_INST_FOREACH_ILI9XXX_STATUS_OKAY(9340);
#endif

#ifdef CONFIG_ILI9488
#include "display_ili9488.h"
DT_INST_FOREACH_ILI9XXX_STATUS_OKAY(9488);
#endif
