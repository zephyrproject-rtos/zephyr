/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NATIVE_POSIX_802154_TYPES_H__
#define NATIVE_POSIX_802154_TYPES_H__

#include <stdint.h>

/**
 * @defgroup nrf_802154_types Type definitions used in the 802.15.4 driver
 * @{
 * @ingroup nrf_802154
 * @brief Definitions of types used in the 802.15.4 driver.
 */

/**
 * @brief Possible errors during the frame reception.
 */
typedef uint8_t nrf_802154_rx_error_t;

/** There is no receive error. */
#define NRF_802154_RX_ERROR_NONE 0x00
/** Received a malformed frame. */
#define NRF_802154_RX_ERROR_INVALID_FRAME 0x01
/** Received a frame with an invalid checksum. */
#define NRF_802154_RX_ERROR_INVALID_FCS 0x02
/** Received a frame with a mismatched destination address. */
#define NRF_802154_RX_ERROR_INVALID_DEST_ADDR 0x03
/** Runtime error occurred (for example, CPU was held for too long). */
#define NRF_802154_RX_ERROR_RUNTIME 0x04
/** Radio timeslot ended during the frame reception. */
#define NRF_802154_RX_ERROR_TIMESLOT_ENDED 0x05
/** Procedure was aborted by another operation. */
#define NRF_802154_RX_ERROR_ABORTED 0x06
/** Delayed reception request was rejected due to a denied timeslot request. */
#define NRF_802154_RX_ERROR_DELAYED_TIMESLOT_DENIED 0x07
/** Delayed reception timeslot ended. */
#define NRF_802154_RX_ERROR_DELAYED_TIMEOUT 0x08
/** Received a frame with invalid length. */
#define NRF_802154_RX_ERROR_INVALID_LENGTH 0x09
/** Delayed operation in the ongoing state was aborted by other request. */
#define NRF_802154_RX_ERROR_DELAYED_ABORTED 0x0A

/**
 * @brief Types of data that can be set in an ACK message.
 */
typedef uint8_t nrf_802154_ack_data_t;

#define NRF_802154_ACK_DATA_PENDING_BIT 0x00
#define NRF_802154_ACK_DATA_IE 0x01

/**
 * @brief Methods of source address matching.
 *
 * You can use one of the following methods that can be set during the
 * initialization phase
 * by calling @ref nrf_802154_src_matching_method:
 *   - For Thread: @ref NRF_802154_SRC_ADDR_MATCH_THREAD -- The pending bit
 *                          is set only for the addresses found in the list.
 *   - For Zigbee: @ref NRF_802154_SRC_ADDR_MATCH_ZIGBEE -- The pending bit
 *                  is cleared only for the short addresses found in the list.
 *     This method does not set pending bit in non-command and non-data-request
 *     frames.
 *   - For standard-compliant implementation:
 *      @ref NRF_802154_SRC_ADDR_MATCH_ALWAYS_1 -- The pending bit is always
 *      set to 1.
 *     This requires an empty data frame with AR set to 0 to be transmitted
 *     immediately afterwards.
 */
typedef uint8_t nrf_802154_src_addr_match_t;

/** Implementation for the Thread protocol. */
#define NRF_802154_SRC_ADDR_MATCH_THREAD 0x00
/** Implementation for the Zigbee protocol. */
#define NRF_802154_SRC_ADDR_MATCH_ZIGBEE 0x01
/** Standard compliant implementation. */
#define NRF_802154_SRC_ADDR_MATCH_ALWAYS_1 0x02

/**
 *@}
 **/

#endif /* NRF_802154_TYPES_H__ */
