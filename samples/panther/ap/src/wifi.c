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
#include <misc/printk.h>
#include <board.h>

#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/wifi_mgmt.h>

#include "wifi.h"

static struct net_mgmt_event_callback mgmt_cb;
static wifi_cb_t wifi_cb;
static void *wifi_arg;

/**
 * called after wifi connetcion is done
 */
static void handler(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event,
		    struct net_if *iface)
{
	int ret;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}
	if (wifi_cb) {
		printk("wifi: run callback function\n");
		ret = wifi_cb(wifi_arg);
		if (ret) {
			printk("wifi: callback function error %d\n", ret);
		}
	}
}

int wifi_init(struct wifi_conn_params *conn_params, wifi_cb_t cb, void* arg)
{
	struct net_if *iface;
	struct wifi_req_params req = {
		.ap_name = conn_params->ap_name,
		.password = conn_params->password,
		.security = conn_params->security,
	};
	int ret;

	printk("wifi_init\n");

	iface = net_if_get_default();
	if (!iface) {
		printk("wifi:ERROR getting iface\n");
		return -1;
	}

	wifi_cb  = cb;
	wifi_arg = arg;

	printk("wifi init net mgmt event callback\n");
	net_mgmt_init_event_callback(&mgmt_cb, handler, NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&mgmt_cb);

	ret = net_mgmt(NET_REQUEST_WIFI_CMD_AP_CONNECT, iface,
			&req, sizeof(req));
	if(ret){
		printk("wifi connect request return error (%d)\n", ret);
	}else{
		printk("wifi connect request OK\n");
	}

	return 0;
}
