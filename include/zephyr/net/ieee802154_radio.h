/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public IEEE 802.15.4 Radio API
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_

#include <zephyr/device.h>
#include <zephyr/sys_clock.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ieee802154
 * @{
 */

/* See section 6.1: "Some of the timing parameters in definition of the MAC are in units of PHY
 * symbols. For PHYs that have multiple symbol periods, the duration to be used for the MAC
 * parameters is defined in that PHY clause."
 */
#define IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_US 20U /* see section 19.1, table 19-1 */
#define IEEE802154_PHY_OQPSK_2450MHZ_SYMBOL_PERIOD_US         16U /* see section 12.3.3 */

/* TODO: Get PHY-specific symbol period from radio API. Requires an attribute getter, see
 *       https://github.com/zephyrproject-rtos/zephyr/issues/50336#issuecomment-1251122582.
 *       For now we assume PHYs that current drivers actually implement.
 */
#define IEEE802154_PHY_SYMBOL_PERIOD(is_subg_phy)                                                  \
	((is_subg_phy) ? IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_US                     \
		       : IEEE802154_PHY_OQPSK_2450MHZ_SYMBOL_PERIOD_US)

/* The inverse of the symbol period as defined in section 6.1. This is not necessarily the true
 * physical symbol period, so take care to use this macro only when either the symbol period used
 * for MAC timing is the same as the physical symbol period or if you actually mean the MAC timing
 * symbol period.
 */
#define IEEE802154_PHY_SYMBOLS_PER_SECOND(symbol_period) (USEC_PER_SEC / symbol_period)

/* see section 19.2.4 */
#define IEEE802154_PHY_SUN_FSK_PHR_LEN 2

/* Default PHY PIB attribute aTurnaroundTime, in PHY symbols, see section 11.3, table 11-1. */
#define IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT 12U

/* PHY PIB attribute aTurnaroundTime for SUN, RS-GFSK, TVWS, and LECIM FSK PHY,
 * in PHY symbols, see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_1MS(symbol_period)                                        \
	DIV_ROUND_UP(USEC_PER_MSEC, symbol_period)

/* TODO: Get PHY-specific turnaround time from radio API, see
 *       https://github.com/zephyrproject-rtos/zephyr/issues/50336#issuecomment-1251122582.
 *       For now we assume PHYs that current drivers actually implement.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME(is_subg_phy)                                              \
	((is_subg_phy)                                                                             \
		 ? IEEE802154_PHY_A_TURNAROUND_TIME_1MS(IEEE802154_PHY_SYMBOL_PERIOD(is_subg_phy)) \
		 : IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT)

/* PHY PIB attribute aCcaTime, in PHY symbols, all PHYs except for SUN O-QPSK,
 * see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_CCA_TIME 8U

/**
 * @brief IEEE 802.15.4 Channel assignments
 *
 * Channel numbering for 868 MHz, 915 MHz, and 2450 MHz bands (channel page zero).
 *
 * - Channel 0 is for 868.3 MHz.
 * - Channels 1-10 are for 906 to 924 MHz with 2 MHz channel spacing.
 * - Channels 11-26 are for 2405 to 2530 MHz with 5 MHz channel spacing.
 *
 * For more information, please refer to section 10.1.3.
 */
enum ieee802154_channel {
	IEEE802154_SUB_GHZ_CHANNEL_MIN = 0,
	IEEE802154_SUB_GHZ_CHANNEL_MAX = 10,
	IEEE802154_2_4_GHZ_CHANNEL_MIN = 11,
	IEEE802154_2_4_GHZ_CHANNEL_MAX = 26,
};

enum ieee802154_hw_caps {
	IEEE802154_HW_FCS = BIT(0),            /* Frame Check-Sum supported */
	IEEE802154_HW_PROMISC = BIT(1),        /* Promiscuous mode supported */
	IEEE802154_HW_FILTER = BIT(2),         /* Filter PAN ID, long/short addr */
	IEEE802154_HW_CSMA = BIT(3),           /* Executes CSMA-CA procedure on TX */
	IEEE802154_HW_RETRANSMISSION = BIT(4), /* Handles retransmission on TX ACK timeout */
	IEEE802154_HW_TX_RX_ACK = BIT(5),      /* Waits for ACK on TX if AR bit is set in TX pkt */
	IEEE802154_HW_RX_TX_ACK = BIT(6),      /* Sends ACK on RX if AR bit is set in RX pkt */
	IEEE802154_HW_ENERGY_SCAN = BIT(7),    /* Energy scan supported */
	IEEE802154_HW_TXTIME = BIT(8),         /* TX at specified time supported */
	IEEE802154_HW_SLEEP_TO_TX = BIT(9),    /* TX directly from sleep supported */
	IEEE802154_HW_TX_SEC = BIT(10),        /* TX security handling supported */
	IEEE802154_HW_RXTIME = BIT(11),        /* RX at specified time supported */
	IEEE802154_HW_2_4_GHZ = BIT(12),       /* 2.4Ghz radio supported
						* TODO: Replace with channel page attribute.
						*/
	IEEE802154_HW_SUB_GHZ = BIT(13),       /* Sub-GHz radio supported
						* TODO: Replace with channel page attribute.
						*/
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
	IEEE802154_EVENT_RX_FAILED, /* Data reception failed */
	IEEE802154_EVENT_SLEEP, /* Sleep pending */
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
		uint8_t *ieee_addr; /* in little endian */
		uint16_t short_addr; /* in CPU byte order */
		uint16_t pan_id; /* in CPU byte order */
	};
/* @endcond */
};

struct ieee802154_key {
	uint8_t *key_value;
	uint32_t key_frame_counter;
	bool frame_counter_per_key;
	uint8_t key_id_mode;
	uint8_t key_index;
};

/** IEEE802.15.4 Transmission mode. */
enum ieee802154_tx_mode {
	/** Transmit packet immediately, no CCA. */
	IEEE802154_TX_MODE_DIRECT,

	/** Perform CCA before packet transmission. */
	IEEE802154_TX_MODE_CCA,

	/**
	 * Perform full CSMA CA procedure before packet transmission.
	 * Requires IEEE802154_HW_CSMA capability.
	 */
	IEEE802154_TX_MODE_CSMA_CA,

	/**
	 * Transmit packet in the future, at specified time, no CCA.
	 * Requires IEEE802154_HW_TXTIME capability.
	 */
	IEEE802154_TX_MODE_TXTIME,

	/**
	 * Transmit packet in the future, perform CCA before transmission.
	 * Requires IEEE802154_HW_TXTIME capability.
	 */
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
	 *  to ``1`` (see section 6.7.3).
	 *  Requires IEEE802154_HW_TX_RX_ACK capability.
	 */
	IEEE802154_CONFIG_AUTO_ACK_FPB,

	/** Indicates whether to set ACK Frame Pending bit for specific address
	 *  or not. Disabling the Frame Pending bit with no address provided
	 *  (NULL pointer) should disable it for all enabled addresses.
	 *  Requires IEEE802154_HW_TX_RX_ACK capability.
	 */
	IEEE802154_CONFIG_ACK_FPB,

	/** Indicates whether the device is a PAN coordinator. */
	IEEE802154_CONFIG_PAN_COORDINATOR,

	/** Enable/disable promiscuous mode. */
	IEEE802154_CONFIG_PROMISCUOUS,

	/** Specifies new radio event handler. Specifying NULL as a handler
	 *  will disable radio events notification.
	 */
	IEEE802154_CONFIG_EVENT_HANDLER,

	/** Updates MAC keys and key index for radios supporting transmit security. */
	IEEE802154_CONFIG_MAC_KEYS,

	/** Sets the current MAC frame counter value for radios supporting transmit security. */
	IEEE802154_CONFIG_FRAME_COUNTER,

	/** Sets the current MAC frame counter value if the provided value is greater than
	 * the current one.
	 */

	IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER,

	/** Configure a radio reception slot. This can be used for any scheduled reception, e.g.:
	 *  Zigbee GP device, CSL, TSCH, etc.
	 *  Requires IEEE802154_HW_RXTIME capability.
	 */
	IEEE802154_CONFIG_RX_SLOT,

	/** Configure CSL receiver (Endpoint) period
	 *
	 *  In order to configure a CSL receiver the upper layer should combine several
	 *  configuration options in the following way:
	 *    1. Use ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` once to inform the radio driver of the
	 *  short and extended addresses of the peer to which it should inject CSL IEs.
	 *    2. Use ``IEEE802154_CONFIG_CSL_RX_TIME`` periodically, before each use of
	 *  ``IEEE802154_CONFIG_CSL_PERIOD`` setting parameters of the nearest CSL RX window, and
	 *  before each use of IEEE_CONFIG_RX_SLOT setting parameters of the following (not the
	 *  nearest one) CSL RX window, to allow the radio driver to calculate the proper CSL Phase
	 *  to the nearest CSL window to inject in the CSL IEs for both transmitted data and ack
	 *  frames.
	 *    3. Use ``IEEE802154_CONFIG_CSL_PERIOD`` on each value change to update the current CSL
	 *  period value which will be injected in the CSL IEs together with the CSL Phase based on
	 *  ``IEEE802154_CONFIG_CSL_RX_TIME``.
	 *    4. Use ``IEEE802154_CONFIG_RX_SLOT`` periodically to schedule the immediate receive
	 *  window earlier enough before the expected window start time, taking into account
	 *  possible clock drifts and scheduling uncertainties.
	 *
	 *  This diagram shows the usage of the four options over time:
	 *        Start CSL                                  Schedule CSL window
	 *
	 *    ENH_ACK_HEADER_IE                        CSL_RX_TIME (following window)
	 *         |                                        |
	 *         | CSL_RX_TIME (nearest window)           | RX_SLOT (nearest window)
	 *         |    |                                   |   |
	 *         |    | CSL_PERIOD                        |   |
	 *         |    |    |                              |   |
	 *         v    v    v                              v   v
	 *    ----------------------------------------------------------[ CSL window ]-----+
	 *                                            ^                                    |
	 *                                            |                                    |
	 *                                            +--------------------- loop ---------+
	 */
	IEEE802154_CONFIG_CSL_PERIOD,

	/** Configure the next CSL receive window center, in units of microseconds,
	 *  based on the radio time.
	 */
	IEEE802154_CONFIG_CSL_RX_TIME,

	/** Indicates whether to inject IE into ENH ACK Frame for specific address
	 *  or not. Disabling the ENH ACK with no address provided (NULL pointer)
	 *  should disable it for all enabled addresses.
	 */
	IEEE802154_CONFIG_ENH_ACK_HEADER_IE,
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
			uint8_t *addr; /* in little endian for both, short and extended address */
			bool extended;
			bool enabled;
		} ack_fpb;

		/** ``IEEE802154_CONFIG_PAN_COORDINATOR`` */
		bool pan_coordinator;

		/** ``IEEE802154_CONFIG_PROMISCUOUS`` */
		bool promiscuous;

		/** ``IEEE802154_CONFIG_EVENT_HANDLER`` */
		ieee802154_event_cb_t event_handler;

		/** ``IEEE802154_CONFIG_MAC_KEYS``
		 *  Pointer to an array containing a list of keys used
		 *  for MAC encryption. Refer to secKeyIdLookupDescriptor and
		 *  secKeyDescriptor in IEEE 802.15.4
		 *
		 *  key_value field points to a buffer containing the 16 byte
		 *  key. The buffer is copied by the callee.
		 *
		 *  The variable length array is terminated by key_value field
		 *  set to NULL.
		 */
		struct ieee802154_key *mac_keys;

		/** ``IEEE802154_CONFIG_FRAME_COUNTER`` */
		uint32_t frame_counter;

		/** ``IEEE802154_CONFIG_RX_SLOT`` */
		struct {
			uint8_t channel;
			uint32_t start;
			uint32_t duration;
		} rx_slot;

		/** ``IEEE802154_CONFIG_CSL_PERIOD`` */
		uint32_t csl_period; /* in CPU byte order */

		/** ``IEEE802154_CONFIG_CSL_RX_TIME`` */
		uint32_t csl_rx_time; /* in microseconds,
				       * based on ieee802154_radio_api.get_time()
				       */

		/** ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` */
		struct {
			const uint8_t *data; /* header IEs to be added to the Enh-Ack frame in
					      * little endian byte order
					      */
			uint16_t data_len;
			uint16_t short_addr; /* in CPU byte order */
			/**
			 * The extended address is expected to be passed starting
			 * with the most significant octet and ending with the
			 * least significant octet.
			 * A device with an extended address 01:23:45:67:89:ab:cd:ef
			 * as written in the usual big-endian hex notation should
			 * provide a pointer to an array containing values in the
			 * exact same order.
			 */
			const uint8_t *ext_addr; /* in big endian */
		} ack_ie;
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

	/** Clear Channel Assessment - Check channel's activity */
	int (*cca)(const struct device *dev);

	/** Set current channel, channel is in CPU byte order. */
	int (*set_channel)(const struct device *dev, uint16_t channel);

	/** Set/Unset filters. Requires IEEE802154_HW_FILTER capability. */
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

	/** Start continuous carrier wave transmission.
	 *  To leave this mode, `start()` or `stop()` API function should be called,
	 *  resulting in changing radio's state to receive or sleep, respectively.
	 */
	int (*continuous_carrier)(const struct device *dev);

	/** Set specific radio driver configuration. */
	int (*configure)(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config);

	/**
	 * Get the available amount of Sub-GHz channels
	 * TODO: Replace with a combination of channel page and channel attributes.
	 */
	uint16_t (*get_subg_channel_count)(const struct device *dev);

	/** Run an energy detection scan.
	 *  Note: channel must be set prior to request this function.
	 *  duration parameter is in ms.
	 *  Requires IEEE802154_HW_ENERGY_SCAN capability.
	 */
	int (*ed_scan)(const struct device *dev,
		       uint16_t duration,
		       energy_scan_done_cb_t done_cb);

	/** Get the current radio time in microseconds
	 *  Requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME capabilities.
	 */
	uint64_t (*get_time)(const struct device *dev);

	/** Get the current accuracy, in units of ± ppm, of the clock used for
	 *  scheduling delayed receive or transmit radio operations.
	 *  Note: Implementations may optimize this value based on operational
	 *  conditions (i.e.: temperature).
	 *  Requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME capabilities.
	 */
	uint8_t (*get_sch_acc)(const struct device *dev);
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
 * @brief Radio driver ACK handling callback into L2 that radio
 *        drivers must call when receiving an ACK package.
 *
 * @details The IEEE 802.15.4 standard prescribes generic procedures for ACK
 *          handling on L2 (MAC) level. L2 stacks therefore have to provides a
 *          fast and re-usable generic implementation of this callback for
 *          radio drivers to call when receiving an ACK packet.
 *
 *          Note: This function is part of Zephyr's 802.15.4 stack L1 -> L2
 *          "inversion-of-control" adaptation API and must be implemented by
 *          all IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface that received the packet
 * @param pkt A valid pointer on a packet to check
 *
 * @return NET_OK if L2 handles the ACK package, NET_CONTINUE or NET_DROP otherwise.
 *
 *         Note: Deviating from other functions in the net stack returning net_verdict,
 *               this function will not unref the package even if it returns NET_OK.
 *
 *         TODO: Fix this deviating behavior.
 */
extern enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief Radio driver initialization callback into L2 called by radio drivers
 *        to initialize the active L2 stack for a given interface.
 *
 * @details Radio drivers must call this function as part of their own
 *          initialization routine.
 *
 *          Note: This function is part of Zephyr's 802.15.4 stack L1 -> L2
 *          "inversion-of-control" adaptation API and must be implemented by
 *          all IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface
 */
#ifndef CONFIG_IEEE802154_RAW_MODE
extern void ieee802154_init(struct net_if *iface);
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
