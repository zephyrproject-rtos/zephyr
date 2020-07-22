/*
 * Copyright (c) 2020
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ilitek_ili9163

#include "display_ili9163.h"
#include <drivers/display.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9163, CONFIG_DISPLAY_LOG_LEVEL);

#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/spi.h>
#include <string.h>

#define ILI9163_X_MAX 162U
#define ILI9163_Y_MAX 132U

/* The number of bytes taken by a RGB pixel */
#ifdef CONFIG_ILI9163_RGB565
#define ILI9163_RGB_SIZE 2U
#elif defined(CONFIG_ILI9163_RGB888)
#define ILI9163_RGB_SIZE 3U
#else
#error "Unsupported pixel format"
#endif

#define RES_HOR DT_INST_PROP(0, width)
#define RES_VER DT_INST_PROP(0, height)

#if (RES_VER > ILI9163_Y_MAX)
#error "Please check height of \"ilitek,ili9163\" device"
#endif

#if (RES_HOR > ILI9163_X_MAX)
#error "Please check height of \"ilitek,ili9163\" device"
#endif

#if defined(DT_ILITEK_ILI9163_LCD_X_OFFSET) && \
	(DT_ILITEK_ILI9163_LCD_X_OFFSET > (ILI9163_X_MAX - RES_HOR))
#error "Please check x-offset of \"ilitek,ili9163\" device"
#endif

#if defined(DT_ILITEK_ILI9163_LCD_Y_OFFSET) && \
	(DT_ILITEK_ILI9163_LCD_Y_OFFSET > (ILI9163_Y_MAX - RES_VER))
#error "Please check y-offset of \"ilitek,ili9163\" device"
#endif

struct ili9163_data {

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	struct device *rst;
#endif
	struct device *cmd;
	struct device *spi_dev;
	struct spi_config spi_config;
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	struct spi_cs_control cs_ctrl;
#endif
	enum display_orientation orientation;
	u8_t height;
	u8_t width;
	u8_t x_offset;
	u8_t y_offset;
};

static void ili9163_transmit(struct ili9163_data *data, u8_t cmd,
				void *tx_data, size_t tx_len)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(data->cmd,
			DT_INST_GPIO_PIN(0, cmd_data_gpios), 0);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (!tx_data)
		return;
	tx_buf.buf = tx_data;
	tx_buf.len = tx_len;
	gpio_pin_set(data->cmd,
			   DT_INST_GPIO_PIN(0, cmd_data_gpios), 1);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
}

static inline void ili9163_reset(struct ili9163_data *data)
{
	LOG_DBG("Resetting display");

#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	gpio_pin_set(data->rst, DT_INST_GPIO_PIN(0, reset_gpios), 1);
	k_busy_wait(5);
	gpio_pin_set(data->rst, DT_INST_GPIO_PIN(0, reset_gpios), 0);
	k_busy_wait(10);
	gpio_pin_set(data->rst, DT_INST_GPIO_PIN(0, reset_gpios), 1);
	k_sleep(K_MSEC(120));
#else
	ili9163_transmit(data, ILI9163_CMD_SOFTWARE_RESET, NULL, 0);
	k_sleep(K_MSEC(120));
#endif
}

static inline void ili9163_exit_sleep(struct ili9163_data *data)
{
	ili9163_transmit(data, ILI9163_CMD_EXIT_SLEEP, NULL, 0);
	k_sleep(K_MSEC(5));
}

static inline void ili9163_set_mem_area(struct ili9163_data *data, u16_t x,
					u16_t y, u16_t w, u16_t h)
{
	u16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1);
	ili9163_transmit(data, ILI9163_CMD_COLUMN_ADDR, &spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1);
	ili9163_transmit(data, ILI9163_CMD_PAGE_ADDR, &spi_data[0], 4);
}

static int ili9163_write(const struct device *dev, const u16_t x,
			 const u16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;
	const u8_t *write_data_start = (u8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	u16_t write_cnt;
	u16_t nbr_of_writes;
	u16_t write_h;
	const u16_t line_size = desc->pitch * ILI9163_RGB_SIZE;

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((line_size * desc->height) <= desc->buf_size,
			"Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
		x, y);
	ili9163_set_mem_area(data, x, y, desc->width, desc->height);

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ili9163_transmit(data, ILI9163_CMD_MEM_WRITE,
			 (void *) write_data_start,
			 desc->width * ILI9163_RGB_SIZE * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += line_size;
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * ILI9163_RGB_SIZE * write_h;
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		write_data_start += line_size;
	}

	return 0;
}

static int ili9163_read(const struct device *dev, const u16_t x,
			const u16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_WRN("Reading not supported");
	return -ENOTSUP;
}

static void *ili9163_get_framebuffer(const struct device *dev)
{
	LOG_WRN("Direct framebuffer access not supported");
	return NULL;
}

static int ili9163_blanking_off(const struct device *dev)
{
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;

	LOG_DBG("Turning display blanking off");
	ili9163_transmit(data, ILI9163_CMD_DISPLAY_ON, NULL, 0);
	return 0;
}

static int ili9163_blanking_on(const struct device *dev)
{
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;

	LOG_DBG("Turning display blanking on");
	ili9163_transmit(data, ILI9163_CMD_DISPLAY_OFF, NULL, 0);
	return 0;
}

static int ili9163_set_brightness(const struct device *dev,
				  const u8_t brightness)
{
	LOG_WRN("Set brightness not supported");
	return -ENOTSUP;
}

static int ili9163_set_contrast(const struct device *dev, const u8_t contrast)
{
	LOG_WRN("Set contrast not supported");
	return -ENOTSUP;
}

static int ili9163_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format
				    pixel_format)
{
	bool valid = false;

	valid |= IS_ENABLED(CONFIG_ILI9163_RGB565)
			&& pixel_format == PIXEL_FORMAT_RGB_565;
	valid |= IS_ENABLED(CONFIG_ILI9163_RGB888)
			&& pixel_format == PIXEL_FORMAT_RGB_888;
	if (valid) {
		return 0;
	}

	LOG_WRN("Pixel format change not supported");
	return -ENOTSUP;
}

static int ili9163_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	u8_t cmd;
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;

	if (orientation == data->orientation) {
		return 0;
	}
	switch (orientation) {
	case DISPLAY_ORIENTATION_NORMAL:
		cmd = 0;
		break;
	case DISPLAY_ORIENTATION_ROTATED_90:
		cmd = ILI9163_DATA_MEM_ACCESS_CTRL_MV
			| ILI9163_DATA_MEM_ACCESS_CTRL_MY;
		break;
	case DISPLAY_ORIENTATION_ROTATED_180:
		cmd = ILI9163_DATA_MEM_ACCESS_CTRL_MX
			| ILI9163_DATA_MEM_ACCESS_CTRL_MY;
		break;
	case DISPLAY_ORIENTATION_ROTATED_270:
		cmd = ILI9163_DATA_MEM_ACCESS_CTRL_MV
			| ILI9163_DATA_MEM_ACCESS_CTRL_MX;
		break;
	default:
		return -EINVAL;
	}
#if DT_INST_NODE_HAS_PROP(0, madc)
	cmd |= DT_INST_PROP(0, madc);
#else
	cmd |= ILI9163_DATA_MEM_ACCESS_CTRL_BGR;
#endif
	data->orientation = orientation;
	ili9163_transmit(data, ILI9163_CMD_MEM_ACCESS_CTRL, &cmd, sizeof(cmd));
	return 0;
}

static void ili9163_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = RES_HOR;
	capabilities->y_resolution = RES_VER;
	if (IS_ENABLED(CONFIG_ILI9163_RGB565)) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_565;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_565;
	} else if (IS_ENABLED(CONFIG_ILI9163_RGB888)) {
		capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_888;
		capabilities->current_pixel_format = PIXEL_FORMAT_RGB_888;
	}
	capabilities->current_orientation = data->orientation;
}

static inline void ili9163_lcd_init(struct ili9163_data *data)
{
	u8_t cmd = 0;
	if (IS_ENABLED(CONFIG_ILI9163_RGB565)) {
		cmd = ILI9163_DATA_PIXEL_FORMAT_MCU_16_BIT |
			  ILI9163_DATA_PIXEL_FORMAT_RGB_16_BIT;
	} else if (IS_ENABLED(CONFIG_ILI9163_RGB888)) {
		cmd = ILI9163_DATA_PIXEL_FORMAT_MCU_18_BIT |
			  ILI9163_DATA_PIXEL_FORMAT_RGB_18_BIT;
	}
	ili9163_transmit(data, ILI9163_CMD_PIXEL_FORMAT_SET, &cmd, sizeof(cmd));

#if DT_INST_NODE_HAS_PROP(0, madc)
    cmd |= DT_INST_PROP(0, madc);
#else
	cmd = ILI9163_DATA_MEM_ACCESS_CTRL_BGR;
#endif
	ili9163_transmit(data, ILI9163_CMD_MEM_ACCESS_CTRL, &cmd, sizeof(cmd));

	ili9163_transmit(data, ILI9163_CMD_MOD_NORMAL, NULL, 0);
}

static int ili9163_init(struct device *dev)
{
	struct ili9163_data *data = (struct ili9163_data *)dev->driver_data;

	data->orientation = DISPLAY_ORIENTATION_NORMAL;

	LOG_DBG("Initializing display driver");

	data->spi_dev = device_get_binding(DT_INST_BUS_LABEL(0));
	if (data->spi_dev == NULL) {
		LOG_ERR("No '%s' device", DT_INST_BUS_LABEL(0));
		return -EPERM;
	}

	data->spi_config.frequency = DT_INST_PROP(0, spi_max_frequency);
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = DT_INST_REG_ADDR(0);


#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	data->cs_ctrl.gpio_dev = device_get_binding(
			DT_INST_SPI_DEV_CS_GPIOS_LABEL(0));
	data->cs_ctrl.gpio_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0);
	data->cs_ctrl.delay = 0U;
	data->spi_config.cs = &(data->cs_ctrl);
#else
	data->spi_config.cs = NULL;
#endif


#if DT_INST_NODE_HAS_PROP(0, reset_gpios)
	data->rst = device_get_binding(
	        DT_INST_GPIO_LABEL(0, reset_gpios));
	if (data->rst == NULL) {
		LOG_ERR("No '%s' device",
			DT_INST_GPIO_LABEL(0, reset_gpios));
		return -EPERM;
	}

	gpio_pin_configure(data->rst, DT_INST_GPIO_PIN(0, reset_gpios),
			   GPIO_OUTPUT);
#endif

	data->cmd = device_get_binding(
			DT_INST_GPIO_LABEL(0, cmd_data_gpios));
	if (data->cmd == NULL) {
		LOG_ERR("No '%s' device",
			DT_INST_GPIO_LABEL(0, cmd_data_gpios));
		return -EPERM;
	}

	gpio_pin_configure(data->cmd,
		DT_INST_GPIO_PIN(0, cmd_data_gpios), GPIO_OUTPUT);

	LOG_DBG("Initializing LCD");

	ili9163_reset(data);

	ili9163_blanking_on(dev);

	ili9163_lcd_init(data);

	ili9163_exit_sleep(data);

	return 0;
}

static const struct display_driver_api ili9163_api = {
	.blanking_on = ili9163_blanking_on,
	.blanking_off = ili9163_blanking_off,
	.write = ili9163_write,
	.read = ili9163_read,
	.get_framebuffer = ili9163_get_framebuffer,
	.set_brightness = ili9163_set_brightness,
	.set_contrast = ili9163_set_contrast,
	.get_capabilities = ili9163_get_capabilities,
	.set_pixel_format = ili9163_set_pixel_format,
	.set_orientation = ili9163_set_orientation,
};

static struct ili9163_data ili9163_data = {
	.height = RES_VER,
	.width = RES_HOR,

#if DT_INST_NODE_HAS_PROP(0, x_offset)
	.x_offset = DT_INST_PROP(0, x_offset),
#endif
#if DT_INST_NODE_HAS_PROP(0, y_offset)
	.y_offset = DT_INST_PROP(0, y_offset),
#endif
};

DEVICE_AND_API_INIT(ili9163, DT_INST_LABEL(0), &ili9163_init,
		    &ili9163_data, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ili9163_api);
