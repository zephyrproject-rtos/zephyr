/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_RM3100_BUS_H_
#define ZEPHYR_DRIVERS_SENSOR_RM3100_BUS_H_

#include <stdint.h>
#include <zephyr/rtio/rtio.h>

#include "rm3100.h"
#include "rm3100_reg.h"

static inline int rm3100_bus_read(const struct device *dev,
				uint8_t reg,
				uint8_t *buf,
				uint16_t len)
{
	struct rm3100_data *data = dev->data;
	struct rtio *ctx = data->rtio.ctx;
	struct rtio_iodev *iodev = data->rtio.iodev;
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int err;

	if (!write_sqe || !read_sqe) {
		return -ENOMEM;
	}

	reg = reg | REG_READ_BIT;

	rtio_sqe_prep_write(write_sqe, iodev, RTIO_PRIO_HIGH, &reg, 1, NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	if (rtio_is_i2c(data->rtio.type)) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	err = rtio_submit(ctx, 2);
	if (err) {
		return err;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return err;
}

static inline int rm3100_bus_write(const struct device *dev,
				uint8_t reg,
				const uint8_t *buf,
				uint16_t len)
{
	struct rm3100_data *data = dev->data;
	struct rtio *ctx = data->rtio.ctx;
	struct rtio_iodev *iodev = data->rtio.iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int err;

	if (!write_reg_sqe || !write_buf_sqe) {
		return -ENOMEM;
	}

	rtio_sqe_prep_write(write_reg_sqe, iodev, RTIO_PRIO_HIGH, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;

	err = rtio_submit(ctx, 2);
	if (err) {
		return err;
	}

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	return err;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_RM3100_BUS_H_ */
