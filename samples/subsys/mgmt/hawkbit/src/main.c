/*
 * Copyright (c) 2020 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/mgmt/hawkbit.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/logging/log.h>

#include "dhcp.h"

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <zephyr/net/tls_credentials.h>
#include "ca_certificate.h"
#endif

LOG_MODULE_REGISTER(main);

int main(void)
{
	int ret = -1;

	LOG_INF("hawkBit sample app started");
	LOG_INF("Image build time: " __DATE__ " " __TIME__);

	app_dhcpv4_startup();

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	tls_credential_add(CA_CERTIFICATE_TAG, TLS_CREDENTIAL_CA_CERTIFICATE,
			   ca_certificate, sizeof(ca_certificate));
#endif

	ret = hawkbit_init();

	if (ret < 0) {
		LOG_ERR("Failed to init hawkBit");
	}

#if defined(CONFIG_HAWKBIT_POLLING)
	LOG_INF("Starting hawkBit polling mode");
	hawkbit_autohandler();
#endif

#if defined(CONFIG_HAWKBIT_MANUAL)
	LOG_INF("Starting hawkBit manual mode");

	switch (hawkbit_probe()) {
	case HAWKBIT_UNCONFIRMED_IMAGE:
		LOG_ERR("Image is unconfirmed");
		LOG_ERR("Rebooting to previous confirmed image");
		LOG_ERR("If this image is flashed using a hardware tool");
		LOG_ERR("Make sure that it is a confirmed image");
		k_sleep(K_SECONDS(1));
		sys_reboot(SYS_REBOOT_WARM);
		break;

	case HAWKBIT_NO_UPDATE:
		LOG_INF("No update found");
		break;

	case HAWKBIT_CANCEL_UPDATE:
		LOG_INF("hawkBit update cancelled from server");
		break;

	case HAWKBIT_OK:
		LOG_INF("Image is already updated");
		break;

	case HAWKBIT_UPDATE_INSTALLED:
		LOG_INF("Update installed");
		break;

	case HAWKBIT_PROBE_IN_PROGRESS:
		LOG_INF("hawkBit is already running");
		break;

	default:
		break;
	}

#endif
	return 0;
}
