/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_SIMPLE_H_
#define ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_SIMPLE_H_

#include <zephyr/rtio/rtio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief RTIO Simple Executor
 *
 * Provides the simplest possible executor without any concurrency
 * or reinterpretation of requests.
 *
 * @defgroup rtio_executor_simple RTIO Simple Executor
 * @ingroup rtio
 * @{
 */


/**
 * @brief Submit to the simple executor
 *
 * @param r RTIO context to submit
 *
 * @retval 0 always succeeds
 */
int rtio_simple_submit(struct rtio *r);

/**
 * @brief Report a SQE has completed successfully
 *
 * @param iodev_sqe RTIO IODEV SQE to report success
 * @param result Result of the SQE
 */
void rtio_simple_ok(struct rtio_iodev_sqe *iodev_sqe, int result);

/**
 * @brief Report a SQE has completed with error
 *
 * @param iodev_sqe RTIO IODEV SQE to report success
 * @param result Result of the SQE
 */
void rtio_simple_err(struct rtio_iodev_sqe *iodev_sqe, int result);

/**
 * @brief Simple Executor
 */
struct rtio_simple_executor {
	struct rtio_executor ctx;
	struct rtio_iodev_sqe task;
};

/**
 * @cond INTERNAL_HIDDEN
 */
static const struct rtio_executor_api z_rtio_simple_api = {
	.submit = rtio_simple_submit,
	.ok = rtio_simple_ok,
	.err = rtio_simple_err
};

/**
 * @endcond INTERNAL_HIDDEN
 */


/**
 * @brief Define a simple executor with a given name
 *
 * @param name Symbol name, must be unique in the context in which its used
 */
#define RTIO_EXECUTOR_SIMPLE_DEFINE(name)                                                          \
	struct rtio_simple_executor name = { .ctx = { .api = &z_rtio_simple_api } };

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* ZEPHYR_INCLUDE_RTIO_RTIO_EXECUTOR_SIMPLE_H_ */
