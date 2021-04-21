/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <mgmt/hawkbit.h>
#include <dfu/mcuboot.h>
#include <sys/printk.h>
#include <logging/log.h>

#include "dhcp.h"

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <net/tls_credentials.h>
#include "ca_certificate.h"
#endif

LOG_MODULE_REGISTER(main);

void main(void)
{
	int ret = -1;

	LOG_INF("Hawkbit sample app started");

	app_dhcpv4_startup();

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca_certificate, sizeof(ca_certificate));
#endif

	ret = hawkbit_init();

	if (ret < 0) {
		LOG_ERR("Failed to init hawkbit");
	}

#if defined(CONFIG_HAWKBIT_POLLING)
	LOG_INF("Starting hawkbit polling mode");
	hawkbit_autohandler();
#endif

#if defined(CONFIG_HAWKBIT_MANUAL)
	LOG_INF("Starting Hawkbit manual mode");

	enum hawkbit_response resp;

	switch (hawkbit_probe()) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		LOG_ERR("Image in unconfirmed. Rebooting to revert back to");
		LOG_ERR("previous confirmed image.");

		sys_reboot(SYS_REBOOT_WARM);
		break;

	case HAWKBIT_NO_UPDATE:
		LOG_INF("No update found");
		break;

	case HAWKBIT_CANCEL_UPDATE:
		LOG_INF("Hawkbit update Cancelled from server");
		break;

	case HAWKBIT_OK:
		LOG_INF("Image is already updated");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		LOG_INF("Update Installed");
		break;

	default:
		break;
	}

#endif
}
