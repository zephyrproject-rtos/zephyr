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

#include <net/ieee802154_radio.h>

static inline
enum ieee802154_hw_caps ieee802154_get_hw_capabilities(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->get_capabilities(iface->dev);
}

static inline int ieee802154_cca(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->cca(iface->dev);
}

static inline int ieee802154_set_channel(struct net_if *iface, u16_t channel)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->set_channel(iface->dev, channel);
}

static inline int ieee802154_set_tx_power(struct net_if *iface, s16_t dbm)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->set_txpower(iface->dev, dbm);
}

static inline int ieee802154_tx(struct net_if *iface,
				struct net_pkt *pkt, struct net_buf *frag)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->tx(iface->dev, pkt, frag);
}

static inline int ieee802154_start(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->start(iface->dev);
}

static inline int ieee802154_stop(struct net_if *iface)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	return radio->stop(iface->dev);
}

static inline void ieee802154_filter_ieee_addr(struct net_if *iface,
					       u8_t *ieee_addr)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	if (radio->get_capabilities(iface->dev) & IEEE802154_HW_FILTER) {
		struct ieee802154_filter filter;

		filter.ieee_addr = ieee_addr;

		if (radio->set_filter(iface->dev,
				      IEEE802154_FILTER_TYPE_IEEE_ADDR,
				      &filter) != 0) {
			NET_WARN("Could not apply IEEE address filter");
		}
	}
}

static inline void ieee802154_filter_short_addr(struct net_if *iface,
						u16_t short_addr)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	if (radio->get_capabilities(iface->dev) & IEEE802154_HW_FILTER) {
		struct ieee802154_filter filter;

		filter.short_addr = short_addr;

		if (radio->set_filter(iface->dev,
				      IEEE802154_FILTER_TYPE_SHORT_ADDR,
				      &filter) != 0) {
			NET_WARN("Could not apply short address filter");
		}
	}
}

static inline void ieee802154_filter_pan_id(struct net_if *iface,
					    u16_t pan_id)
{
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	if (radio->get_capabilities(iface->dev) & IEEE802154_HW_FILTER) {
		struct ieee802154_filter filter;

		filter.pan_id = pan_id;

		if (radio->set_filter(iface->dev,
				      IEEE802154_FILTER_TYPE_PAN_ID,
				      &filter) != 0) {
			NET_WARN("Could not apply PAN ID filter");
		}
	}
}

static inline bool ieee802154_verify_channel(struct net_if *iface,
					     u16_t channel)
{
	if (channel == IEEE802154_NO_CHANNEL) {
		return false;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SUB_GHZ
	const struct ieee802154_radio_api *radio = iface->dev->driver_api;

	if (radio->get_capabilities(iface->dev) & IEEE802154_HW_SUB_GHZ) {
		if (channel > radio->get_subg_channel_count(iface->dev)) {
			return false;
		}
	}
#endif /* CONFIG_NET_L2_IEEE802154_SUB_GHZ */

	return true;
}

#endif /* __IEEE802154_UTILS_H__ */
