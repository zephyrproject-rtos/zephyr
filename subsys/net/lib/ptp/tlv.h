/*
 * Copyright (c) 2024 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file tlv.h
 * @brief Type, Length, Value extension for PTP
 *
 * References are to version 2019 of IEEE 1588, ("PTP")
 */

#ifndef ZEPHYR_INCLUDE_PTP_TLV_H_
#define ZEPHYR_INCLUDE_PTP_TLV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Type of TLV (type, length, value)
 *
 * @note based on IEEE 1588-2019 Section 14.1.1 Table 52
 */
enum ptp_tlv_type {
	PTP_TLV_TYPE_MANAGEMENT = 1,
	PTP_TLV_TYPE_MANAGEMENT_ERROR_STATUS,
	PTP_TLV_TYPE_ORGANIZATION_EXTENSION,
	PTP_TLV_TYPE_REQUEST_UNICAST_TRANSMISSION,
	PTP_TLV_TYPE_GRANT_UNICAST_TRANSMISSION,
	PTP_TLV_TYPE_CANCEL_UNICAST_TRANSMISSION,
	PTP_TLV_TYPE_ACKNOWLEDGE_CANCEL_UNICAST_TRANSMISSION,
	PTP_TLV_TYPE_PATH_TRACE,
	PTP_TLV_TYPE_ORGANIZATION_EXTENSION_PROPAGATE = 0x4000,
	PTP_TLV_TYPE_ENHANCED_ACCURACY_METRICS,
	PTP_TLV_TYPE_ORGANIZATION_EXTENSION_DO_NOT_PROPAGATE = 0x8000,
	PTP_TLV_TYPE_L1_SYNC,
	PTP_TLV_TYPE_PORT_COMMUNICATION_AVAILABILITY,
	PTP_TLV_TYPE_PROTOCOL_ADDRESS,
	PTP_TLV_TYPE_TIME_RECEIVER_RX_SYNC_TIMING_DATA,
	PTP_TLV_TYPE_TIME_RECEIVER_RX_SYNC_COMPUTED_DATA,
	PTP_TLV_TYPE_TIME_RECEIVER_TX_EVENT_TIMESTAMPS,
	PTP_TLV_TYPE_CUMULATIVE_RATE_RATIO,
	PTP_TLV_TYPE_PAD,
	PTP_TLV_TYPE_AUTHENTICATION,
};

/**
 * @brief PAD TLV - used to increase length of any PTP message.
 *
 * @note 14.4.2 - PAD TLV
 */
struct ptp_tlv_pad {
	/** Identify type of TLV. */
	uint16_t type;
	/** Length of pad. */
	uint16_t length;
	/** Pad. */
	uint8_t  pad[];
} __packed;

/**
 * @brief Structure holding TLV. It is used as a helper to retrieve TLVs from PTP messages.
 */
struct ptp_tlv_container {
	/** Object list. */
	sys_snode_t    node;
	/** Pointer to the TLV. */
	struct ptp_tlv *tlv;
};

/**
 * @brief Function allocating memory for TLV container structure.
 *
 * @return Pointer to the TLV container structure.
 */
struct ptp_tlv_container *ptp_tlv_alloc(void);

/**
 * @brief Function freeing memory used by TLV container.
 *
 * @param[in] tlv_container Pointer to the TLV container structure.
 */
void ptp_tlv_free(struct ptp_tlv_container *tlv_container);

/**
 * @brief Function for getting type of the TLV.
 *
 * @param[in] tlv Pointer to the TLV.
 *
 * @return Type of TLV message.
 */
enum ptp_tlv_type ptp_tlv_type(struct ptp_tlv *tlv);

/**
 * @brief Function processing TLV after reception, and before processing by PTP stack.
 *
 * @param[in] tlv Pointer to the received TLV.
 *
 * @return Zero on success, othervise negative.
 */
int ptp_tlv_post_recv(struct ptp_tlv *tlv);

/**
 * @brief Function preparing TLV to on-wire format before transmitting.
 *
 * @param[in] tlv Pointer to the received TLV.
 */
void ptp_tlv_pre_send(struct ptp_tlv *tlv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_PTP_TLV_H_ */
