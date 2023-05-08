/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_CONCURRENT_H_
#define ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_CONCURRENT_H_

#include <zephyr/rtio/rtio.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTIO Concurrent Executor
 *
 * Provides a concurrent executor with a pointer overhead per task and a
 * 2 word overhead over the simple executor to know the order of tasks (fifo).
 *
 * @defgroup rtio_executor_concurrent RTIO concurrent Executor
 * @ingroup rtio
 * @{
 */


/**
 * @brief Submit to the concurrent executor
 *
 * @param r RTIO context to submit
 *
 * @retval 0 always succeeds
 */
int rtio_concurrent_submit(struct rtio *r);

/**
 * @brief Report a SQE has completed successfully
 *
 * @param sqe RTIO IODev SQE to report success
 * @param result Result of the SQE
 */
void rtio_concurrent_ok(struct rtio_iodev_sqe *sqe, int result);

/**
 * @brief Report a SQE has completed with error
 *
 * @param sqe RTIO IODev SQE to report success
 * @param result Result of the SQE
 */
void rtio_concurrent_err(struct rtio_iodev_sqe *sqe, int result);

/**
 * @brief Concurrent Executor
 *
 * Notably all values are effectively owned by each task with the exception
 * of task_in and task_out.
 */
struct rtio_concurrent_executor {
	struct rtio_executor ctx;

	/* Lock around the queues */
	struct k_spinlock lock;

	/* Task ring position and count */
	uint16_t task_in, task_out, task_mask;

	/* First pending sqe to start when a task becomes available */
	struct rtio_sqe *last_sqe;

	/* Array of task statuses */
	uint8_t *task_status;

	/* Array of struct rtio_iodev_sqe *'s one per task' */
	struct rtio_iodev_sqe *task_cur;
};

/**
 * @cond INTERNAL_HIDDEN
 */
static const struct rtio_executor_api z_rtio_concurrent_api = {
	.submit = rtio_concurrent_submit,
	.ok = rtio_concurrent_ok,
	.err = rtio_concurrent_err
};

/**
 * @endcond INTERNAL_HIDDEN
 */


/**
 * @brief Statically define and initialie a concurrent executor
 *
 * @param name Symbol name, must be unique in the context in which its used
 * @param concurrency Allowed concurrency (number of concurrent tasks).
 */
#define RTIO_EXECUTOR_CONCURRENT_DEFINE(name, concurrency)                                         \
	static struct rtio_iodev_sqe _task_cur_##name[(concurrency)];                              \
	uint8_t _task_status_##name[(concurrency)];                                                \
	static struct rtio_concurrent_executor name = {                                            \
		.ctx = { .api = &z_rtio_concurrent_api },                                          \
		.task_in = 0,                                                                      \
		.task_out = 0,                                                                     \
		.task_mask = (concurrency)-1,                                                      \
		.last_sqe = NULL,                                                                  \
		.task_status = _task_status_##name,                                                \
		.task_cur = _task_cur_##name,                                                      \
	};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_CONCURRENT_H_ */
