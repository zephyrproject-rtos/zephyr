/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9xxx.h"
#include <drivers/display.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(display_ili9xxx, CONFIG_DISPLAY_LOG_LEVEL);

#include <drivers/gpio.h>
#include <sys/byteorder.h>
#include <drivers/spi.h>
#include <string.h>

#define ILI9XXX_X_MAX 320U
#define ILI9XXX_Y_MAX 240U
#define ILI9XXX_PIX_FMT_MASK \
	(PIXEL_FORMAT_RGB_888 | PIXEL_FORMAT_RGB_565 | PIXEL_FORMAT_BGR_565)

#define DT_CS_PORT        DT_INST_0_ILITEK_ILI9XXX_CS_GPIOS_CONTROLLER
#define DT_CS_PIN         DT_INST_0_ILITEK_ILI9XXX_CS_GPIOS_PIN
#define DT_CMD_DATA_PORT  DT_INST_0_ILITEK_ILI9XXX_CMD_DATA_GPIOS_CONTROLLER
#define DT_CMD_DATA_PIN   DT_INST_0_ILITEK_ILI9XXX_CMD_DATA_GPIOS_PIN
#define DT_CMD_DATA_FLAGS DT_INST_0_ILITEK_ILI9XXX_CMD_DATA_GPIOS_FLAGS
#define DT_RESET_PORT     DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_CONTROLLER
#define DT_RESET_PIN      DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_PIN
#define DT_RESET_FLAGS    DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_FLAGS

#define DT_PIX_FORMAT     (1 << DT_INST_0_ILITEK_ILI9XXX_PIXEL_FORMAT_ENUM)
#define DT_PIX_FORMAT_STR DT_INST_0_ILITEK_ILI9XXX_PIXEL_FORMAT
#define DT_HEIGHT         DT_INST_0_ILITEK_ILI9XXX_HEIGHT
#define DT_WIDTH          DT_INST_0_ILITEK_ILI9XXX_WIDTH
#define DT_X_OFFSET       DT_INST_0_ILITEK_ILI9XXX_X_OFFSET
#define DT_Y_OFFSET       DT_INST_0_ILITEK_ILI9XXX_Y_OFFSET

#define RESET_FLAGS (GPIO_OUTPUT_INACTIVE | DT_RESET_FLAGS)
#define CMD_DATA_FLAGS (GPIO_OUTPUT_ACTIVE | DT_CMD_DATA_FLAGS)

#if (DT_HEIGHT > ILI9XXX_Y_MAX) && (DT_HEIGHT > ILI9XXX_X_MAX)
#error "Please check height of \"ilitek,ili9xxx"\" device"
#endif

#if (DT_WIDTH > ILI9XXX_X_MAX) && (DT_WIDTH > ILI9XXX_Y_MAX)
#error "Please check width of \"ilitek,ili9xxx\" device"
#endif

#if (DT_X_OFFSET > (ILI9XXX_X_MAX - RES_X_MAX))
#error "Please check x-offset of \"ilitek,ili9xxx\" device"
#endif

#if (DT_Y_OFFSET > (ILI9XXX_Y_MAX - RES_Y_MAX))
#error "Please check y-offset of \"ilitek,ili9xxx\" device"
#endif

#if (ILI9XXX_PIX_FMT_MASK & DT_PIX_FORMAT == 0)
#error "Please check pixel-format of \"ilitek,ili9xxx\" device"
#endif

struct ili9xxx_data {
#ifdef DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_CONTROLLER
	struct device *rst;
#endif
	struct device *cmd;
	struct device *spi_dev;
	struct spi_config spi_config;
#ifdef DT_INST_0_ILITEK_ILI9XXX_CS_GPIOS_CONTROLLER
	struct spi_cs_control cs;
#endif
	enum display_pixel_format pixel_format;
	enum display_orientation orientation;
	u16_t height;
	u16_t width;
	u16_t x_offset;
	u16_t y_offset;
};

void ili9xxx_transmit(struct ili9xxx_data *data, u8_t cmd, void *tx_data,
		      size_t tx_len)
{
	struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	gpio_pin_set(data->cmd, DT_CMD_DATA_PIN, 1);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (tx_data != NULL) {
		tx_buf.buf = tx_data;
		tx_buf.len = tx_len;
		gpio_pin_set(data->cmd, DT_CMD_DATA_PIN, 0);
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	}
}

static void ili9xxx_reset(struct ili9xxx_data *data)
{
	LOG_DBG("Resetting display");
#ifdef DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_CONTROLLER
	k_sleep(K_MSEC(1));
	gpio_pin_set(cmd->rst, DT_RESET_PIN, 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set(cmd->rst, DT_RESET_PIN, 0);
#else
	ili9xxx_transmit(data, ILI9XXX_CMD_SOFTWARE_RESET, NULL, 0);
#endif
	k_sleep(K_MSEC(5));
}


static void ili9xxx_exit_sleep(struct ili9xxx_data *data)
{
	ili9xxx_transmit(data, ILI9XXX_CMD_EXIT_SLEEP, NULL, 0);
	k_sleep(K_MSEC(120));
}

static void ili9xxx_set_mem_area(struct ili9xxx_data *data, const u16_t x,
				 const u16_t y, const u16_t w, const u16_t h)
{
	const u16_t real_x = x + data->x_offset;
	const u16_t real_y = y + data->y_offset;
	u16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(real_x);
	spi_data[1] = sys_cpu_to_be16(real_x + w - 1);
	ili9xxx_transmit(data, ILI9XXX_CMD_COLUMN_ADDR, &spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(real_y);
	spi_data[1] = sys_cpu_to_be16(real_y + h - 1);
	ili9xxx_transmit(data, ILI9XXX_CMD_PAGE_ADDR, &spi_data[0], 4);
}

static int ili9xxx_write(const struct device *dev, const u16_t x,
			 const u16_t y,
			 const struct display_buffer_descriptor *desc,
			 const void *buf)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;
	const u8_t *write_data_start = (u8_t *) buf;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs;
	u16_t write_cnt;
	u16_t nbr_of_writes;
	u16_t write_h;
	const u8_t pix_size = 2 + (data->pixel_format == PIXEL_FORMAT_RGB_888);

	__ASSERT(desc->width <= desc->pitch, "Pitch is smaller then width");
	__ASSERT((desc->pitch * pix_size * desc->height) <= desc->bu_size,
			"Input buffer to small");

	LOG_DBG("Writing %dx%d (w,h) @ %dx%d (x,y)", desc->width, desc->height,
			x, y);
	ili9xxx_set_mem_area(data, x, y, desc->width, desc->height);

	if (desc->pitch > desc->width) {
		write_h = 1U;
		nbr_of_writes = desc->height;
	} else {
		write_h = desc->height;
		nbr_of_writes = 1U;
	}

	ili9xxx_transmit(data, ILI9XXX_CMD_MEM_WRITE,
			 (void *) write_data_start,
			 desc->width * pix_size * write_h);

	tx_bufs.buffers = &tx_buf;
	tx_bufs.count = 1;

	write_data_start += (desc->pitch * pix_size);
	for (write_cnt = 1U; write_cnt < nbr_of_writes; ++write_cnt) {
		tx_buf.buf = (void *)write_data_start;
		tx_buf.len = desc->width * pix_size * write_h;
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
		write_data_start += (desc->pitch * pix_size);
	}

	return 0;
}

static int ili9xxx_read(const struct device *dev, const u16_t x,
			const u16_t y,
			const struct display_buffer_descriptor *desc,
			void *buf)
{
	LOG_ERR("Reading not supported");
	return -ENOTSUP;
}

static void *ili9xxx_get_framebuffer(const struct device *dev)
{
	LOG_ERR("Direct framebuffer access not supported");
	return NULL;
}

static int ili9xxx_blanking_off(const struct device *dev)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;

	LOG_DBG("Turning display blanking off");
	ili9xxx_transmit(data, ILI9XXX_CMD_DISPLAY_ON, NULL, 0);
	return 0;
}

static int ili9xxx_blanking_on(const struct device *dev)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;

	LOG_DBG("Turning display blanking on");
	ili9xxx_transmit(data, ILI9XXX_CMD_DISPLAY_OFF, NULL, 0);
	return 0;
}

static int ili9xxx_set_brightness(const struct device *dev,
				  const u8_t brightness)
{
	LOG_WRN("Set brightness not implemented");
	return -ENOTSUP;
}

static int ili9xxx_set_contrast(const struct device *dev, const u8_t contrast)
{
	LOG_ERR("Set contrast not supported");
	return -ENOTSUP;
}

static int ili9xxx_set_pixel_format(const struct device *dev,
				    const enum display_pixel_format
				    pixel_format)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;
	u8_t arg;

	LOG_DBG("change request %d -> %d", data->pixel_format, pixel_format);
	if (data->pixel_format == pixel_format) {
		return 0;
	}
	switch (pixel_format) {
	case PIXEL_FORMAT_RGB_565:
	case PIXEL_FORMAT_BGR_565:
		arg = ILI9XXX_DATA_PIXEL_FORMAT_MCU_16_BIT
		    | ILI9XXX_DATA_PIXEL_FORMAT_RGB_16_BIT;
		break;
	case PIXEL_FORMAT_RGB_888:
		arg = ILI9XXX_DATA_PIXEL_FORMAT_MCU_18_BIT
		    | ILI9XXX_DATA_PIXEL_FORMAT_RGB_18_BIT;
		break;
	default:
		LOG_ERR("Pixel format 0x%02X not supported", pixel_format);
		return -ENOTSUP;
	}

	ili9xxx_transmit(data, ILI9XXX_CMD_PIXEL_FORMAT_SET, &arg, 1);
	data->pixel_format = pixel_format;
	return 0;
}

static int ili9xxx_set_orientation(const struct device *dev,
				   const enum display_orientation orientation)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;

	LOG_DBG("change request %d -> %d", data->orientation, orientation);
	if (data->orientation == orientation) {
		return 0;
	}
	LOG_ERR("Changing display orientation not implemented");
	return -ENOTSUP;
}

static void ili9xxx_get_capabilities(const struct device *dev,
				     struct display_capabilities *capabilities)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;

	memset(capabilities, 0, sizeof(struct display_capabilities));
	capabilities->x_resolution = data->width;
	capabilities->y_resolution = data->height;
	capabilities->supported_pixel_formats = ILI9XXX_PIX_FMT_MASK;
	capabilities->current_pixel_format = data->pixel_format;
	capabilities->current_orientation = data->orientation;
}

static int ili9xxx_init(struct device *dev)
{
	struct ili9xxx_data *data = (struct ili9xxx_data *)dev->driver_data;

	LOG_DBG("Initializing display driver");

	data->height = DT_HEIGHT;
	data->width = DT_WIDTH;
	data->x_offset = DT_X_OFFSET;
	data->y_offset = DT_Y_OFFSET;

	data->orientation = DISPLAY_ORIENTATION_NORMAL;

	data->spi_dev = device_get_binding(DT_INST_0_ILITEK_ILI9XXX_BUS_NAME);
	if (data->spi_dev == NULL) {
		LOG_ERR("No '%s' device", DT_INST_0_ILITEK_ILI9XXX_BUS_NAME);
		return -EPERM;
	}

	data->spi_config.frequency = DT_INST_0_ILITEK_ILI9XXX_SPI_MAX_FREQUENCY;
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = DT_INST_0_ILITEK_ILI9XXX_BASE_ADDRESS;

#ifdef DT_INST_0_ILITEK_ILI9XXX_CS_GPIOS_CONTROLLER
	data->cs.gpio_dev = device_get_binding(DT_CS_PORT);
	if (!data->cs.gpio_dev) {
		LOG_ERR("No '%s' device", DT_CS_PORT);
		return -EPERM;
	}
	data->cs.gpio_pin = DT_CS_PIN;
	data->cs.delay = 0U;
	data->spi_config.cs = &(data->cs);
#else
	data->spi_config.cs = NULL;
#endif

#ifdef DT_INST_0_ILITEK_ILI9XXX_RESET_GPIOS_CONTROLLER
	data->rst = device_get_binding(DT_RESET_PORT);
	if (data->rst == NULL) {
		LOG_ERR("No '%s' device", DT_RESET_PORT);
		return -EPERM;
	}

	if (gpio_pin_configure(data->rst, DT_RESET_PIN, RESET_FLAGS)) {
		LOG_ERR("Couldn't configure reset pin");
		return -EIO;
	}
#endif

	data->cmd = device_get_binding(DT_CMD_DATA_PORT);
	if (data->cmd == NULL) {
		LOG_ERR("No '%s' device", DT_CMD_DATA_PORT);
		return -EPERM;
	}

	if (gpio_pin_configure(data->cmd, DT_CMD_DATA_PIN, CMD_DATA_FLAGS)) {
		LOG_ERR("Couldn't configure data/command pin");
		return -EIO;
	}

	LOG_DBG("Initializing LCD");

	ili9xxx_reset(data);

	ili9xxx_blanking_on(dev);

	ili9xxx_lcd_init(data);

	if (ili9xxx_set_pixel_format(dev, DT_PIX_FORMAT)) {
		LOG_ERR("Pixel format '%s' is unsupported", DT_PIX_FORMAT_STR);
		return -EINVAL;
	}

	ili9xxx_exit_sleep(data);

	return 0;
}

static const struct display_driver_api ili9xxx_api = {
	.blanking_on = ili9xxx_blanking_on,
	.blanking_off = ili9xxx_blanking_off,
	.write = ili9xxx_write,
	.read = ili9xxx_read,
	.get_framebuffer = ili9xxx_get_framebuffer,
	.set_brightness = ili9xxx_set_brightness,
	.set_contrast = ili9xxx_set_contrast,
	.get_capabilities = ili9xxx_get_capabilities,
	.set_pixel_format = ili9xxx_set_pixel_format,
	.set_orientation = ili9xxx_set_orientation,
};

static struct ili9xxx_data ili9xxx_data;

DEVICE_AND_API_INIT(ili9xxx, DT_INST_0_ILITEK_ILI9XXX_LABEL, &ili9xxx_init,
		    &ili9xxx_data, NULL, APPLICATION,
		    CONFIG_APPLICATION_INIT_PRIORITY, &ili9xxx_api);
