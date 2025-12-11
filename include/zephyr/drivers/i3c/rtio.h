/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I3C_RTIO_H_
#define ZEPHYR_DRIVERS_I3C_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Driver context for implementing i3c with rtio
 */
struct i3c_rtio {
	struct k_sem lock;
	struct k_spinlock slock;
	struct rtio *r;
	struct mpsc io_q;
	struct rtio_iodev iodev;
	struct rtio_iodev_sqe *txn_head;
	struct rtio_iodev_sqe *txn_curr;
	struct i3c_device_desc *i3c_desc;
};

/**
 * @brief Statically define an i3c_rtio context
 *
 * @param _name Symbolic name of the context
 * @param _sq_sz Submission queue entry pool size
 * @param _cq_sz Completion queue entry pool size
 */
#define I3C_RTIO_DEFINE(_name, _sq_sz, _cq_sz)		\
	RTIO_DEFINE(CONCAT(_name, _r), _sq_sz, _cq_sz);	\
	static struct i3c_rtio _name = {		\
		.r = &CONCAT(_name, _r),		\
	};

/**
 * @brief Copy an array of i3c_msgs to rtio submissions and a transaction
 *
 * @retval sqe Last sqe setup in the copy
 * @retval NULL Not enough memory to copy the transaction
 */
struct rtio_sqe *i3c_rtio_copy(struct rtio *r, struct rtio_iodev *iodev, const struct i3c_msg *msgs,
			       uint8_t num_msgs);

/**
 * @brief Initialize an i3c rtio context
 *
 * @param ctx I3C RTIO driver context
 */
void i3c_rtio_init(struct i3c_rtio *ctx);

/**
 * @brief Signal that the current (ctx->txn_curr) submission has been completed
 *
 * @param ctx I3C RTIO driver context
 * @param status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool i3c_rtio_complete(struct i3c_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool i3c_rtio_submit(struct i3c_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Configure the I3C bus controller
 *
 * Provides a compatible API for the existing i3c_configure API, and blocks the
 * caller until the transfer completes.
 *
 * See i3c_configure().
 */
int i3c_rtio_configure(struct i3c_rtio *ctx, enum i3c_config_type type, void *config);

/**
 * @brief Transfer i3c messages in a blocking call
 *
 * Provides a compatible API for the existing i3c_transfer API, and blocks the caller
 * until the transfer completes.
 *
 * See i3c_transfer().
 */
int i3c_rtio_transfer(struct i3c_rtio *ctx, struct i3c_msg *msgs, uint8_t num_msgs,
			       struct i3c_device_desc *desc);

/**
 * @brief Perform an I3C bus recovery in a blocking call
 *
 * Provides a compatible API for the existing i3c_recover API, and blocks the caller
 * until the process completes.
 *
 * See i3c_recover().
 */
int i3c_rtio_recover(struct i3c_rtio *ctx);

/**
 * @brief Perform an I3C CCC in a blocking call
 *
 * Provides a compatible API for the existing i3c_do_ccc API, and blocks the caller
 * until the process completes.
 *
 * See i3c_do_ccc().
 */
int i3c_rtio_ccc(struct i3c_rtio *ctx, struct i3c_ccc_payload *payload);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRVIERS_I3C_RTIO_H_ */
