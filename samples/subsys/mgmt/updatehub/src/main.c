/*
 * Copyright (c) 2018-2020 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <updatehub.h>
#include <net/net_mgmt.h>
#include <net/net_event.h>
#include <net/net_conn_mgr.h>
#include <dfu/mcuboot.h>

#if defined(CONFIG_UPDATEHUB_DTLS)
#include <net/tls_credentials.h>
#include "c_certificates.h"
#endif

#include <logging/log.h>
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
			sys_reboot(SYS_REBOOT_WARM);
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

void main(void)
{
	int ret;

	LOG_INF("UpdateHub sample app started");

#if defined(CONFIG_UPDATEHUB_DTLS)
	if (tls_credential_add(CA_CERTIFICATE_TAG,
			       TLS_CREDENTIAL_SERVER_CERTIFICATE,
			       server_certificate,
			       sizeof(server_certificate)) < 0) {
		LOG_ERR("Failed to register server certificate");
		return;
	}

	if (tls_credential_add(CA_CERTIFICATE_TAG,
			       TLS_CREDENTIAL_PRIVATE_KEY,
			       private_key,
			       sizeof(private_key)) < 0) {
		LOG_ERR("Failed to register private key");
		return;
	}
#endif

	/* The image of application needed be confirmed */
	LOG_INF("Confirming the boot image");
	ret = boot_write_img_confirmed();
	if (ret < 0) {
		LOG_ERR("Error to confirm the image");
	}

	net_mgmt_init_event_callback(&mgmt_cb, event_handler, EVENT_MASK);
	net_mgmt_add_event_callback(&mgmt_cb);
	net_conn_mgr_resend_status();
}
