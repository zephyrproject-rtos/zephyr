/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief WiFi L2 stack public header
 */

#ifndef ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_
#define ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_

#include <net/net_mgmt.h>
#include <net/wifi.h>

/* Management part definitions */

#define _NET_WIFI_LAYER	NET_MGMT_LAYER_L2
#define _NET_WIFI_CODE	0x156
#define _NET_WIFI_BASE	(NET_MGMT_IFACE_BIT |			\
			 NET_MGMT_LAYER(_NET_WIFI_LAYER) |	\
			 NET_MGMT_LAYER_CODE(_NET_WIFI_CODE))
#define _NET_WIFI_EVENT	(_NET_WIFI_BASE | NET_MGMT_EVENT_BIT)

enum net_request_wifi_cmd {
	NET_REQUEST_WIFI_CMD_SCAN = 1,
	NET_REQUEST_WIFI_CMD_CONNECT,
	NET_REQUEST_WIFI_CMD_DISCONNECT,
};

#define NET_REQUEST_WIFI_SCAN					\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_SCAN)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_SCAN);

#define NET_REQUEST_WIFI_CONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CONNECT);

#define NET_REQUEST_WIFI_DISCONNECT				\
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_DISCONNECT)

NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_DISCONNECT);

enum net_event_wifi_cmd {
	NET_EVENT_WIFI_CMD_SCAN_RESULT = 1,
	NET_EVENT_WIFI_CMD_SCAN_DONE,
	NET_EVENT_WIFI_CMD_CONNECT_RESULT,
	NET_EVENT_WIFI_CMD_DISCONNECT_RESULT,
};

#define NET_EVENT_WIFI_SCAN_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_RESULT)

#define NET_EVENT_WIFI_SCAN_DONE				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_DONE)

#define NET_EVENT_WIFI_CONNECT_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_CONNECT_RESULT)

#define NET_EVENT_WIFI_DISCONNECT_RESULT			\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_DISCONNECT_RESULT)


/* Each result is provided to the net_mgmt_event_callback
 * via its info attribute (see net_mgmt.h)
 */
struct wifi_scan_result {
	u8_t ssid[WIFI_SSID_MAX_LEN];
	u8_t ssid_length;

	u8_t channel;
	enum wifi_security_type security;
	s8_t rssi;
};

struct wifi_connect_req_params {
	u8_t *ssid;
	u8_t ssid_length; /* Max 32 */

	u8_t *psk;
	u8_t psk_length; /* Min 8 - Max 64 */

	u8_t channel;
	enum wifi_security_type security;
};

struct wifi_status {
	int status;
};

#ifdef CONFIG_WIFI_OFFLOAD

#include <net/net_if.h>

typedef void (*scan_result_cb_t)(struct net_if *iface, int status,
				 struct wifi_scan_result *entry);

struct net_wifi_mgmt_offload {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	/* cb parameter is the cb that should be called for each
	 * result by the driver. The wifi mgmt part will take care of
	 * raising the necessary event etc...
	 */
	int (*scan)(struct device *dev, scan_result_cb_t cb);
	int (*connect)(struct device *dev,
		       struct wifi_connect_req_params *params);
	int (*disconnect)(struct device *dev);
};

void wifi_mgmt_raise_connect_result_event(struct net_if *iface, int status);
void wifi_mgmt_raise_disconnect_result_event(struct net_if *iface, int status);

#endif /* CONFIG_WIFI_OFFLOAD */

#endif /* ZEPHYR_INCLUDE_NET_WIFI_MGMT_H_ */
