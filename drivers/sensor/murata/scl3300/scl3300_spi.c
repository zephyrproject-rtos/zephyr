/*
 * Copyright (c) 2026 Sacra Systems Private Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/byteorder.h>

#include "scl3300_spi.h"

LOG_MODULE_REGISTER(scl3300_spi, CONFIG_SENSOR_LOG_LEVEL);

static int scl3300_spi_transceive_sync(const struct device *dev, uint32_t command, uint8_t *rx_buf,
				       size_t len)
{
	const struct scl3300_config *cfg = dev->config;
	struct scl3300_data *data = dev->data;
	uint8_t tx_buf[SCL3300_SPI_FRAME_SIZE];
	int ret;

	if (cfg == NULL || data == NULL || rx_buf == NULL) {
		return -EINVAL;
	}

	sys_put_be32(command, tx_buf);
	struct rtio_sqe *sqe;

	sqe = rtio_sqe_acquire(data->rtio);
	if (sqe == NULL) {
		return -ENOMEM;
	}

	rtio_sqe_prep_transceive(sqe, cfg->spi_iodev, RTIO_PRIO_NORM, tx_buf, rx_buf, len, data);

	ret = rtio_submit(data->rtio, 0);
	if (ret < 0) {
		return ret;
	}
	struct rtio_cqe *cqe;

	cqe = rtio_cqe_consume_block(data->rtio);
	if (cqe == NULL) {
		return -EIO;
	}

	ret = cqe->result;
	rtio_cqe_release(data->rtio, cqe);

	return ret;
}

int scl3300_spi_read_command(const struct device *dev, uint32_t command, uint8_t *rx_buf)
{
	return scl3300_spi_transceive_sync(dev, command, rx_buf, SCL3300_SPI_FRAME_SIZE);
}

int scl3300_spi_write_command(const struct device *dev, uint32_t command)
{
	uint8_t rx_buf[SCL3300_SPI_FRAME_SIZE];

	return scl3300_spi_transceive_sync(dev, command, rx_buf, SCL3300_SPI_FRAME_SIZE);
}

int scl3300_spi_init(const struct device *dev)
{
	struct scl3300_data *data = dev->data;
	const struct scl3300_config *cfg = dev->config;

	if (!spi_is_ready_iodev(cfg->spi_iodev)) {
		return -ENODEV;
	}

	data->ctx_spi.read = (scl3300_read_ptr)scl3300_spi_read_command;
	data->ctx_spi.write = (scl3300_write_ptr)scl3300_spi_write_command;

	data->ctx = &data->ctx_spi;
	data->ctx->handle = (void *)dev;

	return 0;
}
