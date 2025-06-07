/*
 *  Copyright (C) 2025 Texas Instruments Incorporated
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_
#define ZEPHYR_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_

#include <stdint.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/firmware/tisci/ti_sci_irq.h>
#include "ti_sci.h"

/**
 * @brief Request to set up an interrupt route.
 *
 * Configures peripherals within the interrupt subsystem according to the
 * valid configuration provided.
 *
 * @param hdr                  Standard TISCI header.
 * @param req                  Request structure containing interrupt route set parameters.
 *
 */
struct tisci_msg_rm_irq_set_req_wrapper {
	struct ti_sci_msg_hdr hdr;
	const struct tisci_msg_rm_irq_set_req *req;
} __packed;

/**
 * @brief Response to setting a peripheral to processor interrupt.
 *
 * @param hdr Standard TISCI header.
 */
struct tisci_msg_rm_irq_set_resp {
	struct ti_sci_msg_hdr hdr;
} __packed;

/**
 * @brief Request to release interrupt peripheral resources.
 *
 * Releases interrupt peripheral resources according to the valid configuration provided.
 *
 * @param hdr                  Standard TISCI header.
 * @param req                  Request structure containing interrupt route release parameters.
 */
struct tisci_msg_rm_irq_release_req_wrapper {
	struct ti_sci_msg_hdr hdr;
	const struct tisci_msg_rm_irq_release_req *req;
} __packed;

/**
 * @brief Response to releasing a peripheral to processor interrupt.
 *
 * @param hdr Standard TISCI header.
 */
struct tisci_msg_rm_irq_release_resp {
	struct ti_sci_msg_hdr hdr;
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_ */
