/* ST Microelectronics IIS2DH 3-axis accelerometer driver
 *
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://www.st.com/resource/en/datasheet/iis2dh.pdf
 */

#define DT_DRV_COMPAT st_iis2dh

#include <string.h>
#include "iis2dh.h"
#include <zephyr/logging/log.h>

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define IIS2DH_SPI_READM	(3 << 6)	/* 0xC0 */
#define IIS2DH_SPI_WRITEM	(1 << 6)	/* 0x40 */

LOG_MODULE_DECLARE(IIS2DH, CONFIG_SENSOR_LOG_LEVEL);

static int iis2dh_spi_read(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
	const struct iis2dh_device_config *config = dev->config;
	uint8_t buffer_tx[2] = { reg | IIS2DH_SPI_READM, 0 };
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

static int iis2dh_spi_write(const struct device *dev, uint8_t reg, uint8_t *data, uint16_t len)
{
	const struct iis2dh_device_config *config = dev->config;
	uint8_t buffer_tx[1] = { reg | IIS2DH_SPI_WRITEM };
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

stmdev_ctx_t iis2dh_spi_ctx = {
	.read_reg = (stmdev_read_ptr) iis2dh_spi_read,
	.write_reg = (stmdev_write_ptr) iis2dh_spi_write,
};

int iis2dh_spi_init(const struct device *dev)
{
	struct iis2dh_data *data = dev->data;
	const struct iis2dh_device_config *config = dev->config;

	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	data->ctx = &iis2dh_spi_ctx;
	data->ctx->handle = (void *)dev;

	return 0;
}
#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
