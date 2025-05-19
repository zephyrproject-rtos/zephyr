/*
 *  Copyright (C) 2025 Texas Instruments Incorporated
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_
#define ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_

#include <stdint.h>
#include <zephyr/sys/util.h>

#define TISCI_MSG_VALUE_RM_DST_ID_VALID                (1u << 0u)
#define TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID          (1u << 1u)
#define TISCI_MSG_VALUE_RM_IA_ID_VALID                 (1u << 2u)
#define TISCI_MSG_VALUE_RM_VINT_VALID                  (1u << 3u)
#define TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID          (1u << 4u)
#define TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID (1u << 5u)

/**
 * @brief Request to set up an interrupt route.
 *
 * Configures peripherals within the interrupt subsystem according to the
 * valid configuration provided.
 *
 * @param valid_params         Bitfield defining validity of interrupt route set parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */
struct tisci_msg_rm_irq_set_req {
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
} __packed;

/**
 * @brief Request to release interrupt peripheral resources.
 *
 * Releases interrupt peripheral resources according to the valid configuration provided.
 *
 * @param valid_params         Bitfield defining validity of interrupt route release parameters.
 *                             Each bit corresponds to a field's validity.
 * @param src_id               ID of interrupt source peripheral.
 * @param src_index            Interrupt source index within source peripheral.
 * @param dst_id               SoC IR device ID (valid if TISCI_MSG_VALUE_RM_DST_ID_VALID is set).
 * @param dst_host_irq         SoC IR output index (valid if TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID
 * is set).
 * @param ia_id                Device ID of interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_IA_ID_VALID is set).
 * @param vint                 Virtual interrupt number (valid if TISCI_MSG_VALUE_RM_VINT_VALID is
 * set).
 * @param global_event         Global event mapped to interrupt aggregator (valid if
 * TISCI_MSG_VALUE_RM_GLOBAL_EVENT_VALID is set).
 * @param vint_status_bit_index Virtual interrupt status bit (valid if
 * TISCI_MSG_VALUE_RM_VINT_STATUS_BIT_INDEX_VALID is set).
 * @param secondary_host       Secondary host value (valid if
 * TISCI_MSG_VALUE_RM_SECONDARY_HOST_VALID is set).
 */

struct tisci_msg_rm_irq_release_req {
	uint32_t valid_params;
	uint16_t src_id;
	uint16_t src_index;
	uint16_t dst_id;
	uint16_t dst_host_irq;
	uint16_t ia_id;
	uint16_t vint;
	uint16_t global_event;
	uint8_t vint_status_bit_index;
	uint8_t secondary_host;
} __packed;

#endif /* ZEPHYR_INCLUDE_DRIVERS_FIRMWARE_TISCI_TISCI_RM_IRQ_H_ */
