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

/**
 * @brief IEEE 802.15.4 Channel assignments
 *
 * Channel numbering for 868 MHz, 915 MHz, and 2450 MHz bands.
 *
 * - Channel 0 is for 868.3 MHz.
 * - Channels 1-10 are for 906 to 924 MHz with 2 MHz channel spacing.
 * - Channels 11-26 are for 2405 to 2530 MHz with 5 MHz channel spacing.
 *
 * For more information, please refer to 802.15.4-2015 Section 10.1.2.2.
 */
enum ieee802154_channel {
	IEEE802154_SUB_GHZ_CHANNEL_MIN = 0,
	IEEE802154_SUB_GHZ_CHANNEL_MAX = 10,
	IEEE802154_2_4_GHZ_CHANNEL_MIN = 11,
	IEEE802154_2_4_GHZ_CHANNEL_MAX = 26,
};

enum ieee802154_hw_caps {
	IEEE802154_HW_FCS	  = BIT(0), /* Frame Check-Sum supported */
	IEEE802154_HW_PROMISC	  = BIT(1), /* Promiscuous mode supported */
	IEEE802154_HW_FILTER	  = BIT(2), /* Filter PAN ID, long/short addr */
	IEEE802154_HW_CSMA	  = BIT(3), /* CSMA-CA supported */
	IEEE802154_HW_2_4_GHZ	  = BIT(4), /* 2.4Ghz radio supported */
	IEEE802154_HW_TX_RX_ACK	  = BIT(5), /* Handles ACK request on TX */
	IEEE802154_HW_SUB_GHZ	  = BIT(6), /* Sub-GHz radio supported */
	IEEE802154_HW_ENERGY_SCAN = BIT(7), /* Energy scan supported */
	IEEE802154_HW_TXTIME	  = BIT(8), /* TX at specified time supported */
	IEEE802154_HW_SLEEP_TO_TX = BIT(9), /* TX directly from sleep supported */
};

enum ieee802154_filter_type {
	IEEE802154_FILTER_TYPE_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SHORT_ADDR,
	IEEE802154_FILTER_TYPE_PAN_ID,
	IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,
	IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR,
};

enum ieee802154_event {
	IEEE802154_EVENT_TX_STARTED, /* Data transmission started */
	IEEE802154_EVENT_RX_FAILED   /* Data reception failed */
};

enum ieee802154_rx_fail_reason {
	IEEE802154_RX_FAIL_NOT_RECEIVED,  /* Nothing received */
	IEEE802154_RX_FAIL_INVALID_FCS,   /* Frame had invalid checksum */
	IEEE802154_RX_FAIL_ADDR_FILTERED, /* Address did not match */
	IEEE802154_RX_FAIL_OTHER	  /* General reason */
};

typedef void (*energy_scan_done_cb_t)(const struct device *dev,
				      int16_t max_ed);

typedef void (*ieee802154_event_cb_t)(const struct device *dev,
				      enum ieee802154_event evt,
				      void *event_params);

struct ieee802154_filter {
/** @cond ignore */
	union {
		uint8_t *ieee_addr;
		uint16_t short_addr;
		uint16_t pan_id;
	};
/* @endcond */
};

/** IEEE802.15.4 Transmission mode. */
enum ieee802154_tx_mode {
	/** Transmit packet immediately, no CCA. */
	IEEE802154_TX_MODE_DIRECT,

	/** Perform CCA before packet transmission. */
	IEEE802154_TX_MODE_CCA,

	/** Perform full CSMA CA procedure before packet transmission. */
	IEEE802154_TX_MODE_CSMA_CA,

	/** Transmit packet in the future, at specified time, no CCA. */
	IEEE802154_TX_MODE_TXTIME,

	/** Transmit packet in the future, perform CCA before transmission. */
	IEEE802154_TX_MODE_TXTIME_CCA,
};

/** IEEE802.15.4 Frame Pending Bit table address matching mode. */
enum ieee802154_fpb_mode {
	/** The pending bit shall be set only for addresses found in the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_THREAD,

	/** The pending bit shall be cleared for short addresses found in
	 *  the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_ZIGBEE,
};

/** IEEE802.15.4 driver configuration types. */
enum ieee802154_config_type {
	/** Indicates how radio driver should set Frame Pending bit in ACK
	 *  responses for Data Requests. If enabled, radio driver should
	 *  determine whether to set the bit or not based on the information
	 *  provided with ``IEEE802154_CONFIG_ACK_FPB`` config and FPB address
	 *  matching mode specified. Otherwise, Frame Pending bit should be set
	 *  to ``1``(see IEEE Std 802.15.4-2006, 7.2.2.3.1).
	 */
	IEEE802154_CONFIG_AUTO_ACK_FPB,

	/** Indicates whether to set ACK Frame Pending bit for specific address
	 *  or not. Disabling the Frame Pending bit with no address provided
	 *  (NULL pointer) should disable it for all enabled addresses.
	 */
	IEEE802154_CONFIG_ACK_FPB,

	/** Indicates whether the device is a PAN coordinator. */
	IEEE802154_CONFIG_PAN_COORDINATOR,

	/** Enable/disable promiscuous mode. */
	IEEE802154_CONFIG_PROMISCUOUS,

	/** Specifies new radio event handler. Specifying NULL as a handler
	 *  will disable radio events notification.
	 */
	IEEE802154_CONFIG_EVENT_HANDLER
};

/** IEEE802.15.4 driver configuration data. */
struct ieee802154_config {
	/** Configuration data. */
	union {
		/** ``IEEE802154_CONFIG_AUTO_ACK_FPB`` */
		struct {
			bool enabled;
			enum ieee802154_fpb_mode mode;
		} auto_ack_fpb;

		/** ``IEEE802154_CONFIG_ACK_FPB`` */
		struct {
			uint8_t *addr;
			bool extended;
			bool enabled;
		} ack_fpb;

		/** ``IEEE802154_CONFIG_PAN_COORDINATOR`` */
		bool pan_coordinator;

		/** ``IEEE802154_CONFIG_PROMISCUOUS`` */
		bool promiscuous;

		/** ``IEEE802154_CONFIG_EVENT_HANDLER`` */
		ieee802154_event_cb_t event_handler;
	};
};

/**
 * @brief IEEE 802.15.4 radio interface API.
 *
 */
struct ieee802154_radio_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	/** Get the device capabilities */
	enum ieee802154_hw_caps (*get_capabilities)(const struct device *dev);

	/** Clear Channel Assesment - Check channel's activity */
	int (*cca)(const struct device *dev);

	/** Set current channel */
	int (*set_channel)(const struct device *dev, uint16_t channel);

	/** Set/Unset filters (for IEEE802154_HW_FILTER ) */
	int (*filter)(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter);

	/** Set TX power level in dbm */
	int (*set_txpower)(const struct device *dev, int16_t dbm);

	/** Transmit a packet fragment */
	int (*tx)(const struct device *dev, enum ieee802154_tx_mode mode,
		  struct net_pkt *pkt, struct net_buf *frag);

	/** Start the device */
	int (*start)(const struct device *dev);

	/** Stop the device */
	int (*stop)(const struct device *dev);

	/** Set specific radio driver configuration. */
	int (*configure)(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config);

	/** Get the available amount of Sub-GHz channels */
	uint16_t (*get_subg_channel_count)(const struct device *dev);

	/** Run an energy detection scan.
	 *  Note: channel must be set prior to request this function.
	 *  duration parameter is in ms.
	 */
	int (*ed_scan)(const struct device *dev,
		       uint16_t duration,
		       energy_scan_done_cb_t done_cb);
};

/* Make sure that the network interface API is properly setup inside
 * IEEE 802154 radio API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct ieee802154_radio_api, iface_api) == 0);

#define IEEE802154_AR_FLAG_SET (0x20)

/**
 * @brief Check if AR flag is set on the frame inside given net_pkt
 *
 * @param frag A valid pointer on a net_buf structure, must not be NULL,
 *        and its length should be at least made of 1 byte (ACK frames
 *        are the smallest frames on 15.4 and made of 3 bytes, not
 *        not counting the FCS part).
 *
 * @return True if AR flag is set, False otherwise
 */
static inline bool ieee802154_is_ar_flag_set(struct net_buf *frag)
{
	return (*frag->data & IEEE802154_AR_FLAG_SET);
}

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
#ifndef CONFIG_IEEE802154_RAW_MODE
extern enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
						    struct net_pkt *pkt);
#else /* CONFIG_IEEE802154_RAW_MODE */

static inline enum net_verdict ieee802154_radio_handle_ack(struct net_if *iface,
							   struct net_pkt *pkt)
{
	return NET_CONTINUE;
}
#endif /* CONFIG_IEEE802154_RAW_MODE */

/**
 * @brief Initialize L2 stack for a given interface
 *
 * @param iface A valid pointer on a network interface
 */
#ifndef CONFIG_IEEE802154_RAW_MODE
void ieee802154_init(struct net_if *iface);
#else
#define ieee802154_init(_iface_)
#endif /* CONFIG_IEEE802154_RAW_MODE */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_ */
