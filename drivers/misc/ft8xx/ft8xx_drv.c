/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ft8xx_drv.h"

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME ft8xx_drv
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DT_DRV_COMPAT ftdi_ft800
#define NODE_ID DT_INST(0, DT_DRV_COMPAT)

/* SPI device */
static const struct device *spi_ft8xx_dev;
static struct spi_cs_control cs_ctrl;
static const struct spi_config spi_cfg = {
	.frequency = 8000000UL,
	.operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER,
	.cs = &cs_ctrl,
};

/* GPIO int line */
#define IRQ_PIN DT_GPIO_PIN(NODE_ID, irq_gpios)
static const struct device *int_ft8xx_dev;

static struct gpio_callback irq_cb_data;

__weak void ft8xx_drv_irq_triggered(const struct device *dev,
				     struct gpio_callback *cb, uint32_t pins)
{
	/* Intentionally empty */
}

/* Protocol details */
#define ADDR_SIZE 3
#define DUMMY_READ_SIZE 1
#define COMMAND_SIZE 3
#define MAX_READ_LEN (UINT16_MAX - ADDR_SIZE - DUMMY_READ_SIZE)
#define MAX_WRITE_LEN (UINT16_MAX - ADDR_SIZE)

#define READ_OP 0x00
#define WRITE_OP 0x80
#define COMMAND_OP 0x40

static void insert_addr(uint32_t addr, uint8_t *buff)
{
	buff[0] = (addr >> 16) & 0x3f;
	buff[1] = (addr >> 8) & 0xff;
	buff[2] = (addr) & 0xff;
}

int ft8xx_drv_init(void)
{
	int ret;

	cs_ctrl = (struct spi_cs_control){
		.gpio_dev = device_get_binding(
				DT_SPI_DEV_CS_GPIOS_LABEL(NODE_ID)),
		.gpio_pin = DT_SPI_DEV_CS_GPIOS_PIN(NODE_ID),
		.gpio_dt_flags = DT_SPI_DEV_CS_GPIOS_FLAGS(NODE_ID),
		.delay = 0,
	};

	spi_ft8xx_dev = device_get_binding(DT_BUS_LABEL(NODE_ID));
	if (!spi_ft8xx_dev) {
		return -ENODEV;
	}

	/* TODO: Verify if such entry in DTS is present.
	 * If not, use polling mode.
	 */
	int_ft8xx_dev = device_get_binding(DT_GPIO_LABEL(NODE_ID, irq_gpios));
	if (!int_ft8xx_dev) {
		return -ENODEV;
	}

	ret = gpio_pin_configure(int_ft8xx_dev, IRQ_PIN,
			GPIO_INPUT | DT_GPIO_FLAGS(NODE_ID, irq_gpios));
	if (ret != 0) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure(int_ft8xx_dev, IRQ_PIN,
			GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&irq_cb_data, ft8xx_drv_irq_triggered, BIT(IRQ_PIN));
	gpio_add_callback(int_ft8xx_dev, &irq_cb_data);

	return 0;
}

int ft8xx_drv_write(uint32_t address, const uint8_t *data, unsigned int length)
{
	int ret;
	uint8_t addr_buf[ADDR_SIZE];

	insert_addr(address, addr_buf);
	addr_buf[0] |= WRITE_OP;

	struct spi_buf tx[] = {
		{
			.buf = addr_buf,
			.len = sizeof(addr_buf),
		},
		{
			/* Discard const, it is implicit for TX buffer */
			.buf = (uint8_t *)data,
			.len = length,
		},
	};

	struct spi_buf_set tx_bufs = {
		.buffers = tx,
		.count = 2,
	};

	ret = spi_write(spi_ft8xx_dev, &spi_cfg, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("SPI write error: %d", ret);
	}

	return ret;
}

int ft8xx_drv_read(uint32_t address, uint8_t *data, unsigned int length)
{
	int ret;
	uint8_t dummy_read_buf[ADDR_SIZE + DUMMY_READ_SIZE];
	uint8_t addr_buf[ADDR_SIZE];

	insert_addr(address, addr_buf);
	addr_buf[0] |= READ_OP;

	struct spi_buf tx = {
		.buf = addr_buf,
		.len = sizeof(addr_buf),
	};

	struct spi_buf_set tx_bufs = {
		.buffers = &tx,
		.count = 1,
	};

	struct spi_buf rx[] = {
		{
			.buf = dummy_read_buf,
			.len = sizeof(dummy_read_buf),
		},
		{
			.buf = data,
			.len = length,
		},
	};

	struct spi_buf_set rx_bufs = {
		.buffers = rx,
		.count = 2,
	};

	ret = spi_transceive(spi_ft8xx_dev, &spi_cfg, &tx_bufs, &rx_bufs);
	if (ret < 0) {
		LOG_ERR("SPI transceive error: %d", ret);
	}

	return ret;
}

int ft8xx_drv_command(uint8_t command)
{
	int ret;
	/* Most commands include COMMAND_OP bit. ACTIVE power mode command is
	 * an exception with value 0x00.
	 */
	uint8_t cmd_buf[COMMAND_SIZE] = {command, 0, 0};

	struct spi_buf tx = {
		.buf = cmd_buf,
		.len = sizeof(cmd_buf),
	};

	struct spi_buf_set tx_bufs = {
		.buffers = &tx,
		.count = 1,
	};

	ret = spi_write(spi_ft8xx_dev, &spi_cfg, &tx_bufs);
	if (ret < 0) {
		LOG_ERR("SPI command error: %d", ret);
	}

	return ret;
}
