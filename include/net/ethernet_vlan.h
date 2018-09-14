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
 * @defgroup vlan Virtual LAN definitions and helpers
 * @ingroup networking
 * @{
 */

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NET_VLAN_TAG_UNSPEC 0x0fff

/* Get VLAN identifier from TCI */
static inline u16_t net_eth_vlan_get_vid(u16_t tci)
{
	return tci & 0x0fff;
}

/* Get Drop Eligible Indicator from TCI */
static inline u8_t net_eth_vlan_get_dei(u16_t tci)
{
	return (tci >> 12) & 0x01;
}

/* Get Priority Code Point from TCI */
static inline u8_t net_eth_vlan_get_pcp(u16_t tci)
{
	return (tci >> 13) & 0x07;
}

/* Set VLAN identifier to TCI */
static inline u16_t net_eth_vlan_set_vid(u16_t tci, u16_t vid)
{
	return (tci & 0xf000) | (vid & 0x0fff);
}

/* Set Drop Eligible Indicator to TCI */
static inline u16_t net_eth_vlan_set_dei(u16_t tci, bool dei)
{
	return (tci & 0xefff) | ((!!dei) << 12);
}

/* Set Priority Code Point to TCI */
static inline u16_t net_eth_vlan_set_pcp(u16_t tci, u8_t pcp)
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
