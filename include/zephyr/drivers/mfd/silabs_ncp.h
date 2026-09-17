/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Silicon Labs network co-processor: CPC bus of a "silabs,ncp" device.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "sl_cpc_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_silabs_ncp MFD Silicon Labs network co-processor interface
 * @ingroup mfd_interfaces
 * @{
 */

/**
 * @brief Common data of the "silabs,ncp" drivers.
 *
 * Must be at the start of the data of every transport.
 */
struct mfd_silabs_ncp_data {
	/** CPC bus of the co-processor */
	struct sl_cpc_bus cpc_bus;
	/** Work queue of the transport, see mfd_silabs_ncp_wq_submit() */
	struct k_work_q wq;
	K_KERNEL_STACK_MEMBER(wq_stack, CONFIG_MFD_SILABS_NCP_WQ_STACK_SIZE);
};

/**
 * @brief Get the CPC bus of a Silicon Labs network co-processor.
 *
 * Child drivers use the bus to open CPC endpoints on the co-processor. The
 * bus is usable once the "silabs,ncp" device is ready.
 *
 * @param dev "silabs,ncp" device.
 * @return The CPC bus.
 */
static inline struct sl_cpc_bus *mfd_silabs_ncp_cpc_bus(const struct device *dev)
{
	struct mfd_silabs_ncp_data *data = dev->data;

	return &data->cpc_bus;
}

/**
 * @brief Submit a work item to the work queue of a Silicon Labs network co-processor.
 *
 * The transport of the device runs on this work queue, and the CPC endpoint
 * callbacks are called from it. Child drivers use it for the work that must
 * make progress while a user of the co-processor blocks the system work
 * queue waiting for a reply. The handlers must not wait for the co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param work Work item.
 * @return See k_work_submit_to_queue().
 */
static inline int mfd_silabs_ncp_wq_submit(const struct device *dev, struct k_work *work)
{
	struct mfd_silabs_ncp_data *data = dev->data;

	return k_work_submit_to_queue(&data->wq, work);
}

/**
 * @brief Schedule a delayable work item on the work queue of a Silicon Labs
 * network co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param dwork Delayable work item.
 * @param delay Delay before the work item is submitted.
 * @return See k_work_schedule_for_queue().
 */
static inline int mfd_silabs_ncp_wq_schedule(const struct device *dev,
					     struct k_work_delayable *dwork, k_timeout_t delay)
{
	struct mfd_silabs_ncp_data *data = dev->data;

	return k_work_schedule_for_queue(&data->wq, dwork, delay);
}

/**
 * @brief Reschedule a delayable work item on the work queue of a Silicon Labs
 * network co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param dwork Delayable work item.
 * @param delay New delay before the work item is submitted.
 * @return See k_work_reschedule_for_queue().
 */
static inline int mfd_silabs_ncp_wq_reschedule(const struct device *dev,
					       struct k_work_delayable *dwork, k_timeout_t delay)
{
	struct mfd_silabs_ncp_data *data = dev->data;

	return k_work_reschedule_for_queue(&data->wq, dwork, delay);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_ */
