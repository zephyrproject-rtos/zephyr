/*
 * Copyright (c) 2024 Croxel Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RTIO_WORKQ_H_
#define ZEPHYR_INCLUDE_RTIO_WORKQ_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/p4wq.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callback API to execute work operation.
 *
 * @param iodev_sqe Associated SQE operation.
 */
typedef void (*rtio_work_submit_t)(struct rtio_iodev_sqe *iodev_sqe);

/**
 * @brief RTIO Work request.
 *
 * This RTIO Work request to perform a work operation decoupled
 * from its submission in the RTIO work-queues.
 */
struct rtio_work_req {
	/** Work item used to submit unit of work. */
	struct k_p4wq_work work;

	/** Handle to IODEV SQE containing the operation.
	 * This is filled inside @ref rtio_work_req_submit.
	 */
	struct rtio_iodev_sqe *iodev_sqe;

	/** Callback handler where synchronous operation may be executed.
	 * This is filled inside @ref rtio_work_req_submit.
	 */
	rtio_work_submit_t handler;
};

/**
 * @brief Allocate item to perform an RTIO work request.
 *
 * @details This allocation utilizes its internal memory slab with
 * pre-allocated elements.
 *
 * @return Pointer to allocated item if successful.
 * @return NULL if allocation failed.
 */
struct rtio_work_req *rtio_work_req_alloc(void);

/**
 * @brief Submit RTIO work request.
 *
 * @param req Item to fill with request information.
 * @param iodev_sqe RTIO Operation information.
 * @param handler Callback to handler where work operation is performed.
 */
void rtio_work_req_submit(struct rtio_work_req *req,
			  struct rtio_iodev_sqe *iodev_sqe,
			  rtio_work_submit_t handler);

/**
 * @brief Obtain number of currently used items from the pre-allocated pool.
 *
 * @return Number of used items.
 */
uint32_t rtio_work_req_used_count_get(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_WORKQ_H_ */
