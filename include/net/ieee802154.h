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
#include <net/net_mgmt.h>
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

#define IEEE802154_MAX_ADDR_LENGTH	8
#define IEEE802154_NO_CHANNEL		USHRT_MAX

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
	enum net_l2_flags flags;
	uint16_t pan_id;
	uint16_t channel;
	struct k_sem ack_lock;
	uint16_t short_addr;
	uint8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];
#ifdef CONFIG_NET_L2_IEEE802154_MGMT
	struct ieee802154_req_params *scan_ctx;
	union {
		struct k_sem res_lock;
		struct k_sem req_lock;
	};
	union {
		uint8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];
		uint16_t short_addr;
	} coord;
	uint8_t coord_addr_len;
#endif
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx sec_ctx;
#endif
	int16_t tx_power;
	uint8_t sequence;
	uint8_t ack_seq;
	uint8_t ack_received	: 1;
	uint8_t ack_requested	: 1;
	uint8_t associated		: 1;
	uint8_t _unused		: 5;
};

#define IEEE802154_L2_CTX_TYPE	struct ieee802154_context

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_H_ */
