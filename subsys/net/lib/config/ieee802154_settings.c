/* IEEE 802.15.4 settings code */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_config, CONFIG_NET_CONFIG_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <errno.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ieee802154_mgmt.h>

int z_net_config_ieee802154_setup(void)
{
	uint16_t channel = CONFIG_NET_CONFIG_IEEE802154_CHANNEL;
	uint16_t pan_id = CONFIG_NET_CONFIG_IEEE802154_PAN_ID;
	int16_t tx_power = CONFIG_NET_CONFIG_IEEE802154_RADIO_TX_POWER;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_params sec_params = {
		.key = CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY,
		.key_len = sizeof(CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY),
		.key_mode = CONFIG_NET_CONFIG_IEEE802154_SECURITY_KEY_MODE,
		.level = CONFIG_NET_CONFIG_IEEE802154_SECURITY_LEVEL,
	};
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	struct net_if *iface;
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_ieee802154));

	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID,
		     iface, &pan_id, sizeof(uint16_t)) ||
	    net_mgmt(NET_REQUEST_IEEE802154_SET_CHANNEL,
		     iface, &channel, sizeof(uint16_t)) ||
	    net_mgmt(NET_REQUEST_IEEE802154_SET_TX_POWER,
		     iface, &tx_power, sizeof(int16_t))) {
		return -EINVAL;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS, iface,
		     &sec_params, sizeof(struct ieee802154_security_params))) {
		return -EINVAL;
	}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	net_if_up(iface);

	return 0;
}
