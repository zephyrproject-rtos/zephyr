/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 L2 stack public header
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_H_

#include <limits.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/crypto/cipher.h>
#include <zephyr/net/ieee802154_radio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IEEE 802.15.4 library
 * @defgroup ieee802154 IEEE 802.15.4 Library
 * @ingroup networking
 * @{
 */

/* References are to the IEEE 802.15.4-2020 standard */
#define IEEE802154_MAX_PHY_PACKET_SIZE	127 /* see section 11.3, aMaxPhyPacketSize */
#define IEEE802154_FCS_LENGTH		2   /* see section 7.2.1.1 */
#define IEEE802154_MTU			(IEEE802154_MAX_PHY_PACKET_SIZE - IEEE802154_FCS_LENGTH)
/* TODO: Support flexible MTU and FCS lengths for IEEE 802.15.4-2015ff */

#define IEEE802154_SHORT_ADDR_LENGTH	2
#define IEEE802154_EXT_ADDR_LENGTH	8
#define IEEE802154_MAX_ADDR_LENGTH	IEEE802154_EXT_ADDR_LENGTH

#define IEEE802154_NO_CHANNEL		USHRT_MAX

/* See IEEE 802.15.4-2020, sections 6.1 and 7.3.5 */
#define IEEE802154_BROADCAST_ADDRESS	     0xffff
#define IEEE802154_NO_SHORT_ADDRESS_ASSIGNED 0xfffe

/* See IEEE 802.15.4-2020, section 6.1 */
#define IEEE802154_BROADCAST_PAN_ID 0xffff

/* See IEEE 802.15.4-2020, section 7.3.5 */
#define IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED IEEE802154_BROADCAST_ADDRESS

/* MAC PIB attribute aUnitBackoffPeriod, see section 8.4.2, table 8-93, in symbol periods, valid for
 * all PHYs except SUN PHY in the 920 MHz band.
 */
#define IEEE802154_A_UNIT_BACKOFF_PERIOD(turnaround_time)                                          \
	(turnaround_time + IEEE802154_PHY_A_CCA_TIME)
#define IEEE802154_A_UNIT_BACKOFF_PERIOD_US(turnaround_time, symbol_period)                        \
	(IEEE802154_A_UNIT_BACKOFF_PERIOD(turnaround_time) * symbol_period)

struct ieee802154_security_ctx {
	uint32_t frame_counter;
	struct cipher_ctx enc;
	struct cipher_ctx dec;
	uint8_t key[16];
	uint8_t key_len;
	uint8_t level	: 3;
	uint8_t key_mode	: 2;
	uint8_t _unused	: 3;
};

/* This not meant to be used by any code but 802.15.4 L2 stack */
struct ieee802154_context {
	uint16_t pan_id; /* in CPU byte order */
	uint16_t channel; /* in CPU byte order */
	/* short address:
	 *   0 == not associated,
	 *   0xfffe == associated but no short address assigned
	 * see section 7.4.2
	 */
	uint16_t short_addr; /* in CPU byte order */
	uint8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH]; /* in little endian */
	struct net_linkaddr_storage linkaddr; /* in big endian */
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx sec_ctx;
#endif
#ifdef CONFIG_NET_L2_IEEE802154_MGMT
	struct ieee802154_req_params *scan_ctx; /* guarded by scan_ctx_lock */
	struct k_sem scan_ctx_lock;

	uint8_t coord_ext_addr[IEEE802154_MAX_ADDR_LENGTH]; /* in little endian */
	uint16_t coord_short_addr; /* in CPU byte order */
#endif
	int16_t tx_power;
	enum net_l2_flags flags;

	uint8_t sequence; /* see section 8.4.3.1, table 8-94, macDsn */

	uint8_t _unused : 6;

	uint8_t ack_received : 1;  /* guarded by ack_lock */
	uint8_t ack_requested : 1; /* guarded by ack_lock */
	uint8_t ack_seq;	   /* guarded by ack_lock */
	struct k_sem ack_lock;

	struct k_sem ctx_lock; /* guards all mutable context attributes unless
				* otherwise mentioned on attribute level
				*/
};

#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_H_ */
