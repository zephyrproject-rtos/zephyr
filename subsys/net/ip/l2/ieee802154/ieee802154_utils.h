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

#endif /* __IEEE802154_UTILS_H__ */
