/*
 * Copyright (c) 2017-2018 Antmicro Ltd <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <misc/printk.h>
#include <i2c.h>
#include <spi.h>

#include "i2c_sc18is600.h"

#define I2C_CS	0

#define I2C_CLOCK_REG	2
#define I2C_TO_REG	3
#define I2C_STATUS_REG	4
#define I2C_ADDR_REG 5

#define I2C_STATUS_SUCCESS		0xF0
#define I2C_STATUS_NO_ACK		0xF1
#define I2C_STATUS_NACK			0xF2
#define I2C_STATUS_BUSY			0xF3
#define I2C_STATUS_TIMEOUT		0xF8
#define I2C_STATUS_INVALID_COUNT	0xF9

#define WRITE_N_BYTES_TO_DEVICE		0x00
#define READ_N_BYTES_FROM_DEVICE	0x01
#define READ_AFTER_WRITE_DEVICE		0x02

#define READ_BRIDGE_RX_BUFFER		0x06
#define WRITE_TO_BRIDGE_REGISTER	0x20
#define READ_FROM_BRIDGE_REGISTER	0x21

struct i2c_sc18is600_runtime {
	struct device *spi_d;
	struct spi_config spi_c;
	u32_t i2c_config;
};

struct i2c_sc18is600_config {};

static struct i2c_sc18is600_runtime i2c_sc18is600_0_runtime;
static struct i2c_sc18is600_config i2c_sc18is600_0_config;


int i2c_sc18is600_spi_send_buffer(struct device *dev, u8_t *data, size_t len)
{
	struct i2c_sc18is600_runtime *run_data =
		(struct i2c_sc18is600_runtime *) dev->driver_data;

	struct spi_buf tx_buf = {
		.buf = data,
		.len = len,
	};

	int err;

	err = spi_write(&(run_data->spi_c), &tx_buf, 1);
	if (err) {
		printk("spi_write failed\n");
	}
	return err;
}

int i2c_sc18is600_spi_transceive_buffer(struct device *dev, u8_t *tx_data,
		size_t tx_len, u8_t *rx_data, size_t rx_len)
{
	struct i2c_sc18is600_runtime *run_data =
		(struct i2c_sc18is600_runtime *) dev->driver_data;

	struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = tx_len,
	};

	struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = rx_len,
	};

	int err;

	err = spi_transceive(&(run_data->spi_c), &tx_buf, 1, &rx_buf, 1);
	if (err) {
		printk("spi_transceive failed\n");
	}

	return err;
}

void i2c_sc18is600_spi_setup(struct device *i2c_dev)
{
	struct i2c_sc18is600_runtime *data =
		(struct i2c_sc18is600_runtime *) i2c_dev->driver_data;
	data->spi_c.dev = data->spi_d;
	data->spi_c.frequency = 1000000UL;
	/* 8-bit data, polarity (inactive state of SCK is logical 1),
	 * and phase (shift on leading edge, sample on trailing edge)
	 */
	data->spi_c.operation = (8 << 5) | (3 << 1);
	data->spi_c.slave = I2C_CS;
	data->spi_c.cs = NULL;
}

u8_t i2c_sc18is600_read_from_bridge_register(struct device *dev, u8_t reg)
{
	size_t n = 8;
	u8_t rx_buf[n];
	u8_t tx_buf[n];

	tx_buf[0] = READ_FROM_BRIDGE_REGISTER;
	tx_buf[1] = reg;
	for (int i = 2; i < n; i++)
		tx_buf[i] = 0xFF;
	i2c_sc18is600_spi_transceive_buffer(dev, tx_buf, n, rx_buf, n);
	return rx_buf[2];
}

void i2c_sc18is600_write_to_bridge_register(struct device *dev, u8_t reg,
		u8_t val)
{
	u8_t buf[3] = {WRITE_TO_BRIDGE_REGISTER, reg, val};

	i2c_sc18is600_spi_send_buffer(dev, buf, 3);
}

static inline void i2c_sc18is600_wait_for_end(struct device *dev)
{
	while (i2c_sc18is600_read_from_bridge_register(dev, I2C_STATUS_REG)
			!= I2C_STATUS_SUCCESS) {
		printk(".");
	}
}

void i2c_sc18is600_dump_bridge_registers(struct device *dev)
{
	int ret;

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x00);
	printk("reg 0x00 (IOCONFIG): 0x%02X\n", ret);

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x01);
	printk("reg 0x01 (IOSTATE):  0x%02X\n", ret);

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x02);
	printk("reg 0x02 (I2C CLK):  0x%02X\n", ret);

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x03);
	printk("reg 0x03 (I2C TO):   0x%02X\n", ret);

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x04);
	printk("reg 0x04 (I2C STAT): 0x%02X\n", ret);

	ret = i2c_sc18is600_read_from_bridge_register(dev, 0x05);
	printk("reg 0x05 (I2C ADDR): 0x%02X\n", ret);
}
void i2c_sc18is600_eeprom_wait(struct device *dev)
{
	while (i2c_sc18is600_read_from_bridge_register(dev, I2C_STATUS_REG)
			!= I2C_STATUS_NO_ACK) {
		printk(".");
	}
}

void i2c_sc18is600_read_after_write_to_device(struct device *dev, u8_t count_w,
		u8_t count_r, u8_t addr_w,
		u8_t addr_r, u8_t *data_w)
{
	size_t bufsize = 5 + count_w;
	u8_t buf[bufsize];
	size_t index = 0;

	buf[index++] = READ_AFTER_WRITE_DEVICE;
	buf[index++] = count_w;
	buf[index++] = count_r;
	buf[index++] = addr_w;
	for (int i = 0; i < count_w; i++)
		buf[index++] = data_w[i];
	buf[index++] = addr_r;
	i2c_sc18is600_spi_send_buffer(dev, buf, bufsize);
}

void i2c_sc18is600_read_from_device(struct device *dev, u8_t addr,
		u8_t count, u8_t *data)
{
	u8_t buf[3] = {READ_N_BYTES_FROM_DEVICE, count, addr};

	i2c_sc18is600_spi_send_buffer(dev, buf, 3);
}

void i2c_sc18is600_read_from_bridge_rx_buffer(struct device *dev,
		u8_t count, u8_t *data)
{
	size_t bufsize = 1 + count;
	u8_t tx_buffer[bufsize];
	u8_t rx_buffer[bufsize];
	size_t index = 0;

	tx_buffer[index++] = READ_BRIDGE_RX_BUFFER;

	for (int i = 0; i < count; i++) {
		tx_buffer[index++] = 0xFF; /* Send dummy bytes */
	}
	/* Set delay between two consecutive spi frames
	 * to 7 clock cycles, SC18IS600 specific
	 */
	SPI1_REG(SPI_REG_DINTERXFR) = 0x07;
	i2c_sc18is600_spi_transceive_buffer(dev, tx_buffer, bufsize,
			rx_buffer, bufsize);
	SPI1_REG(SPI_REG_DINTERXFR) = 0x00; /* Remove delay */

	for (u8_t i = 0; i < count; i++) {
		data[i] = rx_buffer[i + 1];
	}
}

void i2c_sc18is600_write_to_device(struct device *dev, u8_t addr,
		u8_t count, u8_t *data)
{
	size_t bufsize = 3 + count;
	size_t index = 0;
	u8_t tx_buffer[bufsize];

	tx_buffer[index++] = WRITE_N_BYTES_TO_DEVICE;
	tx_buffer[index++] = count;
	tx_buffer[index++] = addr;
	for (int i = 0; i < count; i++)
		tx_buffer[index++] = data[i];
	i2c_sc18is600_spi_send_buffer(dev, tx_buffer, bufsize);
}

/* INITIALIZATION */
static int i2c_sc18is600_init(struct device *dev)
{
	struct i2c_sc18is600_runtime *data =
		(struct i2c_sc18is600_runtime *) dev->driver_data;

	data->spi_d = device_get_binding(CONFIG_SPI_0_NAME);
	if (!data->spi_d)
		printk("spi device binding inside i2c driver failed\n");
	return 0;
}

/* POWER MANAGEMENT */

void i2c_sc18is600_remove_pinmux(void)
{
	GPIO_REG(GPIO_IOF_EN) &= ~((1 << 2) | (1 << 3)
			| (1 << 4) | (1 << 5)
			| (1 << 9) | (1 << 10));
	GPIO_REG(GPIO_INPUT_EN) &= ~((1 << 2) | (1 << 3)
			| (1 << 4) | (1 << 5)
			| (1 << 9) | (1 << 10));
	GPIO_REG(GPIO_OUTPUT_EN) &= ~((1 << 2) | (1 << 3)
			| (1 << 4) | (1 << 5)
			| (1 << 9) | (1 << 10));
}

/* API */
static int i2c_sc18is600_configure(struct device *dev, u32_t dev_config)
{
	struct i2c_sc18is600_runtime *data =
		(struct i2c_sc18is600_runtime *) dev->driver_data;
	i2c_sc18is600_spi_setup(dev);
	data->i2c_config = dev_config;
	i2c_sc18is600_write_to_bridge_register(dev, 0x02, 0x05);
	return 0;
};

static int i2c_sc18is600_transfer(struct device *dev, struct i2c_msg *msgs,
		u8_t num_msgs, u16_t addr)
{
	struct i2c_sc18is600_runtime *data =
		(struct i2c_sc18is600_runtime *) dev->driver_data;
	i2c_sc18is600_configure(dev, data->i2c_config);

	if (num_msgs > 0) {
		if (num_msgs == 2) {
			/* i2c_burst_read */
			if (msgs[0].flags == I2C_MSG_WRITE
					&& msgs[1].flags == (I2C_MSG_RESTART
						| I2C_MSG_READ
						| I2C_MSG_STOP)) {
				u8_t *transmit_data = msgs[0].buf;
				u32_t count_transmit = msgs[0].len;
				u32_t count_read = msgs[1].len;
				u8_t *receive_data = msgs[1].buf;

				i2c_sc18is600_read_after_write_to_device(dev,
						count_transmit,
						count_read, addr & 0xFF,
						(addr & 0xFF) | 0x01,
						transmit_data);
				k_sleep(1);
				i2c_sc18is600_read_from_bridge_rx_buffer(dev,
						count_read, receive_data);
			}
		}
	}

	return 0;
};

static const struct i2c_driver_api sc18is600_i2c_api = {
	.configure = i2c_sc18is600_configure,
	.transfer = i2c_sc18is600_transfer,
};

DEVICE_DEFINE(i2c_0, CONFIG_I2C_0_NAME, i2c_sc18is600_init,
		device_pm_control_nop, &i2c_sc18is600_0_runtime,
		&i2c_sc18is600_0_config, APPLICATION,
		CONFIG_I2C_INIT_PRIORITY, &sc18is600_i2c_api);

