/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_ETHERNET)
#define SYS_LOG_DOMAIN "net/ethernet"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/ethernet_mgmt.h>

static inline bool is_hw_caps_supported(struct device *dev,
					enum ethernet_hw_caps caps)
{
	const struct ethernet_api *api = dev->driver_api;

	return !!(api->get_capabilities(dev) & caps);
}

static int ethernet_set_config(u32_t mgmt_request,
			       struct net_if *iface,
			       void *data, size_t len)
{
	struct ethernet_req_params *params = (struct ethernet_req_params *)data;
	struct device *dev = net_if_get_device(iface);
	const struct ethernet_api *api = dev->driver_api;
	struct ethernet_config config = { 0 };
	enum ethernet_config_type type;

	if (!api->set_config) {
		return -ENOTSUP;
	}

	if (!data || (len != sizeof(struct ethernet_req_params))) {
		return -EINVAL;
	}

	if (mgmt_request == NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION) {
		if (!is_hw_caps_supported(dev,
					  ETHERNET_AUTO_NEGOTIATION_SET)) {
			return -ENOTSUP;
		}

		config.auto_negotiation = params->auto_negotiation;
		type = ETHERNET_CONFIG_TYPE_AUTO_NEG;
	} else if (mgmt_request == NET_REQUEST_ETHERNET_SET_LINK) {
		type = ETHERNET_CONFIG_TYPE_LINK;

		if (params->l.link_10bt) {
			if (!is_hw_caps_supported(dev,
						  ETHERNET_LINK_10BASE_T)) {
				return -ENOTSUP;
			}

			config.l.link_10bt = true;
		} else if (params->l.link_100bt) {
			if (!is_hw_caps_supported(dev,
						  ETHERNET_LINK_100BASE_T)) {
				return -ENOTSUP;
			}

			config.l.link_100bt = true;
		} else if (params->l.link_1000bt) {
			if (!is_hw_caps_supported(dev,
						  ETHERNET_LINK_1000BASE_T)) {
				return -ENOTSUP;
			}

			config.l.link_1000bt = true;
		} else {
			return -EINVAL;
		}

		type = ETHERNET_CONFIG_TYPE_LINK;
	} else if (mgmt_request == NET_REQUEST_ETHERNET_SET_DUPLEX) {
		if (!is_hw_caps_supported(dev, ETHERNET_DUPLEX_SET)) {
			return -ENOTSUP;
		}

		config.full_duplex = params->full_duplex;
		type = ETHERNET_CONFIG_TYPE_DUPLEX;
	} else if (mgmt_request == NET_REQUEST_ETHERNET_SET_MAC_ADDRESS) {
		if (net_if_is_up(iface)) {
			return -EACCES;
		}

		memcpy(&config.mac_address, &params->mac_address,
		       sizeof(struct net_eth_addr));
		type = ETHERNET_CONFIG_TYPE_MAC_ADDRESS;
	} else {
		return -EINVAL;
	}

	return api->set_config(net_if_get_device(iface), type, &config);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_ETHERNET_SET_AUTO_NEGOTIATION,
				  ethernet_set_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_ETHERNET_SET_LINK,
				  ethernet_set_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_ETHERNET_SET_DUPLEX,
				  ethernet_set_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_ETHERNET_SET_MAC_ADDRESS,
				  ethernet_set_config);

void ethernet_mgmt_raise_carrier_on_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_ETHERNET_CARRIER_ON, iface);
}

void ethernet_mgmt_raise_carrier_off_event(struct net_if *iface)
{
	net_mgmt_event_notify(NET_EVENT_ETHERNET_CARRIER_OFF, iface);
}
