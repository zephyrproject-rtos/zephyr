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
 * @note All references to the standard in this file cite IEEE 802.15.4-2020.
 */

#ifndef ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_
#define ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_

#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_time.h>
#include <zephyr/net/ieee802154.h>
#include <zephyr/net/ieee802154_ie.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ieee802154_driver IEEE 802.15.4 Drivers
 * @since 1.0
 * @version 0.8.0
 * @ingroup ieee802154
 *
 * @brief IEEE 802.15.4 driver API
 *
 * @details This API provides a common representation of vendor-specific
 * hardware and firmware to the native IEEE 802.15.4 L2 and OpenThread stacks.
 * **Application developers should never interface directly with this API.** It
 * is of interest to driver maintainers only.
 *
 * The IEEE 802.15.4 driver API consists of two separate parts:
 *    - a basic, mostly PHY-level driver API to be implemented by all drivers,
 *    - several optional MAC-level extension points to offload performance
 *      critical or timing sensitive aspects at MAC level to the driver hardware
 *      or firmware ("hard" MAC).
 *
 * Implementing the basic driver API will ensure integration with the native L2
 * stack as well as basic support for OpenThread. Depending on the hardware,
 * offloading to vendor-specific hardware or firmware features may be required
 * to achieve full compliance with the Thread protocol or IEEE 802.15.4
 * subprotocols (e.g. fast enough ACK packages, precise timing of timed TX/RX in
 * the TSCH or CSL subprotocols).
 *
 * Whether or not MAC-level offloading extension points need to be implemented
 * is to be decided by individual driver maintainers. Upper layers SHOULD
 * provide a "soft" MAC fallback whenever possible.
 *
 * @note All section, table and figure references are to the IEEE 802.15.4-2020
 * standard.
 *
 * @{
 */

/**
 * @name IEEE 802.15.4-2020, Section 6: MAC functional description
 * @{
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

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 8: MAC services
 * @{
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

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 10: General PHY requirements
 * @{
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
 *   That's the reason why it was decided to represent supported channels as a
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
 *   (see section 10.1.3.3). In Zephyr this ambivalence is resolved by
 *   deprecating channel page one.
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
	 * page via @ref IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES attribute.
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

/**
 * Represents a supported channel range, see @ref
 * ieee802154_phy_supported_channels.
 */
struct ieee802154_phy_channel_range {
	uint16_t from_channel; /**< From channel range */
	uint16_t to_channel;   /**< To channel range */
};

/**
 * Represents the list of channels supported by a driver for a given interface,
 * see @ref IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES.
 */
struct ieee802154_phy_supported_channels {
	/**
	 * @brief Pointer to an array of channel range structures.
	 *
	 * @warning The pointer must be valid and constant throughout the life
	 * of the interface.
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

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 11: PHY services
 * @{
 */

/**
 * Default PHY PIB attribute aTurnaroundTime, in PHY symbols, see section 11.3,
 * table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT 12U

/**
 * PHY PIB attribute aTurnaroundTime for SUN, RS-GFSK, TVWS, and LECIM FSK PHY,
 * in PHY symbols, see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_TURNAROUND_TIME_1MS(symbol_period_ns)                                     \
	DIV_ROUND_UP(NSEC_PER_MSEC, symbol_period_ns)

/**
 * PHY PIB attribute aCcaTime, in PHY symbols, all PHYs except for SUN O-QPSK,
 * see section 11.3, table 11-1.
 */
#define IEEE802154_PHY_A_CCA_TIME 8U

/** @} */



/**
 * @name IEEE 802.15.4-2020, Section 12: O-QPSK PHY
 * @{
 */

/** O-QPSK 868Mhz band symbol period, see section 12.3.3 */
#define IEEE802154_PHY_OQPSK_868MHZ_SYMBOL_PERIOD_NS         40000LL

/**
 * O-QPSK 780MHz, 915MHz, 2380MHz and 2450MHz bands symbol period,
 * see section 12.3.3
 */
#define IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS 16000LL

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 13: BPSK PHY
 * @{
 */

/** BPSK 868MHz band symbol period, see section 13.3.3 */
#define IEEE802154_PHY_BPSK_868MHZ_SYMBOL_PERIOD_NS 50000LL

/** BPSK 915MHz band symbol period, see section 13.3.3 */
#define IEEE802154_PHY_BPSK_915MHZ_SYMBOL_PERIOD_NS 25000LL

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 15: HRP UWB PHY
 *
 * @details For HRP UWB the symbol period is derived from the preamble symbol period
 * (T_psym), see section 11.3, table 11-1 and section 15.2.5, table 15-4
 * (confirmed in IEEE 802.15.4z, section 15.1). Choosing among those periods
 * cannot be done based on channel page and channel alone. The mean pulse
 * repetition frequency must also be known, see the 'UwbPrf' parameter of the
 * MCPS-DATA.request primitive (section 8.3.2, table 8-88) and the preamble
 * parameters for HRP-ERDEV length 91 codes (IEEE 802.15.4z, section 15.2.6.2,
 * table 15-7b).
 * @{
 */

/** Nominal PRF 4MHz symbol period */
#define IEEE802154_PHY_HRP_UWB_PRF4_TPSYM_SYMBOL_PERIOD_NS  3974.36F
/** Nominal PRF 16MHz symbol period */
#define IEEE802154_PHY_HRP_UWB_PRF16_TPSYM_SYMBOL_PERIOD_NS 993.59F
/** Nominal PRF 64MHz symbol period */
#define IEEE802154_PHY_HRP_UWB_PRF64_TPSYM_SYMBOL_PERIOD_NS 1017.63F
/** ERDEV symbol period */
#define IEEE802154_PHY_HRP_UWB_ERDEV_TPSYM_SYMBOL_PERIOD_NS 729.17F

/** @brief represents the nominal pulse rate frequency of an HRP UWB PHY */
enum ieee802154_phy_hrp_uwb_nominal_prf {
	/** standard modes, see section 8.3.2, table 8-88. */
	IEEE802154_PHY_HRP_UWB_PRF_OFF = 0,
	IEEE802154_PHY_HRP_UWB_NOMINAL_4_M = BIT(0),
	IEEE802154_PHY_HRP_UWB_NOMINAL_16_M = BIT(1),
	IEEE802154_PHY_HRP_UWB_NOMINAL_64_M = BIT(2),

	/**
	 * enhanced ranging device (ERDEV) modes not specified in table 8-88,
	 * see IEEE 802.15.4z, section 15.1, section 15.2.6.2, table 15-7b,
	 * section 15.3.4.2 and section 15.3.4.3.
	 */
	IEEE802154_PHY_HRP_UWB_NOMINAL_64_M_BPRF = BIT(3),
	IEEE802154_PHY_HRP_UWB_NOMINAL_128_M_HPRF = BIT(4),
	IEEE802154_PHY_HRP_UWB_NOMINAL_256_M_HPRF = BIT(5),
};

/** RDEV device mask */
#define IEEE802154_PHY_HRP_UWB_RDEV                                                                \
	(IEEE802154_PHY_HRP_UWB_NOMINAL_4_M | IEEE802154_PHY_HRP_UWB_NOMINAL_16_M |                \
	 IEEE802154_PHY_HRP_UWB_NOMINAL_64_M)

/** ERDEV device mask */
#define IEEE802154_PHY_HRP_UWB_ERDEV                                                               \
	(IEEE802154_PHY_HRP_UWB_NOMINAL_64_M_BPRF | IEEE802154_PHY_HRP_UWB_NOMINAL_128_M_HPRF |    \
	 IEEE802154_PHY_HRP_UWB_NOMINAL_256_M_HPRF)

/** @} */


/**
 * @name IEEE 802.15.4-2020, Section 19: SUN FSK PHY
 * @{
 */

/** SUN FSK 863Mhz and 915MHz band symbol periods, see section 19.1, table 19-1 */
#define IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_NS 20000LL

/** SUN FSK PHY header length, in bytes, see section 19.2.4 */
#define IEEE802154_PHY_SUN_FSK_PHR_LEN 2

/** @} */

/**
 * @name IEEE 802.15.4 Driver API
 * @{
 */

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

	/** TX directly from sleep supported
	 *
	 *  @note This HW capability does not conform to the requirements
	 *  specified in #61227 as it closely couples the driver to OpenThread's
	 *  capability and device model which is different from Zephyr's:
	 *   - "Sleeping" is a well defined term in Zephyr related to internal
	 *     power and thread management and different from "RX off" as
	 *     defined in OT.
	 *   - Currently all OT-capable drivers have the "sleep to TX"
	 *     capability anyway plus we expect future drivers to implement it
	 *     ootb as well, so no information is actually conveyed by this
	 *     capability.
	 *   - The `start()`/`stop()` API of a net device controls the
	 *     interface's operational state. Drivers MUST respond with
	 *     -ENETDOWN when calling `tx()` while their operational state is
	 *     "DOWN", only devices in the "UP" state MAY transmit packets (RFC
	 *     2863).
	 *   - A migration path has been defined in #63670 for actual removal of
	 *     this capability, see there for details.
	 *
	 * @deprecated Drivers and L2 SHALL not introduce additional references
	 * to this capability and remove existing ones as outlined in #63670.
	 */
	IEEE802154_HW_SLEEP_TO_TX = BIT(9),

	/** Timed RX window scheduling supported */
	IEEE802154_HW_RXTIME = BIT(10),

	/** TX security supported (key management, encryption and authentication) */
	IEEE802154_HW_TX_SEC = BIT(11),

	/** `macRxOnWhenIdle` driver offloading supported */
	IEEE802154_HW_RX_ON_WHEN_IDLE = BIT(12),

	/** Support for timed transmissions on selective channel.
	 *
	 *  This capability informs that transmissions with modes
	 *  @ref IEEE802154_TX_MODE_TXTIME and @ref IEEE802154_TX_MODE_TXTIME_CCA support
	 *  scheduling of timed transmissions on selective tx channel.
	 *  The driver MAY have this capability only if the Kconfig option
	 *  `CONFIG_IEEE802154_SELECTIVE_TXCHANNEL` is set, otherwise the driver MUST
	 *  NOT have this capability.
	 *
	 *  Please refer to the `ieee802154_radio_api::tx` documentation for details.
	 */
	IEEE802154_HW_SELECTIVE_TXCHANNEL = BIT(13),

	/* Note: Update also IEEE802154_HW_CAPS_BITS_COMMON_COUNT when changing
	 * the ieee802154_hw_caps type.
	 */
};

/** @brief Number of bits used by ieee802154_hw_caps type. */
#define IEEE802154_HW_CAPS_BITS_COMMON_COUNT (14)

/** @brief This and higher values are specific to the protocol- or driver-specific extensions. */
#define IEEE802154_HW_CAPS_BITS_PRIV_START IEEE802154_HW_CAPS_BITS_COMMON_COUNT

/** Filter type, see @ref ieee802154_radio_api::filter */
enum ieee802154_filter_type {
	IEEE802154_FILTER_TYPE_IEEE_ADDR,      /**< Address type filter */
	IEEE802154_FILTER_TYPE_SHORT_ADDR,     /**< Short address type filter */
	IEEE802154_FILTER_TYPE_PAN_ID,         /**< PAN id type filter */
	IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,  /**< Source address type filter */
	IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR, /**< Source short address type filter */
};

/** Driver events, see @ref IEEE802154_CONFIG_EVENT_HANDLER */
enum ieee802154_event {
	/** Data transmission started */
	IEEE802154_EVENT_TX_STARTED,
	/** Data reception failed */
	IEEE802154_EVENT_RX_FAILED,
	/**
	 * An RX slot ended, requires @ref IEEE802154_HW_RXTIME.
	 *
	 * @note This event SHALL not be triggered by drivers when RX is
	 * synchronously switched of due to a call to `stop()` or an RX slot
	 * being configured.
	 */
	IEEE802154_EVENT_RX_OFF,
};

/** RX failed event reasons, see @ref IEEE802154_EVENT_RX_FAILED */
enum ieee802154_rx_fail_reason {
	/** Nothing received */
	IEEE802154_RX_FAIL_NOT_RECEIVED,
	/** Frame had invalid checksum */
	IEEE802154_RX_FAIL_INVALID_FCS,
	/** Address did not match */
	IEEE802154_RX_FAIL_ADDR_FILTERED,
	/** General reason */
	IEEE802154_RX_FAIL_OTHER
};

/** Energy scan callback */
typedef void (*energy_scan_done_cb_t)(const struct device *dev,
				      int16_t max_ed);

/** Driver event callback */
typedef void (*ieee802154_event_cb_t)(const struct device *dev,
				      enum ieee802154_event evt,
				      void *event_params);

/** Filter value, see @ref ieee802154_radio_api::filter */
struct ieee802154_filter {
	union {
		/** Extended address, in little endian */
		uint8_t *ieee_addr;
		/** Short address, in CPU byte order */
		uint16_t short_addr;
		/** PAN ID, in CPU byte order */
		uint16_t pan_id;
	};
};

/**
 * Key configuration for transmit security offloading, see @ref
 * IEEE802154_CONFIG_MAC_KEYS.
 */
struct ieee802154_key {
	/** Key material */
	uint8_t *key_value;
	/** Initial value of frame counter associated with the key, see section 9.4.3 */
	uint32_t key_frame_counter;
	/** Indicates if per-key frame counter should be used, see section 9.4.3 */
	bool frame_counter_per_key;
	/** Key Identifier Mode, see section 9.4.2.3, Table 9-7 */
	uint8_t key_id_mode;
	/** Key Identifier, see section 9.4.4 */
	uint8_t *key_id;
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
	 * Transmit packet in the future, at the specified time, no CCA.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 *
	 * @note capability IEEE802154_HW_SELECTIVE_TXCHANNEL may apply.
	 */
	IEEE802154_TX_MODE_TXTIME,

	/**
	 * Transmit packet in the future, perform CCA before transmission.
	 *
	 * @note requires IEEE802154_HW_TXTIME capability.
	 *
	 * @note Required for Thread 1.2 Coordinated Sampled Listening feature
	 * (see Thread specification 1.2.0, ch. 3.2.6.3).
	 *
	 * @note capability IEEE802154_HW_SELECTIVE_TXCHANNEL may apply.
	 */
	IEEE802154_TX_MODE_TXTIME_CCA,

	/** Number of modes defined in ieee802154_tx_mode. */
	IEEE802154_TX_MODE_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_TX_MODE_PRIV_START = IEEE802154_TX_MODE_COMMON_COUNT,
};

/** IEEE 802.15.4 Receiver mode. */
enum ieee802154_rx_mode {
	/** The receiver is continuously on. */
	IEEE802154_RX_ON,

	/** The receiver is continuously off. */
	IEEE802154_RX_OFF,

	/** Number of modes defined in ieee802154_rx_mode. */
	IEEE802154_RX_MODE_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_RX_MODE_PRIV_START = IEEE802154_RX_MODE_COMMON_COUNT,
};

/** IEEE 802.15.4 Frame Pending Bit table address matching mode. */
enum ieee802154_fpb_mode {
	/** The pending bit shall be set only for addresses found in the list. */
	IEEE802154_FPB_ADDR_MATCH_THREAD,

	/** The pending bit shall be cleared for short addresses found in the
	 *  list.
	 */
	IEEE802154_FPB_ADDR_MATCH_ZIGBEE,
};

/** IEEE 802.15.4 driver configuration types. */
enum ieee802154_config_type {
	/**
	 * Indicates how the driver should set the Frame Pending bit in ACK
	 * responses for Data Requests. If enabled, the driver should determine
	 * whether to set the bit or not based on the information provided with
	 * @ref IEEE802154_CONFIG_ACK_FPB config and FPB address matching mode
	 * specified. Otherwise, Frame Pending bit should be set to ``1`` (see
	 * section 6.7.3).
	 *
	 * @note requires @ref IEEE802154_HW_TX_RX_ACK capability and is
	 * available in any interface operational state.
	 */
	IEEE802154_CONFIG_AUTO_ACK_FPB,

	/**
	 * Indicates whether to set ACK Frame Pending bit for specific address
	 * or not. Disabling the Frame Pending bit with no address provided
	 * (NULL pointer) should disable it for all enabled addresses.
	 *
	 * @note requires @ref IEEE802154_HW_TX_RX_ACK capability and is
	 * available in any interface operational state.
	 */
	IEEE802154_CONFIG_ACK_FPB,

	/**
	 * Indicates whether the device is a PAN coordinator. This influences
	 * packet filtering.
	 *
	 * @note Available in any interface operational state.
	 */
	IEEE802154_CONFIG_PAN_COORDINATOR,

	/**
	 * Enable/disable promiscuous mode.
	 *
	 * @note Available in any interface operational state.
	 */
	IEEE802154_CONFIG_PROMISCUOUS,

	/**
	 * Specifies new IEEE 802.15.4 driver event handler. Specifying NULL as
	 * a handler will disable events notification.
	 *
	 * @note Available in any interface operational state.
	 */
	IEEE802154_CONFIG_EVENT_HANDLER,

	/**
	 * Updates MAC keys, key index and the per-key frame counter for drivers
	 * supporting transmit security offloading, see section 9.5, tables 9-9
	 * and 9-10. The key configuration SHALL NOT be accepted if the frame
	 * counter (in case frame counter per key is true) is not strictly
	 * larger than the current frame counter associated with the same key,
	 * see sections 8.2.2, 9.2.4 g/h) and 9.4.3.
	 *
	 * @note Requires @ref IEEE802154_HW_TX_SEC capability and is available
	 * in any interface operational state.
	 */
	IEEE802154_CONFIG_MAC_KEYS,

	/**
	 * Sets the current MAC frame counter value associated with the
	 * interface for drivers supporting transmit security offloading, see
	 * section 9.5, table 9-8, secFrameCounter.
	 *
	 * @warning The frame counter MUST NOT be accepted if it is not
	 * strictly greater than the current frame counter associated with the
	 * interface, see sections 8.2.2, 9.2.4 g/h) and 9.4.3. Otherwise the
	 * replay protection provided by the frame counter may be compromised.
	 * Drivers SHALL return -EINVAL in case the configured frame counter
	 * does not conform to this requirement.
	 *
	 * @note Requires @ref IEEE802154_HW_TX_SEC capability and is available
	 * in any interface operational state.
	 */
	IEEE802154_CONFIG_FRAME_COUNTER,

	/**
	 * Sets the current MAC frame counter value if the provided value is greater than
	 * the current one.
	 *
	 * @note Requires @ref IEEE802154_HW_TX_SEC capability and is available
	 * in any interface operational state.
	 *
	 * @warning This configuration option does not conform to the
	 * requirements specified in #61227 as it is redundant with @ref
	 * IEEE802154_CONFIG_FRAME_COUNTER, and will therefore be deprecated in
	 * the future.
	 */
	IEEE802154_CONFIG_FRAME_COUNTER_IF_LARGER,

	/**
	 * Set or unset a radio reception window (RX slot). This can be used for
	 * any scheduled reception, e.g.: Zigbee GP device, CSL, TSCH, etc.
	 *
	 * @details The `start` and `duration` parameters of the RX slot are relative to
	 * the network subsystem's local clock.
	 *
	 * If the `start` parameter of the RX slot is negative then any previously
	 * configured RX slot SHALL be canceled immediately. If the `start`
	 * parameter is any value in the past (including zero) then the
	 * configuration call SHALL return a `-ETIME` error code. This corresponds
	 * to the MLME-RX-ENABLE.confirm `PAST_TIME` status, see section 8.2.10.3.
	 *
	 * If the `start` parameter points to a valid value in the future but
	 * the `duration` parameter is zero or negative then the configuration
	 * call SHALL return a `-EINVAL` error code.
	 *
	 * Typically, the next higher layer will not configure a new RX slot until
	 * the current slot has been completed. However, a new RX slot configuration
	 * call SHALL disable the receiver if it is enabled at the time the new call
	 * is issued, cancel the outstanding RX slot, and thereafter perform and
	 * execute the newly configured RX slot.
	 *
	 * RX slots MAY be programmed while the driver is "DOWN". If any future RX
	 * slot is configured when calling `start()` then the interface SHALL be
	 * placed in "UP" state but the receiver SHALL not be started before the
	 * start time of the slot. Past RX slots SHALL be canceled and ignored when
	 * calling `start()`.
	 *
	 * The driver SHALL take care to start/stop the receiver autonomously,
	 * asynchronously and automatically around the RX slot. The driver SHALL
	 * resume power just before the RX slot and suspend it again after the slot
	 * or - if RxAutoOff is `true` - after receiving a packet unless another
	 * programmed event forces the driver not to suspend. The driver SHALL
	 * switch to the programmed channel before the RX slot and back to the
	 * channel set with set_channel() after the RX slot. If the driver interface
	 * is "DOWN" when the start time of an RX slot arrives, then the RX slot
	 * SHALL not be observed and the receiver SHALL remain off.
	 *
	 * If the driver is "UP" while configuring an RX slot, the driver SHALL turn
	 * off the receiver immediately and (possibly asynchronously) put the driver
	 * into the lowest possible power saving mode until the start of the RX
	 * slot. While an RX slot is active, the setting of the `macRxOnWhenIdle`
	 * MAC PIB attribute (@see IEEE802154_CONFIG_RX_ON_WHEN_IDLE) SHALL be
	 * ignored.
	 *
	 * If the driver is "UP" while the RX slot is deleted, then the driver SHALL
	 * enable the receiver immediately unless the `macRxOnWhenIdle` MAC PIB
	 * attribute (@see IEEE802154_CONFIG_RX_ON_WHEN_IDLE) is set to `false`. The
	 * receiver SHALL be ready to receive packets before returning from the
	 * `configure()` operation when the RX slot is deleted and `macRxOnWhenIdle`
	 * is `true`.
	 *
	 * @note requires @ref IEEE802154_HW_RXTIME capability and is available
	 * in any interface operational state.
	 *
	 * @note Required for Thread 1.2 Coordinated Sampled Listening feature
	 * (see Thread specification 1.2.0, ch. 3.2.6.3).
	 */
	IEEE802154_CONFIG_RX_SLOT,

	/**
	 * Enables or disables a device as a CSL receiver and configures its CSL
	 * period.
	 *
	 * @details Configures the CSL period in units of 10 symbol periods.
	 * Values greater than zero enable CSL if the driver supports it and the
	 * device starts to operate as a CSL receiver. Setting this to zero
	 * disables CSL on the device. If the driver does not support CSL, the
	 * configuration call SHALL return -ENOTSUP.
	 *
	 * See section 7.4.2.3 and section 8.4.3.6, table 8-104, macCslPeriod.
	 *
	 * @note Confusingly the standard calls the CSL receiver "CSL
	 * coordinator" (i.e. "coordinating the CSL protocol timing", see
	 * section 6.12.2.2), although, typically, a CSL coordinator is NOT also
	 * an IEEE 802.15.4 FFD coordinator or PAN coordintor but a simple RFD
	 * end device (compare the device roles outlined in sections 5.1, 5.3,
	 * 5.5 and 6.1). To avoid confusion we therefore prefer calling CSL
	 * coordinators (typically an RFD end device) "CSL receivers" and CSL
	 * peer devices (typically FFD coordinators or PAN coordinators) "CSL
	 * transmitters". Also note that at this time, we do NOT support
	 * unsynchronized transmission with CSL wake up frames as specified in
	 * section 6.12.2.4.4.
	 *
	 * To offload CSL receiver timing to the driver the upper layer SHALL
	 * combine several configuration options in the following way:
	 *
	 * 1. Use @ref IEEE802154_CONFIG_ENH_ACK_HEADER_IE once with an
	 *    appropriate pre-filled CSL IE and the CSL phase set to an
	 *    arbitrary value or left uninitialized. The CSL phase SHALL be
	 *    injected on-the-fly by the driver at runtime as outlined in 2.
	 *    below. Adding a short and extended address will inform the driver
	 *    of the specific CSL receiver to which it SHALL inject CSL IEs. If
	 *    no addresses are given then the CSL IE will be injected into all
	 *    enhanced ACK frames as soon as CSL is enabled.  This configuration
	 *    SHALL be done before enabling CSL by setting a CSL period greater
	 *    than zero.
	 *
	 * 2. Configure @ref IEEE802154_CONFIG_EXPECTED_RX_TIME immediately
	 *    followed by @ref IEEE802154_CONFIG_CSL_PERIOD. To prevent race
	 *    conditions, the upper layer SHALL ensure that the receiver is not
	 *    enabled during or between the two calls (e.g. by a previously
	 *    configured RX slot) nor SHALL a frame be transmitted concurrently.
	 *
	 *    The expected RX time SHALL point to the end of SFD of an ideally
	 *    timed RX frame in an arbitrary past or future CSL channel sample,
	 *    i.e.  whose "end of SFD" arrives exactly at the locally predicted
	 *    time inside the CSL channel sample.
	 *
	 *    The driver SHALL derive CSL anchor points and the CSL phase from
	 *    the given expected RX time as follows:
	 *
	 *        cslAnchorPointNs = last expected RX time
	 *                           + PHY-specific PHR duration in ns
	 *
	 *        startOfMhrNs = start of MHR of the frame containing the
	 *                       CSL IE relative to the local network clock
	 *
	 *        cslPhase = (startOfMhrNs - cslAnchorPointNs)
	 *                   / (10 * PHY specific symbol period in ns)
	 *                   % cslPeriod
	 *
	 *    The driver SHALL set the CSL phase in the IE configured in 1.  and
	 *    inject that IE on-the-fly into outgoing enhanced ACK frames if the
	 *    destination address conforms to the IE's address filter.
	 *
	 * 3. Use @ref IEEE802154_CONFIG_RX_SLOT periodically to schedule
	 *    each CSL channel sample early enough before its start time. The
	 *    size of the CSL channel sample SHALL take relative clock drift and
	 *    scheduling uncertainties with respect to CSL transmitters into
	 *    account as specified by the standard such that at least the full
	 *    SHR of a legitimate RX frame is guaranteed to land inside the
	 *    channel sample.
	 *
	 *    To this avail, the last configured expected RX time plus an
	 *    integer number of CSL periods SHALL point to a fixed offset of the
	 *    RX slot (not necessarily its center):
	 *
	 *        expectedRxTimeNs_N = last expected RX time
	 *            + N * (cslPeriod * 10 * PHY-specific symbol period in ns)
	 *
	 *        expectedRxTimeNs_N - rxSlot_N.start == const for all N
	 *
	 *    While the configured CSL period is greater than zero, drivers
	 *    SHOULD validate the offset of the expected RX time inside each RX
	 *    slot accordingly. If the driver finds that the offset varies from
	 *    slot to slot, drivers SHOULD log the difference but SHALL
	 *    nevertheless accept and schedule the RX slot with a zero success
	 *    value to work around minor implementation or rounding errors in
	 *    upper layers.
	 *
	 * Configure and start a CSL receiver:
	 *
	 *     ENH_ACK_HEADER_IE
	 *        |
	 *        | EXPECTED_RX_TIME (end of SFD of a perfectly timed RX frame
	 *        |    |              in any past or future channel sample)
	 *        |    |
	 *        |    | CSL_PERIOD (>0)            RX_SLOT
	 *        |    |    |                          |
	 *        v    v    v                          v
	 *     -----------------------------------------------[-CSL channel sample ]----+
	 *                                         ^                                    |
	 *                                         |                                    |
	 *                                         +--------------------- loop ---------+
	 *
	 * Disable CSL on the receiver:
	 *
	 *     CSL_PERIOD (=0)
	 *        |
	 *        v
	 *     ---------------------
	 *
	 * Update the CSL period to a new value:
	 *
	 *     EXPECTED_RX_TIME (based on updated period)
	 *        |
	 *        |  CSL_PERIOD (>0, updated)       RX_SLOT
	 *        |     |                              |
	 *        v     v                              v
	 *     -----------------------------------------------[-CSL channel sample ]----+
	 *                                         ^                                    |
	 *                                         |                                    |
	 *                                         +--------------------- loop ---------+
	 *
	 * @note Available in any interface operational state.
	 *
	 * @note Required for Thread 1.2 Coordinated Sampled Listening feature
	 * (see Thread specification 1.2.0, ch. 3.2.6.3).
	 */
	IEEE802154_CONFIG_CSL_PERIOD,

	/**
	 * Configure a timepoint at which an RX frame is expected to arrive.
	 *
	 * @details Configure the nanosecond resolution timepoint relative to
	 * the network subsystem's local clock at which an RX frame's end of SFD
	 * (i.e. equivalently its end of SHR, start of PHR, or in the case of
	 * PHYs with RDEV or ERDEV capability the RMARKER) is expected to arrive
	 * at the local antenna assuming perfectly synchronized local and remote
	 * network clocks and zero distance between antennas.
	 *
	 * This parameter MAY be used to offload parts of timing sensitive TDMA
	 * (e.g.  TSCH, beacon-enabled PAN including DSME), low-energy (e.g.
	 * CSL, RIT) or ranging (TDoA) protocols to the driver. In these
	 * protocols, medium access is tightly controlled such that the expected
	 * arrival time of a frame can be predicted within a well-defined time
	 * window. This feature will typically be combined with @ref
	 * IEEE802154_CONFIG_RX_SLOT although this is not a hard requirement.
	 *
	 * The "expected RX time" MAY be interpreted slightly differently
	 * depending on the protocol context:
	 * - CSL phase (i.e. time to the next expected CSL transmission) or anchor
	 *   time (i.e. any arbitrary timepoint with "zero CSL phase") SHALL be
	 *   derived by adding the PHY header duration to the expected RX time
	 *   to calculate the "start of MHR" ("first symbol of MAC", see section
	 *   6.12.2.1) required by the CSL protocol, compare @ref
	 *   IEEE802154_CONFIG_CSL_PERIOD.
	 * - In TSCH the expected RX time MAY be set to macTsRxOffset +
	 *   macTsRxWait / 2. Then the time correction SHALL be calculated as
	 *   the expected RX time minus actual arrival timestamp, see section
	 *   6.5.4.3.
	 * - In ranging applications, time difference of arrival (TDOA) MAY be
	 *   calculated inside the driver comparing actual RMARKER timestamps
	 *   against the assumed synchronized time at which the ranging frame
	 *   was sent, see IEEE 802.15.4z.
	 *
	 * In case of periodic protocols (e.g. CSL channel samples, periodic
	 * beacons of a single PAN, periodic ranging "blinks"), a single
	 * timestamp at any time in the past or in the future may be given from
	 * which other expected timestamps can be derived by adding or
	 * subtracting multiples of the RX period. See e.g. the CSL
	 * documentation in this API.
	 *
	 * Additionally this parameter MAY be used by drivers to discipline
	 * their local representation of a distributed network clock by deriving
	 * synchronization instants related to a remote representation of the
	 * same clock (as in PTP).
	 *
	 * @note Available in any interface operational state.
	 *
	 * @note Required for Thread 1.2 Coordinated Sampled Listening feature
	 * (see Thread specification 1.2.0, ch. 3.2.6.3).
	 */
	IEEE802154_CONFIG_EXPECTED_RX_TIME,

	/**
	 * Adds a header information element (IE) to be injected into enhanced
	 * ACK frames generated by the driver if the given destination address
	 * filter matches.
	 *
	 * @details Drivers implementing the @ref IEEE802154_HW_RX_TX_ACK
	 * capability generate ACK frames autonomously. Setting this
	 * configuration will ask the driver to inject the given preconfigured
	 * header IE when generating enhanced ACK frames where appropriate by
	 * the standard. IEs for all other frame types SHALL be provided by L2.
	 *
	 * The driver shall return -ENOTSUP in the following cases:
	 * - It does not support the @ref IEEE802154_HW_RX_TX_ACK,
	 * - It does not support header IE injection,
	 * - It cannot inject the runtime fields on-the-fly required for the
	 *   given IE element ID (see list below).
	 *
	 * Enhanced ACK header IEs (element IDs in parentheses) that either
	 * need to be rejected or explicitly supported and parsed by the driver
	 * because they require on-the-fly timing information injection are:
	 * - CSL IE (0x1a)
	 * - Rendezvous Time IE (0x1d)
	 * - Time Correction IE (0x1e)
	 *
	 * Drivers accepting this configuration option SHALL check the list of
	 * configured IEs for each outgoing enhanced ACK frame, select the ones
	 * appropriate for the received frame based on their element ID, inject
	 * any required runtime information on-the-fly and include the selected
	 * IEs into the enhanced ACK frame's MAC header.
	 *
	 * Drivers supporting enhanced ACK header IE injection SHALL
	 * autonomously inject header termination IEs as required by the
	 * standard.
	 *
	 * A destination short address and extended address MAY be given by L2
	 * to filter the devices to which the given IE is included. Setting the
	 * short address to the broadcast address and the extended address to
	 * NULL will inject the given IE into all ACK frames unless a more
	 * specific filter is also present for any given destination device
	 * (fallback configuration). L2 SHALL take care to either set both
	 * address fields to valid device addresses or none.
	 *
	 * This configuration type may be called several times with distinct
	 * element IDs and/or addresses. The driver SHALL either store all
	 * configured IE/address combinations or return -ENOMEM if no
	 * additional configuration can be stored.
	 *
	 * Configuring a header IE with a previously configured element ID and
	 * address filter SHALL override the previous configuration. This
	 * implies that repetition of the same header IE/address combination is
	 * NOT supported.
	 *
	 * Configuring an existing element ID/address filter combination with
	 * the header IE's length field set to zero SHALL remove that
	 * configuration. SHALL remove the fallback configuration if no address
	 * is given.
	 *
	 * Configuring a header IE for an address filter with the header IE
	 * pointer set to NULL SHALL remove all header IE's for that address
	 * filter. SHALL remove ALL header IE configuration (including but not
	 * limited to fallbacks) if no address is given.
	 *
	 * If any of the deleted configurations didn't previously exist, then
	 * the call SHALL be ignored. Whenever the length field is set to zero,
	 * the content fields MUST NOT be accessed by the driver.
	 *
	 * L2 SHALL minimize the space required to keep IE configuration inside
	 * the driver by consolidating address filters and by removing
	 * configuration that is no longer required.
	 *
	 * @note requires @ref IEEE802154_HW_RX_TX_ACK capability and is
	 * available in any interface operational state. Currently we only
	 * support header IEs but that may change in the future.
	 *
	 * @note Required for Thread 1.2 Coordinated Sampled Listening feature
	 * (see Thread specification 1.2.0, ch. 3.2.6.3).
	 *
	 * @note Required for Thread 1.2 Link Metrics feature (see Thread
	 * specification 1.2.0, ch. 4.11.3.3).
	 */
	IEEE802154_CONFIG_ENH_ACK_HEADER_IE,

	/**
	 * Configure the `macRxOnWhenIdle` MAC PIB attribute (section 8.4.3.1, table
	 * 8-94).
	 *
	 * @details Since there is no clear guidance in the IEEE 802.15.4
	 * specification about the definition of an "idle period", drivers
	 * implementing this API are expected to use the `macRxOnWhenIdle` attribute
	 * to determine the next radio state (false --> rx off, true --> rx on) in
	 * the following situations:
	 * - After finalization of regular frame reception, provided that:
	 *   - The frame is received without errors, passes the filtering and is not
	 *     a spurious ACK.
	 *   - ACK is not requested or transmission of an ACK frame is not possible
	 *     due to internal conditions.
	 * - After finalization of frame transmission, when ACK is not requested in
	 *   the transmitted frame or after an ACK frame was transmitted.
	 * - After finalization of the ACK procedure on the reception side due to:
	 *   - ACK timeout.
	 *   - Reception of an invalid ACK or non-ACK frame.
	 *   - Reception of a valid ACK frame, unless the transmitted frame was a
	 *     Data Request Command and the frame pending bit on the received ACK is
	 *     set to true. In this case the radio driver implementation SHOULD keep
	 *     the receiver on until a determined timeout which triggers an idle
	 *     period start.
	 * - Finalization of a stand alone CCA task.
	 * - Finalization of a CCA operation with busy result during CSMA/CA procedure.
	 * - Finalization of an Energy Detection task.
	 * - Finalization of a scheduled radio reception window (see
	 *   @ref IEEE802154_CONFIG_RX_SLOT).
	 *
	 * @note By default `macRxOnWhenIdle` is assumed to be true to match legacy
	 * behavior of drivers that do not implement this setting.
	 */
	IEEE802154_CONFIG_RX_ON_WHEN_IDLE,

	/** Number of types defined in ieee802154_config_type. */
	IEEE802154_CONFIG_COMMON_COUNT,

	/** This and higher values are specific to the protocol- or driver-specific extensions. */
	IEEE802154_CONFIG_PRIV_START = IEEE802154_CONFIG_COMMON_COUNT,
};

/**
 * Configuring an RX slot with the start parameter set to this value will cancel
 * and delete any previously configured RX slot.
 */
#define IEEE802154_CONFIG_RX_SLOT_NONE -1LL

/**
 * Configuring an RX slot with this start parameter while the driver is "down",
 * will keep RX off when the driver is being started. Configuring an RX slot
 * with this start value while the driver is "up" will immediately switch RX off
 * until either the slot is deleted, see @ref IEEE802154_CONFIG_RX_SLOT_NONE or
 * a slot with a future start parameter is configured and that start time
 * arrives.
 */
#define IEEE802154_CONFIG_RX_SLOT_OFF  0LL

/** IEEE 802.15.4 driver configuration data. */
struct ieee802154_config {
	/** Configuration data. */
	union {
		/** see @ref IEEE802154_CONFIG_AUTO_ACK_FPB */
		struct {
			bool enabled;                  /**< Is auto ACK FPB enabled */
			enum ieee802154_fpb_mode mode; /**< Auto ACK FPB mode */
		} auto_ack_fpb;

		/** see @ref IEEE802154_CONFIG_ACK_FPB */
		struct {
			uint8_t *addr; /**< little endian for both short and extended address */
			bool extended; /**< Is extended address */
			bool enabled;  /**< Is enabled */
		} ack_fpb;

		/** see @ref IEEE802154_CONFIG_PAN_COORDINATOR */
		bool pan_coordinator;

		/** see @ref IEEE802154_CONFIG_PROMISCUOUS */
		bool promiscuous;

		/** see @ref IEEE802154_CONFIG_RX_ON_WHEN_IDLE */
		bool rx_on_when_idle;

		/** see @ref IEEE802154_CONFIG_EVENT_HANDLER */
		ieee802154_event_cb_t event_handler;

		/**
		 * @brief see @ref IEEE802154_CONFIG_MAC_KEYS
		 *
		 * @details Pointer to an array containing a list of keys used
		 * for MAC encryption. Refer to secKeyIdLookupDescriptor and
		 * secKeyDescriptor in IEEE 802.15.4
		 *
		 * The key_value field points to a buffer containing the 16 byte
		 * key. The buffer SHALL be copied by the driver before
		 * returning from the call.
		 *
		 * The variable length array is terminated by key_value field
		 * set to NULL.
		 */
		struct ieee802154_key *mac_keys;

		/** see @ref IEEE802154_CONFIG_FRAME_COUNTER */
		uint32_t frame_counter;

		/** see @ref IEEE802154_CONFIG_RX_SLOT */
		struct {
			/**
			 * Nanosecond resolution timestamp relative to the
			 * network subsystem's local clock defining the start of
			 * the RX window during which the receiver is expected
			 * to be listening (i.e. not including any driver
			 * startup times).
			 *
			 * Configuring an RX slot with the start attribute
			 * set to a negative value will cancel and delete any
			 * previously active RX slot.
			 */
			net_time_t start;

			/**
			 * Nanosecond resolution duration of the RX window
			 * relative to the above RX window start time during
			 * which the receiver is expected to be listening (i.e.
			 * not including any shutdown times). Only positive
			 * values larger than or equal zero are allowed.
			 *
			 * Setting the duration less than zero when the start
			 * parameter greater or equal zero is not allowed.
			 * The configuration call SHALL return `-EINVAL` in
			 * this case.
			 *
			 * When the `start` parameter is set to a negative
			 * value, then `duration` will be ignored.
			 */
			net_time_t duration;

			/**
			 * Used channel
			 *
			 * When the `start` parameter is set to a negative
			 * value, then the `channel` parameter will be
			 * ignored.
			 */
			uint8_t channel;

			/**
			 * Implements MLME `RxAutoOff` (see IEEE 802.15.4z,
			 * section 8.2.10.2, table 8-27) for a single
			 * `start`/`duration` tuple.
			 *
			 * If the driver doesn't support `RxAutoOff`, then
			 * the parameter SHALL be silently ignored and the
			 * receiver is kept on for the duration of the full
			 * RX slot.
			 *
			 * When `RxAutoOff` is `true`, the receiver is disabled
			 * immediately after the reception of a packet, otherwise
			 * it remains enabled for the specified `duration` even
			 * after the reception.
			 *
			 * When the `start` parameter is set to a negative
			 * value, then `rx_auto_off` will be ignored.
			 */
			bool rx_auto_off;
		} rx_slot;

		/**
		 * see @ref IEEE802154_CONFIG_CSL_PERIOD
		 *
		 * in CPU byte order
		 */
		uint32_t csl_period;

		/**
		 * see @ref IEEE802154_CONFIG_EXPECTED_RX_TIME
		 */
		net_time_t expected_rx_time;

		/** see @ref IEEE802154_CONFIG_ENH_ACK_HEADER_IE */
		struct {
			/**
			 * Pointer to the header IE, see section 7.4.2.1,
			 * figure 7-21
			 *
			 * Certain header IEs may be incomplete if they require
			 * timing information to be injected at runtime
			 * on-the-fly, see the list in @ref
			 * IEEE802154_CONFIG_ENH_ACK_HEADER_IE.
			 */
			struct ieee802154_header_ie *header_ie;

			/**
			 * Filters the devices that will receive this IE by
			 * extended address. MAY be set to NULL to configure a
			 * fallback for all devices (implies that short_addr
			 * MUST also be set to @ref
			 * IEEE802154_BROADCAST_ADDRESS).
			 *
			 * in big endian
			 */
			const uint8_t *ext_addr;

			/**
			 * Filters the devices that will receive this IE by
			 * short address. MAY be set to @ref
			 * IEEE802154_BROADCAST_ADDRESS to configure a fallback
			 * for all devices (implies that ext_addr MUST also set
			 * to NULL in this case).
			 *
			 * in CPU byte order
			 */
			uint16_t short_addr;

			/**
			 * Flag for purging enh ACK header IEs.
			 *
			 * When this flag is set to true, the driver SHALL remove
			 * all existing ACK header IEs. All other entries in the
			 * ack_ie config SHALL be ignored.
			 *
			 * This means that purging current header IEs and
			 * configuring a new one in the same call is not allowed.
			 */
			bool purge_ie;
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
 * memory area, SHALL be provided pointing to static memory allocated by the
 * driver and valid throughout the lifetime of the driver instance.
 */
struct ieee802154_attr_value {
	union {
		/* TODO: Implement configuration of phyCurrentPage once drivers
		 * need to support channel page switching at runtime.
		 */
		/**
		 * @brief A bit field that represents the supported channel
		 * pages, see @ref ieee802154_phy_channel_page.
		 *
		 * @note To keep the API extensible as required by the standard,
		 * supported pages are modeled as a bitmap to support drivers
		 * that implement runtime switching between multiple channel
		 * pages.
		 *
		 * @note Currently none of the Zephyr drivers implements more
		 * than one channel page at runtime, therefore only one bit will
		 * be set and the current channel page (see the PHY PIB
		 * attribute phyCurrentPage, section 11.3, table 11-2) is
		 * considered to be read-only, fixed and "well known" via the
		 * supported channel pages attribute.
		 */
		uint32_t phy_supported_channel_pages;

		/**
		 * @brief Pointer to a structure representing channel ranges
		 * currently available on the selected channel page.
		 *
		 * @warning The pointer must be valid and constant throughout
		 * the life of the interface.
		 *
		 * @details The selected channel page corresponds to the
		 * phyCurrentPage PHY PIB attribute, see the description of
		 * phy_supported_channel_pages above. Currently it can be
		 * retrieved via the @ref
		 * IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES attribute.
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

		/* TODO: Allow the PRF to be configured for each TX call once
		 * drivers need to support PRF switching at runtime.
		 */
		/**
		 * @brief A bit field representing supported HRP UWB pulse
		 * repetition frequencies (PRF), see enum
		 * ieee802154_phy_hrp_uwb_nominal_prf.
		 *
		 * @note Currently none of the Zephyr HRP UWB drivers implements
		 * more than one nominal PRF at runtime, therefore only one bit
		 * will be set and the current PRF (UwbPrf, MCPS-DATA.request,
		 * section 8.3.2, table 8-88) is considered to be read-only,
		 * fixed and "well known" via the supported PRF attribute.
		 */
		uint32_t phy_hrp_uwb_supported_nominal_prfs;
	};
};

/**
 * @brief Helper function to handle channel page and range to be called from
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
 * @details While L1-level driver features are exclusively implemented by
 * drivers and MAY be mandatory to support certain application requirements, L2
 * features SHOULD be optional by default and only need to be implemented for
 * performance optimization or precise timing as deemed necessary by driver
 * maintainers. Fallback implementations ("Soft MAC") SHOULD be provided in the
 * driver-independent L2 layer for all L2/MAC features especially if these
 * features are not implemented in vendor hardware/firmware by a majority of
 * existing in-tree drivers. If, however, a driver offers offloading
 * opportunities then L2 implementations SHALL delegate performance critical or
 * resource intensive tasks to the driver.
 *
 * All drivers SHALL support two externally observable interface operational
 * states: "UP" and "DOWN". Drivers MAY additionally support a "TESTING"
 * interface state (see `continuous_carrier()`).
 *
 * The following rules apply:
 * * An interface is considered "UP" when it is able to transmit and receive
 *   packets, "DOWN" otherwise (see precise definitions of the corresponding
 *   ifOperStatus values in RFC 2863, section 3.1.14, @ref net_if_oper_state and
 *   the `continuous_carrier()` exception below). A device that has its receiver
 *   temporarily disabled during "UP" state due to an active receive window
 *   configuration or an `rx()` call that switched off the receiver is still
 *   considered "UP".
 * * Upper layers will assume that the interface managed by the driver is "UP"
 *   after a call to `start()` returned zero or `-EALREADY`. Upper layers assume
 *   that the interface is "DOWN" after calling `stop()` returned zero or
 *   `-EALREADY`.
 * * The driver SHALL block `start()`/`stop()` calls until the interface fully
 *   transitioned to the new state (e.g. the receiver is operational, ongoing
 *   transmissions were finished, etc.). Drivers SHOULD yield the calling thread
 *   (i.e. "sleep") if waiting for the new state without CPU interaction is
 *   possible.
 * * Drivers are responsible of guaranteeing atomicity of state changes.
 *   Appropriate means of synchronization SHALL be implemented (locking, atomic
 *   flags, ...).
 * * While the interface is "DOWN", the driver SHALL be placed in the lowest
 *   possible power state. The driver MAY return from a call to `stop()` before
 *   it reaches the lowest possible power state, i.e. manage power
 *   asynchronously.  While the interface is "UP", the driver SHOULD
 *   autonomously and asynchronously transition to lower power states whenever
 *   possible. If the driver claims to support timed RX/TX capabilities and the
 *   upper layers configure an RX slot, then the driver SHALL immediately
 *   transition (asynchronously) to the lowest possible power state until the
 *   start of the RX slot or until a scheduled packet needs to be transmitted.
 *   The same is true when the driver claims to support `macRxOnWhenIdle`
 *   offloading and the receiver becomes "idle".
 * * The driver SHALL NOT change the interface's "UP"/"DOWN" state on its own.
 *   Initially, the interface SHALL be in the "DOWN" state.
 * * Drivers that implement the optional `continuous_carrier()` operation will
 *   be considered to be in the RFC 2863 "testing" ifOperStatus state if that
 *   operation returns zero. This state is active until either `start()` or
 *   `stop()` is called. If `continuous_carrier()` returns a non-zero value then
 *   the previous state is assumed by upper layers.
 * * If calls to `start()`/`stop()` return any other value than zero or
 *   `-EALREADY`, upper layers will consider the interface to be in a
 *   "lowerLayerDown" state as defined in RFC 2863.
 * * The RFC 2863 "dormant", "unknown" and "notPresent" ifOperStatus states are
 *   currently not supported. The "lowerLevelUp" state.
 * * The `ed_scan()`, `cca()` and `tx()` operations SHALL only be supported in
 *   the "UP" state and return `-ENETDOWN` in any other state. See the
 *   function-level API documentation below for further details.
 *
 * @note In case of devices that support timed RX/TX, `macRxOnWhenIdle` or
 * @ref IEEE802154_RX_OFF, the "UP" state is not equal to "receiver enabled". If
 * a receive window (i.e. RX slot, see @ref IEEE802154_CONFIG_RX_SLOT) is
 * configured before calling `start()` then the receiver will not be enabled
 * when transitioning to the "UP" state. Configuring a receive window while the
 * interface is "UP" will cause the receiver to be disabled immediately until
 * the configured reception time has arrived.
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
	 * @note Implementations SHALL be **isr-ok** and MUST NOT **sleep**. MAY
	 * be called in any interface state once the driver is fully initialized
	 * ("ready").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @return Bit field with all supported device driver capabilities.
	 */
	enum ieee802154_hw_caps (*get_capabilities)(const struct device *dev);

	/**
	 * @brief Clear Channel Assessment - Check channel's activity
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. SHALL
	 * return -ENETDOWN unless the interface is "UP".
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 the channel is available
	 * @retval -EBUSY The channel is busy.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
	 * @retval -ENETDOWN The interface is not "UP".
	 * @retval -ENOTSUP CCA is not supported by this driver.
	 * @retval -EIO The CCA procedure could not be executed.
	 */
	int (*cca)(const struct device *dev);

	/**
	 * @brief Set current channel
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. SHALL
	 * return -EIO unless the interface is either "UP" or "DOWN".
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param channel the number of the channel to be set in CPU byte order
	 *
	 * @retval 0 channel was successfully set
	 * @retval -EALREADY The previous channel is the same as the requested
	 * channel.
	 * @retval -EINVAL The given channel is not within the range of valid
	 * channels of the driver's current channel page, see the
	 * IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES driver attribute.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
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
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. SHALL
	 * return -EIO unless the interface is either "UP" or "DOWN".
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
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
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
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. SHALL
	 * return -EIO unless the interface is either "UP" or "DOWN".
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param dbm TX power in dbm
	 *
	 * @retval 0 The TX power was successfully set.
	 * @retval -EINVAL The given dbm value is invalid or not supported by
	 * the driver.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
	 * @retval -EIO The TX power could not be set.
	 */
	int (*set_txpower)(const struct device *dev, int16_t dbm);

	/**
	 * @brief Transmit a packet fragment as a single frame
	 *
	 * @details Depending on the level of offloading features supported by
	 * the driver, the frame MAY not be fully encrypted/authenticated or it
	 * MAY not contain an FCS. It is the responsibility of L2
	 * implementations to prepare the frame according to the offloading
	 * capabilities announced by the driver and to decide whether CCA,
	 * CSMA/CA, ACK or retransmission procedures need to be executed outside
	 * ("soft MAC") or inside ("hard MAC") the driver .
	 *
	 * All frames originating from L2 SHALL have all required IEs
	 * pre-allocated and pre-filled such that the driver does not have to
	 * parse and manipulate IEs at all. This includes ACK packets if the
	 * driver does not have the @ref IEEE802154_HW_RX_TX_ACK capability.
	 * Also see @ref IEEE802154_CONFIG_ENH_ACK_HEADER_IE for drivers that
	 * have the @ref IEEE802154_HW_RX_TX_ACK capability.
	 *
	 * IEs that cannot be prepared by L2 unless the TX time is known (e.g.
	 * CSL IE, Rendezvous Time IE, Time Correction IE, ...) SHALL be sent in
	 * any of the timed TX modes with appropriate timing information
	 * pre-filled in the IE such that drivers do not have to parse and
	 * manipulate IEs at all unless the frame is generated by the driver
	 * itself.
	 *
	 * In case any of the timed TX modes is supported and used (see @ref
	 * ieee802154_hw_caps and @ref ieee802154_tx_mode), the driver SHALL
	 * take responsibility of scheduling and sending the packet at the
	 * precise programmed time autonomously without further interaction by
	 * upper layers. The call to `tx()` will block until the package has
	 * either been sent successfully (possibly including channel acquisition
	 * and packet acknowledgment) or a terminal transmission error occurred.
	 * The driver SHALL sleep and keep power consumption to the lowest
	 * possible level until the scheduled transmission time arrives or
	 * during any other idle waiting time.
	 *
	 * @warning The driver SHALL NOT take ownership of the given network
	 * packet and frame (fragment) buffer. Any data required by the driver
	 * including the actual frame content must be read synchronously and
	 * copied internally if needed at a later time (e.g. the contents of IEs
	 * required for protocol configuration, states of frame counters,
	 * sequence numbers, etc). Both, the packet and the buffer MAY be
	 * re-used or released by upper layers immediately after the function
	 * returns.
	 *
	 * @note Implementations MAY **sleep** and will usually NOT be
	 * **isr-ok** - especially when timed TX, CSMA/CA, retransmissions,
	 * auto-ACK or any other offloading feature is supported that implies
	 * considerable idle waiting time. SHALL return `-ENETDOWN` unless the
	 * interface is "UP".
	 *
	 * @note The transmission occurs on the radio channel set by the call to
	 * `set_channel()`. However, if the `CONFIG_IEEE802154_SELECTIVE_TXCHANNEL`
	 * is set and the driver has the capability `IEEE802154_HW_SELECTIVE_TXCHANNEL`
	 * then the transmissions requested with `mode` IEEE802154_TX_MODE_TXTIME
	 * or `IEEE802154_TX_MODE_TXTIME_CCA` SHALL use the radio channel
	 * returned by `net_pkt_ieee802154_txchannel()` to transmit the packet
	 * and receive an ACK on that channel if the frame requested it. After
	 * the operation the driver should return to the channel set previously by
	 * `set_channel()` call.
	 * It is responsibility of an upper layer to set the required radio channel
	 * for the packet by a call to `net_pkt_set_ieee802154_txchannel()`.
	 * This feature allows CSL transmissions as stated in IEEE 802.15.4-2020
	 * chapter 6.12.2.7 CSL over multiple channels. This feature allows to perform
	 * a switch of the radio channel as late as possible before transmission without
	 * interrupting possible reception that could occur if separate `set_channel()`
	 * was called.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param mode the transmission mode, some of which require specific
	 * offloading capabilities.
	 * @param pkt pointer to the network packet to be transmitted.
	 * @param frag pointer to a network buffer containing a single fragment
	 * with the frame data to be transmitted
	 *
	 * @retval 0 The frame was successfully sent or scheduled. If the driver
	 * supports ACK offloading and the frame requested acknowledgment (AR bit
	 * set), this means that the packet was successfully acknowledged by its
	 * peer.
	 * @retval -EINVAL Invalid packet (e.g. an expected IE is missing or the
	 * encryption/authentication state is not as expected).
	 * @retval -EBUSY The frame could not be sent because the medium was
	 * busy (CSMA/CA or CCA offloading feature only).
	 * @retval -ENOMSG The frame was not confirmed by an ACK packet (TX ACK
	 * offloading feature only) or the received ACK packet was invalid.
	 * @retval -ENOBUFS The frame could not be scheduled due to missing
	 * internal resources (timed TX offloading feature only).
	 * @retval -ENETDOWN The interface is not "UP".
	 * @retval -ENOTSUP The given TX mode is not supported.
	 * @retval -EIO The frame could not be sent due to some unspecified
	 * driver error (e.g. the driver being busy).
	 */
	int (*tx)(const struct device *dev, enum ieee802154_tx_mode mode,
		  struct net_pkt *pkt, struct net_buf *frag);

	/**
	 * @brief Sets the next RX mode, e.g. receiver on/off, wake-on-radio, etc.
	 *
	 * @details When called with @ref IEEE802154_RX_OFF while "UP" will switch
	 * the receiver off immediately while leaving the interface "UP". Calling
	 * with @ref IEEE802154_RX_ON while "UP" will switch the receiver back on
	 * immediately. When "UP" the driver SHALL block until the receiver has
	 * fully reached the given mode, e.g. until the receiver is able to receive
	 * a packet when switching the receiver back on.
	 *
	 * When called while "DOWN" will program the receiver mode to be reached as
	 * soon as the `start()` operation is called.
	 *
	 * Additional non-standard modes may be defined by individual drivers or
	 * protocols, see @ref ieee802154_rx_mode for details.
	 *
	 * @note Implementations SHALL be **isr-ok**. SHOULD NOT **sleep** when "UP"
	 * and SHALL NOT **sleep** when "DOWN". SHALL return -EIO unless the
	 * interface is either "UP" or "DOWN".
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param mode the receiver mode to be set
	 *
	 * @retval 0 The receiver state was successfully changed.
	 * @retval -EALREADY The receiver was already in the requested state.
	 * @retval -EBUSY The receiver currently receives a packet, waits for ACK or
	 * serves a programmed RX slot.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but cannot
	 * be executed without sleeping.
	 * @retval -ENOTSUP The given RX mode is not supported by the driver.
	 * @retval -EIO will be returned when the receiver state cannot be changed:
	 * * The interface is neither "UP" nor "DOWN".
	 * * The `macRxOnWhenIdle` setting is configured to `false` and manages
	 *   receiver state autonomously.
	 * * Some other unspecified driver error occurred.
	 */
	int (*rx)(const struct device *dev, enum ieee802154_rx_mode mode);

	/**
	 * @brief Start the device (interface "UP").
	 *
	 * @details Upper layers will assume the interface is "UP" if this
	 * operation returns with zero or `-EALREADY`. The interface is placed
	 * in receive mode before returning from this operation unless an RX
	 * slot has been configured (even if it lies in the past, see @ref
	 * IEEE802154_CONFIG_RX_SLOT) or if the `macRxOnWhenIdle` setting has been
	 * configured to "false".
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. MAY be
	 * called in any interface state once the driver is fully initialized
	 * ("ready").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 The driver was successfully started.
	 * @retval -EALREADY The driver was already "UP".
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
	 * @retval -EIO The driver could not be started.
	 */
	int (*start)(const struct device *dev);

	/**
	 * @brief Stop the device (interface "DOWN").
	 *
	 * @details Upper layers will assume the interface is "DOWN" if this
	 * operation returns with zero or `-EALREADY`. The driver switches off
	 * the receiver before returning if it was previously on. The driver
	 * enters the lowest possible power mode after this operation is called.
	 * This MAY happen asynchronously (i.e. after the operation already
	 * returned control).
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. MAY be
	 * called in any interface state once the driver is fully initialized
	 * ("ready").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 The driver was successfully stopped.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
	 * @retval -EALREADY The driver was already "DOWN".
	 * @retval -EIO The driver could not be stopped.
	 */
	int (*stop)(const struct device *dev);

	/**
	 * @brief Start continuous carrier wave transmission ("TESTING" state).
	 *
	 * @details The method blocks until the interface has started to emit a
	 * continuous carrier. Upper layers will assume that the interface is in
	 * "TESTING" state if the operation returns with zero or `-EALREADY`. To
	 * leave this mode, `start()` or `stop()` should be called, which will put
	 * the driver back into the "UP" or "DOWN" states, respectively.
	 *
	 * @note Implementations MAY **sleep** and will usually NOT be
	 * **isr-ok**. MAY be called in any interface state once the driver is
	 * fully initialized ("ready").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @retval 0 continuous carrier wave transmission started
	 * @retval -EALREADY The driver was already in "TESTING" state and
	 * emitting a continuous carrier.
	 * @retval -EIO not started
	 */
	int (*continuous_carrier)(const struct device *dev);

	/**
	 * @brief Set or update driver configuration.
	 *
	 * @details The method blocks until the interface has been reconfigured
	 * atomically with respect to ongoing package reception, transmission or
	 * any other ongoing driver operation.
	 *
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. MAY be
	 * called in any interface state once the driver is fully initialized
	 * ("ready"). Some configuration options may not be supported in all
	 * interface operational states, see the detailed specifications in @ref
	 * ieee802154_config_type. In this case the operation returns `-EACCES`.
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 * @param type the configuration type to be set
	 * @param config the configuration parameters to be set for the given
	 * configuration type
	 *
	 * @retval 0 configuration successful
	 * @retval -EINVAL The configuration parameters are invalid for the
	 * given configuration type.
	 * @retval -ENOTSUP The given configuration type is not supported by
	 * this driver.
	 * @retval -EACCES The given configuration type is supported by this
	 * driver but cannot be configured in the current interface operational
	 * state.
	 * @retval -ENOMEM The configuration cannot be saved due to missing
	 * memory resources.
	 * @retval -ENOENT The resource referenced in the configuration
	 * parameters cannot be found in the configuration.
	 * @retval -EWOULDBLOCK The operation is called from ISR context but
	 * temporarily cannot be executed without blocking.
	 * @retval -EIO An internal error occurred while trying to configure the
	 * given configuration parameter.
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
	 * @note Implementations SHALL be **isr-ok** and MAY **sleep**. SHALL
	 * return `-ENETDOWN` unless the interface is "UP".
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
	 * @retval -ENETDOWN The interface is not "UP".
	 * @retval -ENOTSUP This driver does not support energy scans.
	 * @retval -EIO The energy detection procedure could not be executed.
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
	 * capabilities. Implementations SHALL be **isr-ok** and MUST NOT
	 * **sleep**. MAY be called in any interface state once the driver is
	 * fully initialized ("ready").
	 *
	 * @param dev pointer to IEEE 802.15.4 driver device
	 *
	 * @return nanoseconds relative to the network subsystem's local clock,
	 * -1 if an error occurred or the operation is not supported
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
	 * operating conditions (e.g. temperature). Implementations SHALL be
	 * **isr-ok** and MUST NOT **sleep**. MAY be called in any interface
	 * state once the driver is fully initialized ("ready").
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
	 * @note Implementations SHALL be **isr-ok** and MUST NOT **sleep**. MAY
	 * be called in any interface state once the driver is fully initialized
	 * ("ready").
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

/** @} */

/**
 * @name IEEE 802.15.4 driver utils
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define IEEE802154_AR_FLAG_SET (0x20)
/** INTERNAL_HIDDEN @endcond */

/**
 * @brief Check if the AR flag is set on the frame inside the given @ref
 * net_pkt.
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

/** @} */

/**
 * @name IEEE 802.15.4 driver callbacks
 * @{
 */

/* TODO: Fix drivers to either unref the packet before they return NET_OK or to
 * return NET_CONTINUE instead. See note below.
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
 *          Note: This function is part of Zephyr's 802.15.4 stack driver -> L2
 *          "inversion-of-control" adaptation API and must be implemented by all
 *          IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface that received the packet
 * @param pkt A valid pointer on a packet to check
 *
 * @return NET_OK if L2 handles the ACK package, NET_CONTINUE or NET_DROP otherwise.
 *
 * @warning Deviating from other functions in the net stack returning
 * net_verdict, this function will not unref the package even if it returns
 * NET_OK.
 */
extern enum net_verdict ieee802154_handle_ack(struct net_if *iface, struct net_pkt *pkt);

/**
 * @brief IEEE 802.15.4 driver initialization callback into L2 called by drivers
 *        to initialize the active L2 stack for a given interface.
 *
 * @details Drivers must call this function as part of their own initialization
 *          routine.
 *
 *          Note: This function is part of Zephyr's 802.15.4 stack driver -> L2
 *          "inversion-of-control" adaptation API and must be implemented by all
 *          IEEE 802.15.4 L2 stacks.
 *
 * @param iface A valid pointer on a network interface
 */
#ifndef CONFIG_IEEE802154_RAW_MODE
extern void ieee802154_init(struct net_if *iface);
#else
#define ieee802154_init(_iface_)
#endif /* CONFIG_IEEE802154_RAW_MODE */

/** @} */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_IEEE802154_RADIO_H_ */
