/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 internal MAC and PHY Utils
 *
 * All references to the standard in this file cite IEEE 802.15.4-2020.
 */

#ifndef __IEEE802154_UTILS_H__
#define __IEEE802154_UTILS_H__

#include <zephyr/net/ieee802154_radio.h>
#include <zephyr/sys/util_macro.h>

/**
 * PHY utilities
 */

static inline enum ieee802154_hw_caps ieee802154_radio_get_hw_capabilities(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return 0;
	}

	return radio->get_capabilities(net_if_get_device(iface));
}

static inline int ieee802154_radio_cca(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->cca(net_if_get_device(iface));
}

static inline int ieee802154_radio_set_channel(struct net_if *iface, uint16_t channel)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->set_channel(net_if_get_device(iface), channel);
}

static inline int ieee802154_radio_set_tx_power(struct net_if *iface, int16_t dbm)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->set_txpower(net_if_get_device(iface), dbm);
}

static inline int ieee802154_radio_tx(struct net_if *iface, enum ieee802154_tx_mode mode,
				      struct net_pkt *pkt, struct net_buf *buf)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->tx(net_if_get_device(iface), mode, pkt, buf);
}

static inline int ieee802154_radio_start(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->start(net_if_get_device(iface));
}

static inline int ieee802154_radio_stop(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->stop(net_if_get_device(iface));
}

static inline int ieee802154_radio_attr_get(struct net_if *iface,
					    enum ieee802154_attr attr,
					    struct ieee802154_attr_value *value)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio || !radio->attr_get) {
		return -ENOENT;
	}

	return radio->attr_get(net_if_get_device(iface), attr, value);
}

/**
 * Sets the radio drivers extended address filter.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 * @param ieee_addr Pointer to an extended address in little endian byte order
 */
static inline void ieee802154_radio_filter_ieee_addr(struct net_if *iface, uint8_t *ieee_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.ieee_addr = ieee_addr;

		if (radio->filter(net_if_get_device(iface), true,
				  IEEE802154_FILTER_TYPE_IEEE_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not apply IEEE address filter");
		}
	}
}

static inline void ieee802154_radio_filter_short_addr(struct net_if *iface, uint16_t short_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.short_addr = short_addr;

		if (radio->filter(net_if_get_device(iface), true,
				  IEEE802154_FILTER_TYPE_SHORT_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not apply short address filter");
		}
	}
}

static inline void ieee802154_radio_filter_pan_id(struct net_if *iface, uint16_t pan_id)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.pan_id = pan_id;

		if (radio->filter(net_if_get_device(iface), true,
				  IEEE802154_FILTER_TYPE_PAN_ID,
				  &filter) != 0) {
			NET_WARN("Could not apply PAN ID filter");
		}
	}
}

static inline void ieee802154_radio_filter_src_ieee_addr(struct net_if *iface, uint8_t *ieee_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.ieee_addr = ieee_addr;

		if (radio->filter(net_if_get_device(iface), true,
				  IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not apply SRC IEEE address filter");
		}
	}
}

static inline void ieee802154_radio_filter_src_short_addr(struct net_if *iface, uint16_t short_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.short_addr = short_addr;

		if (radio->filter(net_if_get_device(iface), true,
				  IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not apply SRC short address filter");
		}
	}
}

static inline void ieee802154_radio_remove_src_ieee_addr(struct net_if *iface, uint8_t *ieee_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.ieee_addr = ieee_addr;

		if (radio->filter(net_if_get_device(iface), false,
				  IEEE802154_FILTER_TYPE_SRC_IEEE_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not remove SRC IEEE address filter");
		}
	}
}

static inline void ieee802154_radio_remove_src_short_addr(struct net_if *iface, uint16_t short_addr)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.short_addr = short_addr;

		if (radio->filter(net_if_get_device(iface), false,
				  IEEE802154_FILTER_TYPE_SRC_SHORT_ADDR,
				  &filter) != 0) {
			NET_WARN("Could not remove SRC short address filter");
		}
	}
}

static inline void ieee802154_radio_remove_pan_id(struct net_if *iface, uint16_t pan_id)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (radio && (radio->get_capabilities(net_if_get_device(iface)) &
		      IEEE802154_HW_FILTER)) {
		struct ieee802154_filter filter;

		filter.pan_id = pan_id;

		if (radio->filter(net_if_get_device(iface), false,
				  IEEE802154_FILTER_TYPE_PAN_ID,
				  &filter) != 0) {
			NET_WARN("Could not remove PAN ID filter");
		}
	}
}


/**
 * MAC utilities
 *
 * @note While MAC utilities may refer to PHY utilities, the inverse is not
 * true.
 */

/**
 * @brief Retrieves the currently selected channel page from the driver (see
 * phyCurrentPage, section 11.3, table 11-2). This is PHY-related information
 * not configured by L2 but directly provided by the driver.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 *
 * @returns The currently active channel page.
 * @retval 0 if an error occurred
 */
static inline enum ieee802154_phy_channel_page
ieee802154_radio_current_channel_page(struct net_if *iface)
{
	struct ieee802154_attr_value value;

	/* Currently we assume that drivers are statically configured to only
	 * support a single channel page. Once drivers need to switch channels at
	 * runtime this can be changed here w/o affecting clients.
	 */
	if (ieee802154_radio_attr_get(iface, IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_PAGES, &value)) {
		return 0;
	}

	return value.phy_supported_channel_pages;
}

/**
 * @brief Calculates a multiple of the PHY's symbol period in nanoseconds.
 *
 * @details The PHY's symbol period depends on the interface's current PHY
 * configuration which usually can be derived from the currently chosen channel
 * page and channel (phyCurrentPage and phyCurrentChannel, section 11.3, table
 * 11-2).
 *
 * To calculate the symbol period of HRP UWB PHYs, the nominal pulse repetition
 * frequency (PRF) is required. HRP UWB drivers will be expected to expose the
 * supported norminal PRF rates as a driver attribute. Existing drivers do not
 * allow for runtime switching of the PRF, so currently the PRF is considered to
 * be read-only and known.
 *
 * TODO: Add an UwbPrf argument once drivers need to support PRF switching at
 * runtime.
 *
 * @note We do not expose an API for a single symbol period to avoid having to
 * deal with floats for PHYs that don't require it while maintaining precision
 * in calculations where PHYs operate at symbol periods involving fractions of
 * nanoseconds.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 * @param channel The channel for which the symbol period is to be calculated.
 * @param multiplier The factor by which the symbol period is to be multiplied.
 *
 * @returns A multiple of the symbol period for the given interface with
 * nanosecond precision.
 * @retval 0 if an error occurred.
 */
static inline net_time_t ieee802154_radio_get_multiple_of_symbol_period(struct net_if *iface,
									uint16_t channel,
									uint16_t multiplier)
{
	/* To keep things simple we only calculate symbol periods for channel
	 * pages that are implemented by existing in-tree drivers. Add additional
	 * channel pages as required.
	 */
	switch (ieee802154_radio_current_channel_page(iface)) {
	case IEEE802154_ATTR_PHY_CHANNEL_PAGE_ZERO_OQPSK_2450_BPSK_868_915:
		return (channel >= 11
				? IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS
				: (channel > 0 ? IEEE802154_PHY_BPSK_915MHZ_SYMBOL_PERIOD_NS
					       : IEEE802154_PHY_BPSK_868MHZ_SYMBOL_PERIOD_NS)) *
		       multiplier;

	case IEEE802154_ATTR_PHY_CHANNEL_PAGE_TWO_OQPSK_868_915:
		return (channel > 0 ? IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS
				    : IEEE802154_PHY_OQPSK_868MHZ_SYMBOL_PERIOD_NS) *
		       multiplier;

	case IEEE802154_ATTR_PHY_CHANNEL_PAGE_FOUR_HRP_UWB: {
		struct ieee802154_attr_value value;

		/* Currently we assume that drivers are statically configured to
		 * only support a single PRF. Once drivers support switching PRF
		 * at runtime an UWB PRF argument needs to be added to this
		 * function which then must be validated against the set of
		 * supported PRFs.
		 */
		if (ieee802154_radio_attr_get(iface, IEEE802154_ATTR_PHY_HRP_UWB_SUPPORTED_PRFS,
					      &value)) {
			return 0;
		}

		switch (value.phy_hrp_uwb_supported_nominal_prfs) {
		case IEEE802154_PHY_HRP_UWB_NOMINAL_4_M:
			return IEEE802154_PHY_HRP_UWB_PRF4_TPSYM_SYMBOL_PERIOD_NS * multiplier;

		case IEEE802154_PHY_HRP_UWB_NOMINAL_16_M:
			return IEEE802154_PHY_HRP_UWB_PRF16_TPSYM_SYMBOL_PERIOD_NS * multiplier;

		case IEEE802154_PHY_HRP_UWB_NOMINAL_64_M:
			return IEEE802154_PHY_HRP_UWB_PRF64_TPSYM_SYMBOL_PERIOD_NS * multiplier;

		case IEEE802154_PHY_HRP_UWB_NOMINAL_64_M_BPRF:
		case IEEE802154_PHY_HRP_UWB_NOMINAL_128_M_HPRF:
		case IEEE802154_PHY_HRP_UWB_NOMINAL_256_M_HPRF:
			return IEEE802154_PHY_HRP_UWB_ERDEV_TPSYM_SYMBOL_PERIOD_NS * multiplier;

		default:
			CODE_UNREACHABLE;
		}
	}

	case IEEE802154_ATTR_PHY_CHANNEL_PAGE_FIVE_OQPSK_780:
		return IEEE802154_PHY_OQPSK_780_TO_2450MHZ_SYMBOL_PERIOD_NS * multiplier;

	case IEEE802154_ATTR_PHY_CHANNEL_PAGE_NINE_SUN_PREDEFINED:
		/* Current SUN FSK drivers only implement legacy IEEE 802.15.4g
		 * 863 MHz (Europe) and 915 MHz (US ISM) bands, see IEEE
		 * 802.15.4g, section 5.1, table 0. Once more bands are required
		 * we need to request the currently active frequency band from
		 * the driver.
		 */
		return IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_NS * multiplier;

	default:
		CODE_UNREACHABLE;
	}
}

/**
 * @brief Calculates the PHY's turnaround time for the current channel page (see
 * section 11.3, table 11-1, aTurnaroundTime) in PHY symbols.
 *
 * @details The PHY's turnaround time is used to calculate - among other
 * parameters - the TX-to-RX turnaround time (see section 10.2.2) and the
 * RX-to-TX turnaround time (see section 10.2.3).
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 *
 * @returns The turnaround time for the given interface in symbols.
 * @retval 0 if an error occurred.
 */
static inline uint32_t ieee802154_radio_get_a_turnaround_time(struct net_if *iface)
{
	enum ieee802154_phy_channel_page channel_page =
		ieee802154_radio_current_channel_page(iface);

	if (!channel_page) {
		return 0;
	}

	/* Section 11.3, table 11-1, aTurnaroundTime: "For the SUN [...] PHYs,
	 * the value is 1 ms expressed in symbol periods, rounded up to the next
	 * integer number of symbol periods using the ceiling() function. [...]
	 * The value is 12 [symbol periods] for all other PHYs.
	 */

	if (channel_page == IEEE802154_ATTR_PHY_CHANNEL_PAGE_NINE_SUN_PREDEFINED) {
		/* Current SUN FSK drivers only implement legacy IEEE 802.15.4g
		 * 863 MHz (Europe) and 915 MHz (US ISM) bands, see IEEE
		 * 802.15.4g, section 5.1, table 0. Once more bands are required
		 * we need to request the currently active frequency band from
		 * the driver.
		 */
		return IEEE802154_PHY_A_TURNAROUND_TIME_1MS(
			IEEE802154_PHY_SUN_FSK_863MHZ_915MHZ_SYMBOL_PERIOD_NS);
	}

	return IEEE802154_PHY_A_TURNAROUND_TIME_DEFAULT;
}

/**
 * @brief Verify if the given channel lies within the allowed range of available
 * channels of the driver's currently selected channel page.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 * @param channel The channel to verify or IEEE802154_NO_CHANNEL
 *
 * @returns true if the channel is available, false otherwise
 */
bool ieee802154_radio_verify_channel(struct net_if *iface, uint16_t channel);

/**
 * @brief Counts all available channels of the driver's currently selected
 * channel page.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 *
 * @returns The number of available channels.
 */
uint16_t ieee802154_radio_number_of_channels(struct net_if *iface);

/**
 * @brief Calculates the MAC's superframe duration (see section 8.4.2,
 * table 8-93, aBaseSuperframeDuration) in microseconds.
 *
 * @details The number of symbols forming a superframe when the superframe order
 * is equal to zero.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 *
 * @returns The base superframe duration for the given interface in microseconds.
 */
static inline uint32_t ieee802154_get_a_base_superframe_duration(struct net_if *iface)
{
	struct ieee802154_context *ctx = net_if_l2_data(iface);

	return ieee802154_radio_get_multiple_of_symbol_period(
		       iface, ctx->channel, IEEE802154_MAC_A_BASE_SUPERFRAME_DURATION) /
	       NSEC_PER_USEC;
}

/**
 * @brief Retrieves macResponseWaitTime, see section 8.4.3.1, table 8-94,
 * converted to microseconds.
 *
 * @details The maximum time, in multiples of aBaseSuperframeDuration converted
 * to microseconds, a device shall wait for a response command to be available
 * following a request command.
 *
 * macResponseWaitTime is a network-topology-dependent parameter and may be set
 * to match the specific requirements of the network that a device is operating
 * on.
 *
 * @note Currently this parameter is read-only and uses the specified default of 32.
 *
 * @param iface pointer to the IEEE 802.15.4 interface
 *
 * @returns The response wait time for the given interface in microseconds.
 */
static inline uint32_t ieee802154_get_response_wait_time_us(struct net_if *iface)
{
	/* TODO: Make this parameter configurable. */
	return IEEE802154_MAC_RESPONSE_WAIT_TIME_DEFAULT *
	       ieee802154_get_a_base_superframe_duration(iface);
}

#endif /* __IEEE802154_UTILS_H__ */
