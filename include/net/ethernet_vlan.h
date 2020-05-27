/** @file
 * @brief VLAN specific definitions.
 *
 * Virtual LAN specific definitions.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_ETHERNET_VLAN_H_
#define ZEPHYR_INCLUDE_NET_ETHERNET_VLAN_H_

/**
 * @brief VLAN definitions and helpers
 * @defgroup vlan_api Virtual LAN definitions and helpers
 * @ingroup networking
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Unspecified VLAN tag value */
#define NET_VLAN_TAG_UNSPEC 0x0fff

/**
 * @brief Get VLAN identifier from TCI.
 *
 * @param tci VLAN tag control information.
 *
 * @return VLAN identifier.
 */
static inline uint16_t net_eth_vlan_get_vid(uint16_t tci)
{
	return tci & 0x0fff;
}

/**
 * @brief Get Drop Eligible Indicator from TCI.
 *
 * @param tci VLAN tag control information.
 *
 * @return Drop eligible indicator.
 */
static inline uint8_t net_eth_vlan_get_dei(uint16_t tci)
{
	return (tci >> 12) & 0x01;
}

/**
 * @brief Get Priority Code Point from TCI.
 *
 * @param tci VLAN tag control information.
 *
 * @return Priority code point.
 */
static inline uint8_t net_eth_vlan_get_pcp(uint16_t tci)
{
	return (tci >> 13) & 0x07;
}

/**
 * @brief Set VLAN identifier to TCI.
 *
 * @param tci VLAN tag control information.
 * @param vid VLAN identifier.
 *
 * @return New TCI value.
 */
static inline uint16_t net_eth_vlan_set_vid(uint16_t tci, uint16_t vid)
{
	return (tci & 0xf000) | (vid & 0x0fff);
}

/**
 * @brief Set Drop Eligible Indicator to TCI.
 *
 * @param tci VLAN tag control information.
 * @param dei Drop eligible indicator.
 *
 * @return New TCI value.
 */
static inline uint16_t net_eth_vlan_set_dei(uint16_t tci, bool dei)
{
	return (tci & 0xefff) | ((!!dei) << 12);
}

/**
 * @brief Set Priority Code Point to TCI.
 *
 * @param tci VLAN tag control information.
 * @param pcp Priority code point.
 *
 * @return New TCI value.
 */
static inline uint16_t net_eth_vlan_set_pcp(uint16_t tci, uint8_t pcp)
{
	return (tci & 0x1fff) | ((pcp & 0x07) << 13);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_NET_ETHERNET_VLAN_H_ */
