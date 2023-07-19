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

/**
 * Sets the radio drivers extended address filter.
 *
 * @param iface Pointer to the IEEE 802.15.4 interface
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
 * @brief Calculates the PHY's symbol period in microseconds.
 *
 * @details The PHY's symbol period depends on the interface's current PHY which
 * can be derived from the currently chosen channel page (phyCurrentPage).
 *
 * Examples:
 *  * SUN FSK: see section 19.1, table 19-1
 *  * O-QPSK:  see section 12.3.3
 *  * HRP UWB: derived from the preamble symbol period (T_psym), see section
 *             11.3, table 11-1 and section 15.2.5, table 15-4
 *
 * @note Currently the symbol period can only be calculated for SUN FSK and O-QPSK.
 *
 * @param iface The interface for which the symbol period should be calculated.
 *
 * @returns The symbol period for the given interface in microseconds.
 */
static inline uint32_t ieee802154_radio_get_symbol_period_us(struct net_if *iface)
{
	/* TODO: Move symbol period calculation to radio driver. */

	if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_SUB_GHZ) &&
	    ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_SUB_GHZ) {
		return IEEE802154_PHY_SYMBOL_PERIOD_US(true);
	}

	return IEEE802154_PHY_SYMBOL_PERIOD_US(false);
}

/**
 * @brief Calculates the PHY's turnaround time (see section 11.3, table 11-1,
 * aTurnaroundTime) in PHY symbols.
 *
 * @details The PHY's turnaround time is used to calculate - among other
 * parameters - the TX-to-RX turnaround time (see section 10.2.2) and the
 * RX-to-TX turnaround time (see section 10.2.3).
 *
 * @note Currently the turnaround time can only be calculated for SUN FSK and O-QPSK.
 *
 * @param iface The interface for which the turnaround time should be calculated.
 *
 * @returns The turnaround time for the given interface.
 */
static inline uint32_t ieee802154_radio_get_a_turnaround_time(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_NET_L2_IEEE802154_SUB_GHZ) &&
	    ieee802154_radio_get_hw_capabilities(iface) & IEEE802154_HW_SUB_GHZ) {
		return IEEE802154_PHY_A_TURNAROUND_TIME(true);
	}

	return IEEE802154_PHY_A_TURNAROUND_TIME(false);
}

static inline bool ieee802154_radio_verify_channel(struct net_if *iface, uint16_t channel)
{
	if (channel == IEEE802154_NO_CHANNEL) {
		return false;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SUB_GHZ
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return false;
	}

	if (radio->get_capabilities(net_if_get_device(iface)) &
	    IEEE802154_HW_SUB_GHZ) {
		if (channel >
		    radio->get_subg_channel_count(net_if_get_device(iface))) {
			return false;
		}
	}
#endif /* CONFIG_NET_L2_IEEE802154_SUB_GHZ */

	return true;
}


/**
 * MAC utilities
 *
 * Note: While MAC utilities may refer to PHY utilities,
 *       the inverse is not true.
 */

/**
 * The number of PHY symbols forming a superframe slot when the superframe order
 * is equal to zero, see sections 8.4.2, table 8-93, aBaseSlotDuration and 6.2.1.
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
 * @brief Calculates the MAC's superframe duration (see section 8.4.2,
 * table 8-93, aBaseSuperframeDuration) in microseconds.
 *
 * @details The number of symbols forming a superframe when the superframe order
 * is equal to zero.
 *
 * @param iface The interface for which the base superframe duration should be
 *              calculated.
 *
 * @returns The base superframe duration for the given interface in microseconds.
 */
static inline uint32_t ieee802154_get_a_base_superframe_duration(struct net_if *iface)
{
	return IEEE802154_MAC_A_BASE_SUPERFRAME_DURATION *
	       ieee802154_radio_get_symbol_period_us(iface);
}

/**
 * Default macResponseWaitTime in multiples of aBaseSuperframeDuration as
 * defined in section 8.4.3.1, table 8-94.
 */
#define IEEE802154_MAC_RESONSE_WAIT_TIME_DEFAULT 32U

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
 * @param iface The interface for which the response wait time should be calculated.
 *
 * @returns The response wait time for the given interface in microseconds.
 */
static inline uint32_t ieee802154_get_response_wait_time_us(struct net_if *iface)
{
	/* TODO: Make this parameter configurable. */
	return IEEE802154_MAC_RESONSE_WAIT_TIME_DEFAULT *
	       ieee802154_get_a_base_superframe_duration(iface);
}


#endif /* __IEEE802154_UTILS_H__ */
