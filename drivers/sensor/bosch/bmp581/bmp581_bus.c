/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmp581.h"
#include "bmp581_bus.h"

#include <string.h>

#define BMP581_REG_WRITE_PAYLOAD_MAX 8

static int bmp581_reg_read_rtio_submit(const struct bmp581_bus *bus, const uint8_t *reg_addr,
				       uint8_t *buf, int size)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = bmp581_prep_reg_read_rtio_async(bus, reg_addr, buf, size, NULL);
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

static int bmp581_reg_write_rtio_submit(const struct bmp581_bus *bus, const uint8_t *reg_addr,
					const uint8_t *buf, int size)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_cqe *cqe;
	int ret;

	ret = bmp581_prep_reg_write_rtio_async(bus, reg_addr, buf, size, NULL);
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

int bmp581_prep_reg_read_rtio_async(const struct bmp581_bus *bus, const uint8_t *reg_addr,
				    uint8_t *buf, size_t size, struct rtio_sqe **out)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *read_buf_sqe;

	if (reg_addr == NULL || buf == NULL) {
		return -EINVAL;
	}

	write_reg_sqe = rtio_sqe_acquire(ctx);
	read_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !read_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	/* SPI reads require bit 7 of the register address to be set */
	if (bus->rtio.type == BMP581_BUS_TYPE_SPI) {
		uint8_t reg_addr_spi = *reg_addr | BMP5_SPI_RD_MASK;

		rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg_addr_spi, 1,
					 NULL);
	} else {
		/* @p reg_addr is sampled at transmit time; keep it stable for the transaction. */
		rtio_sqe_prep_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, reg_addr, 1U, NULL);
	}
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_read(read_buf_sqe, iodev, RTIO_PRIO_NORM, buf, size, NULL);
	if (bus->rtio.type == BMP581_BUS_TYPE_I2C) {
		read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	} else if (bus->rtio.type == BMP581_BUS_TYPE_I3C) {
		read_buf_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP | RTIO_IODEV_I3C_RESTART;
	}
	/* SPI does not require setting additional flags to the read-buf SQE */

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = read_buf_sqe;
	}

	return 2;
}

int bmp581_prep_reg_write_rtio_async(const struct bmp581_bus *bus, const uint8_t *reg_addr,
				     const uint8_t *buf, size_t size, struct rtio_sqe **out)
{
	struct rtio *ctx = bus->rtio.ctx;
	struct rtio_iodev *iodev = bus->rtio.iodev;
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *write_buf_sqe;

	if (size == 0 || size > UINT8_MAX) {
		return -EINVAL;
	}

	if (reg_addr == NULL || buf == NULL) {
		return -EINVAL;
	}

	write_reg_sqe = rtio_sqe_acquire(ctx);
	write_buf_sqe = rtio_sqe_acquire(ctx);

	if (!write_reg_sqe || !write_buf_sqe) {
		rtio_sqe_drop_all(ctx);
		return -ENOMEM;
	}

	/* SPI writes use the register address with bit 7 cleared (default) */
	if (bus->rtio.type == BMP581_BUS_TYPE_SPI) {
		uint8_t reg_byte = *reg_addr;

		rtio_sqe_prep_tiny_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, &reg_byte, 1, NULL);
	} else {
		rtio_sqe_prep_write(write_reg_sqe, iodev, RTIO_PRIO_NORM, reg_addr, 1U, NULL);
	}
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;
	rtio_sqe_prep_write(write_buf_sqe, iodev, RTIO_PRIO_NORM, buf, (uint32_t)size, NULL);
	if (bus->rtio.type == BMP581_BUS_TYPE_I2C) {
		write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	} else if (bus->rtio.type == BMP581_BUS_TYPE_I3C) {
		write_buf_sqe->iodev_flags |= RTIO_IODEV_I3C_STOP;
	}
	/* SPI does not require setting additional flags to the write-buf SQE */

	/** Send back last SQE so it can be concatenated later. */
	if (out) {
		*out = write_buf_sqe;
	}

	return 2;
}

int bmp581_reg_read_rtio(const struct bmp581_bus *bus, uint8_t start, uint8_t *buf, int size)
{
	uint8_t reg_byte = start;

	if (buf == NULL || size <= 0) {
		return -EINVAL;
	}

	if (bus->rtio.type == BMP581_BUS_TYPE_I2C && bus->i2c_spec != NULL) {
		return i2c_write_read_dt(bus->i2c_spec, &reg_byte, 1U, buf, (size_t)size);
	}

	return bmp581_reg_read_rtio_submit(bus, &reg_byte, buf, size);
}

int bmp581_bus_i2c_burst_write(const struct bmp581_bus *bus, const uint8_t *data, size_t len)
{
	uint8_t tmp[8];

	if (data == NULL || len == 0 || len > sizeof(tmp)) {
		return -EINVAL;
	}

	if (bus->i2c_spec == NULL || bus->rtio.type != BMP581_BUS_TYPE_I2C) {
		return -ENOTSUP;
	}

	memcpy(tmp, data, len);
	return i2c_write_dt(bus->i2c_spec, tmp, (uint32_t)len);
}

int bmp581_reg_write_rtio(const struct bmp581_bus *bus, uint8_t reg, const uint8_t *buf, int size)
{
	uint8_t wr[1 + BMP581_REG_WRITE_PAYLOAD_MAX];

	if (buf == NULL || size <= 0) {
		return -EINVAL;
	}

	if (size > BMP581_REG_WRITE_PAYLOAD_MAX) {
		return -EINVAL;
	}

	if (bus->rtio.type == BMP581_BUS_TYPE_I2C && bus->i2c_spec != NULL) {
		wr[0] = reg;
		memcpy(&wr[1], buf, (size_t)size);

		return i2c_write_dt(bus->i2c_spec, wr, (uint32_t)(1 + size));
	}

	return bmp581_reg_write_rtio_submit(bus, &reg, buf, size);
}
