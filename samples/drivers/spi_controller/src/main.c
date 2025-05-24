/*
 * Copyright (c) 2025 Toney Abraham
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_controller, LOG_LEVEL_INF);

#define SPI_NODE DT_ALIAS(spi0)

static const struct device *spi_dev = DEVICE_DT_GET(SPI_NODE);

static const struct spi_config spis_config = {
	.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8) | SPI_MODE_LOOP,
	.frequency = 100000,
	.slave = 0,
};

static const char tx_data[] = "Test Data from SPI";
struct spi_buf tx_buf = {
	.buf = (void *)tx_data, 
	.len = sizeof(tx_data)
};
struct spi_buf_set tx_bufs = {
	.buffers = &tx_buf, 
	.count = 1
};

uint8_t rx_data[sizeof(tx_data)];
struct spi_buf rx_buf = {
	.buf = rx_data, 
	.len = sizeof(rx_data)
};
struct spi_buf_set rx_bufs = {
	.buffers = &rx_buf, 
	.count = 1
};

uint8_t rx_data_async[sizeof(tx_data)];
struct spi_buf rx_buf_async = {
	.buf = rx_data_async, 
	.len = sizeof(rx_data_async)
};
struct spi_buf_set rx_bufs_async = {
	.buffers = &rx_buf_async, 
	.count = 1
};

void spi_callback(const struct device *dev, int result, void *data)
{
	printk("SPI ASYNC MODE Callback Function\n");
	printk("Received Data: %s\n", rx_data_async);
}

int main(void)
{
	int ret;

	if (!device_is_ready(spi_dev)) {
		printk("Error: SPI device not ready\n");
		return 0;
	}

	ret = spi_transceive(spi_dev, &spis_config, &tx_bufs, &rx_bufs);

	if (ret == 0) {
		printk("Sent Data : %s\n", tx_data);
		printk("Received Data : %s\n", rx_data);
	} else {
		printk("SPI transceive error: %d\n", ret);
	}
	k_msleep(1000);

	if (!device_is_ready(spi_dev)) {
		printk("Error: SPI device not ready\n");
		return 0;
	}
	ret = spi_transceive_cb(spi_dev, &spis_config, &tx_bufs, &rx_bufs_async, spi_callback,
				NULL);
}
