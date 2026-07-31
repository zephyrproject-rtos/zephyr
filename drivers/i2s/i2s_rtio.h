/*
 *  SPDX-FileCopyrightText: 2026 Basalte bv
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_RTIO_H_
#define ZEPHYR_DRIVERS_I2S_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Per-instance RTIO context shared by I2S drivers implementing iodev_submit().
 *
 * Tracks the FIFO of pending submissions and which one is currently being processed by
 * the backend driver. A driver owns one of these per I2S device instance (allocated with
 * I2S_RTIO_DEFINE()) and drives it via i2s_rtio_submit()/_advance()/_complete().
 */
struct i2s_rtio {
	/** I2S device this context belongs to. */
	const struct device *i2s_dev;
	/** RTIO context the current transaction was submitted against. */
	struct rtio *r;

	/** Lock-free FIFO of submitted, not-yet-started requests. */
	struct mpsc io_q;
	/** iodev registered against this context; its .data points back at i2s_dev. */
	struct rtio_iodev iodev;
	/** First entry of the transaction currently being processed. */
	struct rtio_iodev_sqe *txn_head;
	/** Entry of the transaction currently being processed. */
	struct rtio_iodev_sqe *txn_curr;

	/** Initialized here; available for the backend driver's own serialization needs. */
	struct k_sem lock;
	/** Protects io_q/txn_head/txn_curr against concurrent submit/advance/complete calls. */
	struct k_spinlock slock;
};

/**
 * @brief Statically define an i2s_rtio context.
 *
 * @param _name Symbol name for the context.
 */
#define I2S_RTIO_DEFINE(_name) static struct i2s_rtio _name;

/**
 * @brief Initialize an i2s_rtio context.
 *
 * Call once from the backend driver's device init function, before any submissions can
 * arrive.
 *
 * @param ctx Context to initialize.
 * @param dev I2S device the context belongs to.
 */
void i2s_rtio_init(struct i2s_rtio *ctx, const struct device *dev);

/**
 * @brief Queue a newly submitted RTIO request.
 *
 * Call from the driver's iodev_submit() callback. If no transaction is currently in
 * flight, this makes @p iodev_sqe (or the head of its transaction chain) the current one.
 *
 * @param ctx Context to submit into.
 * @param iodev_sqe Submission queue entry passed to iodev_submit().
 *
 * @return true if the caller should start processing ctx->txn_curr now, false if a
 *         transaction was already in flight and this request was merely queued.
 */
bool i2s_rtio_submit(struct i2s_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief Advance to the next queued request without completing the current one.
 *
 * Used for multi-part transactions where an intermediate step doesn't produce its own CQE.
 *
 * @param ctx Context to advance.
 *
 * @return true if a new current transaction is available and should be started, false if
 *         the queue is empty.
 */
bool i2s_rtio_advance(struct i2s_rtio *ctx);

/**
 * @brief Complete the current transaction and advance to the next queued one.
 *
 * Posts the CQE for the transaction that was current (via rtio_iodev_sqe_ok()/_err()
 * depending on @p status), then pops the next queued request.
 *
 * @param ctx Context to complete.
 * @param status 0 (or positive) on success, negative errno on failure.
 *
 * @return true if a new current transaction is available and should be started, false if
 *         the queue is empty.
 */
bool i2s_rtio_complete(struct i2s_rtio *ctx, int status);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_I2S_RTIO_H_ */
