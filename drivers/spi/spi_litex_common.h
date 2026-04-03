/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/spi/rtio.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys_clock.h>

#include "spi_context.h"
#include <soc.h>

static inline uint8_t get_dfs_value(const struct spi_config *config)
{
	switch (SPI_WORD_SIZE_GET(config->operation)) {
	case 8:
		return 1;
	case 16:
		return 2;
	case 24:
		return 3;
	case 32:
		return 4;
	default:
		return 1;
	}
}

static inline void litex_spi_tx_put(uint8_t len, uint32_t *txd, const uint8_t *tx_buf)
{
	switch (len) {
	case 4:
		*txd = sys_get_be32(tx_buf);
		break;
	case 3:
		*txd = sys_get_be24(tx_buf);
		break;
	case 2:
		*txd = sys_get_be16(tx_buf);
		break;
	default:
		*txd = *tx_buf;
		break;
	}
}

static inline void litex_spi_rx_put(uint8_t len, uint32_t *rxd, uint8_t *rx_buf)
{
	switch (len) {
	case 4:
		sys_put_be32(*rxd, rx_buf);
		break;
	case 3:
		sys_put_be24(*rxd, rx_buf);
		break;
	case 2:
		sys_put_be16(*rxd, rx_buf);
		break;
	default:
		*rx_buf = *rxd;
		break;
	}
}
