/*
 * Copyright (c) 2022 Thomas Stranger
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_w1_serial

/**
 * @brief 1-Wire Bus Master driver using Zephyr serial interface.
 *
 * This driver implements the 1-Wire interface using an uart.
 * The driver uses a uart peripheral with a baudrate of 115.2 kBd to send
 * and receive data bits and a baurade of 9.6 kBd for slave reset and
 * presence detection as suggested for normal speed operating mode in:
 * https://www.maximintegrated.com/en/design/technical-documents/tutorials/2/214.html
 * For overdrive speed communication baudrates of 1 MBd and 115.2 kBd
 * are used, respectively.
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/w1.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

LOG_MODULE_REGISTER(w1_serial, CONFIG_W1_LOG_LEVEL);

#define W1_SERIAL_READ_REQ_BYTE   0xFF
#define W1_SERIAL_BIT_1           0xFF
#define W1_SERIAL_BIT_0           0x00

/* Standard speed signal parameters:
 * RST: t_RSTL=520us; t_slot=1041us
 * DATA: t_low1=8.68us; t_low0=78.1us; t_slot=86.8us
 */
#define W1_SERIAL_STD_RESET_BYTE   0xF0
#define W1_SERIAL_STD_RESET_BAUD   9600u
#define W1_SERIAL_STD_DATA_BAUD    115200u

/*
 * Overdrive speed signal parameters:
 * RST: t_RSTL=52.1us; t_slot=86.8us
 * DATA: t_low1=1.0us; t_low0=9.0us; t_slot=10.0us
 */
#define W1_SERIAL_OD_RESET_BYTE    0xE0
#define W1_SERIAL_OD_RESET_BAUD    115200u
#define W1_SERIAL_OD_DATA_BAUD     1000000u

struct w1_serial_config {
	/** w1 master config, common to all drivers */
	struct w1_master_config master_config;
	/** UART device used for 1-Wire communication */
	const struct device *uart_dev;
};

struct w1_serial_data {
	/** w1 master data, common to all drivers */
	struct w1_master_data master_data;
	struct uart_config uart_cfg;
	bool overdrive_active;
};

/*
 * Concurrently transmits and receives one 1-Wire bit
 * by sending and receiving one uart byte
 */
static int serial_tx_rx(const struct device *dev, const uint8_t *tx_data,
			uint8_t *rx_data, size_t len, uint32_t timeout)
{
	const struct w1_serial_config *cfg = dev->config;
	uint64_t end;
	uint8_t dummy;
	int ret = 0;

	__ASSERT_NO_MSG(tx_data != NULL);
	__ASSERT_NO_MSG(rx_data != NULL);

	for (int i = 0; i < len; ++i) {
		while (uart_poll_in(cfg->uart_dev, &dummy) == 0) {
			/* poll in any buffered data */
		}

		uart_poll_out(cfg->uart_dev, tx_data[i]);
		end = sys_clock_timeout_end_calc(K_USEC(timeout));

		do {
			ret = uart_poll_in(cfg->uart_dev, &rx_data[i]);
		} while (ret != 0 && end > k_uptime_ticks());
	}

	return ret;
}

/* Concurretly tranmits and receives one 1-Wire byte */
static int serial_tx_rx_byte(const struct device *dev, uint8_t tx_byte,
			     uint8_t *rx_byte)
{
	__ASSERT_NO_MSG(rx_byte != NULL);
	uint8_t byte_representation[8];

	for (int i = 0; i < 8; ++i) {
		/*
		 * Transmitting 0xFF the uart start bit pulls the line low to
		 * indicate either write Bit 1, or read low time.
		 * Write Bit 0 is represented as 0x00
		 */
		byte_representation[i] =
			((tx_byte & (1 << i)) != 0) ? W1_SERIAL_BIT_1 : W1_SERIAL_BIT_0;
	}

	if (serial_tx_rx(dev, &byte_representation[0], &byte_representation[0],
			 8, CONFIG_W1_ZEPHYR_SERIAL_BIT_TIMEOUT) < 0) {
		return -EIO;
	}

	*rx_byte = 0;
	for (int i = 0; i < 8; ++i) {
		/*
		 * rx-byte different from 0xFF indicates that a slave has
		 * pulled line low to transmit a 0 bit, otherwise a 1 bit.
		 */
		*rx_byte |= (uint8_t)(byte_representation[i] == 0xFF) << i;
	}

	return 0;
}

static int w1_serial_reset_bus(const struct device *dev)
{
	const struct w1_serial_config *cfg = dev->config;
	struct w1_serial_data *data = dev->data;
	uint8_t reset_byte = data->overdrive_active ?
			     W1_SERIAL_OD_RESET_BYTE : W1_SERIAL_STD_RESET_BYTE;
	/* reset uses 115200/9600=12 slower baudrate,
	 * adjust timeout accordingly, also valid for overdrive speed.
	 */
	const uint32_t reset_timeout = CONFIG_W1_ZEPHYR_SERIAL_BIT_TIMEOUT * 12;

	data->uart_cfg.baudrate = data->overdrive_active ?
			W1_SERIAL_OD_RESET_BAUD : W1_SERIAL_STD_RESET_BAUD;
	if (uart_configure(cfg->uart_dev, &data->uart_cfg) != 0) {
		LOG_ERR("Failed set baud rate for reset pulse");
		return -EIO;
	}

	if (serial_tx_rx(dev, &reset_byte, &reset_byte, 1, reset_timeout) < 0) {
		LOG_ERR("tx_rx_error reset_present");
		return -EIO;
	}

	data->uart_cfg.baudrate = data->overdrive_active ?
			W1_SERIAL_OD_DATA_BAUD : W1_SERIAL_STD_DATA_BAUD;
	if (uart_configure(cfg->uart_dev, &data->uart_cfg) != 0) {
		LOG_ERR("Failed set baud rate for data transfer");
		return -EIO;
	}

	/* At least 1 device is present on bus, if reset_byte is different
	 * from 0xF0. But Bus probably shorted if reset_byte is 0x00.
	 */
	return (reset_byte != W1_SERIAL_STD_RESET_BYTE) && (reset_byte != 0x00);
}

static int w1_serial_read_bit(const struct device *dev)
{
	uint8_t tx_bit = W1_SERIAL_READ_REQ_BYTE;
	uint8_t rx_bit;

	if (serial_tx_rx(dev, &tx_bit, &rx_bit, 1,
			 CONFIG_W1_ZEPHYR_SERIAL_BIT_TIMEOUT) != 0) {
		return -EIO;
	};

	return rx_bit == W1_SERIAL_READ_REQ_BYTE;
}

static int w1_serial_write_bit(const struct device *dev, const bool bit)
{
	uint8_t tx_bit;
	uint8_t rx_bit;

	tx_bit = bit ? W1_SERIAL_BIT_1 : W1_SERIAL_BIT_0;
	if (serial_tx_rx(dev, &tx_bit, &rx_bit, 1,
			 CONFIG_W1_ZEPHYR_SERIAL_BIT_TIMEOUT) != 0) {
		return -EIO;
	}
	return 0;
}

static int w1_serial_read_byte(const struct device *dev)
{
	uint8_t tx_byte = 0xFF;
	uint8_t rx_byte;

	if (serial_tx_rx_byte(dev, tx_byte, &rx_byte) != 0) {
		return -EIO;
	}

	return rx_byte;
}

static int w1_serial_write_byte(const struct device *dev, const uint8_t byte)
{
	uint8_t rx_byte;

	return serial_tx_rx_byte(dev, byte, &rx_byte);
}

static int w1_serial_configure(const struct device *dev,
			       enum w1_settings_type type, uint32_t value)
{
	const struct w1_serial_config *cfg = dev->config;
	struct w1_serial_data *data = dev->data;
	int ret = 0;
	bool temp;

	switch (type) {
	case W1_SETTING_SPEED:
		temp = (bool)value;
		if (temp == data->overdrive_active) {
			break;
		}

		data->overdrive_active = temp;
		data->uart_cfg.baudrate = data->overdrive_active ?
				W1_SERIAL_OD_DATA_BAUD : W1_SERIAL_STD_DATA_BAUD;
		if (uart_configure(cfg->uart_dev, &data->uart_cfg) != 0) {
			LOG_ERR("Failed set baud rate for data transfer");
			ret = -EIO;
		}
		break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}

static int w1_serial_init(const struct device *dev)
{
	const struct w1_serial_config *cfg = dev->config;
	struct w1_serial_data *data = dev->data;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("Serial device not ready");
		return -ENODEV;
	}

	data->uart_cfg.baudrate = W1_SERIAL_STD_DATA_BAUD;
	data->uart_cfg.parity = UART_CFG_PARITY_NONE;
	data->uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
	data->uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
	data->uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	if (uart_configure(cfg->uart_dev, &data->uart_cfg) != 0) {
		LOG_ERR("Failed to configure UART");
		return -EINVAL;
	}

	data->overdrive_active = false;

	LOG_DBG("w1-serial initialized, with %d slave devices",
		cfg->master_config.slave_count);
	return 0;
}

static const struct w1_driver_api w1_serial_driver_api = {
	.reset_bus = w1_serial_reset_bus,
	.read_bit = w1_serial_read_bit,
	.write_bit = w1_serial_write_bit,
	.read_byte = w1_serial_read_byte,
	.write_byte = w1_serial_write_byte,
	.configure = w1_serial_configure,
};

#define W1_ZEPHYR_SERIAL_INIT(inst)					   \
static const struct w1_serial_config w1_serial_cfg_##inst = {		   \
	.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),			   \
	.master_config.slave_count = W1_INST_SLAVE_COUNT(inst)		   \
};									   \
static struct w1_serial_data w1_serial_data_##inst = {};		   \
DEVICE_DT_INST_DEFINE(inst, &w1_serial_init, NULL, &w1_serial_data_##inst, \
		      &w1_serial_cfg_##inst, POST_KERNEL,		   \
		      CONFIG_W1_INIT_PRIORITY, &w1_serial_driver_api);	   \

DT_INST_FOREACH_STATUS_OKAY(W1_ZEPHYR_SERIAL_INIT)
