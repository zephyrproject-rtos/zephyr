/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 Management
 */

#ifndef __IEEE802154_UTILS_H__
#define __IEEE802154_UTILS_H__

#include <zephyr/net/ieee802154_radio.h>

static inline
enum ieee802154_hw_caps ieee802154_get_hw_capabilities(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return 0;
	}

	return radio->get_capabilities(net_if_get_device(iface));
}

static inline int ieee802154_cca(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->cca(net_if_get_device(iface));
}

static inline int ieee802154_set_channel(struct net_if *iface, uint16_t channel)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->set_channel(net_if_get_device(iface), channel);
}

static inline int ieee802154_set_tx_power(struct net_if *iface, int16_t dbm)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->set_txpower(net_if_get_device(iface), dbm);
}

static inline int ieee802154_tx(struct net_if *iface,
				enum ieee802154_tx_mode mode,
				struct net_pkt *pkt,
				struct net_buf *buf)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->tx(net_if_get_device(iface), mode, pkt, buf);
}

static inline int ieee802154_start(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio =
		net_if_get_device(iface)->api;

	if (!radio) {
		return -ENOENT;
	}

	return radio->start(net_if_get_device(iface));
}

static inline int ieee802154_stop(struct net_if *iface)
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
static inline void ieee802154_filter_ieee_addr(struct net_if *iface, uint8_t *ieee_addr)
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

static inline void ieee802154_filter_short_addr(struct net_if *iface,
						uint16_t short_addr)
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

static inline void ieee802154_filter_pan_id(struct net_if *iface,
					    uint16_t pan_id)
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

static inline void ieee802154_filter_src_ieee_addr(struct net_if *iface,
						   uint8_t *ieee_addr)
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

static inline void ieee802154_filter_src_short_addr(struct net_if *iface,
						    uint16_t short_addr)
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

static inline void ieee802154_remove_src_ieee_addr(struct net_if *iface,
						   uint8_t *ieee_addr)
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

static inline void ieee802154_remove_src_short_addr(struct net_if *iface,
						    uint16_t short_addr)
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

static inline bool ieee802154_verify_channel(struct net_if *iface,
					     uint16_t channel)
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

#endif /* __IEEE802154_UTILS_H__ */
