/*
 * Copyright (c) 2017 ipTronix S.r.l.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/net_core.h>

#include <errno.h>

#include <net/net_if.h>
#include <net/wifi_mgmt.h>

static int wifi_scan(u32_t mgmt_request, struct net_if *iface,
		     void *data, size_t len)
{
	struct wifi_api *wifi =
		(struct wifi_api *)iface->dev->driver_api;
	struct wifi_context *ctx =
		(struct wifi_context *)net_if_get_device(iface)->driver_data;
	struct wifi_req_params *scan =
		(struct wifi_req_params *)data;
	int ret;

	NET_DBG("WiFi scan requested");

	if (ctx->scan_ctx) {
		return -EALREADY;
	}

	ctx->scan_ctx = scan;
	ret = 0;

	if (wifi->scan(iface->dev)) {
		NET_DBG("Could not start scanning");
		ret = -EIO;

		goto out;
	}

out:
	ctx->scan_ctx = NULL;

	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_AP_SCAN,
				  wifi_scan);

static int wifi_ap_connect(u32_t mgmt_request, struct net_if *iface,
			   void *data, size_t len)
{
	struct wifi_api *wifi =
		(struct wifi_api *)iface->dev->driver_api;
	struct wifi_context *ctx =
		(struct wifi_context *)net_if_get_device(iface)->driver_data;
	struct wifi_req_params *req =
		(struct wifi_req_params *)data;

	NET_DBG("connection requested to SSID: %s", req->ap_name);

	if (ctx->scan_ctx) {
		/* TODO: stop scanning if necessary */
	}

	if (wifi->ap_connect(iface->dev, req->ap_name, req->security,
			     req->password)) {
		NET_DBG("Could not connect");
		return -EIO;
	}

	ctx->ap_name = req->ap_name;
	ctx->security = req->security;

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_AP_CONNECT,
				  wifi_ap_connect);

static int wifi_ap_disconnect(u32_t mgmt_request, struct net_if *iface,
			      void *data, size_t len)
{
	struct wifi_api *wifi =
		(struct wifi_api *)iface->dev->driver_api;
	struct wifi_context *ctx =
		(struct wifi_context *)net_if_get_device(iface)->driver_data;
	int ret;

	NET_DBG("disconnection requested");

	ret = 0;

	if (wifi->ap_disconnect(iface->dev)) {
		NET_DBG("Could not disconnect");
		ret = -EIO;

		goto out;
	}

	ctx->ap_name = NULL;
	ctx->security = WIFI_SECURITY_UNKNOWN;

out:
	return ret;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_WIFI_CMD_AP_DISCONNECT,
				  wifi_ap_disconnect);
