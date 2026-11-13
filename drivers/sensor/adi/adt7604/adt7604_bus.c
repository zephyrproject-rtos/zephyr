/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "adt7604.h"
#include "adt7604_bus.h"

LOG_MODULE_DECLARE(adt7604, CONFIG_SENSOR_LOG_LEVEL);

int adt7604_bus_read(const struct device *dev, uint16_t reg, uint8_t *buf, size_t len)
{
	struct adt7604_data *data = dev->data;
	struct rtio_iodev *iodev = data->iodev;
	struct rtio *ctx = data->ctx;
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int ret;

	if (!read_sqe || !write_sqe) {
		return -ENOMEM;
	}

	uint8_t tx_buf[3] = {ADT7604_SPI_READ_CMD, (reg >> 8) & 0xFF, reg & 0xFF};

	rtio_sqe_prep_tiny_write(write_sqe, iodev, RTIO_PRIO_NORM, tx_buf, sizeof(tx_buf), NULL);
	write_sqe->flags = RTIO_SQE_CHAINED | RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_sqe, iodev, RTIO_PRIO_NORM, buf, len, NULL);

	ret = rtio_submit(ctx, 2);
	if (ret < 0) {
		LOG_ERR("Failed to submit RTIO SQEs: %d", ret);
		return ret;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			ret = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return ret;
}

int adt7604_bus_write(const struct device *dev, uint16_t reg, const uint8_t *buf, size_t len)
{
	struct adt7604_data *data = dev->data;
	struct rtio_iodev *iodev = data->iodev;
	struct rtio *ctx = data->ctx;
	struct rtio_sqe *write_cmd_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int ret;

	if (!write_cmd_sqe || !write_buf_sqe) {
		return -ENOMEM;
	}

	uint8_t tx_buf[3] = {ADT7604_SPI_WRITE_CMD, (reg >> 8) & 0xFF, reg & 0xFF};

	rtio_sqe_prep_tiny_write(write_cmd_sqe, iodev, RTIO_PRIO_NORM, tx_buf, sizeof(tx_buf),
				 NULL);
	write_cmd_sqe->flags = RTIO_SQE_CHAINED | RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_NORM, buf, len, NULL);

	ret = rtio_submit(ctx, 2);
	if (ret < 0) {
		LOG_ERR("Failed to submit RTIO SQEs: %d", ret);
		return ret;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			ret = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return ret;
}
