/*
 * Copyright (c) 2024 TDK Invensense
 * Copyright (c) 2022 Esco Medical ApS
 * Copyright (c) 2020 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for ICM42X70 accessed via SPI.
 */

#include "icm42x70.h"

#if CONFIG_SPI

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(ICM42X70, CONFIG_SENSOR_LOG_LEVEL);

static int icm42x70_bus_check_spi(const union icm42x70_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int icm42x70_reg_read_spi(const union icm42x70_bus *bus, uint8_t start, uint8_t *buf,
				 uint32_t size)

{
	uint8_t cmd[] = {(start | 0x80)};
	const struct spi_buf tx_buf = {.buf = cmd, .len = sizeof(cmd)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};
	int ret;

	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;

	rx_buf[1].len = size;
	rx_buf[1].buf = buf;

	ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret) {
		LOG_ERR("spi_transceive FAIL %d\n", ret);
		return ret;
	}
	return 0;
}

static int icm42x70_reg_write_spi(const union icm42x70_bus *bus, uint8_t reg, uint8_t *buf,
				  uint32_t size)
{
	uint8_t cmd[] = {reg & 0x7F};
	const struct spi_buf tx_buf[2] = {{.buf = cmd, .len = sizeof(cmd)},
					  {.buf = buf, .len = size}};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = 2};
	int ret = spi_write_dt(&bus->spi, &tx);

	if (ret) {
		LOG_ERR("spi_write FAIL %d\n", ret);
		return ret;
	}
	return 0;
}

const struct icm42x70_bus_io icm42x70_bus_io_spi = {
	.check = icm42x70_bus_check_spi,
	.read = icm42x70_reg_read_spi,
	.write = icm42x70_reg_write_spi,
};

#endif /* CONFIG_SPI */
