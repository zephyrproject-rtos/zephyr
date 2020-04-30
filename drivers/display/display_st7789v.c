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

#define ST7789V_CS_PIN		DT_INST_SPI_DEV_CS_GPIOS_PIN(0)
#define ST7789V_CMD_DATA_PIN	DT_INST_GPIO_PIN(0, cmd_data_gpios)
#define ST7789V_CMD_DATA_FLAGS	DT_INST_GPIO_FLAGS(0, cmd_data_gpios)
#define ST7789V_RESET_PIN	DT_INST_GPIO_PIN(0, reset_gpios)
#define ST7789V_RESET_FLAGS	DT_INST_GPIO_FLAGS(0, reset_gpios)

static uint8_t st7789v_porch_param[] = DT_INST_PROP(0, porch_param);
static uint8_t st7789v_cmd2en_param[] = DT_INST_PROP(0, cmd2en_param);
static uint8_t st7789v_pwctrl1_param[] = DT_INST_PROP(0, pwctrl1_param);
static uint8_t st7789v_pvgam_param[] = DT_INST_PROP(0, pvgam_param);
static uint8_t st7789v_nvgam_param[] = DT_INST_PROP(0, nvgam_param);
static uint8_t st7789v_ram_param[] = DT_INST_PROP(0, ram_param);
static uint8_t st7789v_rgb_param[] = DT_INST_PROP(0, rgb_param);

struct st7789v_data {
	const struct device *spi_dev;
	struct spi_config spi_config;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	const struct device *reset_gpio;
#endif
	const struct device *cmd_data_gpio;

	uint16_t height;
	uint16_t width;
	uint16_t x_offset;
	uint16_t y_offset;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t pm_state;
#endif
};

#ifdef CONFIG_ST7789V_RGB565
#define ST7789V_PIXEL_SIZE 2u
#else
#define ST7789V_PIXEL_SIZE 3u
#endif

static void st7789v_set_lcd_margins(struct st7789v_data *data,
			     uint16_t x_offset, uint16_t y_offset)
{
	data->x_offset = x_offset;
	data->y_offset = y_offset;
}

static void st7789v_set_cmd(struct st7789v_data *data, int is_cmd)
{
	gpio_pin_set(data->cmd_data_gpio, ST7789V_CMD_DATA_PIN, is_cmd);
}

static void st7789v_transmit(struct st7789v_data *data, uint8_t cmd,
		uint8_t *tx_data, size_t tx_count)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	st7789v_set_cmd(data, 1);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (tx_data != NULL) {
		tx_buf.buf = tx_data;
		tx_buf.len = tx_count;
		st7789v_set_cmd(data, 0);
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	}
}

static void st7789v_exit_sleep(struct st7789v_data *data)
{
	st7789v_transmit(data, ST7789V_CMD_SLEEP_OUT, NULL, 0);
	k_sleep(K_MSEC(120));
}

static void st7789v_reset_display(struct st7789v_data *data)
{
	LOG_DBG("Resetting display");
#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	k_sleep(K_MSEC(1));
	gpio_pin_set(data->reset_gpio, ST7789V_RESET_PIN, 1);
	k_sleep(K_MSEC(6));
	gpio_pin_set(data->reset_gpio, ST7789V_RESET_PIN, 0);
	k_sleep(K_MSEC(20));
#else
	st7789v_transmit(p_st7789v, ST7789V_CMD_SW_RESET, NULL, 0);
	k_sleep(K_MSEC(5));
#endif
}

static int st7789v_blanking_on(const struct device *dev)
{
	struct st7789v_data *driver = (struct st7789v_data *)dev->data;

	st7789v_transmit(driver, ST7789V_CMD_DISP_OFF, NULL, 0);
	return 0;
}

static int st7789v_blanking_off(const struct device *dev)
{
	struct st7789v_data *driver = (struct st7789v_data *)dev->data;

	st7789v_transmit(driver, ST7789V_CMD_DISP_ON, NULL, 0);
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

static void st7789v_set_mem_area(struct st7789v_data *data, const uint16_t x,
				 const uint16_t y, const uint16_t w, const uint16_t h)
{
	uint16_t spi_data[2];

	uint16_t ram_x = x + data->x_offset;
	uint16_t ram_y = y + data->y_offset;

	spi_data[0] = sys_cpu_to_be16(ram_x);
	spi_data[1] = sys_cpu_to_be16(ram_x + w - 1);
	st7789v_transmit(data, ST7789V_CMD_CASET, (uint8_t *)&spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(ram_y);
	spi_data[1] = sys_cpu_to_be16(ram_y + h - 1);
	st7789v_transmit(data, ST7789V_CMD_RASET, (uint8_t *)&spi_data[0], 4);
}

static int st7789v_write(const struct device *dev,
			 const uint16_t x,
			 const uint16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct st7789v_data *data = (struct st7789v_data *)dev->data;
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
	st7789v_set_mem_area(data, x, y, desc->width, desc->height);

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	st7789v_transmit(data, ST7789V_CMD_RAMWR,
			 (void *) write_data_start,
			 desc->width * ST7789V_PIXEL_SIZE * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * ST7789V_PIXEL_SIZE);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ST7789V_PIXEL_SIZE * write_h;
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
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
	struct st7789v_data *data = (struct st7789v_data *)dev->data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = data->width;
	capabilities->y_resolution = data->height;

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

static void st7789v_lcd_init(struct st7789v_data *p_st7789v)
{
	uint8_t tmp;

	st7789v_set_lcd_margins(p_st7789v, p_st7789v->x_offset,
				p_st7789v->y_offset);

	st7789v_transmit(p_st7789v, ST7789V_CMD_CMD2EN, st7789v_cmd2en_param,
			 sizeof(st7789v_cmd2en_param));

	st7789v_transmit(p_st7789v, ST7789V_CMD_PORCTRL, st7789v_porch_param,
			 sizeof(st7789v_porch_param));

	/* Digital Gamma Enable, default disabled */
	tmp = 0x00;
	st7789v_transmit(p_st7789v, ST7789V_CMD_DGMEN, &tmp, 1);

	/* Frame Rate Control in Normal Mode, default value */
	tmp = 0x0f;
	st7789v_transmit(p_st7789v, ST7789V_CMD_FRCTRL2, &tmp, 1);

	tmp = DT_INST_PROP(0, gctrl);
	st7789v_transmit(p_st7789v, ST7789V_CMD_GCTRL, &tmp, 1);

	tmp = DT_INST_PROP(0, vcom);
	st7789v_transmit(p_st7789v, ST7789V_CMD_VCOMS, &tmp, 1);

#if (DT_INST_NODE_HAS_PROP(0, vrhs) && \
	DT_INST_NODE_HAS_PROP(0, vdvs))
	tmp = 0x01;
	st7789v_transmit(p_st7789v, ST7789V_CMD_VDVVRHEN, &tmp, 1);

	tmp = DT_INST_PROP(0, vrhs);
	st7789v_transmit(p_st7789v, ST7789V_CMD_VRH, &tmp, 1);

	tmp = DT_INST_PROP(0, vdvs);
	st7789v_transmit(p_st7789v, ST7789V_CMD_VDS, &tmp, 1);
#endif

	st7789v_transmit(p_st7789v, ST7789V_CMD_PWCTRL1, st7789v_pwctrl1_param,
			 sizeof(st7789v_pwctrl1_param));

	/* Memory Data Access Control */
	tmp = DT_INST_PROP(0, mdac);
	st7789v_transmit(p_st7789v, ST7789V_CMD_MADCTL, &tmp, 1);

	/* Interface Pixel Format */
	tmp = DT_INST_PROP(0, colmod);
	st7789v_transmit(p_st7789v, ST7789V_CMD_COLMOD, &tmp, 1);

	tmp = DT_INST_PROP(0, lcm);
	st7789v_transmit(p_st7789v, ST7789V_CMD_LCMCTRL, &tmp, 1);

	tmp = DT_INST_PROP(0, gamma);
	st7789v_transmit(p_st7789v, ST7789V_CMD_GAMSET, &tmp, 1);

	st7789v_transmit(p_st7789v, ST7789V_CMD_INV_ON, NULL, 0);

	st7789v_transmit(p_st7789v, ST7789V_CMD_PVGAMCTRL, st7789v_pvgam_param,
			 sizeof(st7789v_pvgam_param));

	st7789v_transmit(p_st7789v, ST7789V_CMD_NVGAMCTRL, st7789v_nvgam_param,
			 sizeof(st7789v_nvgam_param));

	st7789v_transmit(p_st7789v, ST7789V_CMD_RAMCTRL, st7789v_ram_param,
			 sizeof(st7789v_ram_param));

	st7789v_transmit(p_st7789v, ST7789V_CMD_RGBCTRL, st7789v_rgb_param,
			 sizeof(st7789v_rgb_param));
}

static int st7789v_init(const struct device *dev)
{
	struct st7789v_data *data = (struct st7789v_data *)dev->data;

	data->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (data->spi_dev == NULL) {
		LOG_ERR("Could not get SPI device for LCD");
		return -EPERM;
	}

	data->spi_config.frequency =
		DT_INST_PROP(0, spi_max_frequency);
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = DT_INST_REG_ADDR(0);

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->cs_ctrl.gpio_dev = device_get_binding(
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	data->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctrl.gpio_dt_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0);
	data->cs_ctrl.delay = 0U;
	data->spi_config.cs = &(data->cs_ctrl);
#else
	data->spi_config.cs = NULL;
#endif

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	data->reset_gpio = device_get_binding(
			DT_INST_GPIO_LABEL(0, reset_gpios));
	if (data->reset_gpio == NULL) {
		LOG_ERR("Could not get GPIO port for display reset");
		return -EPERM;
	}

	if (gpio_pin_configure(data->reset_gpio, ST7789V_RESET_PIN,
			       GPIO_OUTPUT_INACTIVE | ST7789V_RESET_FLAGS)) {
		LOG_ERR("Couldn't configure reset pin");
		return -EIO;
	}
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	data->pm_state = DEVICE_PM_ACTIVE_STATE;
#endif

	data->cmd_data_gpio = device_get_binding(
			DT_INST_GPIO_LABEL(0, cmd_data_gpios));
	if (data->cmd_data_gpio == NULL) {
		LOG_ERR("Could not get GPIO port for cmd/DATA port");
		return -EPERM;
	}
	if (gpio_pin_configure(data->cmd_data_gpio, ST7789V_CMD_DATA_PIN,
			       GPIO_OUTPUT | ST7789V_CMD_DATA_FLAGS)) {
		LOG_ERR("Couldn't configure cmd/DATA pin");
		return -EIO;
	}

	st7789v_reset_display(data);

	st7789v_blanking_on(dev);

	st7789v_lcd_init(data);

	st7789v_exit_sleep(data);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void st7789v_enter_sleep(struct st7789v_data *data)
{
	st7789v_transmit(data, ST7789V_CMD_SLEEP_IN, NULL, 0);
}

static int st7789v_pm_control(const struct device *dev, uint32_t ctrl_command,
				 void *context, device_pm_cb cb, void *arg)
{
	int ret = 0;
	struct st7789v_data *data = (struct st7789v_data *)dev->data;

	switch (ctrl_command) {
	case DEVICE_PM_SET_POWER_STATE:
		if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			st7789v_exit_sleep(data);
			data->pm_state = DEVICE_PM_ACTIVE_STATE;
			ret = 0;
		} else {
			st7789v_enter_sleep(data);
			data->pm_state = DEVICE_PM_LOW_POWER_STATE;
			ret = 0;
		}
		break;
	case DEVICE_PM_GET_POWER_STATE:
		*((uint32_t *)context) = data->pm_state;
		break;
	default:
		ret = -EINVAL;
	}

	if (cb != NULL) {
		cb(dev, ret, context, arg);
	}
	return ret;
}
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

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

static struct st7789v_data st7789v_data = {
	.width = DT_INST_PROP(0, width),
	.height = DT_INST_PROP(0, height),
	.x_offset = DT_INST_PROP(0, x_offset),
	.y_offset = DT_INST_PROP(0, y_offset),
};

#ifndef CONFIG_DEVICE_POWER_MANAGEMENT
DEVICE_AND_API_INIT(st7789v, DT_INST_LABEL(0), &st7789v_init,
		    &st7789v_data, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &st7789v_api);
#else
DEVICE_DEFINE(st7789v, DT_INST_LABEL(0), &st7789v_init,
	      st7789v_pm_control, &st7789v_data, NULL, APPLICATION,
	      CONFIG_APPLICATION_INIT_PRIORITY, &st7789v_api);
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */
