/*
 * Copyright (c) 2026 Jjateen Gundesha <jjateen97@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SPI slave async callback sample
 *
 * Demonstrates how to use spi_transceive_cb() to receive a transfer
 * completion notification via callback, equivalent to nrfx spis_event_handler
 * with NRF_DRV_SPIS_XFER_DONE. The device waits for a SPI master to initiate
 * a transfer and prints the received data on completion.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spis_async, LOG_LEVEL_INF);

#define BUF_SIZE 32

static const struct device *const spis_dev = DEVICE_DT_GET(DT_ALIAS(spis));
static const struct spi_config spis_cfg = {
	.operation = SPI_OP_MODE_SLAVE | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
};

static K_SEM_DEFINE(xfer_done, 0, 1);
static int xfer_result;

static uint8_t tx_buf[BUF_SIZE] = "Hello from SPIS async!";
static uint8_t rx_buf[BUF_SIZE];

/**
 * @brief Transfer completion callback - equivalent to spis_event_handler with
 *        NRF_DRV_SPIS_XFER_DONE check in the legacy nrfx API.
 *
 * This runs in the context of the SPI driver work queue after EasyDMA
 * completes. Do minimal work here; signal a semaphore and process in main.
 */
static void spis_callback(const struct device *dev, int result, void *userdata)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(userdata);

	xfer_result = result;
	k_sem_give(&xfer_done);
}

int main(void)
{
	int ret;
	struct spi_buf tx_spi_buf = {.buf = tx_buf, .len = sizeof(tx_buf)};
	struct spi_buf rx_spi_buf = {.buf = rx_buf, .len = sizeof(rx_buf)};
	struct spi_buf_set tx_set = {.buffers = &tx_spi_buf, .count = 1};
	struct spi_buf_set rx_set = {.buffers = &rx_spi_buf, .count = 1};

	if (!device_is_ready(spis_dev)) {
		LOG_ERR("SPIS device not ready");
		return -ENODEV;
	}

	LOG_INF("SPIS async sample started on %s", CONFIG_BOARD_TARGET);

	while (1) {
		memset(rx_buf, 0, sizeof(rx_buf));

		LOG_INF("Waiting for SPI master transfer...");

		/*
		 * spi_transceive_cb() arms the SPIS peripheral and returns
		 * immediately. spis_callback() fires when the master completes
		 * the transfer - this is the Zephyr equivalent of registering
		 * spis_event_handler and checking NRF_DRV_SPIS_XFER_DONE.
		 */
		ret = spi_transceive_cb(spis_dev, &spis_cfg,
					 &tx_set, &rx_set,
					 spis_callback, NULL);
		if (ret < 0) {
			LOG_ERR("spi_transceive_cb failed: %d", ret);
			return ret;
		}

		/* Block until callback signals completion */
		k_sem_take(&xfer_done, K_FOREVER);

		if (xfer_result < 0) {
			LOG_ERR("Transfer error: %d", xfer_result);
			continue;
		}

		LOG_INF("Transfer complete: %.*s",
			(int)sizeof(rx_buf), rx_buf);
	}

	return 0;
}
