/*
 * Copyright (c) 2024 Bosch Sensortec GmbH
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmm350.h"

static int bmm350_bus_check_rtio(const struct bmm350_bus *bus)
{
	if (bus->rtio.type == BMM350_BUS_TYPE_I2C) {
		return i2c_is_ready_iodev(bus->rtio.iodev) ? 0 : -ENODEV;
	}

	return -EIO;
}

static int bmm350_reg_read_rtio(const struct bmm350_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int ret;

	if (!write_sqe || !read_sqe) {
		return -ENOMEM;
	}

	rtio_sqe_prep_write(write_sqe, iodev, RTIO_PRIO_NORM, &start, 1, NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	if (bus->rtio.type == BMM350_BUS_TYPE_I2C) {
		read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	}

	ret = rtio_submit(ctx, 2);
	if (ret) {
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

static int bmm350_reg_write_rtio(const struct bmm350_bus *bus, uint8_t reg, uint8_t val)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);
	struct rtio_cqe *cqe;
	int ret;

	if (!write_reg_sqe || !write_buf_sqe) {
		return -ENOMEM;
	}

	rtio_sqe_prep_write(write_reg_sqe, iodev, RTIO_PRIO_HIGH, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_HIGH, &val, 1, NULL);
	if (bus->rtio.type == BMM350_BUS_TYPE_I2C) {
		write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	}

	ret = rtio_submit(ctx, 2);
	if (ret) {
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

const struct bmm350_bus_io bmm350_bus_rtio = {
	.check = bmm350_bus_check_rtio,
	.read = bmm350_reg_read_rtio,
	.write = bmm350_reg_write_rtio,
};
