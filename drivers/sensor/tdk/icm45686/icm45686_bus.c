/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "icm45686_bus.h"

int icm45686_prep_reg_read_rtio_async(const struct icm45686_bus *bus,
				    uint8_t reg, uint8_t *buf, size_t size,
				    struct rtio_sqe **out)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *read_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !read_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	if (bus->rtio.type == ICM45686_BUS_I2C) {
		read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (bus->rtio.type == ICM45686_BUS_I3C) {
		read_buf_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	} else {
		/* SPI does not require setting additional flags to the read-buf SQE */
	}

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = read_buf_sqe;
	}

	return 2;
}

int icm45686_prep_reg_write_rtio_async(const struct icm45686_bus *bus,
				     uint8_t reg, const uint8_t *buf, size_t size,
				     struct rtio_sqe **out)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_reg_sqe = rtio_sqe_acquire(ctx);
	struct rtio_sqe *write_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !write_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	/** More than 7 won't work with tiny-write */
	if (size > 7) {
		return -EINVAL;
	}

	rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_tiny_write(write_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	if (bus->rtio.type == ICM45686_BUS_I2C) {
		write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	} else if (bus->rtio.type == ICM45686_BUS_I3C) {
		write_buf_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP;
	} else {
		/* SPI does not require setting additional flags to the write-buf SQE */
	}

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = write_buf_sqe;
	}

	return 2;
}

int icm45686_reg_read_rtio(const struct icm45686_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = icm45686_prep_reg_read_rtio_async(bus, start, buf, size, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
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

int icm45686_reg_write_rtio(const struct icm45686_bus *bus, uint8_t reg, const uint8_t *buf,
			    int size)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = icm45686_prep_reg_write_rtio_async(bus, reg, buf, size, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = rtio_submit(ctx, ret);
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
