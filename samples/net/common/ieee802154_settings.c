/* IEEE 802.15.4 Samples settings code */

/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_core.h>
#include <net/net_mgmt.h>
#include <net/ieee802154.h>

int ieee802154_sample_setup(void)
{
	uint16_t channel = CONFIG_NET_APP_IEEE802154_CHANNEL;
	uint16_t pan_id = CONFIG_NET_APP_IEEE802154_PAN_ID;

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	struct ieee802154_security_params sec_params = {
		.key = CONFIG_NET_APP_IEEE802154_SECURITY_KEY,
		.key_len = sizeof(CONFIG_NET_APP_IEEE802154_SECURITY_KEY),
		.key_mode = CONFIG_NET_APP_IEEE802154_SECURITY_KEY_MODE,
		.level = CONFIG_NET_APP_IEEE802154_SECURITY_LEVEL,
	};
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	struct net_if *iface;
	struct device *dev;

	dev = device_get_binding(CONFIG_NET_APP_IEEE802154_DEV_NAME);
	if (!dev) {
		return -ENODEV;
	}

	iface = net_if_lookup_by_dev(dev);
	if (!iface) {
		return -EINVAL;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID,
		     iface, &pan_id, sizeof(uint16_t)) ||
	    net_mgmt(NET_REQUEST_IEEE802154_SET_CHANNEL,
		     iface, &channel, sizeof(uint16_t))) {
		return -EINVAL;
	}

#ifdef CONFIG_NET_L2_IEEE802154_SECURITY
	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SECURITY_SETTINGS, iface,
		     &sec_params, sizeof(struct ieee802154_security_params))) {
		return -EINVAL;
	}
#endif /* CONFIG_NET_L2_IEEE802154_SECURITY */

	return 0;
}
