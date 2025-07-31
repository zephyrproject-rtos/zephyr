/*
 * Copyright (c) 2025 Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flexcomm_sample, LOG_LEVEL_INF);

/* Device tree node identifiers */
#define UART_NODE DT_NODELABEL(flexcomm0)
#define I2C_NODE  DT_NODELABEL(flexcomm4)
#define SPI_NODE  DT_NODELABEL(flexcomm5)

/* Test data */
static uint8_t tx_buffer[] = "Hello FLEXCOMM!";
static uint8_t rx_buffer[32];

static void test_uart(void)
{
	const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
	
	if (!device_is_ready(uart_dev)) {
		LOG_ERR("UART device not ready");
		return;
	}
	
	LOG_INF("UART (FLEXCOMM0) - Console interface ready");
	LOG_INF("Baudrate: 115200");
	
	/* UART is already configured for console, just print a message */
	printk("UART test message via console\n");
}

static void test_i2c(void)
{
	const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);
	uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
	
	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("I2C device not ready");
		return;
	}
	
	LOG_INF("I2C (FLEXCOMM4) Configuration:");
	LOG_INF("- Speed: 400 kHz (Fast mode)");
	LOG_INF("- Pins: P0.25 (SCL), P0.26 (SDA)");
	
	/* Configure I2C */
	if (i2c_configure(i2c_dev, i2c_cfg) != 0) {
		LOG_ERR("I2C configuration failed");
		return;
	}
	
	/* Example: Scan for I2C devices */
	LOG_INF("Scanning I2C bus for devices...");
	for (uint8_t addr = 0x08; addr < 0x78; addr++) {
		struct i2c_msg msg;
		uint8_t dummy = 0;
		
		msg.buf = &dummy;
		msg.len = 0;
		msg.flags = I2C_MSG_WRITE | I2C_MSG_STOP;
		
		if (i2c_transfer(i2c_dev, &msg, 1, addr) == 0) {
			LOG_INF("Found I2C device at address 0x%02x", addr);
		}
	}
}

static void test_spi(void)
{
	const struct device *spi_dev = DEVICE_DT_GET(SPI_NODE);
	
	struct spi_config spi_cfg = {
		.frequency = 8000000U, /* 8 MHz */
		.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8),
		.slave = 0,
		.cs = NULL,
	};
	
	struct spi_buf tx_buf = {
		.buf = tx_buffer,
		.len = sizeof(tx_buffer),
	};
	
	struct spi_buf rx_buf = {
		.buf = rx_buffer,
		.len = sizeof(rx_buffer),
	};
	
	struct spi_buf_set tx_set = {
		.buffers = &tx_buf,
		.count = 1,
	};
	
	struct spi_buf_set rx_set = {
		.buffers = &rx_buf,
		.count = 1,
	};
	
	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device not ready");
		return;
	}
	
	LOG_INF("SPI (FLEXCOMM5) Configuration:");
	LOG_INF("- Mode: Master");
	LOG_INF("- Speed: 8 MHz");
	LOG_INF("- Pins: P0.19 (SCK), P0.18 (MISO), P0.20 (MOSI), P1.1 (CS)");
	
	/* Example: Loopback test (connect MOSI to MISO) */
	if (spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set) == 0) {
		LOG_INF("SPI transaction completed");
		LOG_HEXDUMP_INF(rx_buffer, 16, "Received data:");
	} else {
		LOG_ERR("SPI transaction failed");
	}
}

int main(void)
{
	LOG_INF("*** LPC54S018 FLEXCOMM Sample ***");
	
	/* Test UART */
	test_uart();
	k_sleep(K_MSEC(100));
	
	/* Test I2C */
	test_i2c();
	k_sleep(K_MSEC(100));
	
	/* Test SPI */
	test_spi();
	
	LOG_INF("FLEXCOMM sample completed");
	
	return 0;
}