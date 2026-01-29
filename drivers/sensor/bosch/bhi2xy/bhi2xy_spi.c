/*
 * Copyright (c) 2025, Bojan Sofronievski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bhi2xy.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(bhi2xy, CONFIG_SENSOR_LOG_LEVEL);

static int bhi2xy_bus_check_spi(const union bhi2xy_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int bhi2xy_reg_read_spi(const union bhi2xy_bus *bus, uint8_t reg, uint8_t *data,
			       uint16_t len)
{
	int ret;
	const struct spi_buf tx_buf = {.buf = &reg, .len = 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf[1];
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = 1};

	rx_buf[0].len = len;
	rx_buf[0].buf = data;

	ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret < 0) {
		LOG_DBG("spi_transceive failed %i", ret);
		return ret;
	}

	return 0;
}

static int bhi2xy_reg_write_spi(const union bhi2xy_bus *bus, uint8_t reg, const uint8_t *data,
				uint16_t len)
{
	int ret;
	const struct spi_buf tx_buf[2] = {{.buf = &reg, .len = 1},
					  {.buf = (uint8_t *)data, .len = len}};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret < 0) {
		LOG_ERR("spi_write_dt failed %i", ret);
		return ret;
	}

	return 0;
}

static enum bhy2_intf bhi2xy_get_intf_spi(const union bhi2xy_bus *bus)
{
	return BHY2_SPI_INTERFACE;
}

const struct bhi2xy_bus_io bhi2xy_bus_io_spi = {.check = bhi2xy_bus_check_spi,
						.read = bhi2xy_reg_read_spi,
						.write = bhi2xy_reg_write_spi,
						.get_intf = bhi2xy_get_intf_spi};
