/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Silicon Labs network co-processor: services of a "silabs,ncp" device.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_silabs_ncp MFD Silicon Labs network co-processor interface
 * @ingroup mfd_interfaces
 * @{
 */

struct sl_cpc_bus;

/**
 * @brief Get the CPC bus of a Silicon Labs network co-processor.
 *
 * Child drivers use the bus to open CPC endpoints on the co-processor. The
 * bus is usable once the "silabs,ncp" device is ready.
 *
 * @param dev "silabs,ncp" device.
 * @return The CPC bus.
 */
struct sl_cpc_bus *mfd_silabs_ncp_cpc_bus(const struct device *dev);

/**
 * @brief Submit a work item to the work queue of a Silicon Labs network co-processor.
 *
 * The transport of the device runs on this work queue. The CPC core calls the
 * endpoint callbacks from its own thread; child drivers use this work queue
 * for the work they defer from these callbacks, so that it makes progress
 * while a user of the co-processor blocks the system work queue waiting for
 * a reply. The handlers must not wait for the co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param work Work item.
 * @return See k_work_submit_to_queue().
 */
int mfd_silabs_ncp_wq_submit(const struct device *dev, struct k_work *work);

/**
 * @brief Schedule a delayable work item on the work queue of a Silicon Labs
 * network co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param dwork Delayable work item.
 * @param delay Delay before the work item is submitted.
 * @return See k_work_schedule_for_queue().
 */
int mfd_silabs_ncp_wq_schedule(const struct device *dev, struct k_work_delayable *dwork,
			       k_timeout_t delay);

/**
 * @brief Reschedule a delayable work item on the work queue of a Silicon Labs
 * network co-processor.
 *
 * @param dev "silabs,ncp" device.
 * @param dwork Delayable work item.
 * @param delay New delay before the work item is submitted.
 * @return See k_work_reschedule_for_queue().
 */
int mfd_silabs_ncp_wq_reschedule(const struct device *dev, struct k_work_delayable *dwork,
				 k_timeout_t delay);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_SILABS_NCP_H_ */
