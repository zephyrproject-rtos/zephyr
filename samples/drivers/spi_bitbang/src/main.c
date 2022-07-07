/*
 * Copyright (c) 2021 Marc Reilly, Creative Product Design
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#include <zephyr/zephyr.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/drivers/spi.h>

#define SPIBB_NODE	DT_NODELABEL(spibb0)

/*
 * writes 5 9bit words, you can check the output with a logic analyzer
 */
void test_basic_write_9bit_words(const struct device *dev,
				 struct spi_cs_control *cs)
{
	struct spi_config config;

	config.frequency = 125000;
	config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(9);
	config.slave = 0;
	config.cs = cs;

	uint16_t buff[5] = { 0x0101, 0x00ff, 0x00a5, 0x0000, 0x0102};
	int len = 5 * sizeof(buff[0]);

	struct spi_buf tx_buf = { .buf = buff, .len = len };
	struct spi_buf_set tx_bufs = { .buffers = &tx_buf, .count = 1 };

	int ret = spi_write(dev, &config, &tx_bufs);

	printf("basic_write_9bit_words; ret: %d\n", ret);
	printf(" wrote %04x %04x %04x %04x %04x\n",
		buff[0], buff[1], buff[2], buff[3], buff[4]);
}

/*
 * A more complicated xfer, sends two words, then sends and receives another
 * 3 words. Connect MOSI to MISO to test read
 */
void test_9bit_loopback_partial(const struct device *dev,
				struct spi_cs_control *cs)
{
	struct spi_config config;

	config.frequency = 125000;
	config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(9);
	config.slave = 0;
	config.cs = cs;

	enum { datacount = 5 };
	uint16_t buff[datacount] = { 0x0101, 0x0102, 0x0003, 0x0004, 0x0105};
	uint16_t rxdata[3];

	const int stride = sizeof(buff[0]);

	struct spi_buf tx_buf[2] = {
		{.buf = buff, .len = (2) * stride},
		{.buf = buff + (2), .len = (datacount - 2)*stride},
	};
	struct spi_buf rx_buf[2] = {
		{.buf = 0, .len = (2) * stride},
		{.buf = rxdata, .len = (datacount - 2) * stride},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 2 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 2 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);

	printf("9bit_loopback_partial; ret: %d\n", ret);
	printf(" tx (i)  : %04x %04x\n", buff[0], buff[1]);
	printf(" tx (ii) : %04x %04x %04x\n", buff[2], buff[3], buff[4]);
	printf(" rx (ii) : %04x %04x %04x\n", rxdata[0], rxdata[1], rxdata[2]);
}

/*
 * Tests 8 bit transfer at higher frequency, at this frequency there won't be
 * any busy waits between clock edges, the rate is limited by gpio calls etc.
 */
void test_8bit_xfer(const struct device *dev, struct spi_cs_control *cs)
{
	struct spi_config config;

	config.frequency = 1000000;
	config.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
	config.slave = 0;
	config.cs = cs;

	enum { datacount = 5 };
	uint8_t buff[datacount] = { 0x01, 0x02, 0x03, 0x04, 0x05};
	uint8_t rxdata[datacount];

	struct spi_buf tx_buf[1] = {
		{.buf = buff, .len = datacount},
	};
	struct spi_buf rx_buf[1] = {
		{.buf = rxdata, .len = datacount},
	};

	struct spi_buf_set tx_set = { .buffers = tx_buf, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = rx_buf, .count = 1 };

	int ret = spi_transceive(dev, &config, &tx_set, &rx_set);

	printf("8bit_loopback_partial; ret: %d\n", ret);
	printf(" tx (i)  : %02x %02x %02x %02x %02x\n",
	       buff[0], buff[1], buff[2], buff[3], buff[4]);
	printf(" rx (i)  : %02x %02x %02x %02x %02x\n",
	       rxdata[0], rxdata[1], rxdata[2], rxdata[3], rxdata[4]);
}

void main(void)
{
	const struct device *dev;

	dev = device_get_binding(DT_LABEL(SPIBB_NODE));

	if (!dev) {
		printf("SPI bitbang driver %s was not found!\n",
				DT_LABEL(SPIBB_NODE));
		return;
	}

	struct spi_cs_control cs_ctrl = (struct spi_cs_control){
		.gpio_dev = device_get_binding(
				DT_GPIO_LABEL(SPIBB_NODE, cs_gpios)),
		.gpio_pin = DT_GPIO_PIN(SPIBB_NODE, cs_gpios),
		.gpio_dt_flags = DT_GPIO_FLAGS(SPIBB_NODE, cs_gpios),
		.delay = 0u,
	};

	/*
	 * Loop through the various demo functions, the delays make it easier to
	 * locate on a scope/analyzer, the longer delay at the end helps discern
	 * where the pattern repeats.
	 */
	while (1) {
		test_basic_write_9bit_words(dev, &cs_ctrl);
		k_sleep(K_MSEC(200));

		test_9bit_loopback_partial(dev, &cs_ctrl);
		k_sleep(K_MSEC(200));

		test_8bit_xfer(dev, &cs_ctrl);
		k_sleep(K_MSEC(1000));
	}
}
