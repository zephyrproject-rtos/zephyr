/** @file
 * @brief IPv6 Networking over CAN definitions.
 *
 * Definitions for IPv6 Networking over CAN support.
 */

/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_CAN_H_
#define ZEPHYR_INCLUDE_NET_CAN_H_

#include <zephyr/types.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <can.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPv6 over CAN library
 * @defgroup net_can Network Core Library
 * @ingroup networking
 * @{
 */

/**
 * CAN L2 driver API. Used by 6loCAN.
 */

/*
 * Abbreviations
 * BS      Block Size
 * CAN_DL  CAN LL data size
 * CF      Consecutive Frame
 * CTS     Continue to send
 * DLC     Data length code
 * FC      Flow Control
 * FF      First Frame
 * FS      Flow Status
 */

/** @cond INTERNAL_HIDDEN */

#define NET_CAN_DL 8
#define NET_CAN_MTU 0x0FFF

/* 0x3DFF - bit 4 to 10 must not be zero. Also prevent stuffing bit*/
#define NET_CAN_MULTICAST_ADDR      0x3DFF
#define NET_CAN_DAD_ADDR            0x3DFE
#define NET_CAN_ETH_TRANSLATOR_ADDR 0x3DF0
#define NET_CAN_MAX_ADDR            0x3DEF
#define NET_CAN_MIN_ADDR            0x0100

#define CAN_NET_IF_ADDR_MASK       0x3FFF
#define CAN_NET_IF_ADDR_BYTE_LEN   2U
#define CAN_NET_IF_ADDR_DEST_POS   14U
#define CAN_NET_IF_ADDR_DEST_MASK  (CAN_NET_IF_ADDR_MASK << CAN_NET_IF_ADDR_DEST_POS)
#define CAN_NET_IF_ADDR_SRC_POS    0U
#define CAN_NET_IF_ADDR_SRC_MASK   (CAN_NET_IF_ADDR_MASK << CAN_NET_IF_ADDR_SRC_POS)
#define CAN_NET_IF_ADDR_MCAST_POS  28U
#define CAN_NET_IF_ADDR_MCAST_MASK (1UL << CAN_NET_IF_ADDR_MCAST_POS)

#define CAN_NET_IF_IS_MCAST_BIT    (1U << 14)

#define CAN_NET_FILTER_NOT_SET -1

/** @endcond */

/*
 *             +-----------+           +-----------+
 *             |           |           |           |
 *             |   IPv6    |           |   IPv6    |
 *             |  6LoCAN   |           |  6LoCAN   |
 *             |           |           |           |
 *             +----+-+----+           +----+-+----+
 *                  | |                     | |                   +---+
 *  +----+          | |                     | |                  /     \   +-+
 *  |    |          | |                     | |          +--+   /       \_/   \
 * +++   +---+   +--+----+   +-------+   +--+----+   +--/    \_/               \
 * | |        \ /     |   \ /         \ /     |   \ /  /                       |
 * | |         X      |    X    CAN    X      |    X  |         Internet       |
 * | |        / \     |   / \         / \     |   / \  \                       /
 * +++   +---+   +----+--+   +-------+   +----+--+   +--+                     /
 *  |    |                                               +-------------------+
 *  +----+
 */
struct net_can_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Send a single CAN frame */
	int (*send)(struct device *dev, const struct zcan_frame *frame,
		    can_tx_callback_t cb, void *cb_arg, s32_t timeout);
	/** Attach a filter with it's callback */
	int (*attach_filter)(struct device *dev, can_rx_callback_t cb,
			     void *cb_arg, const struct zcan_filter *filter);
	/** Detach a filter */
	void (*detach_filter)(struct device *dev, int filter_id);
	/** Enable or disable the reception of frames for net CAN */
	int (*enable)(struct device *dev, bool enable);
};

/** @cond INTERNAL_HIDDEN */

#define CANBUS_L2_CTX_TYPE	struct net_canbus_context *

/**
 * Context for canbus net device.
 */
struct canbus_net_ctx {
	/** Filter ID for link layer duplicate address detection. */
	int dad_filter_id;
	/** Work item for responding to link layer DAD requests. */
	struct k_work dad_work;
	/** The interface associated with this device */
	struct net_if *iface;
	/** The link layer address chosen for this interface */
	u16_t ll_addr;
};

/**
 * Canbus link layer addresses have a length of 14 bit for source and destination.
 * Both together are 28 bit to fit a CAN extended identifier with 29 bit length.
 */
struct net_canbus_lladdr {
	u16_t addr : 14;
};

/**
 * STmin is split in two valid ranges:
 *   0-127: 0ms-127ms
 * 128-240: Reserved
 * 241-249: 100us-900us (multiples of 100us)
 * 250-   : Reserved
 */
struct canbus_fc_opts {
	/** Block size. Number of CF PDUs before next CF is sent */
	u8_t bs;
	/**< Minimum separation time. Min time between frames */
	u8_t stmin;
};

/**
 * Context for a transmission of messages that didn't fit in a single frame.
 * These messages Start with a FF (First Frame) that is in case of unicast
 * acknowledged by a FC (Frame Control). After that, one or more CF
 * (Consecutive frames) carry the rest of the message.
 */
struct canbus_isotp_tx_ctx {
	/** Pkt containing the data to transmit */
	struct net_pkt *pkt;
	/** Timeout for TX Timeout and separation time */
	struct _timeout timeout;
	/** Frame Control options received from FC frame */
	struct canbus_fc_opts opts;
	/** CAN destination address */
	struct net_canbus_lladdr dest_addr;
	/** Remaining data to transmit in bytes */
	u16_t rem_len;
	/** Number of bytes in the tx queue */
	s8_t tx_backlog;
	/** State of the transmission */
	u8_t state;
	/** Actual block number that is transmitted. Counts from BS to 0 */
	u8_t act_block_nr;
	/** Number of WAIT frames received */
	u8_t wft;
	/** Sequence number that is added to CF */
	u8_t sn : 4;
	/** Transmission is multicast */
	u8_t is_mcast : 1;
};

/**
 * Context for reception of messages that are not single frames.
 * This is the counterpart of the canbus_isotp_tx_ctx.
 */
struct canbus_isotp_rx_ctx {
	/** Pkt that is large enough to hold the entire message */
	struct net_pkt *pkt;
	/** Timeout for RX timeout*/
	struct _timeout timeout;
	/** Remaining data to receive. Goes from message length to zero */
	u16_t rem_len;
	/** State of the reception */
	u8_t state;
	/** Number of frames received in this block. Counts from BS to 0 */
	u8_t act_block_nr;
	/** Number of WAIT frames transmitted */
	u8_t wft;
	/** Expected sequence number in CF */
	u8_t sn : 4;
};

/**
 * Initialization of the canbus L2.
 *
 * This function starts the TX workqueue and does some initialization.
 */
void net_6locan_init(struct net_if *iface);

/**
 * Ethernet frame input function for Ethernet to 6LoCAN translation
 *
 * This function checks the destination link layer address for addresses
 * that has to be forwarded. Frames that need to be forwarded are forwarded here.
 */
enum net_verdict net_canbus_translate_eth_frame(struct net_if *iface,
						struct net_pkt *pkt);

/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CAN_H_ */
