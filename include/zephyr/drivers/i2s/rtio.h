/*
 * SPDX-FileCopyrightText: Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_I2S_RTIO_H_
#define ZEPHYR_DRIVERS_I2S_RTIO_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/sys/atomic.h>

/**
 * @brief Driver context for implementing I2S with RTIO
 */
struct i2s_rtio {
	struct k_spinlock lock;
	struct mpsc io_q;
	struct rtio_iodev_sqe *curr;
	struct mpsc prev_q;
	atomic_t io_q_size;
	bool cancelled;
};

/**
 * @brief Statically define an i2s_rtio context
 *
 * @param _name Symbolic name of the context
 */
#define I2S_RTIO_DEFINE(_name) \
	static struct i2s_rtio _name

/**
 * @brief Initialize a I2S RTIO context
 *
 * @param ctx I2S RTIO driver context
 */
void i2s_rtio_init(struct i2s_rtio *ctx);

/**
 * @brief Signal that the next submission is ready to be queued up
 *
 * @details I2S hardware typically supports quing up buffers (submissions) while a submission is
 *          currently being executed. This function will "peek" the IO queue (io_q), and if a
 *          successive submission exists, set it as the current submission to be queued up in
 *          hardware, while the currently executing submission completes.
 *
 *          Once a submission completes, call @ref i2s_rtio_complete.
 *
 * @note A call to this function must be followed by a call to @ref i2s_rtio_complete once the
 *       submission completes.
 *
 * @note When peeking the IO queue, if the next submission's configuration and operation differs
 *       from the currently executing submission, this function will return false as the stream
 *       must be reconfigured to match the configuration and operation.
 *
 * @param[in] ctx I2S RTIO driver context
 *
 * @retval true Next submission is ready to be queued up
 * @retval false Current submission is last submission of stream
 */
bool i2s_rtio_continue(struct i2s_rtio *ctx);

/**
 * @brief Signal that a submission was cancelled
 *
 * @details When one or more submissions are cancelled, we need to purge them from the IO queue,
 *          and if one or more of the submissions are part of the stream, the stream must be
 *          stopped.
 *
 * @param[in] ctx I2S RTIO driver context
 *
 * @note @ref i2s_rtio_complete must be called when a submission completes, even if cancelled.
 *
 * @retval true Stream has been cancelled and must be stopped if possible
 * @retval false Stream has not been cancelled and must continue
 */
bool i2s_rtio_cancel(struct i2s_rtio *ctx);

/**
 * @brief Check whether the current submission has been cancelled
 *
 * @param[in] ctx I2S RTIO driver context
 *
 * @retval true Currenty executing
 * @retval false No action required
 */
bool i2s_rtio_is_cancelled(struct i2s_rtio *ctx);

/**
 * @brief Signal that a previous submission has been completed
 *
 * @param[in] ctx I2S RTIO driver context
 * @param[in] status Completion status, negative values are errors
 *
 * @retval true Next submission is ready to start
 * @retval false No more submissions to work on
 */
bool i2s_rtio_complete(struct i2s_rtio *ctx, int status);

/**
 * @brief Submit, atomically, a submission to work on at some point
 *
 * @retval true Next submission is ready to start
 * @retval false No new submission to start or submissions are in progress already
 */
bool i2s_rtio_submit(struct i2s_rtio *ctx, struct rtio_iodev_sqe *iodev_sqe);

#endif /* ZEPHYR_DRIVERS_I2S_RTIO_H_ */
