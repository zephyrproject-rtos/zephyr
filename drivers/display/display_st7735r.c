/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 * Copyright (c) 2020 Kim Bøndergaard <kim@fam-boendergaard.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7735r

#include "display_st7735r.h"

#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/display.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_st7735r, CONFIG_DISPLAY_LOG_LEVEL);

#define ST7735R_RESET_TIME              K_MSEC(1)
#define ST7735R_EXIT_SLEEP_TIME K_MSEC(120)

#define ST7735R_PIXEL_SIZE 2u

struct st7735r_gpio_data {
	const char *const name;
	gpio_dt_flags_t flags;
	gpio_pin_t pin;
};

struct st7735r_config {
	const char *spi_name;
	const char *cs_name;
	struct spi_config spi_config;
	struct st7735r_gpio_data cmd_data;
	struct st7735r_gpio_data reset;
	uint16_t height;
	uint16_t width;
	uint8_t madctl;
	uint8_t colmod;
	uint8_t caset[4];
	uint8_t raset[4];
	uint8_t vmctr1;
	uint8_t invctr;
	uint8_t pwctr1[3];
	uint8_t pwctr2[1];
	uint8_t pwctr3[2];
	uint8_t pwctr4[2];
	uint8_t pwctr5[2];
	uint8_t frmctr1[3];
	uint8_t frmctr2[3];
	uint8_t frmctr3[6];
	uint8_t gamctrp1[16];
	uint8_t gamctrn1[16];
};

struct st7735r_data {
	const struct st7735r_config *config;
	const struct device *spi_dev;
	struct spi_cs_control cs_ctrl;
	const struct device *cmd_data_dev;
	const struct device *reset_dev;
	uint16_t x_offset;
	uint16_t y_offset;
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

static void st7735r_set_lcd_margins(struct st7735r_data *data,
				    uint16_t x_offset, uint16_t y_offset)
{
	data->x_offset = x_offset;
	data->y_offset = y_offset;
}

static void st7735r_set_cmd(struct st7735r_data *data, int is_cmd)
{
	gpio_pin_set(data->cmd_data_dev, data->config->cmd_data.pin, is_cmd);
}

static int st7735r_transmit(struct st7735r_data *data, uint8_t cmd,
			    const uint8_t *tx_data, size_t tx_count)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };
	int ret;

	st7735r_set_cmd(data, 1);
	ret = spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
	if (ret < 0) {
		return ret;
	}

	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_count;
		st7735r_set_cmd(data, 0);
		ret = spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static int st7735r_exit_sleep(struct st7735r_data *data)
{
	int ret;

	ret = st7735r_transmit(data, ST7735R_CMD_SLEEP_OUT, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	k_sleep(ST7735R_EXIT_SLEEP_TIME);

	return 0;
}

static int st7735r_reset_display(struct st7735r_data *data)
{
	int ret;

	LOG_DBG("Resetting display");
	if (data->config->reset.name) {
		gpio_pin_set(data->reset_dev, data->config->reset.pin, 1);
		k_sleep(ST7735R_RESET_TIME);
		gpio_pin_set(data->reset_dev, data->config->reset.pin, 0);
	} else {
		ret = st7735r_transmit(data, ST7735R_CMD_SW_RESET, NULL, 0);
		if (ret < 0) {
			return ret;
		}
	}

	k_sleep(ST7735R_EXIT_SLEEP_TIME);

	return 0;
}

static int st7735r_blanking_on(const struct device *dev)
{
	struct st7735r_data *data = (struct st7735r_data *)dev->data;

	return st7735r_transmit(data, ST7735R_CMD_DISP_OFF, NULL, 0);
}

static int st7735r_blanking_off(const struct device *dev)
{
	struct st7735r_data *data = (struct st7735r_data *)dev->data;

	return st7735r_transmit(data, ST7735R_CMD_DISP_ON, NULL, 0);
}

static int st7735r_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static int st7735r_set_mem_area(struct st7735r_data *data,
				const uint16_t x, const uint16_t y,
				const uint16_t w, const uint16_t h)
{
	uint16_t spi_data[2];

	int ret;

	uint16_t ram_x = x + data->x_offset;
	uint16_t ram_y = y + data->y_offset;

	spi_data[0] = sys_cpu_to_be16(ram_x);
	spi_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	ret = st7735r_transmit(data, ST7735R_CMD_CASET, (uint8_t *)&spi_data[0], 4);
	if (ret < 0) {
		return ret;
	}

	spi_data[0] = sys_cpu_to_be16(ram_y);
	spi_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	ret = st7735r_transmit(data, ST7735R_CMD_RASET, (uint8_t *)&spi_data[0], 4);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int st7735r_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct st7735r_data *data = (struct st7735r_data *)dev->data;
	const uint8_t *write_data_start = (uint8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;
	int ret;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller than width");
	__ASSERT((desc->pitch * ST7735R_PIXEL_SIZE * desc->height)
		 <= desc->buf_size, "Input buffer too small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)",
		desc->width, desc->height, x, y);
	ret = st7735r_set_mem_area(data, x, y, desc->width, desc->height);
	if (ret < 0) {
		return ret;
	}

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_RAMWR,
			       (void *) write_data_start,
			       desc->width * ST7735R_PIXEL_SIZE * write_h);
	if (ret < 0) {
		return ret;
	}

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * ST7735R_PIXEL_SIZE);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ST7735R_PIXEL_SIZE * write_h;
		ret = spi_write(data->spi_dev, &data->config->spi_config, &tx_bufs);
		if (ret < 0) {
			return ret;
		}

		write_data_start += (desc->pitch * ST7735R_PIXEL_SIZE);
	}

	return 0;
}

static void *st7735r_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st7735r_set_brightness(const struct device *dev,
				  const uint8_t brightness)
{
	return -ENOTSUP;
}

static int st7735r_set_contrast(const struct device *dev,
				const uint8_t contrast)
{
	return -ENOTSUP;
}

static void st7735r_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct st7735r_config *config = (struct st7735r_config *)dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;
	if (config->madctl & ST7735R_MADCTL_BGR) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_BGR_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_BGR_565;
	} else {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	}

	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7735r_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format pixel_format)
{
	struct st7735r_config *config = (struct st7735r_config *)dev->config;

	if ((pixel_format == PIXEL_FORMAT_RGB_565) &&
	    (~config->madctl & ST7735R_MADCTL_BGR)) {
		return 0;
	}

	if ((pixel_format == PIXEL_FORMAT_BGR_565) &&
	    (config->madctl & ST7735R_MADCTL_BGR)) {
		return 0;
	}

	LOG_ERR("Pixel format change not implemented");

	return -ENOTSUP;
}

static int st7735r_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}

	LOG_ERR("Changing display orientation not implemented");

	return -ENOTSUP;
}

static int st7735r_lcd_init(struct st7735r_data *data)
{
	const struct st7735r_config *config = data->config;
	int ret;

	st7735r_set_lcd_margins(data, data->x_offset, data->y_offset);

	ret = st7735r_transmit(data, ST7735R_CMD_FRMCTR1, config->frmctr1,
			       sizeof(config->frmctr1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_FRMCTR2, config->frmctr2,
			       sizeof(config->frmctr2));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_FRMCTR3, config->frmctr3,
			       sizeof(config->frmctr3));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_INVCTR, &config->invctr, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_PWCTR1, config->pwctr1,
			       sizeof(config->pwctr1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_PWCTR2, config->pwctr2,
			       sizeof(config->pwctr2));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_PWCTR3, config->pwctr3,
			       sizeof(config->pwctr3));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_PWCTR4, config->pwctr4,
			       sizeof(config->pwctr4));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_PWCTR5, config->pwctr5,
			       sizeof(config->pwctr5));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_VMCTR1, &config->vmctr1, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_INV_OFF, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_MADCTL, &config->madctl, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_COLMOD, &config->colmod, 1);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_CASET, config->caset,
			       sizeof(config->caset));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_RASET, config->raset,
			       sizeof(config->raset));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_GAMCTRP1, config->gamctrp1,
			       sizeof(config->gamctrp1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_GAMCTRN1, config->gamctrn1,
			       sizeof(config->gamctrn1));
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_NORON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	ret = st7735r_transmit(data, ST7735R_CMD_DISP_ON, NULL, 0);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int st7735r_init(const struct device *dev)
{
	struct st7735r_data *data = (struct st7735r_data *)dev->data;
	struct st7735r_config *config = (struct st7735r_config *)dev->config;
	int ret;

	data->spi_dev = device_get_binding(config->spi_name);
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LCD");
		return -ENODEV;
	}

	if (config->cs_name) {
		data->cs_ctrl.gpio_dev = device_get_binding(config->cs_name);
		if (data->cs_ctrl.gpio_dev == NULL) {
			LOG_ERR("Could not get device for SPI CS");
			return -ENODEV;
		}
	}

	if (config->reset.name) {
		data->reset_dev = device_get_binding(config->reset.name);
		if (data->reset_dev == NULL) {
			LOG_ERR("Could not get GPIO port for display reset");
			return -ENODEV;
		}

		ret = gpio_pin_configure(data->reset_dev, config->reset.pin,
					 GPIO_OUTPUT_INACTIVE | config->reset.flags);
		if (ret) {
			LOG_ERR("Couldn't configure reset pin");
			return ret;
		}
	}

#ifdef CONFIG_PM_DEVICE
	data->pm_state = PM_DEVICE_STATE_ACTIVE;
#endif

	data->cmd_data_dev = device_get_binding(config->cmd_data.name);
	if (data->cmd_data_dev == NULL) {
		LOG_ERR("Could not get GPIO port for cmd/DATA port");
		return -ENODEV;
	}

	ret = gpio_pin_configure(data->cmd_data_dev, config->cmd_data.pin,
				 GPIO_OUTPUT | config->cmd_data.flags);
	if (ret) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return ret;
	}

	ret = st7735r_reset_display(data);
	if (ret < 0) {
		LOG_ERR("Couldn't reset display");
		return ret;
	}

	ret = st7735r_exit_sleep(data);
	if (ret < 0) {
		LOG_ERR("Couldn't exit sleep");
		return ret;
	}

	ret = st7735r_lcd_init(data);
	if (ret < 0) {
		LOG_ERR("Couldn't init LCD");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int st7735r_enter_sleep(struct st7735r_data *data)
{
	return st7735r_transmit(data, ST7735R_CMD_SLEEP_IN, NULL, 0);
}

static int st7735r_pm_control(const struct device *dev, uint32_t ctrl_command,
			      enum pm_device_state *state)
{
	int ret = 0;
	struct st7735r_data *data = (struct st7735r_data *)dev->data;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		if (*state == PM_DEVICE_STATE_ACTIVE) {
			ret = st7735r_exit_sleep(data);
			if (ret < 0) {
				return ret;
			}
			data->pm_state = PM_DEVICE_STATE_ACTIVE;
		} else {
			ret = st7735r_enter_sleep(data);
			if (ret < 0) {
				return ret;
			}
			data->pm_state = PM_DEVICE_STATE_LOW_POWER;
		}

		break;

	case PM_DEVICE_STATE_GET:
		*state = data->pm_state;

		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api st7735r_api = {
	.blanking_on = st7735r_blanking_on,
	.blanking_off = st7735r_blanking_off,
	.write = st7735r_write,
	.read = st7735r_read,
	.get_framebuffer = st7735r_get_framebuffer,
	.set_brightness = st7735r_set_brightness,
	.set_contrast = st7735r_set_contrast,
	.get_capabilities = st7735r_get_capabilities,
	.set_pixel_format = st7735r_set_pixel_format,
	.set_orientation = st7735r_set_orientation,
};


#define ST7735R_INIT(inst)							\
	static struct st7735r_data st7735r_data_ ## inst;			\
										\
	const static struct st7735r_config st7735r_config_ ## inst = {		\
		.spi_name = DT_INST_BUS_LABEL(inst),				\
		.cs_name = UTIL_AND(						\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(inst)),			\
		.spi_config.slave = DT_INST_REG_ADDR(inst),			\
		.spi_config.frequency = UTIL_AND(				\
			DT_HAS_PROP(inst, spi_max_frequency),			\
			DT_INST_PROP(inst, spi_max_frequency)),			\
		.spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),	\
		.spi_config.cs = UTIL_AND(					\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			&(st7735r_data_ ## inst.cs_ctrl)),			\
		.cmd_data.name = DT_INST_GPIO_LABEL(inst, cmd_data_gpios),	\
		.cmd_data.pin = DT_INST_GPIO_PIN(inst, cmd_data_gpios),		\
		.cmd_data.flags = DT_INST_GPIO_FLAGS(inst, cmd_data_gpios),	\
		.reset.name = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_LABEL(inst, reset_gpios)),			\
		.reset.pin = UTIL_AND(						\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_PIN(inst, reset_gpios)),			\
		.reset.flags = UTIL_AND(					\
			DT_INST_HAS_PROP(inst, reset_gpios),			\
			DT_INST_GPIO_FLAGS(inst, reset_gpios)),			\
		.width = DT_INST_PROP(inst, width),				\
		.height = DT_INST_PROP(inst, height),				\
		.madctl = DT_INST_PROP(inst, madctl),				\
		.colmod = DT_INST_PROP(inst, colmod),				\
		.caset = DT_INST_PROP(inst, caset),				\
		.raset = DT_INST_PROP(inst, raset),				\
		.vmctr1 = DT_INST_PROP(inst, vmctr1),				\
		.invctr = DT_INST_PROP(inst, invctr),				\
		.pwctr1 = DT_INST_PROP(inst, pwctr1),				\
		.pwctr2 = DT_INST_PROP(inst, pwctr2),				\
		.pwctr3 = DT_INST_PROP(inst, pwctr3),				\
		.pwctr4 = DT_INST_PROP(inst, pwctr4),				\
		.pwctr5 = DT_INST_PROP(inst, pwctr5),				\
		.frmctr1 = DT_INST_PROP(inst, frmctr1),				\
		.frmctr2 = DT_INST_PROP(inst, frmctr2),				\
		.frmctr3 = DT_INST_PROP(inst, frmctr3),				\
		.gamctrp1 = DT_INST_PROP(inst, gamctrp1),			\
		.gamctrn1 = DT_INST_PROP(inst, gamctrn1),			\
	};									\
										\
	static struct st7735r_data st7735r_data_ ## inst = {			\
		.config = &st7735r_config_ ## inst,				\
		.cs_ctrl.gpio_pin = UTIL_AND(					\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_PIN(inst)),			\
		.cs_ctrl.gpio_dt_flags = UTIL_AND(				\
			DT_INST_SPI_DEV_HAS_CS_GPIOS(inst),			\
			DT_INST_SPI_DEV_CS_GPIOS_FLAGS(inst)),			\
		.cs_ctrl.delay = 0U,						\
		.x_offset = DT_INST_PROP(inst, x_offset),			\
		.y_offset = DT_INST_PROP(inst, y_offset),			\
	};									\
	DEVICE_DT_INST_DEFINE(inst, st7735r_init, st7735r_pm_control,		\
			      &st7735r_data_ ## inst, &st7735r_config_ ## inst,	\
			      APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY,	\
			      &st7735r_api);

DT_INST_FOREACH_STATUS_OKAY(ST7735R_INIT)
