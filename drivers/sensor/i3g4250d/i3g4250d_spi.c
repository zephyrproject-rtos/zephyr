/*
 * Copyright (c) 2021 Jonathan Hahn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_i3g4250d

#include <zephyr/logging/log.h>
#include "i3g4250d.h"

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

#define I3G4250D_SPI_READM      (3 << 6)        /* 0xC0 */
#define I3G4250D_SPI_WRITEM     (1 << 6)        /* 0x40 */

LOG_MODULE_DECLARE(i3g4250d, CONFIG_SENSOR_LOG_LEVEL);

static int i3g4250d_spi_read(const struct device *dev, uint8_t reg,
				 uint8_t *data, uint16_t len)
{
	int ret;
	const struct i3g4250d_device_config *config = dev->config;
	uint8_t buffer_tx[2] = { reg | I3G4250D_SPI_READM, 0 };
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = 2,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
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
		.count = 2,
	};

	ret = spi_transceive_dt(&config->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int i3g4250d_spi_write(const struct device *dev, uint8_t reg,
				  uint8_t *data, uint16_t len)
{
	int ret;
	const struct i3g4250d_device_config *config = dev->config;
	uint8_t buffer_tx[2] = { reg | I3G4250D_SPI_WRITEM, 0 };
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
		.count = 2,
	};

	ret = spi_write_dt(&config->spi, &tx);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

stmdev_ctx_t i3g4250d_spi_ctx = {
	.read_reg = (stmdev_read_ptr) i3g4250d_spi_read,
	.write_reg = (stmdev_write_ptr) i3g4250d_spi_write,
	.mdelay = (stmdev_mdelay_ptr) stmemsc_mdelay,
};

int i3g4250d_spi_init(const struct device *dev)
{
	struct i3g4250d_data *i3g4250d = dev->data;
	const struct i3g4250d_device_config *cfg = dev->config;

	if (!spi_is_ready_dt(&cfg->spi)) {
		LOG_ERR("spi not ready");
		return -ENODEV;
	}

	i3g4250d->ctx = &i3g4250d_spi_ctx;
	i3g4250d->ctx->handle = (void *)dev;

	return 0;
}

#endif /* DT_ANY_INST_ON_BUS_STATUS_OKAY(spi) */
