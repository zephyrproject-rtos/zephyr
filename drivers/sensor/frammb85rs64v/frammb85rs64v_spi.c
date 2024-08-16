/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for FRAMMB85RS64Vs accessed via SPI.
 */

#include <zephyr/logging/log.h>
#include "frammb85rs64v.h"

#if FRAMMB85RS64V_BUS_SPI

LOG_MODULE_DECLARE(FRAMMB85RS64V, CONFIG_SENSOR_LOG_LEVEL);

static int frammb85rs64v_bus_check_spi(const union frammb85rs64v_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int frammb85rs64v_read_id_spi(const union frammb85rs64v_bus *bus, uint8_t *data)
{
	LOG_DBG("%s", __func__);
	uint8_t addr;
	const struct spi_buf tx_buf = { .buf = &addr, .len = 1 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = { .buffers = rx_buf, .count = ARRAY_SIZE(rx_buf) };
	// ignore first RX for transceive when also sending data
	rx_buf[0].buf = NULL;
	rx_buf[0].len = 1;
	rx_buf[1].len = 4;
	addr = MB85RS64V_MANUFACTURER_ID_CMD;
	rx_buf[1].buf = &data[0];
	int ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}
	return 0;
}

static int frammb85rs64v_reg_read_spi(const union frammb85rs64v_bus *bus, uint16_t addr,
				      uint8_t *data, size_t len)
{
	LOG_DBG("%s", __func__);
	uint8_t access[3];
	access[0] = MB85RS64V_READ_CMD;
	access[1] = (addr >> 8) & 0xFF;
	access[2] = addr & 0xFF;

	struct spi_buf tx_buf = { .buf = &access, .len = 3 };
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	struct spi_buf rx_buf[2];
	const struct spi_buf_set rx = { .buffers = rx_buf, .count = ARRAY_SIZE(rx_buf) };

	// ignore first RX for transceive when also sending data
	rx_buf[0].buf = NULL;
	rx_buf[0].len = 3;
	rx_buf[1].len = len;
	int ret;

	rx_buf[1].buf = &data[0];
	ret = spi_transceive_dt(&bus->spi, &tx, &rx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	return 0;
}

static int frammb85rs64v_reg_write_spi(const union frammb85rs64v_bus *bus, uint16_t addr,
				       const uint8_t *data, size_t len)
{
	int ret;
	uint8_t access[3];
	access[0] = MB85RS64V_WRITE_CMD;
	access[1] = (addr >> 8) & 0xFF;
	access[2] = addr & 0xFF;

	uint8_t write_enable[] = { MB85RS64V_WRITE_ENABLE_CMD };
	struct spi_buf write_enable_tx_buf = { .buf = &write_enable, .len = 1 };
	const struct spi_buf_set write_enable_tx = { .buffers = &write_enable_tx_buf, .count = 1 };
	// Write enable latch command before writing data
	ret = spi_write_dt(&bus->spi, &write_enable_tx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	const struct spi_buf tx_buf[2] = { { .buf = &access, .len = 3 },
					   { .buf = (uint8_t *)data, .len = len } };
	const struct spi_buf_set tx = { .buffers = tx_buf, .count = 2 };

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	return 0;
}

const struct frammb85rs64v_bus_io frammb85rs64v_bus_io_spi = {
	.check = frammb85rs64v_bus_check_spi,
	.read = frammb85rs64v_reg_read_spi,
	.write = frammb85rs64v_reg_write_spi,
	.read_id = frammb85rs64v_read_id_spi,
};
#endif /* FRAMMB85RS64V_BUS_SPI */
