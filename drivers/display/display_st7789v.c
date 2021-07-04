/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2019 Marc Reilly
 * Copyright (c) 2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2020 Endian Technologies AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sitronix_st7789v

#include "display_st7789v.h"

#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/display.h>

#define LOG_LEVEL CONFIG_DISPLAY_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(display_st7789v);

struct st7789v_config {
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec cmd_data_gpio;
	uint16_t height;
	uint16_t width;
	uint8_t vcom;
	uint8_t gctrl;
	bool vdv_vrh_enable;
	uint8_t vrh_value;
	uint8_t vdv_value;
	uint8_t mdac;
	uint8_t gamma;
	uint8_t colmod;
	uint8_t lcm;
	uint8_t porch_param[5];
	uint8_t cmd2en_param[4];
	uint8_t pwctrl1_param[2];
	uint8_t pvgam_param[14];
	uint8_t nvgam_param[14];
	uint8_t ram_param[2];
	uint8_t rgb_param[3];
};

struct st7789v_data {
	uint16_t x_offset;
	uint16_t y_offset;
#ifdef CONFIG_PM_DEVICE
	enum pm_device_state pm_state;
#endif
};

#ifdef CONFIG_ST7789V_RGB565
#define ST7789V_PIXEL_SIZE 2u
#else
#define ST7789V_PIXEL_SIZE 3u
#endif

static void st7789v_set_lcd_margins(const struct device *dev,
				    uint16_t x_offset, uint16_t y_offset)
{
	struct st7789v_data *data = dev->data;

	data->x_offset = x_offset;
	data->y_offset = y_offset;
}

static void st7789v_set_cmd(const struct device *dev, int is_cmd)
{
	const struct st7789v_config *config = dev->config;

	gpio_pin_set_dt(&config->cmd_data_gpio, is_cmd);
}

static void st7789v_transmit(const struct device *dev, uint8_t cmd,
			     const uint8_t *tx_data, size_t tx_count)
{
	const struct st7789v_config *config = dev->config;

	{
		const struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
		const struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

		st7789v_set_cmd(dev, 1);
		spi_write_dt(&config->spi, &tx_bufs);
	}
	if (tx_data != NULL) {
		const struct spi_buf tx_buf = { .buf = (void *)tx_data, .len = tx_count };
		const struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

		st7789v_set_cmd(dev, 0);
		spi_write_dt(&config->spi, &tx_bufs);
	}
}

static void st7789v_exit_sleep(const struct device *dev)
{
	st7789v_transmit(dev, ST7789V_CMD_SLEEP_OUT, NULL, 0);
	k_sleep(K_MSEC(120));
}

static void st7789v_reset_display(const struct device *dev)
{
	const struct st7789v_config *config = dev->config;

	LOG_DBG("Resetting display");
	if (config->reset_gpio.port != NULL) {
		k_sleep(K_MSEC(1));
		gpio_pin_set_dt(&config->reset_gpio, 1);
		k_sleep(K_MSEC(6));
		gpio_pin_set_dt(&config->reset_gpio, 0);
		k_sleep(K_MSEC(20));
	} else {
		st7789v_transmit(dev, ST7789V_CMD_SW_RESET, NULL, 0);
		k_sleep(K_MSEC(5));
	}
}

static int st7789v_blanking_on(const struct device *dev)
{
	st7789v_transmit(dev, ST7789V_CMD_DISP_OFF, NULL, 0);
	return 0;
}

static int st7789v_blanking_off(const struct device *dev)
{
	st7789v_transmit(dev, ST7789V_CMD_DISP_ON, NULL, 0);
	return 0;
}

static int st7789v_read(const struct device *dev,
			const uint16_t x,
			const uint16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	return -ENOTSUP;
}

static void st7789v_set_mem_area(const struct device *dev, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	struct st7789v_data *data = dev->data;
	uint16_t spi_data[2];

	uint16_t ram_x = x + data->x_offset;
	uint16_t ram_y = y + data->y_offset;

	spi_data[0] = sys_cpu_to_be16(ram_x);
	spi_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	st7789v_transmit(dev, ST7789V_CMD_CASET, (uint8_t *)&spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(ram_y);
	spi_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	st7789v_transmit(dev, ST7789V_CMD_RASET, (uint8_t *)&spi_data[0], 4);
}

static int st7789v_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	const struct st7789v_config *config = dev->config;
	const uint8_t *write_data_start = (uint8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	uint16_t write_cnt;
	uint16_t nbr_of_writes;
	uint16_t write_h;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((desc->pitch * ST7789V_PIXEL_SIZE * desc->height) <= desc->buf_size,
			"Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)",
		desc->width, desc->height, x, y);
	st7789v_set_mem_area(dev, x, y, desc->width, desc->height);

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	st7789v_transmit(dev, ST7789V_CMD_RAMWR,
			 (void *) write_data_start,
			 desc->width * ST7789V_PIXEL_SIZE * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * ST7789V_PIXEL_SIZE);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ST7789V_PIXEL_SIZE * write_h;
		spi_write_dt(&config->spi, &tx_bufs);
		write_data_start += (desc->pitch * ST7789V_PIXEL_SIZE);
	}

	return 0;
}

static void *st7789v_get_framebuffer(const struct device *dev)
{
	return NULL;
}

static int st7789v_set_brightness(const struct device *dev,
			   const uint8_t brightness)
{
	return -ENOTSUP;
}

static int st7789v_set_contrast(const struct device *dev,
			 const uint8_t contrast)
{
	return -ENOTSUP;
}

static void st7789v_get_capabilities(const struct device *dev,
			      struct display_capabilities *capabilities)
{
	const struct st7789v_config *config = dev->config;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = config->width;
	capabilities->y_resolution = config->height;

#ifdef CONFIG_ST7789V_RGB565
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
#else
	capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
	capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
#endif
	capabilities->current_orientation = DISPLAY_ORIENTATION_NORMAL;
}

static int st7789v_set_pixel_format(const struct device *dev,
			     const enum display_pixel_format pixel_format)
{
#ifdef CONFIG_ST7789V_RGB565
	if (pixel_format == PIXEL_FORMAT_RGB_565) {
#else
	if (pixel_format == PIXEL_FORMAT_RGB_888) {
#endif
		return 0;
	}
	LOG_ERR("Pixel format change not implemented");
	return -ENOTSUP;
}

static int st7789v_set_orientation(const struct device *dev,
			    const enum display_orientation orientation)
{
	if (orientation == DISPLAY_ORIENTATION_NORMAL) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void st7789v_lcd_init(const struct device *dev)
{
	struct st7789v_data *data = dev->data;
	const struct st7789v_config *config = dev->config;
	uint8_t tmp;

	st7789v_set_lcd_margins(dev, data->x_offset,
				data->y_offset);

	st7789v_transmit(dev, ST7789V_CMD_CMD2EN, config->cmd2en_param,
			 sizeof(config->cmd2en_param));

	st7789v_transmit(dev, ST7789V_CMD_PORCTRL, config->porch_param,
			 sizeof(config->porch_param));

	/* Digital Gamma Enable, default disabled */
	tmp = 0x00;
	st7789v_transmit(dev, ST7789V_CMD_DGMEN, &tmp, 1);

	/* Frame Rate Control in Normal Mode, default value */
	tmp = 0x0f;
	st7789v_transmit(dev, ST7789V_CMD_FRCTRL2, &tmp, 1);

	st7789v_transmit(dev, ST7789V_CMD_GCTRL, &config->gctrl, 1);

	st7789v_transmit(dev, ST7789V_CMD_VCOMS, &config->vcom, 1);

	if (config->vdv_vrh_enable) {
		tmp = 0x01;
		st7789v_transmit(dev, ST7789V_CMD_VDVVRHEN, &tmp, 1);

		st7789v_transmit(dev, ST7789V_CMD_VRH, &config->vrh_value, 1);

		st7789v_transmit(dev, ST7789V_CMD_VDS, &config->vdv_value, 1);
	}

	st7789v_transmit(dev, ST7789V_CMD_PWCTRL1, config->pwctrl1_param,
			 sizeof(config->pwctrl1_param));

	/* Memory Data Access Control */
	st7789v_transmit(dev, ST7789V_CMD_MADCTL, &config->mdac, 1);

	/* Interface Pixel Format */
	st7789v_transmit(dev, ST7789V_CMD_COLMOD, &config->colmod, 1);

	st7789v_transmit(dev, ST7789V_CMD_LCMCTRL, &config->lcm, 1);

	st7789v_transmit(dev, ST7789V_CMD_GAMSET, &config->gamma, 1);

	st7789v_transmit(dev, ST7789V_CMD_INV_ON, NULL, 0);

	st7789v_transmit(dev, ST7789V_CMD_PVGAMCTRL, config->pvgam_param,
			 sizeof(config->pvgam_param));

	st7789v_transmit(dev, ST7789V_CMD_NVGAMCTRL, config->nvgam_param,
			 sizeof(config->nvgam_param));

	st7789v_transmit(dev, ST7789V_CMD_RAMCTRL, config->ram_param,
			 sizeof(config->ram_param));

	st7789v_transmit(dev, ST7789V_CMD_RGBCTRL, config->rgb_param,
			 sizeof(config->rgb_param));
}

static int st7789v_init(const struct device *dev)
{
	const struct st7789v_config *config = dev->config;

	if (!spi_is_ready(&config->spi)) {
		LOG_ERR("SPI device for LCD is not ready");
		return -ENODEV;
	}

	if (config->reset_gpio.port != NULL) {
		if (!device_is_ready(config->reset_gpio.port)) {
			LOG_ERR("GPIO port for display reset is not ready");
			return -ENODEV;
		}

		if (gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE)) {
			LOG_ERR("Couldn't configure reset pin");
			return -EIO;
		}
	}

	if (!device_is_ready(config->cmd_data_gpio.port)) {
		LOG_ERR("GPIO port for cmd/DATA port is not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&config->cmd_data_gpio, GPIO_OUTPUT)) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return -EIO;
	}

	st7789v_reset_display(dev);

	st7789v_blanking_on(dev);

	st7789v_lcd_init(dev);

	st7789v_exit_sleep(dev);

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static void st7789v_enter_sleep(const struct device *dev)
{
	st7789v_transmit(dev, ST7789V_CMD_SLEEP_IN, NULL, 0);
}

static int st7789v_pm_control(const struct device *dev, uint32_t ctrl_command,
				 enum pm_device_state *state, pm_device_cb cb, void *arg)
{
	int ret = 0;
	struct st7789v_data *data = dev->data;

	switch (ctrl_command) {
	case PM_DEVICE_STATE_SET:
		if (*state == PM_DEVICE_STATE_ACTIVE) {
			st7789v_exit_sleep(dev);
			data->pm_state = PM_DEVICE_STATE_ACTIVE;
			ret = 0;
		} else {
			st7789v_enter_sleep(dev);
			data->pm_state = PM_DEVICE_STATE_LOW_POWER;
			ret = 0;
		}
		break;
	case PM_DEVICE_STATE_GET:
		*state = data->pm_state;
		break;
	default:
		ret = -EINVAL;
	}

	if (cb != NULL) {
		cb(dev, ret, state, arg);
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct display_driver_api st7789v_api = {
	.blanking_on = st7789v_blanking_on,
	.blanking_off = st7789v_blanking_off,
	.write = st7789v_write,
	.read = st7789v_read,
	.get_framebuffer = st7789v_get_framebuffer,
	.set_brightness = st7789v_set_brightness,
	.set_contrast = st7789v_set_contrast,
	.get_capabilities = st7789v_get_capabilities,
	.set_pixel_format = st7789v_set_pixel_format,
	.set_orientation = st7789v_set_orientation,
};

const static struct st7789v_config st7789v_config = {
	.spi = SPI_DT_SPEC_INST_GET(0, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),
	.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(0, reset_gpios, { 0 }),
	.cmd_data_gpio = GPIO_DT_SPEC_INST_GET(0, cmd_data_gpios),
	.width = DT_INST_PROP(0, width),
	.height = DT_INST_PROP(0, height),
	.vcom = DT_INST_PROP(0, vcom),
	.gctrl = DT_INST_PROP(0, gctrl),
	.vdv_vrh_enable = (DT_INST_NODE_HAS_PROP(0, vrhs) && DT_INST_NODE_HAS_PROP(0, vdvs)),
	.vrh_value = DT_INST_PROP(0, vrhs),
	.vdv_value = DT_INST_PROP(0, vdvs),
	.mdac = DT_INST_PROP(0, mdac),
	.gamma = DT_INST_PROP(0, gamma),
	.colmod = DT_INST_PROP(0, colmod),
	.lcm = DT_INST_PROP(0, lcm),
	.porch_param = DT_INST_PROP(0, porch_param),
	.cmd2en_param = DT_INST_PROP(0, cmd2en_param),
	.pwctrl1_param = DT_INST_PROP(0, pwctrl1_param),
	.pvgam_param = DT_INST_PROP(0, pvgam_param),
	.nvgam_param = DT_INST_PROP(0, nvgam_param),
	.ram_param = DT_INST_PROP(0, ram_param),
	.rgb_param = DT_INST_PROP(0, rgb_param),
};

static struct st7789v_data st7789v_data = {
	.x_offset = DT_INST_PROP(0, x_offset),
	.y_offset = DT_INST_PROP(0, y_offset),
#ifdef CONFIG_PM_DEVICE
	.pm_state = PM_DEVICE_STATE_ACTIVE,
#endif
};

DEVICE_DT_INST_DEFINE(0, &st7789v_init,
	      st7789v_pm_control, &st7789v_data, &st7789v_config, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &st7789v_api);
