/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_L2_WIFI_MGMT)
#define SYS_LOG_DOMAIN "net/wifi_mgmt"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>

#include <net/net_core.h>
#include <net/net_if.h>
#include <net/wifi_mgmt.h>

static int wifi_connect(u32_t mgmt_request, struct net_if *iface,
			void *data, size_t len)
{
	struct wifi_connect_req_params *params =
		(struct wifi_connect_req_params *)data;

	if ((params->security > WIFI_SECURITY_TYPE_PSK) ||
	    (params->psk_length < 8) || (params->psk_length > 64) ||
	    (params->ssid_length > WIFI_SSID_MAX_LEN) ||
	    ((params->security == WIFI_SECURITY_TYPE_PSK) &&
	     (params->psk_length == 0)) ||
	    ((params->channel != WIFI_CHANNEL_ANY) &&
	     (params->channel > WIFI_CHANNEL_MAX)) ||
	    !params->ssid || !params->psk) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(iface)) {
		struct net_wifi_mgmt_offload *off_api =
			(struct net_wifi_mgmt_offload *) iface->dev->driver_api;

		return off_api->connect(iface, params);
	}
#endif

	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT, wifi_connect);

/* Guard needed here until non-offload scan supported */
#if defined(CONFIG_NET_OFFLOAD)
static void _scan_result_cb(struct net_if *iface,
			    struct wifi_scan_result *entry)
{
	if (!iface || !entry) {
		return;
	}

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_SCAN_RESULT, iface,
					entry, sizeof(struct wifi_scan_result));
}
#endif

static int wifi_scan(u32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{

#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(iface)) {
		struct net_wifi_mgmt_offload *off_api =
			(struct net_wifi_mgmt_offload *) iface->dev->driver_api;

		return off_api->scan(iface, _scan_result_cb);
	}
#endif

	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN, wifi_scan);

static int wifi_disconnect(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
#if defined(CONFIG_NET_OFFLOAD)
	if (net_if_is_ip_offloaded(iface)) {
		struct net_wifi_mgmt_offload *off_api =
			(struct net_wifi_mgmt_offload *) iface->dev->driver_api;

		return off_api->disconnect(iface);
	}
#endif

	return -ENETDOWN;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT, wifi_disconnect);

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_CONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}

void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status)
{
	struct wifi_status cnx_status = {
		.status = status,
	};

	net_mgmt_event_notify_with_info(NET_EVENT_WIFI_DISCONNECT_RESULT,
					iface, &cnx_status,
					sizeof(struct wifi_status));
}
