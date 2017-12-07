/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 L2 stack public header
 */

#ifndef __IEEE802154_H__
#define __IEEE802154_H__

#include <net/net_l2.h>
#include <net/ieee802154_mgmt.h>
#include <crypto/cipher_structs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IEEE 802.15.4 library
 * @defgroup ieee802154 IEEE 802.15.4 Library
 * @ingroup networking
 * @{
 */

struct ieee802154_security_ctx {
	u32_t frame_counter;
	struct cipher_ctx enc;
	struct cipher_ctx dec;
	u8_t key[16];
	u8_t key_len;
	u8_t level	: 3;
	u8_t key_mode	: 2;
	u8_t _unused	: 3;
};

/* This not meant to be used by any code but 802.15.4 L2 stack */
struct ieee802154_context {
	u16_t pan_id;
	u16_t channel;
	struct k_sem ack_lock;
	u16_t short_addr;
	u8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];
#ifdef CONFIG_NET_L2_IEEE802154_MGMT
	struct ieee802154_req_params *scan_ctx;
	union {
		struct k_sem res_lock;
		struct k_sem req_lock;
	};
	union {
		u8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];
		u16_t short_addr;
	} coord;
	u8_t coord_addr_len;
#endif
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx sec_ctx;
#endif
	s16_t tx_power;
	u8_t sequence;
	u8_t ack_seq;
	u8_t ack_received	: 1;
	u8_t ack_requested	: 1;
	u8_t associated		: 1;
	u8_t _unused		: 5;
};

#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __IEEE802154_H__ */
