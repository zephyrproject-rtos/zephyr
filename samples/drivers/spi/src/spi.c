/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL SYS_LOG_LEVEL_INFO
#include <logging/sys_log.h>

#include <zephyr.h>
#include <misc/printk.h>
#include <string.h>

#include <spi.h>

#define SPI_CS &spi_cs

#if defined(CONFIG_SOC_QUARK_SE_C1000)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 1
#define CS_CTRL_GPIO_DRV_NAME CONFIG_GPIO_QMSI_0_NAME

struct spi_cs_control spi_cs = {
	.gpio_pin = 25,
	.delay = 0
};

#elif defined(CONFIG_SOC_QUARK_SE_C1000_SS)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 0
#define CS_CTRL_GPIO_DRV_NAME CONFIG_GPIO_DW_0_NAME

struct spi_cs_control spi_cs = {
	.gpio_pin = 0,
	.delay = 0
};

#elif defined(CONFIG_SOC_EM7D) || defined(CONFIG_SOC_EM9D)

#define SPI_DRV_NAME CONFIG_SPI_0_NAME
#define SPI_SLAVE 0

#undef SPI_CS
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""

#else
#undef SPI_CS
#define SPI_CS NULL
#define CS_CTRL_GPIO_DRV_NAME ""
#endif

#define BUF_SIZE 17
u8_t buffer_tx[] = "0123456789abcdef\0";
u8_t buffer_rx[BUF_SIZE] = {};

struct spi_config spi_slow = {
	.frequency = 128000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

struct spi_config spi_fast = {
	.frequency = 16000000,
	.operation = SPI_OP_MODE_MASTER | SPI_MODE_CPOL |
	SPI_MODE_CPHA | SPI_WORD_SET(8) | SPI_LINES_SINGLE,
	.slave = SPI_SLAVE,
	.cs = SPI_CS,
};

struct device *spi_dev;

static int cs_ctrl_gpio_config(struct spi_cs_control *cs)
{
	if (cs) {
		cs->gpio_dev = device_get_binding(CS_CTRL_GPIO_DRV_NAME);
		if (!cs->gpio_dev) {
			SYS_LOG_ERR("Cannot find %s!\n",
				    CS_CTRL_GPIO_DRV_NAME);
			return -1;
		}
	}

	return 0;
}

static int spi_complete_loop(struct spi_config *spi_conf)
{
	struct spi_buf tx = {
		.buf = buffer_tx,
		.len = BUF_SIZE,
	};
	struct spi_buf rx = {
		.buf = buffer_rx,
		.len = BUF_SIZE,
	};
	const struct spi_buf *tx_bufs[] = { &tx, NULL };
	struct spi_buf *rx_bufs[] = { &rx, NULL };
	int ret;

	ret = spi_transceive(spi_dev, spi_conf, tx_bufs, rx_bufs);
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx, buffer_rx, BUF_SIZE)) {
		SYS_LOG_ERR("Buffer contents are different %s vs %s",
			    buffer_tx, buffer_rx);
		return -1;
	}

	return 0;
}

static int spi_rx_half_start(struct spi_config *spi_conf)
{
	struct spi_buf tx = {
		.buf = buffer_tx,
		.len = BUF_SIZE,
	};
	struct spi_buf rx = {
		.buf = buffer_rx,
		.len = 8,
	};
	const struct spi_buf *tx_bufs[] = { &tx, NULL };
	struct spi_buf *rx_bufs[] = { &rx, NULL };
	int ret;

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_dev, spi_conf, tx_bufs, rx_bufs);
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx, buffer_rx, 8)) {
		SYS_LOG_ERR("Buffer contents are different");
		return -1;
	}

	return 0;
}

static int spi_rx_half_end(struct spi_config *spi_conf)
{
	struct spi_buf tx = {
		.buf = buffer_tx,
		.len = BUF_SIZE,
	};
	struct spi_buf rx_1st_half = {
		.buf = NULL,
		.len = 8,
	};
	struct spi_buf rx_last_half = {
		.buf = buffer_rx,
		.len = 8,
	};
	const struct spi_buf *tx_bufs[] = { &tx, NULL };
	struct spi_buf *rx_bufs[] = { &rx_1st_half, &rx_last_half, NULL };
	int ret;

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_dev, spi_conf, tx_bufs, rx_bufs);
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx+8, buffer_rx, 8)) {
		SYS_LOG_ERR("Buffer contents are different");
		return -1;
	}

	return 0;
}

static int spi_rx_every_4(struct spi_config *spi_conf)
{
	struct spi_buf tx = {
		.buf = buffer_tx,
		.len = BUF_SIZE,
	};
	struct spi_buf rx_start = {
		.buf = NULL,
		.len = 4,
	};
	struct spi_buf rx_1 = {
		.buf = buffer_rx,
		.len = 4,
	};
	struct spi_buf rx_2 = {
		.buf = buffer_rx+4,
		.len = 4,
	};
	const struct spi_buf *tx_bufs[] = { &tx, NULL };
	struct spi_buf *rx_bufs[] = { &rx_start, &rx_1,
				      &rx_start, &rx_2, NULL };
	int ret;

	memset(buffer_rx, 0, BUF_SIZE);

	ret = spi_transceive(spi_dev, spi_conf, tx_bufs, rx_bufs);
	if (ret) {
		SYS_LOG_ERR("Code %d", ret);
		return -1;
	}

	if (memcmp(buffer_tx+4, buffer_rx, 4) ||
	    memcmp(buffer_tx+12, buffer_rx+4, 4)) {
		SYS_LOG_ERR("Buffer contents are different");
		return -1;
	}

	return 0;
}


void main(void)
{
	SYS_LOG_INF("SPI test on buffex TX/RX %p/%p", buffer_tx, buffer_rx);

	if (cs_ctrl_gpio_config(spi_slow.cs) ||
	    cs_ctrl_gpio_config(spi_fast.cs)) {
		return;
	}

	spi_dev = device_get_binding(SPI_DRV_NAME);
	if (!spi_dev) {
		SYS_LOG_ERR("Cannot find %s!\n", SPI_DRV_NAME);
		return;
	}

	if (spi_complete_loop(&spi_slow) ||
	    spi_rx_half_start(&spi_slow) ||
	    spi_rx_half_end(&spi_slow) ||
	    spi_rx_every_4(&spi_slow)) {
		return;
	}

	if (spi_complete_loop(&spi_fast) ||
	    spi_rx_half_start(&spi_fast) ||
	    spi_rx_half_end(&spi_fast) ||
	    spi_rx_every_4(&spi_fast)) {
		return;
	}

	SYS_LOG_INF("All tx/rx passed");
}
