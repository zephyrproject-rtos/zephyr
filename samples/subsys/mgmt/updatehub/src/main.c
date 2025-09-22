/*
 * Copyright (c) 2018-2023 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/mgmt/updatehub.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/conn_mgr_monitor.h>

#ifdef CONFIG_NET_L2_WIFI_MGMT
#include <zephyr/net/wifi_mgmt.h>
#endif /* CONFIG_NET_L2_WIFI_MGMT */

#if defined(CONFIG_UPDATEHUB_DTLS)
#include <zephyr/net/tls_credentials.h>
#include "c_certificates.h"
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#define EVENT_MASK (NET_EVENT_L4_CONNECTED | \
		    NET_EVENT_L4_DISCONNECTED)

static struct net_mgmt_event_callback mgmt_cb;

void start_updatehub(void)
{
#if defined(CONFIG_UPDATEHUB_SAMPLE_POLLING)
	LOG_INF("Starting UpdateHub polling mode");
	updatehub_autohandler();
#endif

#if defined(CONFIG_UPDATEHUB_SAMPLE_MANUAL)
	LOG_INF("Starting UpdateHub manual mode");

	switch (updatehub_probe()) {
	case UPDATEHUB_HAS_UPDATE:
		switch (updatehub_update()) {
		case UPDATEHUB_OK:
			ret = 0;
			updatehub_reboot();
			break;

		default:
			LOG_ERR("Error installing update.");
			break;
		}

	case UPDATEHUB_NO_UPDATE:
		LOG_INF("No update found");
		ret = 0;
		break;

	default:
		LOG_ERR("Invalid response");
		break;
	}
#endif
}

static void event_handler(struct net_mgmt_event_callback *cb,
			  uint32_t mgmt_event, struct net_if *iface)
{
	if ((mgmt_event & EVENT_MASK) != mgmt_event) {
		return;
	}

	if (mgmt_event == NET_EVENT_L4_CONNECTED) {
		LOG_INF("Network connected");
		start_updatehub();
		return;
	}

	if (mgmt_event == NET_EVENT_L4_DISCONNECTED) {
		LOG_INF("Network disconnected");
		return;
	}
}

int main(void)
{
	int ret;

	LOG_INF("UpdateHub sample app started");

#if defined(CONFIG_UPDATEHUB_DTLS)
	if (tls_credential_add(CA_CERTIFICATE_TAG,
			       TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
			       server_certificate,
			       sizeof(server_certificate)) < 0) {
		LOG_ERR("Failed to register server certificate");
		return 0;
	}

	if (tls_credential_add(CA_CERTIFICATE_TAG,
			       TLS_CREDENTIAL_PRIVATE_KEY,
			       private_key,
			       sizeof(private_key)) < 0) {
		LOG_ERR("Failed to register private key");
		return 0;
	}
#endif

	/* The image of application needed be confirmed */
	LOG_INF("Confirming the boot image");
	ret = updatehub_confirm();
	if (ret < 0) {
		LOG_ERR("Error to confirm the image");
	}

#if defined(CONFIG_WIFI)
	int nr_tries = 10;
	struct net_if *iface = net_if_get_default();
	static struct wifi_connect_req_params cnx_params = {
		.ssid = CONFIG_UPDATEHUB_SAMPLE_WIFI_SSID,
		.ssid_length = 0,
		.psk = CONFIG_UPDATEHUB_SAMPLE_WIFI_PSK,
		.psk_length = 0,
		.channel = 0,
		.security = WIFI_SECURITY_TYPE_PSK,
	};

	cnx_params.ssid_length = strlen(CONFIG_UPDATEHUB_SAMPLE_WIFI_SSID);
	cnx_params.psk_length = strlen(CONFIG_UPDATEHUB_SAMPLE_WIFI_PSK);

	/* Let's wait few seconds to allow wifi device be on-line */
	while (nr_tries-- > 0) {
		ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params,
			       sizeof(struct wifi_connect_req_params));
		if (ret == 0) {
			break;
		}

		LOG_INF("Connect request failed %d. Waiting iface be up...", ret);
		k_msleep(500);
	}
#endif

	net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);
	conn_mgr_mon_resend_status();
	return 0;
}
