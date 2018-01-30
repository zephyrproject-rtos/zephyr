/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <device.h>
#include <gpio.h>
#include <board.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/wifi_mgmt.h>
#include <logging/sys_log.h>

#include "wifi.h"

static struct net_mgmt_event_callback mgmt_cb;
static wifi_cb_t wifi_cb;
static void *wifi_arg;

/**
 * called after wifi connetcion is done
 */
static void handler(struct net_mgmt_event_callback *cb,
		    u32_t mgmt_event,
		    struct net_if *iface)
{
	int ret;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	if (wifi_cb) {
		SYS_LOG_INF("wifi: run callback function");
		ret = wifi_cb(wifi_arg);
		if (ret) {
			SYS_LOG_ERR("wifi: callback function error %d", ret);
		}
	}
}

int wifi_init(struct wifi_conn_params *conn_params, wifi_cb_t cb, void *arg)
{
	struct net_if *iface;
	struct wifi_req_params req = {
		.ap_name = conn_params->ap_name,
		.password = conn_params->password,
		.security = conn_params->security,
	};
	int ret;

	SYS_LOG_INF("wifi_init");

	iface = net_if_get_default();
	if (!iface) {
		SYS_LOG_ERR("wifi:ERROR getting iface");
		return -1;
	}

	wifi_cb  = cb;
	wifi_arg = arg;

	SYS_LOG_INF("wifi init net mgmt event callback");
	net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	ret = net_mgmt(NET_REQUEST_WIFI_CMD_AP_CONNECT, iface,
		       &req, sizeof(req));
	if (ret) {
		SYS_LOG_ERR("wifi connect request return error (%d)", ret);
	} else {
		SYS_LOG_INF("wifi connect request OK");
	}

	return 0;
}
