/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2C_RTIO_H_
#define ZEPHYR_DRIVERS_I2C_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver context for implementing i2c with rtio
 */
struct i2c_rtio {
	struct k_sem lock;
	struct k_spinlock slock;
	struct rtio *r;
	struct rtio_mpsc io_q;
	struct rtio_iodev iodev;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct i2c_dt_spec dt_spec;
};

/**
 * @brief Statically define an i2c_rtio context
 *
 * @param _name Symbolic name of the context
 * @param _sq_sz Submission queue entry pool size
 * @param _cq_sz Completeion queue entry pool size
 */
#define I2C_RTIO_DEFINE(_name, _sq_sz, _cq_sz)		\
	RTIO_DEFINE(CONCAT(_name, _r), _sq_sz, _cq_sz);	\
	static struct i2c_rtio _name = {		\
		.r = &CONCAT(_name, _r),		\
	};

/**
 * @brief Copy an an array of i2c_msgs to rtio submissions and a transaction
 *
 * @retval sqe Last sqe setup in the copy
 * @retval NULL Not enough memory to copy the transaction
 */
struct rtio_sqe *i2c_rtio_copy(struct rtio *r, struct rtio_iodev *iodev, const struct i2c_msg *msgs,
			       uint8_t num_msgs);

/**
 * @brief Initialize an i2c rtio context
 *
 * @param ctx I2C RTIO driver context
 * @param dev I2C bus
 */
void i2c_rtio_init(struct i2c_rtio *ctx, const struct device *dev);

/**
 * @brief Signal that the current (ctx->txn_curr) submission has been completed
 *
 * @param ctx I2C RTIO driver context
 * @param status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool i2c_rtio_complete(struct i2c_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool i2c_rtio_submit(struct i2c_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Configure the I2C bus controller
 *
 * Provides a compatible API for the existing i2c_configure API, and blocks the
 * caller until the transfer completes.
 *
 * See i2c_configure().
 */
int i2c_rtio_configure(struct i2c_rtio *ctx, uint32_t i2c_config);

/**
 * @brief Transfer i2c messages in a blocking call
 *
 * Provides a compatible API for the existing i2c_transfer API, and blocks the caller
 * until the transfer completes.
 *
 * See i2c_transfer().
 */
int i2c_rtio_transfer(struct i2c_rtio *ctx, struct i2c_msg *msgs, uint8_t num_msgs, uint16_t addr);

/**
 * @brief Perform an I2C bus recovery in a blocking call
 *
 * Provides a compatible API for the existing i2c_recover API, and blocks the caller
 * until the process completes.
 *
 * See i2c_recover().
 */
int i2c_rtio_recover(struct i2c_rtio *ctx);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRVIERS_I2C_RTIO_H_ */
