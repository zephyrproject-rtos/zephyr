/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Bus-specific functionality for BMI270s accessed via I2C.
 */

#include <string.h>
#include "bmi270.h"

static int bmi270_bus_check_i2c(const union bmi270_bus *bus)
{
	return device_is_ready(bus->i2c.bus) ? 0 : -ENODEV;
}

static int bmi270_reg_read_i2c(const union bmi270_bus *bus,
			       uint8_t start, uint8_t *data, uint16_t len)
{
	return i2c_burst_read_dt(&bus->i2c, start, data, len);
}

static int bmi270_reg_write_i2c(const union bmi270_bus *bus, uint8_t start,
				const uint8_t *data, uint16_t len)
{
	/* Combine register address and data into a single buffer and
	 * use i2c_write_dt() instead of i2c_burst_write_dt() which
	 * may not be supported on all I2C devices.
	 * Maximum write length is BMI270_WR_LEN + 1 byte for address.
	 */
	uint8_t buf[1 + BMI270_WR_LEN];

	if (len > BMI270_WR_LEN) {
		return -EINVAL;
	}

	buf[0] = start;
	if (len > 0) {
		memcpy(&buf[1], data, len);
	}

	return i2c_write_dt(&bus->i2c, buf, 1 + len);
}

static int bmi270_bus_init_i2c(const union bmi270_bus *bus)
{
	/* I2C is used by default
	 */
	return 0;
}

const struct bmi270_bus_io bmi270_bus_io_i2c = {
	.check = bmi270_bus_check_i2c,
	.read = bmi270_reg_read_i2c,
	.write = bmi270_reg_write_i2c,
	.init = bmi270_bus_init_i2c,
};

#if defined(CONFIG_BMI270_STREAM)
int bmi270_i2c_prep_reg_read_async(const struct device *dev, uint8_t reg, uint8_t *buf,
				   size_t len, uint8_t flags)
{
	struct bmi270_data *data = dev->data;
	struct rtio_sqe *sqes[2];
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *read_buf_sqe;

	if (rtio_sqe_acquire_array(data->rtio_ctx, ARRAY_SIZE(sqes), sqes) != 0) {
		return -ENOMEM;
	}

	write_reg_sqe = sqes[0];
	read_buf_sqe = sqes[1];

	rtio_sqe_prep_tiny_write(write_reg_sqe, data->iodev, RTIO_PRIO_HIGH, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_buf_sqe, data->iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	read_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	read_buf_sqe->flags |= flags;

	return 2;
}

int bmi270_i2c_prep_reg_write_async(const struct device *dev, uint8_t reg, const uint8_t *buf,
				    size_t len, uint8_t flags)
{
	struct bmi270_data *data = dev->data;
	struct rtio_sqe *sqes[2];
	struct rtio_sqe *write_reg_sqe;
	struct rtio_sqe *write_buf_sqe;

	if (rtio_sqe_acquire_array(data->rtio_ctx, ARRAY_SIZE(sqes), sqes) != 0) {
		return -ENOMEM;
	}

	write_reg_sqe = sqes[0];
	write_buf_sqe = sqes[1];

	rtio_sqe_prep_tiny_write(write_reg_sqe, data->iodev, RTIO_PRIO_HIGH, &reg, 1, NULL);
	write_reg_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_write(write_buf_sqe, data->iodev, RTIO_PRIO_HIGH, buf, len, NULL);
	write_buf_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP;
	write_buf_sqe->flags |= flags;

	return 2;
}
#endif
