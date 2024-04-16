/*
 * Copyright (c) 2023 Florian Grandel, Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IEEE 802.15.4 internal MAC and PHY Utils Implementation
 *
 * All references to the standard in this file cite IEEE 802.15.4-2020.
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ieee802154, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include "ieee802154_utils.h"

/**
 * PHY utilities
 */

bool ieee802154_radio_verify_channel(struct net_if *iface, uint16_t channel)
{
	struct ieee802154_attr_value value;

	if (channel == IEEE802154_NO_CHANNEL) {
		return false;
	}

	if (ieee802154_radio_attr_get(iface, IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES,
				      &value)) {
		return false;
	}

	for (int channel_range_index = 0;
	     channel_range_index < value.phy_supported_channels->num_ranges;
	     channel_range_index++) {
		const struct ieee802154_phy_channel_range *const channel_range =
			&value.phy_supported_channels->ranges[channel_range_index];

		if (channel >= channel_range->from_channel &&
		    channel <= channel_range->to_channel) {
			return true;
		}
	}

	return false;
}

uint16_t ieee802154_radio_number_of_channels(struct net_if *iface)
{
	struct ieee802154_attr_value value;
	uint16_t num_channels = 0;

	if (ieee802154_radio_attr_get(iface, IEEE802154_ATTR_PHY_SUPPORTED_CHANNEL_RANGES,
				      &value)) {
		return 0;
	}

	for (int channel_range_index = 0;
	     channel_range_index < value.phy_supported_channels->num_ranges;
	     channel_range_index++) {
		const struct ieee802154_phy_channel_range *const channel_range =
			&value.phy_supported_channels->ranges[channel_range_index];

		__ASSERT_NO_MSG(channel_range->to_channel >= channel_range->from_channel);
		num_channels += channel_range->to_channel - channel_range->from_channel + 1U;
	}

	return num_channels;
}
