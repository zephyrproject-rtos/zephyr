/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2020 Teslabs Engineering S.L.
 * Copyright (c) 2021 Krivorot Oleg <krivorot.oleg@gmail.com>
 * Copyright (c) 2024 Kim Bøndergaard <kim@fam-boendergaard.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9xxx.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(display_ili9xxx, CONFIG_DISPLAY_LOG_LEVEL);

int ili9xxx_transmit(const struct device *dev, uint8_t cmd, const void *tx_data, size_t tx_len)
{
	const struct ili9xxx_config *config = dev->config;

	int r;
	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1U};

	/* send command */
	tx_buf.buf = &cmd;
	tx_buf.len = 1U;

	gpio_pin_set_dt(&config->cmd_data, ILI9XXX_CMD);
	r = spi_write_dt(&config->spi, &tx_bufs);
	if (r < 0) {
		return r;
	}

	/* send data (if any) */
	if (tx_data != NULL) {
		tx_buf.buf = (void *)tx_data;
		tx_buf.len = tx_len;

		gpio_pin_set_dt(&config->cmd_data, ILI9XXX_DATA);
		r = spi_write_dt(&config->spi, &tx_bufs);
		if (r < 0) {
			return r;
		}
	}

	return 0;
}

int ili9xxx_transmit_data(const struct device *dev, const void *tx_data, size_t tx_len)
{
	const struct ili9xxx_config *config = dev->config;

	struct spi_buf tx_buf;
	struct spi_buf_set tx_bufs = {.buffers = &tx_buf, .count = 1U};

	tx_buf.buf = (void *)tx_data;
	tx_buf.len = tx_len;

	return spi_write_dt(&config->spi, &tx_bufs);
}

int ili9xxx_bus_init(const struct ili9xxx_config *config)
{
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI device is not ready");
		return -ENODEV;
	}
	return 0;
}
