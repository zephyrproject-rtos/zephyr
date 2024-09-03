/*
 * Copyright (c) 2016, 2017 Intel Corporation
 * Copyright (c) 2017 IpTronix S.r.l.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMP388s accessed via SPI.
 */

#include <zephyr/logging/log.h>
#include "bmp388.h"

#ifdef BMP3XX_USE_SPI_BUS

LOG_MODULE_DECLARE(BMP388, CONFIG_SENSOR_LOG_LEVEL);

static int bmp388_bus_check_spi(const union bmp388_bus *bus)
{
	return spi_is_ready_dt(&bus->spi) ? 0 : -ENODEV;
}

static int bmp388_reg_read_spi(const union bmp388_bus *bus, uint8_t regaddr, uint8_t *buf, int size)
{
	int ret;

	if ((size <= 0) || (size > BMP388_SAMPLE_BUFFER_SIZE)) {
		return -EINVAL;
	}

	uint8_t buffer[size + 2];
	const struct spi_buf rxtx_buf = {
		.buf = &buffer,
		.len = ARRAY_SIZE(buffer),
	};
	const struct spi_buf_set rxtx = {
		.buffers = &rxtx_buf,
		.count = 1
	};

	memset(buffer, 0x00, ARRAY_SIZE(buffer));
	buffer[0] = (regaddr) | 0x80;

	ret = spi_transceive_dt(&bus->spi, &rxtx, &rxtx);
	if (ret) {
		LOG_DBG("spi_transceive FAIL %d\n", ret);
		return ret;
	}

	memcpy(buf, (uint8_t *)&buffer[2], size);

	return 0;
}

static int bmp388_reg_write_spi(const union bmp388_bus *bus,
				uint8_t reg, uint8_t val)
{
	uint8_t cmd[] = { reg & 0x7F, val };
	const struct spi_buf tx_buf = {
		.buf = cmd,
		.len = sizeof(cmd)
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	int ret;

	ret = spi_write_dt(&bus->spi, &tx);
	if (ret) {
		LOG_DBG("spi_write FAIL %d\n", ret);
		return ret;
	}
	return 0;
}

const struct bmp388_bus_io bmp388_bus_io_spi = {
	.check = bmp388_bus_check_spi,
	.read = bmp388_reg_read_spi,
	.write = bmp388_reg_write_spi,
};
#endif /* BMP3XX_USE_SPI_BUS */
