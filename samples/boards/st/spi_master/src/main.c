/*
 * Copyright (c) 2025 Waqar Ahmed
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <string.h>

LOG_MODULE_REGISTER(spi_master_sample, LOG_LEVEL_DBG);

/* SPI device from devicetree */
#define SPI_NODE DT_NODELABEL(spi_master)

/* Message buffer size */
#define MSG_BUF_SIZE 32

/* Separate CS control struct for proper initialization */
static const struct spi_cs_control cs_ctrl = {
	.gpio = GPIO_DT_SPEC_GET(SPI_NODE, cs_gpios),
	.delay = 10,
	.cs_is_gpio = true,
};

/* SPI configuration */
static struct spi_config spi_cfg = {
	.frequency = 312500,
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB | SPI_OP_MODE_MASTER,
	.slave = 0,
	.cs = cs_ctrl,
};

/**
 * @brief Print SPI configuration for debugging
 *
 * @param cfg Pointer to SPI configuration
 */
static void print_spi_config(const struct spi_config *cfg)
{
	LOG_INF("=== SPI Configuration ===");
	LOG_INF("Frequency: %u Hz", cfg->frequency);
	LOG_INF("Operation: 0x%08X", cfg->operation);
	LOG_INF("Slave: %u", cfg->slave);

	if (cfg->cs.cs_is_gpio) {
		LOG_INF("CS GPIO port: %p", cfg->cs.gpio.port);
		LOG_INF("CS GPIO pin: %u", cfg->cs.gpio.pin);
		LOG_INF("CS GPIO flags: 0x%04X", cfg->cs.gpio.dt_flags);
		LOG_INF("CS delay: %u us", cfg->cs.delay);
	}

	LOG_INF("========================");
}

/**
 * @brief Prepare transmission buffer with message and padding
 *
 * @param buf Buffer to prepare
 * @param buf_size Size of buffer
 * @param msg_number Message sequence number
 * @return Length of prepared message
 */
static size_t prepare_tx_buffer(uint8_t *buf, size_t buf_size, uint32_t msg_number)
{
	int msg_len;

	/* Format message */
	msg_len = snprintf((char *)buf, buf_size,
			   "Hello from master %u", msg_number);

	/* Pad remaining bytes with pattern for debugging */
	if (msg_len < buf_size) {
		memset(buf + msg_len, 0xAA, buf_size - msg_len);
	}

	return buf_size;
}

int main(void)
{
	const struct device *spi_dev;
	uint8_t tx_buf[MSG_BUF_SIZE];
	uint8_t rx_buf[MSG_BUF_SIZE];
	uint32_t msg_number = 0;
	struct spi_buf tx_spi_buf;
	struct spi_buf rx_spi_buf;
	struct spi_buf_set tx_set;
	struct spi_buf_set rx_set;
	int ret;

	LOG_INF("SPI Master Sample");

	/* Get SPI device from devicetree */
	spi_dev = DEVICE_DT_GET(SPI_NODE);
	if (!device_is_ready(spi_dev)) {
		LOG_ERR("SPI device not ready");
		return -ENODEV;
	}

	LOG_INF("SPI device ready");

	/* Print configuration for debugging */
	print_spi_config(&spi_cfg);

	/* Initialize SPI buffer structures */
	tx_spi_buf.buf = tx_buf;
	rx_spi_buf.buf = rx_buf;

	tx_set.buffers = &tx_spi_buf;
	tx_set.count = 1;

	rx_set.buffers = &rx_spi_buf;
	rx_set.count = 1;

	/* Wait for slave device to initialize */
	LOG_INF("Waiting for slave initialization...");
	k_sleep(K_SECONDS(2));

	LOG_INF("Starting SPI communication loop");

	while (1) {
		msg_number++;

		/* Prepare transmission buffer */
		size_t msg_len = prepare_tx_buffer(tx_buf, MSG_BUF_SIZE, msg_number);

		/* Set buffer lengths */
		tx_spi_buf.len = msg_len;
		rx_spi_buf.len = msg_len;

		/* Clear receive buffer */
		memset(rx_buf, 0, MSG_BUF_SIZE);

		LOG_DBG("[%lld ms] Sending message #%u", k_uptime_get(), msg_number);
		LOG_INF("TX string: [%s] Message Length: %d", tx_buf, msg_len);

		/* Perform SPI transaction */
		ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);

		if (ret == 0) {
			/* Null-terminate for safe string printing */
			rx_buf[msg_len < MSG_BUF_SIZE ? msg_len : MSG_BUF_SIZE - 1] = '\0';
			LOG_INF("RX string: [%s] Message Length: %d", rx_buf, msg_len);
		} else {
			LOG_ERR("SPI transaction failed: %d", ret);
		}

		/* Wait before next transmission */
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
