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
#define IEEE802154_PAN_ID_NOT_ASSOCIATED	IEEE802154_BROADCAST_PAN_ID

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

/* This not meant to be used by any code but the IEEE 802.15.4 L2 stack */
struct ieee802154_context {
	/* PAN ID
	 *
	 * The identifier of the PAN on which the device is operating. If this
	 * value is 0xffff, the device is not associated. See section 8.4.3.1,
	 * table 8-94, macPanId.
	 *
	 * in CPU byte order
	 */
	uint16_t pan_id;

	/* Channel Number
	 *
	 * The RF channel to use for all transmissions and receptions, see
	 * section 11.3, table 11-2, phyCurrentChannel. The allowable range
	 * of values is PHY dependent as defined in section 10.1.3.
	 *
	 * in CPU byte order
	 */
	uint16_t channel;

	/* Short Address
	 *
	 * Range:
	 *  * 0x0000–0xfffd: associated, short address was assigned
	 *  * 0xfffe: associated but no short address assigned
	 *  * 0xffff: not associated (default),
	 *
	 * See section 6.4.1, table 6-4 (Usage of the shart address) and
	 * section 8.4.3.1, table 8-94, macShortAddress.
	 *
	 * in CPU byte order
	 */
	uint16_t short_addr;

	/* Extended Address
	 *
	 * The extended address is device specific, usually permanently stored
	 * on the device and immutable.
	 *
	 * See section 8.4.3.1, table 8-94, macExtendedAddress.
	 *
	 * in little endian
	 */
	uint8_t ext_addr[IEEE802154_MAX_ADDR_LENGTH];

	struct net_linkaddr_storage linkaddr; /* in big endian */
#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_ctx sec_ctx;
#endif
#ifdef CONFIG_NET_L2_IEEE802154_MGMT
	struct ieee802154_req_params *scan_ctx; /* guarded by scan_ctx_lock */
	struct k_sem scan_ctx_lock;

	/* see section 8.4.3.1, table 8-94, macCoordExtendedAddress, the address
	 * of the coordinator through which the device is associated.
	 *
	 * A value of zero indicates that a coordinator extended address is
	 * unknown (default).
	 *
	 * in little endian
	 */
	uint8_t coord_ext_addr[IEEE802154_MAX_ADDR_LENGTH];

	/* see section 8.4.3.1, table 8-94, macCoordShortAddress, the short
	 * address assigned to the coordinator through which the device is
	 * associated.
	 *
	 * A value of 0xfffe indicates that the coordinator is only using its
	 * extended address. A value of 0xffff indicates that this value is
	 * unknown.
	 *
	 * in CPU byte order
	 */
	uint16_t coord_short_addr;
#endif
	int16_t tx_power;
	enum net_l2_flags flags;

	/* The sequence number added to the transmitted Data frame or MAC
	 * command, see section 8.4.3.1, table 8-94, macDsn.
	 */
	uint8_t sequence;

	uint8_t _unused : 7;

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
