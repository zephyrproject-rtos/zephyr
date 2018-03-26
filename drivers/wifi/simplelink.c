/**
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_WIFI_LEVEL
#define SYS_LOG_DOMAIN "dev/simplelink"
#include <logging/sys_log.h>

#include <zephyr.h>
#include <kernel.h>
#include <device.h>
#include <net/net_if.h>
#include <net/wifi_mgmt.h>
#include <net/net_offload.h>

#include "simplelink_support.h"

struct simplelink_data {
	struct net_if *iface;
	unsigned char mac[6];
};

static struct simplelink_data simplelink_data;

/* Handle connection events from the SimpleLink Event Handlers: */
static void simplelink_wifi_cb(u32_t mgmt_event, struct sl_connect_state *conn)
{
	struct in_addr addr;
	struct in_addr gwaddr;

	/*
	 * TBD: Once Zephyr wifi_mgmt wifi_status codes are defined, will need
	 * to map SimpleLink error codes.  For now, just return vendor's code.
	 */

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CMD_CONNECT_RESULT:
		/* Only get this event if connect succeeds: */
		wifi_mgmt_raise_connect_result_event(simplelink_data.iface,
							conn->error);
		break;

	case NET_EVENT_WIFI_CMD_DISCONNECT_RESULT:
		/* Could be during connect, disconnect, or async error: */
		wifi_mgmt_raise_disconnect_result_event(simplelink_data.iface,
							conn->error);
		break;

	case NET_EVENT_IPV4_ADDR_ADD:
		addr.s_addr = htonl(conn->ip_addr);
		gwaddr.s_addr = htonl(conn->gateway_ip);
		net_if_ipv4_set_gw(simplelink_data.iface, &gwaddr);
		net_if_ipv4_addr_add(simplelink_data.iface, &addr,
				     NET_ADDR_DHCP, 0);
		break;

	default:
		SYS_LOG_DBG("Unrecognized mgmt event: 0x%x", mgmt_event);
		break;
	}
}

/* TBD: Only here to link/test WiFi mgmnt part */
static struct net_offload simplelink_offload = {
	.get            = NULL,
	.bind		= NULL,
	.listen		= NULL,
	.connect	= NULL,
	.accept		= NULL,
	.send		= NULL,
	.sendto		= NULL,
	.recv		= NULL,
	.put		= NULL,
};

static int simplelink_mgmt_scan(struct net_if *iface, scan_result_cb_t cb)
{
	int index = 0;
	int num_results;
	struct wifi_scan_result scan_result;

	/*
	 * Simplelink returns a table of scan results.
	 */
	/* Get the table of scan results: */
	num_results = _simplelink_scan();

	/* Now iterate over the table, and call the scan_result callback. */
	if (cb) {
		while (index < num_results) {
			_simplelink_get_scan_result(index, &scan_result);
			cb(iface, &scan_result);
			index++;
		}
		/* Send the end of table sentinel */
		memset(&scan_result, 0x0, sizeof(scan_result));
		scan_result.idx = WIFI_END_SCAN_RESULTS;
		cb(iface, &scan_result);
	}
	return (num_results < 0 ? num_results : 0);
}

static int simplelink_mgmt_connect(struct net_if *iface,
				   struct wifi_connect_req_params *params)
{
	int ret;

	ret = _simplelink_connect(params);

	return ret ? -EIO : ret;
}

static int simplelink_mgmt_disconnect(struct net_if *iface)
{
	int ret;

	ret = _simplelink_disconnect();

	return ret ? -EIO : ret;
}

static void simplelink_iface_init(struct net_if *iface)
{
	SYS_LOG_DBG("MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
		    simplelink_data.mac[0], simplelink_data.mac[1],
		    simplelink_data.mac[2],
		    simplelink_data.mac[3], simplelink_data.mac[4],
		    simplelink_data.mac[5]);

	net_if_set_link_addr(iface, simplelink_data.mac,
			     sizeof(simplelink_data.mac),
			     NET_LINK_ETHERNET);

	/* TBD: Pending support for socket offload: */
	iface->offload = &simplelink_offload;

	simplelink_data.iface = iface;
}

static const struct net_wifi_mgmt_offload simplelink_api = {
	.iface_api.init = simplelink_iface_init,
	.scan		= simplelink_mgmt_scan,
	.connect	= simplelink_mgmt_connect,
	.disconnect	= simplelink_mgmt_disconnect,
};

static int simplelink_init(struct device *dev)
{
	int ret;

	ARG_UNUSED(dev);

	/* Initialize and configure NWP to defaults: */
	ret = _simplelink_init(simplelink_wifi_cb);
	if (ret) {
		SYS_LOG_ERR("_simplelink_init failed!");
		return(-EIO);
	}

	/* Grab our MAC address: */
	_simplelink_get_mac(simplelink_data.mac);

	SYS_LOG_DBG("SimpleLink driver Initialized");

	return 0;
}

NET_DEVICE_OFFLOAD_INIT(simplelink, CONFIG_WIFI_SIMPLELINK_NAME,
			simplelink_init, &simplelink_data, NULL,
			CONFIG_WIFI_INIT_PRIORITY, &simplelink_api,
			CONFIG_WIFI_SIMPLELINK_MAX_PACKET_SIZE);
