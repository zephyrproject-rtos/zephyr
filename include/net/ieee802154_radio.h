/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public IEEE 802.15.4 Radio API
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_

#include <device.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/ieee802154.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ieee802154
 * @{
 */

enum ieee802154_hw_caps {
	IEEE802154_HW_FCS	= BIT(0), /* Frame Check-Sum supported */
	IEEE802154_HW_PROMISC	= BIT(1), /* Promiscuous mode supported */
	IEEE802154_HW_FILTER	= BIT(2), /* Filters PAN ID, long/short addr */
	IEEE802154_HW_CSMA	= BIT(3), /* CSMA-CA supported */
	IEEE802154_HW_2_4_GHZ	= BIT(4), /* 2.4Ghz radio supported */
	IEEE802154_HW_TX_RX_ACK = BIT(5), /* Handles ACK request on TX */
	IEEE802154_HW_SUB_GHZ	= BIT(6), /* Sub-GHz radio supported */
};

enum ieee802154_filter_type {
	IEEE802154_FILTER_TYPE_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SHORT_ADDR,
	IEEE802154_FILTER_TYPE_PAN_ID,
	IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR,
};

struct ieee802154_filter {
/** @cond ignore */
	union {
		u8_t *ieee_addr;
		u16_t short_addr;
		u16_t pan_id;
	};
/* @endcond */
};

struct ieee802154_radio_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	/** Get the device capabilities */
	enum ieee802154_hw_caps (*get_capabilities)(struct device *dev);

	/** Clear Channel Assesment - Check channel's activity */
	int (*cca)(struct device *dev);

	/** Set current channel */
	int (*set_channel)(struct device *dev, u16_t channel);

	/** Set/Unset filters (for IEEE802154_HW_FILTER ) */
	int (*filter)(struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter);

	/** Set TX power level in dbm */
	int (*set_txpower)(struct device *dev, s16_t dbm);

	/** Transmit a packet fragment */
	int (*tx)(struct device *dev,
		  struct net_pkt *pkt,
		  struct net_buf *frag);

	/** Start the device */
	int (*start)(struct device *dev);

	/** Stop the device */
	int (*stop)(struct device *dev);

#ifdef CONFIG_NET_L2_IEEE802154_SUB_GHZ
	/** Get the available amount of Sub-GHz channels */
	u16_t (*get_subg_channel_count)(struct device *dev);
#endif /* CONFIG_NET_L2_IEEE802154_SUB_GHZ */

#ifdef CONFIG_NET_L2_OPENTHREAD
	/** Run an energy detection scan.
	 * Note: channel must be set prior to request this function.
	 * duration parameter is in ms.
	 */
	int (*ed_scan)(struct device *dev,
		       u16_t duration,
		       void (*done_cb)(struct device *dev,
				       s16_t max_ed));
#endif /* CONFIG_NET_L2_OPENTHREAD */
} __packed;

#define IEEE802154_AR_FLAG_SET (0x20)

/**
 * @brief Check if AR flag is set on the frame inside given net_pkt
 *
 * @param pkt A valid pointer on a net_pkt structure, must not be NULL,
 *        and its length should be at least made of 1 byte (ACK frames
 *        are the smallest frames on 15.4 and made of 3 bytes, not
 *        not counting the FCS part).
 *
 * @return True if AR flag is set, False otherwise
 */
static inline bool ieee802154_is_ar_flag_set(struct net_pkt *pkt)
{
	return (*net_pkt_ll(pkt) & IEEE802154_AR_FLAG_SET);
}

#ifndef CONFIG_IEEE802154_RAW_MODE

/**
 * @brief Radio driver sending function that hw drivers should use
 *
 * @details This function should be used to fill in struct net_if's send
 * pointer.
 *
 * @param iface A valid pointer on a network interface to send from
 * @param pkt A valid pointer on a packet to send
 *
 * @return 0 on success, negative value otherwise
 */
extern int ieee802154_radio_send(struct net_if *iface,
				 struct net_pkt *pkt);

/**
 * @brief Radio driver ACK handling function that hw drivers should use
 *
 * @details ACK handling requires fast handling and thus such function
 *          helps to hook directly the hw drivers to the radio driver.
 *
 * @param iface A valid pointer on a network interface that received the packet
 * @param pkt A valid pointer on a packet to check
 *
 * @return NET_OK if it was handled, NET_CONTINUE otherwise
 */
extern enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
						    struct net_pkt *pkt);

/**
 * @brief Initialize L2 stack for a given interface
 *
 * @param iface A valid pointer on a network interface
 */
void ieee802154_init(struct net_if *iface);

#else /* CONFIG_IEEE802154_RAW_MODE */

static inline int ieee802154_radio_send(struct net_if *iface,
					struct net_pkt *pkt)
{
	return 0;
}

static inline enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
							   struct net_pkt *pkt)
{
	return NET_CONTINUE;
}

#define ieee802154_init(_iface_)

#endif /* CONFIG_IEEE802154_RAW_MODE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_ */
