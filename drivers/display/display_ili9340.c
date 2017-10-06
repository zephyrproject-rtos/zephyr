/* Copyright (c) 2017 dXplore
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9340.h"
#include <drivers/display/ili9340.h>

#define SYS_LOG_DOMAIN "ILI9340"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ILI9340_LEVEL
#include <logging/sys_log.h>

#include <gpio.h>
#include <misc/byteorder.h>
#include <spi.h>

struct ili9340_data {
	struct device *reset_gpio;
	struct device *command_data_gpio;
	struct device *spi_dev;
	struct spi_config spi_config;
#ifdef CONFIG_ILI9340_GPIO_CS
	struct spi_cs_control cs_ctrl;
#endif
};

#define ILI9340_CMD_DATA_PIN_COMMAND 0
#define ILI9340_CMD_DATA_PIN_DATA 1

static void ili9340_exit_sleep(struct ili9340_data *data);
static void ili9340_set_mem_area(struct ili9340_data *data, const u16_t x,
				 const u16_t y, const u16_t w, const u16_t h);
static int ili9340_init(struct device *dev);

int ili9340_init(struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	SYS_LOG_DBG("Initializing display driver");

	data->spi_dev = device_get_binding(CONFIG_ILI9340_SPI_DEV_NAME);
	if (data->spi_dev == NULL) {
		SYS_LOG_ERR("Could not get SPI device for ILI9340");
		return -EPERM;
	}

	data->spi_config.frequency = CONFIG_ILI9340_SPI_FREQ;
	data->spi_config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	data->spi_config.slave = CONFIG_ILI9340_SPI_SLAVE_NUMBER;

#ifdef CONFIG_ILI9340_GPIO_CS
	data->cs_ctrl.gpio_dev =
		device_get_binding(CONFIG_ILI9340_CS_GPIO_PORT_NAME);
	data->cs_ctrl.gpio_pin = CONFIG_ILI9340_CS_GPIO_PIN;
	data->cs_ctrl.delay = 0;
	data->spi_config.cs = &(data->cs_ctrl);
#else
	data->spi_config.cs = NULL;
#endif

	data->reset_gpio =
	    device_get_binding(CONFIG_ILI9340_RESET_GPIO_PORT_NAME);
	if (data->reset_gpio == NULL) {
		SYS_LOG_ERR("Could not get GPIO port for ILI9340 reset");
		return -EPERM;
	}

	gpio_pin_configure(data->reset_gpio, CONFIG_ILI9340_RESET_PIN,
			   GPIO_DIR_OUT);

	data->command_data_gpio =
	    device_get_binding(CONFIG_ILI9340_CMD_DATA_GPIO_PORT_NAME);
	if (data->command_data_gpio == NULL) {
		SYS_LOG_ERR("Could not get GPIO port for ILI9340 command/data");
		return -EPERM;
	}

	gpio_pin_configure(data->command_data_gpio, CONFIG_ILI9340_CMD_DATA_PIN,
			   GPIO_DIR_OUT);

	SYS_LOG_DBG("Resetting display driver");
	gpio_pin_write(data->reset_gpio, CONFIG_ILI9340_RESET_PIN, 1);
	k_sleep(1);
	gpio_pin_write(data->reset_gpio, CONFIG_ILI9340_RESET_PIN, 0);
	k_sleep(1);
	gpio_pin_write(data->reset_gpio, CONFIG_ILI9340_RESET_PIN, 1);
	k_sleep(5);

	SYS_LOG_DBG("Initializing LCD");
	ili9340_lcd_init(data);

	SYS_LOG_DBG("Exiting sleep mode");
	ili9340_exit_sleep(data);

	/* device_get_binding checks if driver_api is not zero before checking
	 * device name.
	 * So just set driver_api to 1 else the function call will fail
	 */
	dev->driver_api = (void *)1;

	return 0;
}

void ili9340_write_pixel(const struct device *dev, const u16_t x, const u16_t y,
			 const u8_t r, const u8_t g, const u8_t b)
{
	u8_t rgb_data[] = {r, g, b};

	SYS_LOG_DBG("Writing pixel @ %dx%d (x,y)", x, y);
	ili9340_write_bitmap(dev, x, y, 1, 1, &rgb_data[0]);
}

void ili9340_write_bitmap(const struct device *dev, const u16_t x,
			  const u16_t y, const u16_t w, const u16_t h,
			  const u8_t *rgb_data)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	SYS_LOG_DBG("Writing %dx%d (w,h) bitmap @ %dx%d (x,y)", w, h, x, y);
	ili9340_set_mem_area(data, x, y, w, h);
	ili9340_transmit(data, ILI9340_CMD_MEM_WRITE, (void *)rgb_data,
			 3 * w * h);
}

void ili9340_display_on(struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	SYS_LOG_DBG("Turning display on");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_ON, NULL, 0);
}

void ili9340_display_off(struct device *dev)
{
	struct ili9340_data *data = (struct ili9340_data *)dev->driver_data;

	SYS_LOG_DBG("Turning display off");
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_OFF, NULL, 0);
}

void ili9340_transmit(struct ili9340_data *data, u8_t cmd, void *tx_data,
		      size_t tx_len)
{
	struct spi_buf tx_buf = {.buf = &cmd, .len = 1};
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1};

	gpio_pin_write(data->command_data_gpio, CONFIG_ILI9340_CMD_DATA_PIN,
		       ILI9340_CMD_DATA_PIN_COMMAND);
	spi_write(data->spi_dev, &data->spi_config, &tx_bufs);

	if (tx_data != NULL) {
		tx_buf.buf = tx_data;
		tx_buf.len = tx_len;
		gpio_pin_write(data->command_data_gpio,
			       CONFIG_ILI9340_CMD_DATA_PIN,
			       ILI9340_CMD_DATA_PIN_DATA);
		spi_write(data->spi_dev, &data->spi_config, &tx_bufs);
	}
}

void ili9340_exit_sleep(struct ili9340_data *data)
{
	ili9340_transmit(data, ILI9340_CMD_EXIT_SLEEP, NULL, 0);
	k_sleep(120);
}

void ili9340_set_mem_area(struct ili9340_data *data, const u16_t x,
			  const u16_t y, const u16_t w, const u16_t h)
{
	u16_t spi_data[2];

	spi_data[0] = sys_cpu_to_be16(x);
	spi_data[1] = sys_cpu_to_be16(x + w - 1);
	ili9340_transmit(data, ILI9340_CMD_COLUMN_ADDR, &spi_data[0], 4);

	spi_data[0] = sys_cpu_to_be16(y);
	spi_data[1] = sys_cpu_to_be16(y + h - 1);
	ili9340_transmit(data, ILI9340_CMD_PAGE_ADDR, &spi_data[0], 4);
}

static struct ili9340_data ili9340_data;

DEVICE_INIT(ili9340, CONFIG_ILI9340_DEV_NAME, &ili9340_init, &ili9340_data,
	    NULL, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
