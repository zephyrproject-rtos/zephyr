/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio_spsc.h>
#include <zephyr/sys/__assert.h>

const struct rtio_iodev_api i2c_iodev_api = {
	.submit = i2c_iodev_submit,
};

struct rtio_sqe *i2c_rtio_copy(struct rtio *r,
			       struct rtio_iodev *iodev,
			       const struct i2c_msg *msgs,
			       uint8_t num_msgs)
{
	__ASSERT(num_msgs > 0, "Expecting at least one message to copy");

	struct rtio_sqe *sqe = NULL;

	for (uint8_t i = 0; i < num_msgs; i++) {
		sqe = rtio_sqe_acquire(r);

		if (sqe == NULL) {
			rtio_spsc_drop_all(r->sq);
			return NULL;
		}

		if (msgs[i].flags & I2C_MSG_READ) {
			rtio_sqe_prep_read(sqe, iodev, RTIO_PRIO_NORM,
					   msgs[i].buf, msgs[i].len, NULL);
		} else {
			rtio_sqe_prep_write(sqe, iodev, RTIO_PRIO_NORM,
					    msgs[i].buf, msgs[i].len, NULL);
		}
		sqe->flags |= RTIO_SQE_TRANSACTION;
		sqe->iodev_flags = ((msgs[i].flags & I2C_MSG_STOP) ? RTIO_IODEV_I2C_STOP : 0) |
			((msgs[i].flags & I2C_MSG_RESTART) ? RTIO_IODEV_I2C_RESTART : 0) |
			((msgs[i].flags & I2C_MSG_ADDR_10_BITS) ? RTIO_IODEV_I2C_10_BITS : 0);
	}

	sqe->flags &= ~RTIO_SQE_TRANSACTION;

	return sqe;
}
