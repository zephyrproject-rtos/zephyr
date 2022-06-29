/* ST Microelectronics IIS3DHHC accelerometer sensor
 *
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis3dhhc.pdf
 */

#define DT_DRV_COMPAT st_iis3dhhc

#include <string.h>
#include "iis3dhhc.h"
#include <zephyr/logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define IIS3DHHC_SPI_READ		(1 << 7)

LOG_MODULE_DECLARE(IIS3DHHC, CONFIG_SENSOR_LOG_LEVEL);

static int iis3dhhc_spi_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
	const struct iis3dhhc_config *config = dev->config;
	uint8_t buffer_tx[2] = { reg | IIS3DHHC_SPI_READ, 0 };
	const struct spi_buf tx_buf = {
			.buf = buffer_tx,
			.len = 2,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1
	};
	const struct spi_buf rx_buf[2] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf,
		.count = 2
	};

	if (spi_transceive_dt(&config->spi, &tx, &rx)) {
		return -EIO;
	}

	return 0;
}

static int iis3dhhc_spi_write(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
	const struct iis3dhhc_config *config = dev->config;
	uint8_t buffer_tx[1] = { reg & ~IIS3DHHC_SPI_READ };
	const struct spi_buf tx_buf[2] = {
		{
			.buf = buffer_tx,
			.len = 1,
		},
		{
			.buf = data,
			.len = len,
		}
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf,
		.count = 2
	};


	if (spi_write_dt(&config->spi, &tx)) {
		return -EIO;
	}

	return 0;
}

stmdev_ctx_t iis3dhhc_spi_ctx = {
	.read_reg = (stmdev_read_ptr) iis3dhhc_spi_read,
	.write_reg = (stmdev_write_ptr) iis3dhhc_spi_write,
};

int iis3dhhc_spi_init(const struct device *dev)
{
	struct iis3dhhc_data *data = dev->data;
	const struct iis3dhhc_config *config = dev->config;

	if (!spi_is_ready(&config->spi)) {
		LOG_ERR("SPI bus is not ready");
		return -ENODEV;
	};

	data->ctx = &iis3dhhc_spi_ctx;
	data->ctx->handle = (void *)dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
