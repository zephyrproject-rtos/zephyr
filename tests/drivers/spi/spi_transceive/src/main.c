/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/spi.h>
#include <zephyr/ztest.h>

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define SPI_NODE DT_NODELABEL(dut)
#else
#error "No SPI device 'dut' defined in devicetree"
#endif

#define TEST_BUFFER_LEN 10

static const struct device *const spi_dev = DEVICE_DT_GET(SPI_NODE);

uint8_t spi_tx_buffer[TEST_BUFFER_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55,
						0x66, 0x77, 0x88, 0x99, 0xAA};

/* SPI Configuration */
static const struct spi_config spi_cfg = {
	.frequency = 1000000,
	.operation = SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	.slave = 0,
};

struct spi_buf tx_buf = {
	.buf = spi_tx_buffer,
	.len = TEST_BUFFER_LEN,
};

uint8_t spi_rx_buffer[TEST_BUFFER_LEN];
struct spi_buf rx_buf = {
	.buf = spi_rx_buffer,
	.len = TEST_BUFFER_LEN,
};

struct spi_buf_set tx_set = {
	.buffers = &tx_buf,
	.count = 1,
};

struct spi_buf_set rx_set = {
	.buffers = &rx_buf,
	.count = 1,
};

void spi_callback(const struct device *dev, int result, void *userdata)
{
	/* Verify received data */
	for (int i = 0; i < TEST_BUFFER_LEN; i++) {
		zassert_equal(spi_rx_buffer[i], spi_tx_buffer[i],
			      "Mismatch at index %d: received 0x%x, expected 0x%x", i,
			      spi_rx_buffer[i], spi_tx_buffer[i]);
	}
}

/* Combined Test case for SPI transmission and reception */
ZTEST(spi_transceive, test_spi_transmission_reception)
{

	int err = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);

	zassert_equal(err, 0, "SPI transceive failed: %d", err);

	/* Verify received data */
	for (int i = 0; i < TEST_BUFFER_LEN; i++) {
		zassert_equal(spi_rx_buffer[i], spi_tx_buffer[i],
			      "Mismatch at index %d: received 0x%x, expected 0x%x", i,
			      spi_rx_buffer[i], spi_tx_buffer[i]);
	}
}
ZTEST(spi_transceive, test_spi_transceive_cb)
{
	int err = spi_transceive_cb(spi_dev, &spi_cfg, &tx_set, &rx_set, spi_callback, NULL);

	zassert_equal(err, 0, "SPI transceive failed: %d", err);
}

/* Test setup */
void *test_setup(void)
{
	zassert_true(device_is_ready(spi_dev), "SPI device is not ready");
	return NULL;
}

ZTEST_SUITE(spi_transceive, NULL, test_setup, NULL, NULL, NULL);
