/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2023 F. Grandel, Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public IEEE 802.15.4 Driver API
 *
 * All references to the spec refer to IEEE 802.15.4-2020.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_time.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup ieee802154
 * @{
 */

/**
 * MAC functional description (section 6)
 */

/**
 * The symbol period (and therefore symbol rate) is defined in section 6.1: "Some
 * of the timing parameters in definition of the MAC are in units of PHY symbols.
 * For PHYs that have multiple symbol periods, the duration to be used for the
 * MAC parameters is defined in that PHY clause."
 *
 * This is not necessarily the true physical symbol period, so take care to use
 * this macro only when either the symbol period used for MAC timing is the same
 * as the physical symbol period or if you actually mean the MAC timing symbol
 * period.
 *
 * PHY specific symbol periods are defined in PHY specific sections below.
 */
#define IEEE802154_PHY_SYMBOLS_PER_SECOND(symbol_period_ns) (NSEC_PER_SEC / symbol_period_ns)

/**
 * MAC services (section 8)
 */

/**
 * The number of PHY symbols forming a superframe slot when the superframe order
 * is equal to zero, see sections 8.4.2, table 8-93, aBaseSlotDuration and
 * section 6.2.1.
 */
#define IEEE802154_MAC_A_BASE_SLOT_DURATION 60U

/**
 * The number of slots contained in any superframe, see section 8.4.2,
 * table 8-93, aNumSuperframeSlots.
 */
#define IEEE802154_MAC_A_NUM_SUPERFRAME_SLOTS 16U

/**
 * The number of PHY symbols forming a superframe when the superframe order is
 * equal to zero, see section 8.4.2, table 8-93, aBaseSuperframeDuration.
 */
#define IEEE802154_MAC_A_BASE_SUPERFRAME_DURATION                                                  \
	(IEEE802154_MAC_A_BASE_SLOT_DURATION * IEEE802154_MAC_A_NUM_SUPERFRAME_SLOTS)

/**
 * MAC PIB attribute aUnitBackoffPeriod, see section 8.4.2, table 8-93, in symbol
 * periods, valid for all PHYs except SUN PHY in the 920 MHz band.
 */
#define IEEE802154_MAC_A_UNIT_BACKOFF_PERIOD(turnaround_time)                                      \
	(turnaround_time + IEEE802154_PHY_A_CCA_TIME)

/**
 * Default macResponseWaitTime in multiples of aBaseSuperframeDuration as
 * defined in section 8.4.3.1, table 8-94.
 */
#define IEEE802154_MAC_RESPONSE_WAIT_TIME_DEFAULT 32U

/**
 * General PHY requirements (section 10)
 */

/**
 * @brief PHY channel pages, see section 10.1.3
 *
 * @details A device driver must support the mandatory channel pages, frequency
 * bands and channels of at least one IEEE 802.15.4 PHY.
 *
 * Channel page and number assignments have developed over several versions of
 * the standard and are not particularly well documented. Therefore some notes
 * about peculiarities of channel pages and channel numbering:
 * - The 2006 version of the standard had a read-only phyChannelsSupported PHY
 *   PIB attribute that represented channel page/number combinations as a
 *   bitmap. This attribute was removed in later versions of the standard as the
 *   number of channels increased beyond what could be represented by a bit map.
 *   That's the reason why we chose to represent supported channels as a
 *   combination of channel pages and ranges instead.
 * - In the 2020 version of the standard, 13 channel pages are explicitly
 *   defined, but up to 32 pages could in principle be supported. This was a
 *   hard requirement in the 2006 standard. In later standards it is implicit
 *   from field specifications, e.g. the MAC PIB attribute macChannelPage
 *   (section 8.4.3.4, table 8-100) or channel page fields used in the SRM
 *   protocol (see section 8.2.26.5).
 * - ASK PHY (channel page one) was deprecated in the 2015 version of the
 *   standard. The 2020 version of the standard is a bit ambivalent whether
 *   channel page one disappeared as well or should be interpreted as O-QPSK now
 *   (see section 10.1.3.3). We resolve this ambivalence by deprecating channel
 *   page one.
 * - For some PHYs the standard doesn't clearly specify a channel page, namely
 *   the GFSK, RS-GFSK, CMB and TASK PHYs. These are all rather new and left out
 *   in our list as long as no driver wants to implement them.
 *
 * @warning The bit numbers are not arbitrary but represent the channel
 * page numbers as defined by the standard. Therefore do not change the
 * bit numbering.
 */
enum ieee802154_phy_channel_page {
	/**
	 * Channel page zero supports the 2.4G channels of the O-QPSK PHY and
	 * all channels from the BPSK PHYs initially defined in the 2003
	 * editions of the standard. For channel page zero, 16 channels are
	 * available in the 2450 MHz band (channels 11-26, O-QPSK), 10 in the
	 * 915 MHz band (channels 1-10, BPSK), and 1 in the 868 MHz band
	 * (channel 0, BPSK).
	 *
	 * You can retrieve the channels supported by a specific driver on this
	 * page via IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES attribute.
	 *
	 * see section 10.1.3.3
	 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915 = BIT(0),

	/** Formerly ASK PHY - deprecated in IEEE 802.15.4-2015 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_ONE_DEPRECATED = BIT(1),

	/** O-QPSK PHY - 868 MHz and 915 MHz bands, see section 10.1.3.3 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915 = BIT(2),

	/** CSS PHY - 2450 MHz band, see section 10.1.3.4 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_THREE_CSS = BIT(3),

	/** UWB PHY - SubG, low and high bands, see section 10.1.3.5 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_FOUR_HRP_UWB = BIT(4),

	/** O-QPSK PHY - 780 MHz band, see section 10.1.3.2 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_FIVE_OQPSK_780 = BIT(5),

	/** reserved - not currently assigned */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_SIX_RESERVED = BIT(6),

	/** MSK PHY - 780 MHz and 2450 MHz bands, see sections 10.1.3.6, 10.1.3.7 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_SEVEN_MSK = BIT(7),

	/** LRP UWB PHY, see sections 10.1.3.8 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_EIGHT_LRP_UWB = BIT(8),

	/**
	 * SUN FSK/OFDM/O-QPSK PHYs - predefined bands, operating modes and
	 * channels, see sections 10.1.3.9
	 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_NINE_SUN_PREDEFINED = BIT(9),

	/**
	 * SUN FSK/OFDM/O-QPSK PHYs - generic modulation and channel
	 * description, see sections 10.1.3.9, 7.4.4.11
	 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_TEN_SUN_FSK_GENERIC = BIT(10),

	/** O-QPSK PHY - 2380 MHz band, see section 10.1.3.10 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_ELEVEN_OQPSK_2380 = BIT(11),

	/** LECIM DSSS/FSK PHYs, see section 10.1.3.11 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWELVE_LECIM = BIT(12),

	/** RCC PHY, see section 10.1.3.12 */
	IEEE802154_ATTR_PHY_CHANNEL_PAGE_THIRTEEN_RCC = BIT(13),
};

struct ieee802154_phy_channel_range {
	uint16_t from_channel;
	uint16_t to_channel;
};

struct ieee802154_phy_supported_channels {
	/**
	 * @brief Pointer to an array of channel range structures.
	 *
	 * @warning The pointer must be stable and valid throughout the life of
	 * the interface.
	 */
	const struct ieee802154_phy_channel_range *const ranges;

	/** @brief The number of currently available channel ranges. */
	const uint8_t num_ranges;
};

/**
 * @brief Allocate memory for the supported channels driver attribute with a
 * single channel range constant across all driver instances. This is what most
 * IEEE 802.15.4 drivers need.
 *
 * @details Example usage:
 *
 * @code{.c}
 *   IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, 11, 26);
 * @endcode
 *
 * The attribute may then be referenced like this:
 *
 * @code{.c}
 *   ... &drv_attr.phy_supported_channels ...
 * @endcode
 *
 * See @ref ieee802154_attr_get_channel_page_and_range() for a further shortcut
 * that can be combined with this macro.
 *
 * @param drv_attr name of the local static variable to be declared for the
 * local attributes structure
 * @param from the first channel to be supported
 * @param to the last channel to be supported
 */
#define IEEE802154_DEFINE_PHY_SUPPORTED_CHANNELS(drv_attr, from, to)                               \
	static const struct {                                                                      \
		const struct ieee802154_phy_channel_range phy_channel_range;                       \
		const struct ieee802154_phy_supported_channels phy_supported_channels;             \
	} drv_attr = {                                                                             \
		.phy_channel_range = {.from_channel = (from), .to_channel = (to)},                 \
		.phy_supported_channels =                                                          \
			{                                                                          \
				.ranges = &drv_attr.phy_channel_range,                             \
				.num_ranges = 1U,                                                  \
			},                                                                         \
	}


/**
 * PHY services (section 11)
 */

/* Default PHY PIB attribute aTurnaroundTime, in PHY symbols,
 * see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT 12U

/* PHY PIB attribute aTurnaroundTime for SUN, RS-GFSK, TVWS, and LECIM FSK PHY,
 * in PHY symbols, see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_1MS(symbol_period_ns)                                     \
	DIV_ROUND_UP(NSEC_PER_MSEC, symbol_period_ns)

/* PHY PIB attribute aCcaTime, in PHY symbols, all PHYs except for SUN O-QPSK,
 * see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_CCA_TIME 8U


/**
 * Q-OPSK PHY (section 12)
 */

/* Symbol periods, section 12.3.3 */
#define IEEE802154_PHY_OQPSK_868MHZ_SYMBOL_PERIOD_NS         40000LL
#define IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS 16000LL


/**
 * BPSK PHY (section 13)
 */

/* Symbol periods, section 13.3.3 */
#define IEEE802154_PHY_BPSK_868MHZ_SYMBOL_PERIOD_NS 50000LL
#define IEEE802154_PHY_BPSK_915MHZ_SYMBOL_PERIOD_NS 25000LL


/**
 * HRP UWB PHY (section 15)
 */

/* For HRP UWB the symbol period is derived from the preamble symbol period
 * (T_psym), see section 11.3, table 11-1 and section 15.2.5, table 15-4
 * (confirmed in IEEE 802.15.4z, section 15.1). Choosing among those periods
 * cannot be done based on channel page and channel alone. The mean pulse
 * repetition frequency must also be known, see the 'UwbPrf' parameter of the
 * MCPS-DATA.request primitive (section 8.3.2, table 8-88) and the preamble
 * parameters for HRP-ERDEV length 91 codes (IEEE 802.15.4z, section 15.2.6.2,
 * table 15-7b).
 */
#define IEEE802154_PHY_HRP_UWB_PRF4_TPSYM_SYMBOL_PERIOD_NS  3974.36F
#define IEEE802154_PHY_HRP_UWB_PRF16_TPSYM_SYMBOL_PERIOD_NS 993.59F
#define IEEE802154_PHY_HRP_UWB_PRF64_TPSYM_SYMBOL_PERIOD_NS 1017.63F
#define IEEE802154_PHY_HRP_UWB_ERDEV_TPSYM_SYMBOL_PERIOD_NS 729.17F

/** @brief represents the nominal pulse rate frequency of an HRP UWB PHY */
enum ieee802154_phy_hrp_uwb_nominal_prf {
	/* standard modes, see section 8.3.2, table 8-88. */
	IEEE802154_PHY_HRP_UWB_PRF_OFF = 0,
	IEEE802154_PHY_HRP_UWB_NOMINAL_4_M = BIT(0),
	IEEE802154_PHY_HRP_UWB_NOMINAL_16_M = BIT(1),
	IEEE802154_PHY_HRP_UWB_NOMINAL_64_M = BIT(2),
	/* enhanced ranging device (ERDEV) modes not specified in table 8-88,
	 * see IEEE 802.15.4z, section 15.1, section 15.2.6.2, table 15-7b,
	 * section 15.3.4.2 and section 15.3.4.3.
	 */
	IEEE802154_PHY_HRP_UWB_NOMINAL_64_M_BPRF = BIT(3),
	IEEE802154_PHY_HRP_UWB_NOMINAL_128_M_HPRF = BIT(4),
	IEEE802154_PHY_HRP_UWB_NOMINAL_256_M_HPRF = BIT(5),
};

#define IEEE802154_PHY_HRP_UWB_RDEV                                                                \
	(IEEE802154_PHY_HRP_UWB_NOMINAL_4_M | IEEE802154_PHY_HRP_UWB_NOMINAL_16_M |                \
	 IEEE802154_PHY_HRP_UWB_NOMINAL_64_M)

#define IEEE802154_PHY_HRP_UWB_ERDEV                                                               \
	(IEEE802154_PHY_HRP_UWB_NOMINAL_64_M_BPRF | IEEE802154_PHY_HRP_UWB_NOMINAL_128_M_HPRF |    \
	 IEEE802154_PHY_HRP_UWB_NOMINAL_256_M_HPRF)


/**
 * SUN FSK PHY (section 19)
 */

/* symbol periods, section 19.1, table 19-1 */
#define IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_NS 20000LL

/* in bytes, see section 19.2.4 */
#define IEEE802154_PHY_SUN_FSK_PHR_LEN 2

/**
 * IEEE 802.15.4 driver capabilities
 *
 * Any driver properties that can be represented in binary form should be
 * modeled as capabilities. These are called "hardware" capabilities for
 * historical reasons but may also represent driver firmware capabilities (e.g.
 * MAC offloading features).
 */
enum ieee802154_hw_caps {

	/*
	 * PHY capabilities
	 *
	 * The following capabilities describe features of the underlying radio
	 * hardware (PHY/L1).
	 */

	/** Energy detection (ED) supported (optional) */
	IEEE802154_HW_ENERGY_SCAN = BIT(0),

	/*
	 * MAC offloading capabilities (optional)
	 *
	 * The following MAC/L2 features may optionally be offloaded to
	 * specialized hardware or proprietary driver firmware ("hard MAC").
	 *
	 * L2 implementations will have to provide a "soft MAC" fallback for
	 * these features in case the driver does not support them natively.
	 *
	 * Note: Some of these offloading capabilities may be mandatory in
	 * practice to stay within timing requirements of certain IEEE 802.15.4
	 * protocols, e.g. CPUs may not be fast enough to send ACKs within the
	 * required delays in the 2.4 GHz band without hard MAC support.
	 */

	/** Frame checksum verification supported */
	IEEE802154_HW_FCS = BIT(1),

	/** Filtering of PAN ID, extended and short address supported */
	IEEE802154_HW_FILTER = BIT(2),

	/** Promiscuous mode supported */
	IEEE802154_HW_PROMISC = BIT(3),

	/** CSMA-CA procedure supported on TX */
	IEEE802154_HW_CSMA = BIT(4),

	/** Waits for ACK on TX if AR bit is set in TX pkt */
	IEEE802154_HW_TX_RX_ACK = BIT(5),

	/** Supports retransmission on TX ACK timeout */
	IEEE802154_HW_RETRANSMISSION = BIT(6),

	/** Sends ACK on RX if AR bit is set in RX pkt */
	IEEE802154_HW_RX_TX_ACK = BIT(7),

	/** TX at specified time supported */
	IEEE802154_HW_TXTIME = BIT(8),

	/** TX directly from sleep supported */
	IEEE802154_HW_SLEEP_TO_TX = BIT(9),

	/** Timed RX window scheduling supported */
	IEEE802154_HW_RXTIME = BIT(10),

	/** TX security supported (key management, encryption and authentication) */
	IEEE802154_HW_TX_SEC = BIT(11),

	/* Note: Update also IEEE802154_HW_CAPS_BITS_COMMON_COUNT when changing
	 * the ieee802154_hw_caps type.
	 */
};

/** @brief Number of bits used by ieee802154_hw_caps type. */
#define IEEE802154_HW_CAPS_BITS_COMMON_COUNT (12)

/** @brief This and higher values are specific to the protocol- or driver-specific extensions. */
#define IEEE802154_HW_CAPS_BITS_PRIV_START IEEE802154_HW_CAPS_BITS_COMMON_COUNT

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
/** @cond INTERNAL_HIDDEN */
	union {
		uint8_t *ieee_addr; /* in little endian */
		uint16_t short_addr; /* in CPU byte order */
		uint16_t pan_id; /* in CPU byte order */
	};
/** @endcond */
};

struct ieee802154_key {
	uint8_t *key_value;
	uint32_t key_frame_counter;
	bool frame_counter_per_key;
	uint8_t key_id_mode;
	uint8_t key_index;
};

/** IEEE 802.15.4 Transmission mode. */
enum ieee802154_tx_mode {
	/** Transmit packet immediately, no CCA. */
	IEEE802154_TX_MODE_DIRECT,

	/** Perform CCA before packet transmission. */
	IEEE802154_TX_MODE_CCA,

	/**
	 * Perform full CSMA/CA procedure before packet transmission.
	 *
	 * @note requires IEEE802154_HW_CSMA capability.
	 */
	IEEE802154_TX_MODE_CSMA_CA,

	/**
	 * Transmit packet in the future, at specified time, no CCA.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 */
	IEEE802154_TX_MODE_TXTIME,

	/**
	 * Transmit packet in the future, perform CCA before transmission.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 */
	IEEE802154_TX_MODE_TXTIME_CCA,

	/** Number of modes defined in ieee802154_tx_mode. */
	IEEE802154_TX_MODE_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_TX_MODE_PRIV_START = IEEE802154_TX_MODE_COMMON_COUNT,
};

/** IEEE 802.15.4 Frame Pending Bit table address matching mode. */
enum ieee802154_fpb_mode {
	/** The pending bit shall be set only for addresses found in the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_THREAD,

	/** The pending bit shall be cleared for short addresses found in
	 *  the list.
	 */
	IEEE802154_FPB_ADDR_MATCH_ZIGBEE,
};

/** IEEE 802.15.4 driver configuration types. */
enum ieee802154_config_type {
	/** Indicates how the driver should set Frame Pending bit in ACK
	 *  responses for Data Requests. If enabled, the driver should
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

	/** Specifies new IEEE 802.15.4 driver event handler. Specifying NULL as
	 *  a handler will disable events notification.
	 */
	IEEE802154_CONFIG_EVENT_HANDLER,

	/** Updates MAC keys and key index for drivers supporting transmit
	 *  security offloading.
	 */
	IEEE802154_CONFIG_MAC_KEYS,

	/** Sets the current MAC frame counter value for drivers supporting
	 * transmit security offloading.
	 */
	IEEE802154_CONFIG_FRAME_COUNTER,

	/** Sets the current MAC frame counter value if the provided value is greater than
	 *  the current one.
	 */
	IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER,

	/** Configure a radio reception window. This can be used for any
	 *  scheduled reception, e.g.: Zigbee GP device, CSL, TSCH, etc.
	 *
	 *  @note requires IEEE802154_HW_RXTIME capability.
	 */
	IEEE802154_CONFIG_RX_SLOT,

	/** Configure CSL receiver (Endpoint) period
	 *
	 *  In order to configure a CSL receiver the upper layer should combine several
	 *  configuration options in the following way:
	 *    1. Use ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` once to inform the driver of the
	 *       short and extended addresses of the peer to which it should inject CSL IEs.
	 *    2. Use ``IEEE802154_CONFIG_CSL_RX_TIME`` periodically, before each use of
	 *       ``IEEE802154_CONFIG_CSL_PERIOD`` setting parameters of the nearest CSL RX window,
	 *       and before each use of IEEE_CONFIG_RX_SLOT setting parameters of the following (not
	 *       the nearest one) CSL RX window, to allow the driver to calculate the proper
	 *       CSL Phase to the nearest CSL window to inject in the CSL IEs for both transmitted
	 *       data and ACK frames.
	 *    3. Use ``IEEE802154_CONFIG_CSL_PERIOD`` on each value change to update the current CSL
	 *       period value which will be injected in the CSL IEs together with the CSL Phase
	 *       based on ``IEEE802154_CONFIG_CSL_RX_TIME``.
	 *    4. Use ``IEEE802154_CONFIG_RX_SLOT`` periodically to schedule the immediate receive
	 *       window earlier enough before the expected window start time, taking into account
	 *       possible clock drifts and scheduling uncertainties.
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

	/** Configure the next CSL receive window (i.e. "channel sample") center,
	 *  in units of nanoseconds relative to the network subsystem's local clock.
	 */
	IEEE802154_CONFIG_CSL_RX_TIME,

	/** Indicates whether to inject IE into ENH ACK Frame for specific address
	 *  or not. Disabling the ENH ACK with no address provided (NULL pointer)
	 *  should disable it for all enabled addresses.
	 */
	IEEE802154_CONFIG_ENH_ACK_HEADER_IE,

	/** Number of types defined in ieee802154_config_type. */
	IEEE802154_CONFIG_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_CONFIG_PRIV_START = IEEE802154_CONFIG_COMMON_COUNT,
};

/** IEEE 802.15.4 driver configuration data. */
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
			/**
			 * Nanosecond resolution timestamp relative to the
			 * network subsystem's local clock defining the start of
			 * the RX window during which the receiver is expected
			 * to be listening (i.e. not including any startup
			 * times).
			 */
			net_time_t start;

			/**
			 * Nanosecond resolution duration of the RX window
			 * relative to the above RX window start time during
			 * which the receiver is expected to be listening (i.e.
			 * not including any shutdown times).
			 */
			net_time_t duration;

			uint8_t channel;
		} rx_slot;

		/**
		 * ``IEEE802154_CONFIG_CSL_PERIOD``
		 *
		 * The CSL period in units of 10 symbol periods,
		 * see section 7.4.2.3.
		 *
		 * in CPU byte order
		 */
		uint32_t csl_period;

		/**
		 * ``IEEE802154_CONFIG_CSL_RX_TIME``
		 *
		 * Nanosecond resolution timestamp relative to the network
		 * subsystem's local clock defining the center of the CSL RX window
		 * at which the receiver is expected to be fully started up
		 * (i.e.  not including any startup times).
		 */
		net_time_t csl_rx_time;

		/** ``IEEE802154_CONFIG_ENH_ACK_HEADER_IE`` */
		struct {
			/**
			 * header IEs to be added to the Enh-Ack frame
			 *
			 * in little endian
			 */
			const uint8_t *data;
			uint16_t data_len;
			/** in CPU byte order */
			uint16_t short_addr;
			/** in big endian */
			const uint8_t *ext_addr;
		} ack_ie;
	};
};

/**
 * @brief IEEE 802.15.4 driver attributes.
 *
 * See @ref ieee802154_attr_value and @ref ieee802154_radio_api for usage
 * details.
 */
enum ieee802154_attr {
	/**
	 * Retrieves a bit field with supported channel pages. This attribute
	 * SHALL be implemented by all drivers.
	 */
	IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES,

	/**
	 * Retrieves a pointer to the array of supported channel ranges within
	 * the currently configured channel page. This attribute SHALL be
	 * implemented by all drivers.
	 */
	IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES,

	/**
	 * Retrieves a bit field with supported HRP UWB nominal pulse repetition
	 * frequencies. This attribute SHALL be implemented by all devices that
	 * support channel page four (HRP UWB).
	 */
	IEEE802154_ATTR_PHY_HRP_UWB_SUPPORTED_PRFS,

	/** Number of attributes defined in ieee802154_attr. */
	IEEE802154_ATTR_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or
	 * driver-specific extensions.
	 */
	IEEE802154_ATTR_PRIV_START = IEEE802154_ATTR_COMMON_COUNT,
};

/**
 * @brief IEEE 802.15.4 driver attribute values.
 *
 * @details This structure is reserved to scalar and structured attributes that
 * originate in the driver implementation and can neither be implemented as
 * boolean @ref ieee802154_hw_caps nor be derived directly or indirectly by the
 * MAC (L2) layer. In particular this structure MUST NOT be used to return
 * configuration data that originate from L2.
 *
 * @note To keep this union reasonably small, any attribute requiring a large
 * memory area, SHALL be provided pointing to stable memory allocated by the
 * driver.
 */
struct ieee802154_attr_value {
	union {
		/**
		 * @brief A bit field that represents the supported channel
		 * pages, see ieee802154_phy_channel_page.
		 *
		 * @note To keep the API extensible as required by the standard,
		 * we model supported pages as a bitmap to support drivers that
		 * implement runtime switching between multiple channel pages.
		 *
		 * @note Currently none of the Zephyr drivers implements more
		 * than one channel page at runtime, therefore only one bit will
		 * be set and we consider the current channel page (see the PHY
		 * PIB attribute phyCurrentPage, section 11.3, table 11-2) to be
		 * read-only, fixed and "well known" via the supported channel
		 * pages attribute.
		 *
		 * TODO: Implement configuration of phyCurrentPage once drivers
		 * need to support channel page switching at runtime.
		 */
		uint32_t phy_supported_channel_pages;

		/**
		 * @brief Pointer to a structure representing channel ranges
		 * currently available on the selected channel page.
		 *
		 * @warning The pointer must be stable and valid throughout the
		 * life of the interface.
		 *
		 * @details The selected channel page corresponds to the
		 * phyCurrentPage PHY PIB attribute, see the description of
		 * phy_supported_channel_pages above. Currently it can be
		 * retrieved via the IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES
		 * attribute.
		 *
		 * Most drivers will expose a single channel page with a single,
		 * often zero-based, fixed channel range.
		 *
		 * Some notable exceptions:
		 * * The legacy channel page (zero) exposes ranges in different
		 *   bands and even PHYs that are usually not implemented by a
		 *   single driver.
		 * * SUN and LECIM PHYs specify a large number of bands and
		 *   operating modes on a single page with overlapping channel
		 *   ranges each. Some of these ranges are not zero-based or
		 *   contain "holes". This explains why several ranges may be
		 *   necessary to represent all available channels.
		 * * UWB PHYs often support partial channel ranges on the same
		 *   channel page depending on the supported bands.
		 *
		 * In these cases, drivers may expose custom configuration
		 * attributes (Kconfig, devicetree, runtime, ...) that allow
		 * switching between sub-ranges within the same channel page
		 * (e.g. switching between SubG and 2.4G bands on channel page
		 * zero or switching between multiple operating modes in the SUN
		 * or LECIM PHYs.
		 */
		const struct ieee802154_phy_supported_channels *phy_supported_channels;

		/**
		 * @brief A bit field representing supported HRP UWB pulse
		 * repetition frequencies (PRF), see enum
		 * ieee802154_phy_hrp_uwb_nominal_prf.
		 *
		 * @note Currently none of the Zephyr HRP UWB drivers implements
		 * more than one nominal PRF at runtime, therefore only one bit
		 * will be set and we consider the current PRF (UwbPrf,
		 * MCPS-DATA.request, section 8.3.2, table 8-88) to be
		 * read-only, fixed and "well known" via the supported PRF
		 * attribute.
		 *
		 * TODO: Allow the PRF to be configured for each TX call once
		 * drivers need to support PRF switching at runtime.
		 */
		uint32_t phy_hrp_uwb_supported_nominal_prfs;
	};
};

/**
 * @brief Helper function to handle channel page and rank to be called from
 * drivers' attr_get() implementation. This only applies to drivers with a
 * single channel page.
 *
 * @param attr The attribute to be retrieved.
 * @param phy_supported_channel_page The driver's unique channel page.
 * @param phy_supported_channels Pointer to the structure that contains the
 * driver's channel range or ranges.
 * @param value The pointer to the value struct provided by the user.
 *
 * @retval 0 if the attribute could be resolved
 * @retval -ENOENT if the attribute could not be resolved
 */
static inline int ieee802154_attr_get_channel_page_and_range(
	enum ieee802154_attr attr,
	const enum ieee802154_phy_channel_page phy_supported_channel_page,
	const struct ieee802154_phy_supported_channels *phy_supported_channels,
	struct ieee802154_attr_value *value)
{
	switch (attr) {
	case IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES:
		value->phy_supported_channel_pages = phy_supported_channel_page;
		return 0;

	case IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES:
		value->phy_supported_channels = phy_supported_channels;
		return 0;

	default:
		return -ENOENT;
	}
}

/**
 * @brief IEEE 802.15.4 driver interface API.
 *
 * @note This structure is called "radio" API for backwards compatibility. A
 * better name would be "IEEE 802.15.4 driver API" as typical drivers will not
 * only implement L1/radio (PHY) features but also L2 (MAC) features if the
 * vendor-specific driver hardware or firmware offers offloading opportunities.
 *
 * While L1-level driver features are exclusively implemented by drivers and MAY
 * be mandatory to support certain application requirements, L2 features SHOULD
 * be optional by default and only need to be implemented for performance
 * optimization or precise timing as deemed necessary by driver maintainers.
 * Fallback implementations ("Soft MAC") SHOULD be provided in the
 * driver-independent L2 layer for all L2/MAC features especially if these
 * features are not implemented in vendor hardware/firmware by a majority of
 * existing in-tree drivers. If, however, a driver offers offloading
 * opportunities then L2 implementations SHALL delegate performance critical or
 * resource intensive tasks to the driver.
 */
struct ieee802154_radio_api {
	/**
	 * @brief network interface API
	 *
	 * @note Network devices must extend the network interface API. It is
	 * therefore mandatory to place it at the top of the driver API struct so
	 * that it can be cast to a network interface.
	 */
	struct net_if_api iface_api;

	/**
	 * @brief Get the device driver capabilities.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @return Bit field with all supported device driver capabilities.
	 */
	enum ieee802154_hw_caps (*get_capabilities)(const struct device *dev);

	/**
	 * @brief Clear Channel Assessment - Check channel's activity
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 the channel is available
	 * @retval -EBUSY The channel is busy.
	 * @retval -EIO The CCA procedure could not be executed.
	 * @retval -ENOTSUP CCA is not supported by this driver.
	 */
	int (*cca)(const struct device *dev);

	/**
	 * @brief Set current channel
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param channel the number of the channel to be set in CPU byte order
	 *
	 * @retval 0 channel was successfully set
	 * @retval -EINVAL The given channel is not within the range of valid
	 * channels of the driver's current channel page, see the
	 * IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES driver attribute.
	 * @retval -ENOTSUP The given channel is within the range of valid
	 * channels of the driver's current channel page but unsupported by the
	 * current driver.
	 * @retval -EIO The channel could not be set.
	 */
	int (*set_channel)(const struct device *dev, uint16_t channel);

	/**
	 * @brief Set/Unset PAN ID, extended or short address filters.
	 *
	 * @note requires IEEE802154_HW_FILTER capability.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param set true to set the filter, false to remove it
	 * @param type the type of entity to be added/removed from the filter
	 * list (a PAN ID or a source/destination address)
	 * @param filter the entity to be added/removed from the filter list
	 *
	 * @retval 0 The filter was successfully added/removed.
	 * @retval -EINVAL The given filter entity or filter entity type
	 * was not valid.
	 * @retval -ENOTSUP Setting/removing this filter or filter type
	 * is not supported by this driver.
	 * @retval -EIO Error while setting/removing the filter.
	 */
	int (*filter)(const struct device *dev,
		      bool set,
		      enum ieee802154_filter_type type,
		      const struct ieee802154_filter *filter);

	/**
	 * @brief Set TX power level in dbm
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param dbm TX power in dbm
	 *
	 * @retval 0 The TX power was successfully set.
	 * @retval -EINVAL The given dbm value is invalid or not supported by
	 * the driver.
	 * @retval -EIO The TX power could not be set.
	 */
	int (*set_txpower)(const struct device *dev, int16_t dbm);

	/**
	 * @brief Transmit a packet fragment as a single frame
	 *
	 * @warning The driver must not take ownership of the given network
	 * packet and frame (fragment) buffer. Any data required by the driver
	 * (including the actual frame content) must be read synchronously and
	 * copied internally if transmission is delayed or executed
	 * asynchronously. Both, the packet and the buffer may be re-used or
	 * released immediately after the function returns.
	 *
	 * @note Depending on the level of offloading features supported by the
	 * driver, the frame may not be fully encrypted/authenticated, may not
	 * contain an FCS or may contain incomplete information elements (IEs).
	 * It is the responsibility of L2 implementations to prepare the frame
	 * according to the offloading capabilities announced by the driver and
	 * to decide whether CCA, CSMA/CA or ACK procedures need to be executed
	 * in software ("soft MAC") or will be provided by the driver itself
	 * ("hard MAC").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param mode the transmission mode, some of which require specific
	 * offloading capabilities.
	 * @param pkt pointer to the network packet to be transmitted.
	 * @param frag pointer to a network buffer containing a single fragment
	 * with the frame data to be transmitted
	 *
	 * @retval 0 The frame was successfully sent or scheduled. If the driver
	 * supports ACK offloading and the frame requested acknowlegment (AR bit
	 * set), this means that the packet was successfully acknowledged by its
	 * peer.
	 * @retval -ENOTSUP The given TX mode is not supported.
	 * @retval -EIO The frame could not be sent due to some unspecified
	 * error.
	 * @retval -EBUSY The frame could not be sent because the medium was
	 * busy (CSMA/CA or CCA offloading feature only).
	 * @retval -ENOMSG The frame was not confirmed by an ACK packet (TX ACK
	 * offloading feature only).
	 * @retval -ENOBUFS The frame could not be scheduled due to missing
	 * internal buffer resources (timed TX offloading feature only).
	 */
	int (*tx)(const struct device *dev, enum ieee802154_tx_mode mode,
		  struct net_pkt *pkt, struct net_buf *frag);

	/**
	 * @brief Start the device and place it in receive mode.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 The driver was successfully started.
	 * @retval -EIO The driver could not be started.
	 */
	int (*start)(const struct device *dev);

	/**
	 * @brief Stop the device and switch off the receiver (sleep mode).
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 The driver was successfully stopped.
	 * @retval -EIO The driver could not be stopped.
	 */
	int (*stop)(const struct device *dev);

	/**
	 * @brief Start continuous carrier wave transmission.
	 *
	 * @details To leave this mode, `start()` or `stop()` should be called,
	 * putting the driver in receive or sleep mode, respectively.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 continuous carrier wave transmission started
	 * @retval -EIO not started
	 */
	int (*continuous_carrier)(const struct device *dev);

	/**
	 * @brief Set driver configuration.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param type the configuration type to be set
	 * @param config the configuration parameters to be set for the given
	 * configuration type
	 *
	 * @retval 0 configuration successful
	 * @retval -ENOTSUP The given configuration type is not supported by
	 * this driver.
	 * @retval -EINVAL The configuration parameters are invalid for the
	 * given configuration type.
	 * @retval -ENOMEM The configuration cannot be saved due to missing
	 * memory resources.
	 * @retval -ENOENT The resource referenced in the configuration
	 * parameters cannot be found in the configuration.
	 */
	int (*configure)(const struct device *dev,
			 enum ieee802154_config_type type,
			 const struct ieee802154_config *config);

	/**
	 * @brief Run an energy detection scan.
	 *
	 * @note requires IEEE802154_HW_ENERGY_SCAN capability
	 *
	 * @note The radio channel must be set prior to calling this function.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param duration duration of energy scan in ms
	 * @param done_cb function called when the energy scan has finished
	 *
	 * @retval 0 the energy detection scan was successfully scheduled
	 *
	 * @retval -EBUSY the energy detection scan could not be scheduled at
	 * this time
	 * @retval -EALREADY a previous energy detection scan has not finished
	 * yet.
	 * @retval -ENOTSUP This driver does not support energy scans.
	 */
	int (*ed_scan)(const struct device *dev,
		       uint16_t duration,
		       energy_scan_done_cb_t done_cb);

	/**
	 * @brief Get the current time in nanoseconds relative to the network
	 * subsystem's local uptime clock as represented by this network
	 * interface.
	 *
	 * See @ref net_time_t for semantic details.
	 *
	 * @note requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME
	 * capabilities.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @return nanoseconds relative to the network subsystem's local clock
	 */
	net_time_t (*get_time)(const struct device *dev);

	/**
	 * @brief Get the current estimated worst case accuracy (maximum ±
	 * deviation from the nominal frequency) of the network subsystem's
	 * local clock used to calculate tolerances and guard times when
	 * scheduling delayed receive or transmit radio operations.
	 *
	 * The deviation is given in units of PPM (parts per million).
	 *
	 * @note requires IEEE802154_HW_TXTIME and/or IEEE802154_HW_RXTIME
	 * capabilities.
	 *
	 * @note Implementations may estimate this value based on current
	 * operating conditions (e.g. temperature).
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @return current estimated clock accuracy in PPM
	 */
	uint8_t (*get_sch_acc)(const struct device *dev);

	/**
	 * @brief Get the value of a driver specific attribute.
	 *
	 * @note This function SHALL NOT return any values configurable by the
	 * MAC (L2) layer. It is reserved to non-boolean (i.e. scalar or
	 * structured) attributes that originate from the driver implementation
	 * and cannot be directly or indirectly derived by L2. Boolean
	 * attributes SHALL be implemented as @ref ieee802154_hw_caps.
	 *
	 * @retval 0 The requested attribute is supported by the driver and the
	 * value can be retrieved from the corresponding @ref ieee802154_attr_value
	 * member.
	 *
	 * @retval -ENOENT The driver does not provide the requested attribute.
	 * The value structure has not been updated with attribute data. The
	 * content of the value attribute is undefined.
	 */
	int (*attr_get)(const struct device *dev,
			enum ieee802154_attr attr,
			struct ieee802154_attr_value *value);
};

/* Make sure that the network interface API is properly setup inside
 * IEEE 802.15.4 driver API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct ieee802154_radio_api, iface_api) == 0);

/**
 * IEEE 802.15.4 driver utils
 */

#define IEEE802154_AR_FLAG_SET (0x20)

/**
 * @brief Check if AR flag is set on the frame inside given net_pkt
 *
 * @param frag A valid pointer on a net_buf structure, must not be NULL,
 *        and its length should be at least 1 byte (ImmAck frames are the
 *        shortest supported frames with 3 bytes excluding FCS).
 *
 * @return true if AR flag is set, false otherwise
 */
static inline bool ieee802154_is_ar_flag_set(struct net_buf *frag)
{
	return (*frag->data & IEEE802154_AR_FLAG_SET);
}

/**
 * IEEE 802.15.4 driver callbacks
 */

/**
 * @brief IEEE 802.15.4 driver ACK handling callback into L2 that drivers must
 *        call when receiving an ACK package.
 *
 * @details The IEEE 802.15.4 standard prescribes generic procedures for ACK
 *          handling on L2 (MAC) level. L2 stacks therefore have to provides a
 *          fast and re-usable generic implementation of this callback for
 *          drivers to call when receiving an ACK packet.
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
 * @brief IEEE 802.15.4 driver initialization callback into L2 called by drivers
 *        to initialize the active L2 stack for a given interface.
 *
 * @details Drivers must call this function as part of their own initialization
 *          routine.
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
