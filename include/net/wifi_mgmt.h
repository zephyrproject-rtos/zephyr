/*
 * Copyright (c) 2017 IpTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public WiFi API
 */

#ifndef __WIFI_MGMT_H__
#define __WIFI_MGMT_H__

#include <device.h>
#include <net/net_if.h>

#include <net/net_mgmt.h>

/* Management part definitions */

#define _NET_WIFI_LAYER	NET_MGMT_LAYER_L2
#define _NET_WIFI_CODE	0x11
#define _NET_WIFI_BASE	(NET_MGMT_IFACE_BIT |			\
				 NET_MGMT_LAYER(_NET_WIFI_LAYER) |\
				 NET_MGMT_LAYER_CODE(_NET_WIFI_CODE))
#define _NET_WIFI_EVENT	(_NET_WIFI_BASE | NET_MGMT_EVENT_BIT)

enum net_request_wifi_cmd {
	NET_REQUEST_WIFI_CMD_SCAN = 1,
	NET_REQUEST_WIFI_CMD_CANCEL_SCAN,
	NET_REQUEST_WIFI_CMD_AP_CONNECT,
	NET_REQUEST_WIFI_CMD_AP_DISCONNECT,
	NET_REQUEST_WIFI_CMD_SET_CHAN,
	NET_REQUEST_WIFI_CMD_GET_CHAN,
};

#define NET_REQUEST_WIFI_CMD_SCAN					 \
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_SCAN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_SCAN);

#define NET_REQUEST_WIFI_CMD_CANCEL_SCAN	 \
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_CANCEL_SCAN)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_CANCEL_SCAN);

#define NET_REQUEST_WIFI_CMD_AP_CONNECT		 \
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_AP_CONNECT)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_AP_CONNECT);

#define NET_REQUEST_WIFI_CMD_AP_DISCONNECT \
	(_NET_WIFI_BASE | NET_REQUEST_WIFI_CMD_AP_DISCONNECT)
NET_MGMT_DEFINE_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_AP_DISCONNECT);

enum net_event_wifi_cmd {
	NET_EVENT_WIFI_CMD_SCAN_RESULT = 1,
};

#define NET_EVENT_WIFI_SCAN_RESULT				\
	(_NET_WIFI_EVENT | NET_EVENT_WIFI_CMD_SCAN_RESULT)

#define WEP            (1<<0)
#define TKIP           (1<<1)
#define AES            (1<<2)
#define SHARED         (1<<3)
#define WPA            (1<<4)
#define WPA2           (1<<5)
#define ENTERPRISE     (1<<6)
#define WPS            (1<<7)
#define IBSS           (1<<8)

typedef enum {
	WIFI_SECURITY_OPEN           = 0,
	WIFI_SECURITY_WEP_PSK        = WEP,
	WIFI_SECURITY_WEP_SHARED     = (WEP  | SHARED),
	WIFI_SECURITY_WPA_TKIP_PSK   = (WPA  | TKIP),
	WIFI_SECURITY_WPA_AES_PSK    = (WPA  | AES),
	WIFI_SECURITY_WPA_MIXED_PSK  = (WPA  | AES | TKIP),
	WIFI_SECURITY_WPA2_AES_PSK   = (WPA2 | AES),
	WIFI_SECURITY_WPA2_TKIP_PSK  = (WPA2 | TKIP),
	WIFI_SECURITY_WPA2_MIXED_PSK = (WPA2 | AES | TKIP),

	WIFI_SECURITY_WPA_TKIP_ENT   = (ENTERPRISE | WPA  | TKIP),
	WIFI_SECURITY_WPA_AES_ENT    = (ENTERPRISE | WPA  | AES),
	WIFI_SECURITY_WPA_MIXED_ENT  = (ENTERPRISE | WPA  | AES | TKIP),
	WIFI_SECURITY_WPA2_TKIP_ENT  = (ENTERPRISE | WPA2 | TKIP),
	WIFI_SECURITY_WPA2_AES_ENT   = (ENTERPRISE | WPA2 | AES),
	WIFI_SECURITY_WPA2_MIXED_ENT = (ENTERPRISE | WPA2 | AES | TKIP),

	WIFI_SECURITY_IBSS_OPEN      = (IBSS),
	WIFI_SECURITY_WPS_OPEN       = (WPS),
	WIFI_SECURITY_WPS_SECURE     = (WPS | AES),

	WIFI_SECURITY_UNKNOWN        = -1,
} wifi_security_t;

/**
 * @brief Scanning parameters
 *
 * Used to request a scan and get results as well
 */
struct wifi_req_params {

	/** Current channel in use as a result */
	u16_t		channel;

	char			*ap_name;
	char			*password;
	wifi_security_t	security;
} __packed;

/* This not meant to be used by any code but 802.15.4 L2 stack */
struct wifi_context {
	struct wifi_req_params	*scan_ctx;
	char			*ap_name;
	wifi_security_t  security;
} S__packed;


struct wifi_api {
	/**
	 * Mandatory to get in first position.
	 * A network device should indeed provide a pointer on such
	 * net_if_api structure. So we make current structure pointer
	 * that can be casted to a net_if_api structure pointer.
	 */
	struct net_if_api iface_api;

	/** scan available access points */
	int (*scan)(struct device *dev);

	/** Connect to access point */
	int (*ap_connect)(struct device *dev, char *ssid,
			  wifi_security_t sec_type, char *psk);

	/** Disconnect from access point */
	int (*ap_disconnect)(struct device *dev);
} __packed;

#endif /* __WIFI_MGMT_H__ */
