/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_virtual_mgmt, CONFIG_NET_L2_VIRTUAL_LOG_LEVEL);

#include <errno.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/virtual_mgmt.h>

static int virtual_interface_set_config(uint32_t mgmt_request,
					struct net_if *iface,
					void *data, size_t len)
{
	struct virtual_interface_req_params *params =
				(struct virtual_interface_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	const struct virtual_interface_api *api = dev->api;
	struct virtual_interface_config config = { 0 };
	enum virtual_interface_config_type type;

	if (!api) {
		return -ENOENT;
	}

	if (!api->set_config) {
		return -ENOTSUP;
	}

	if (!data || (len != sizeof(struct virtual_interface_req_params))) {
		return -EINVAL;
	}

	if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS) {
		if (net_if_is_up(iface)) {
			return -EACCES;
		}

		config.family = params->family;
		memcpy(&config.peer6addr, &params->peer6addr,
		       sizeof(config.peer6addr));
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_PEER_ADDRESS;

	} else if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU) {
		if (net_if_is_up(iface)) {
			return -EACCES;
		}

		config.family = params->family;
		config.mtu = params->mtu;
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_MTU;

	} else if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_SET_LINK_TYPE) {
		/* We can update the link types even if the interface is up */
		config.family = params->family;
		memcpy(&config.link_types, &params->link_types,
		       sizeof(config.link_types));
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_LINK_TYPE;
	} else {
		return -EINVAL;
	}

	return api->set_config(iface, type, &config);
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
				  virtual_interface_set_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
				  virtual_interface_set_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_SET_LINK_TYPE,
				  virtual_interface_set_config);

static int virtual_interface_get_config(uint32_t mgmt_request,
					struct net_if *iface,
					void *data, size_t len)
{
	struct virtual_interface_req_params *params =
				(struct virtual_interface_req_params *)data;
	const struct device *dev = net_if_get_device(iface);
	const struct virtual_interface_api *api = dev->api;
	struct virtual_interface_config config = { 0 };
	enum virtual_interface_config_type type;
	int ret = 0;

	if (!api) {
		return -ENOENT;
	}

	if (!api->get_config) {
		return -ENOTSUP;
	}

	if (!data || (len != sizeof(struct virtual_interface_req_params))) {
		return -EINVAL;
	}

	if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_GET_PEER_ADDRESS) {
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_PEER_ADDRESS;

		ret = api->get_config(iface, type, &config);
		if (ret) {
			return ret;
		}

		params->family = config.family;
		memcpy(&params->peer6addr, &config.peer6addr,
		       sizeof(params->peer6addr));

	} else if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_GET_MTU) {
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_MTU;

		ret = api->get_config(iface, type, &config);
		if (ret) {
			return ret;
		}

		params->mtu = config.mtu;

	} else if (mgmt_request == NET_REQUEST_VIRTUAL_INTERFACE_GET_LINK_TYPE) {
		type = VIRTUAL_INTERFACE_CONFIG_TYPE_LINK_TYPE;

		ret = api->get_config(iface, type, &config);
		if (ret) {
			return ret;
		}

		memcpy(&params->link_types, &config.link_types,
		       sizeof(params->link_types));
	} else {
		return -EINVAL;
	}

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_GET_PEER_ADDRESS,
				  virtual_interface_get_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_GET_MTU,
				  virtual_interface_get_config);

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_VIRTUAL_INTERFACE_GET_LINK_TYPE,
				  virtual_interface_get_config);
