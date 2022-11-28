/*
 * Copyright (c) 2022 Nuvoton Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

#include "wifi.h"

// NOTE: Though doc says we can register multiple event ID in single net_mgmt_init_event_callback(),
//		 actually it doesn't. Still one event ID for one net_mgmt_init_event_callback().
K_SEM_DEFINE(wifi_iface_up_sem, 0, 1);
K_SEM_DEFINE(wifi_connect_sem, 0, 1);
static struct net_mgmt_event_callback wifi_iface_up_cb;
static struct net_mgmt_event_callback wifi_connect_cb;
static bool wifi_connected;

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
							uint32_t mgmt_event,
							struct net_if *iface)
{
	if (iface != net_if_get_default()) {
		printf("Found Interface %d (%p) not being default\n", net_if_get_by_iface(iface), iface);
		return;
	}

	if (mgmt_event == NET_EVENT_IF_UP) {
		k_sem_give(&wifi_iface_up_sem);
	} else if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
		const struct wifi_status *status = (const struct wifi_status *) cb->info;

		if (status->status) {
			printf("Connection request failed (%d)\n", status->status);
			wifi_connected = false;
		} else {
			printf("WIFI Connected\n");
			wifi_connected = true;
		}

		k_sem_give(&wifi_connect_sem);
	}
}

void wifi_connect(const char *ssid, const char *psk, uint32_t conn_timeout_ms)
{
	struct net_if *iface = net_if_get_default();
	printf("WiFi connecting thru default Interface %d (%p)\n", net_if_get_by_iface(iface), iface);

	// Prepare for waiting on WiFi interface up
	k_sem_reset(&wifi_iface_up_sem);

	// Register event callback for WiFi interface up
	//
	// NOTE: Register event callback first, then check interface up, so that we can catch
	//		 both cases: interface has been up or not.
	net_mgmt_init_event_callback(&wifi_iface_up_cb,
								 wifi_mgmt_event_handler,
								 NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&wifi_iface_up_cb);

	if (iface && net_if_is_up(iface)) {
		k_sem_give(&wifi_iface_up_sem);
	}

	// Wait on WiFi interface up
	if (k_sem_take(&wifi_iface_up_sem, K_MSEC(conn_timeout_ms))) {
		printf("WiFi interface up timeout\n");
		net_mgmt_del_event_callback(&wifi_iface_up_cb);
		exit(EXIT_FAILURE);
	}

	// Prepare for waiting on WiFi connect
	k_sem_reset(&wifi_connect_sem);

	// Register event callback for WiFi connect
	net_mgmt_init_event_callback(&wifi_connect_cb,
								 wifi_mgmt_event_handler,
								 NET_EVENT_WIFI_CONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_connect_cb);

	int rc = 0;

	static struct wifi_connect_req_params cnx_params = {
		.channel = 0,
		.security = WIFI_SECURITY_TYPE_PSK,
	};
	cnx_params.ssid = ssid;
	cnx_params.ssid_length = strlen(ssid);
	cnx_params.psk = psk;
	cnx_params.psk_length = strlen(psk);

	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params, sizeof(struct wifi_connect_req_params));
	if (rc) {
		printf("NET_REQUEST_WIFI_CONNECT failed (%d)\n", rc);
		exit(EXIT_FAILURE);
	}

	// Wait on WiFi connected
	if (k_sem_take(&wifi_connect_sem, K_MSEC(conn_timeout_ms))) {
		printf("WiFi connect timeout\n");
		net_mgmt_del_event_callback(&wifi_connect_cb);
		exit(EXIT_FAILURE);
	}

	if (!wifi_connected) {
		printf("WiFi connect failed\n");
		exit(EXIT_FAILURE);
	}
}
