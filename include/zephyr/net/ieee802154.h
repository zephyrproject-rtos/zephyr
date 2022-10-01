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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IEEE 802.15.4 library
 * @defgroup ieee802154 IEEE 802.15.4 Library
 * @ingroup networking
 * @{
 */

/* See IEEE 802.15.4-2006, sections 5.5.3.2, 6.4.1 and 7.2.1.9 */
#define IEEE802154_MAX_PHY_PACKET_SIZE	127
#define IEEE802154_FCS_LENGTH		2
#define IEEE802154_MTU			(IEEE802154_MAX_PHY_PACKET_SIZE - IEEE802154_FCS_LENGTH)
/* TODO: Support flexible MTU for IEEE 802.15.4-2015 */

#define IEEE802154_SHORT_ADDR_LENGTH	2
#define IEEE802154_EXT_ADDR_LENGTH	8
#define IEEE802154_MAX_ADDR_LENGTH	IEEE802154_EXT_ADDR_LENGTH

#define IEEE802154_NO_CHANNEL		USHRT_MAX

/* See IEEE 802.15.4-2006, section 7.2.1.4 */
#define IEEE802154_BROADCAST_ADDRESS 0xFFFF
#define IEEE802154_NO_SHORT_ADDRESS_ASSIGNED 0xFFFE
#define IEEE802154_SHORT_ADDRESS_NOT_ASSOCIATED 0x0000
#define IEEE802154_BROADCAST_PAN_ID  0xFFFF

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
	uint16_t channel;
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
	uint8_t sequence;

	uint8_t ack_seq;	   /* guarded by ack_lock */
	uint8_t ack_received : 1;  /* guarded by ack_lock */
	uint8_t ack_requested : 1; /* guarded by ack_lock */
	uint8_t _unused : 6;
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
