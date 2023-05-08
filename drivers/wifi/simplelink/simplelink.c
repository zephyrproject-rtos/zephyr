/**
 * Copyright (c) 2018 Texas Instruments, Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "simplelink_log.h"
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_offload.h>
#ifdef CONFIG_NET_SOCKETS_OFFLOAD
#include <zephyr/net/socket_offload.h>
#endif

#include <ti/drivers/net/wifi/wlan.h>
#include "simplelink_support.h"
#include "simplelink_sockets.h"

#define SCAN_RETRY_DELAY 2000  /* ms */
#define FC_TIMEOUT K_SECONDS(CONFIG_WIFI_SIMPLELINK_FAST_CONNECT_TIMEOUT)

#define SIMPLELINK_IPV4 0x1
#define SIMPLELINK_IPV6 0x2

struct simplelink_data {
	struct net_if *iface;
	unsigned char mac[6];

	/* Fields for scan API to emulate an asynchronous scan: */
	struct k_work_delayable work;
	scan_result_cb_t cb;
	int num_results_or_err;
	int scan_retries;
	bool initialized;
	uint8_t mask;
};

static struct simplelink_data simplelink_data;
static K_SEM_DEFINE(ip_acquired, 0, 1);

/* Handle connection events from the SimpleLink Event Handlers: */
static void simplelink_wifi_cb(uint32_t event, struct sl_connect_state *conn)
{
	int status;

	/*
	 * Once Zephyr wifi_mgmt wifi_status codes are defined, will need
	 * to map from SimpleLink error codes.  For now, just return -EIO.
	 */
	status = (conn->error ? -EIO : 0);

	switch (event) {
	case SL_WLAN_EVENT_CONNECT:
		/* Only get this event if connect succeeds: */
		wifi_mgmt_raise_connect_result_event(simplelink_data.iface,
						     status);
		break;

	case SL_WLAN_EVENT_DISCONNECT:
		/* Could be during a connect, disconnect, or async error: */
		wifi_mgmt_raise_disconnect_result_event(simplelink_data.iface,
							status);
		break;

	case SIMPLELINK_WIFI_CB_IPACQUIRED:
		simplelink_data.mask &= ~SIMPLELINK_IPV4;
		if ((simplelink_data.mask == 0) &&
			(!simplelink_data.initialized)) {
			simplelink_data.initialized = true;
			k_sem_give(&ip_acquired);
		}
		break;

	case SIMPLELINK_WIFI_CB_IPV6ACQUIRED:
		simplelink_data.mask &= ~SIMPLELINK_IPV6;
		if ((simplelink_data.mask == 0) &&
			(!simplelink_data.initialized)) {
			simplelink_data.initialized = true;
			k_sem_give(&ip_acquired);
		}
		break;

	default:
		LOG_DBG("Unrecognized mgmt event: 0x%x", event);
		break;
	}
}

static void simplelink_scan_work_handler(struct k_work *work)
{
	if (simplelink_data.num_results_or_err > 0) {
		int index = 0;
		struct wifi_scan_result scan_result;

		/* Iterate over the table, and call the scan_result callback. */
		while (index < simplelink_data.num_results_or_err) {
			z_simplelink_get_scan_result(index, &scan_result);
			simplelink_data.cb(simplelink_data.iface, 0,
					   &scan_result);
			/* Yield, to ensure notifications get delivered:  */
			k_yield();
			index++;
		}

		/* Sending a NULL entry indicates e/o results, and
		 * triggers the NET_EVENT_WIFI_SCAN_DONE event:
		 */
		simplelink_data.cb(simplelink_data.iface, 0, NULL);

	} else if ((simplelink_data.num_results_or_err ==
		    SL_ERROR_WLAN_GET_NETWORK_LIST_EAGAIN) &&
		   (simplelink_data.scan_retries++ <
		    CONFIG_WIFI_SIMPLELINK_MAX_SCAN_RETRIES)) {
		int32_t delay;

		/* Try again: */
		simplelink_data.num_results_or_err = z_simplelink_start_scan();
		simplelink_data.scan_retries++;
		delay = (simplelink_data.num_results_or_err > 0 ? 0 :
			 SCAN_RETRY_DELAY);
		if (delay > 0) {
			LOG_DBG("Retrying scan...");
		}
		k_work_reschedule(&simplelink_data.work, K_MSEC(delay));

	} else {
		/* Encountered an error, or max retries exceeded: */
		LOG_ERR("Scan failed: retries: %d; err: %d",
			simplelink_data.scan_retries,
			simplelink_data.num_results_or_err);
		simplelink_data.cb(simplelink_data.iface, -EIO, NULL);
	}
}

static int simplelink_mgmt_scan(const struct device *dev, scan_result_cb_t cb)
{
	int err;
	int status;

	/* Cancel any previous scan processing in progress: */
	k_work_cancel_delayable(&simplelink_data.work);

	/* "Request" the scan: */
	err = z_simplelink_start_scan();

	/* Now, launch a delayed work handler to do retries and reporting.
	 * Indicate (to the work handler) either a positive number of results
	 * already returned, or indicate a retry is required:
	 */
	if ((err > 0) || (err == SL_ERROR_WLAN_GET_NETWORK_LIST_EAGAIN)) {
		int32_t delay = (err > 0 ? 0 : SCAN_RETRY_DELAY);

		/* Store for later reference by delayed work handler: */
		simplelink_data.cb = cb;
		simplelink_data.num_results_or_err = err;
		simplelink_data.scan_retries = 0;

		k_work_reschedule(&simplelink_data.work, K_MSEC(delay));
		status = 0;
	} else {
		status = -EIO;
	}

	return status;
}

static int simplelink_mgmt_connect(const struct device *dev,
				   struct wifi_connect_req_params *params)
{
	int ret;

	ret = z_simplelink_connect(params);

	return ret ? -EIO : ret;
}

static int simplelink_mgmt_disconnect(const struct device *dev)
{
	int ret;

	ret = z_simplelink_disconnect();

	return ret ? -EIO : ret;
}

static int simplelink_dummy_get(sa_family_t family,
				enum net_sock_type type,
				enum net_ip_protocol ip_proto,
				struct net_context **context)
{

	LOG_ERR("NET_SOCKETS_OFFLOAD must be configured for this driver");

	return -1;
}

/* Placeholders, until Zephyr IP stack updated to handle a NULL net_offload */
static struct net_offload simplelink_offload = {
	.get	       = simplelink_dummy_get,
	.bind	       = NULL,
	.listen	       = NULL,
	.connect       = NULL,
	.accept	       = NULL,
	.send	       = NULL,
	.sendto	       = NULL,
	.recv	       = NULL,
	.put	       = NULL,
};

static void simplelink_iface_init(struct net_if *iface)
{
	int ret;

	simplelink_data.iface = iface;
	simplelink_data.mask = 0;

	simplelink_data.mask |= IS_ENABLED(CONFIG_NET_IPV4) ?
		SIMPLELINK_IPV4 : 0;
	simplelink_data.mask |= IS_ENABLED(CONFIG_NET_IPV6) ?
		SIMPLELINK_IPV6 : 0;

	/* Direct socket offload used instead of net offload: */
	iface->if_dev->offload = &simplelink_offload;

	/* Initialize and configure NWP to defaults: */
	ret = z_simplelink_init(simplelink_wifi_cb);
	if (ret) {
		LOG_ERR("z_simplelink_init failed!");
		return;
	}

	ret = k_sem_take(&ip_acquired, FC_TIMEOUT);
	if (ret < 0) {
		simplelink_data.initialized = false;
		LOG_ERR("FastConnect timed out connecting to previous AP.");
		LOG_ERR("Please re-establish WiFi connection.");
	}

	/* Grab our MAC address: */
	z_simplelink_get_mac(simplelink_data.mac);

	LOG_DBG("MAC Address %02X:%02X:%02X:%02X:%02X:%02X",
		simplelink_data.mac[0], simplelink_data.mac[1],
		simplelink_data.mac[2],
		simplelink_data.mac[3], simplelink_data.mac[4],
		simplelink_data.mac[5]);

	net_if_set_link_addr(iface, simplelink_data.mac,
			     sizeof(simplelink_data.mac),
			     NET_LINK_ETHERNET);

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
	/* Direct socket offload: */
	socket_offload_dns_register(&simplelink_dns_ops);
	simplelink_sockets_init();

	net_if_socket_offload_set(iface, simplelink_socket_create);
#endif

}

static const struct net_wifi_mgmt_offload simplelink_api = {
	.wifi_iface.iface_api.init = simplelink_iface_init,
	.scan			   = simplelink_mgmt_scan,
	.connect		   = simplelink_mgmt_connect,
	.disconnect		   = simplelink_mgmt_disconnect,
};

static int simplelink_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* We use system workqueue to deal with scan retries: */
	k_work_init_delayable(&simplelink_data.work,
			      simplelink_scan_work_handler);

	LOG_DBG("SimpleLink driver Initialized");

	return 0;
}

NET_DEVICE_OFFLOAD_INIT(simplelink, CONFIG_WIFI_SIMPLELINK_NAME,
			simplelink_init, NULL,
			&simplelink_data, NULL,
			CONFIG_WIFI_INIT_PRIORITY, &simplelink_api,
			CONFIG_WIFI_SIMPLELINK_MAX_PACKET_SIZE);
