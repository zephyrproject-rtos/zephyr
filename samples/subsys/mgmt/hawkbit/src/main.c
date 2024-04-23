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
#include <zephyr/data/json.h>

#include "dhcp.h"

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
#include <zephyr/net/tls_credentials.h>
#include "ca_certificate.h"
#endif

LOG_MODULE_REGISTER(main);

#ifdef CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES
struct hawkbit_cfg_data {
	const char *VIN;
	const char *customAttr;
};

struct hawkbit_cfg {
	const char *mode;
	struct hawkbit_cfg_data data;
};

static const struct json_obj_descr json_cfg_data_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, VIN, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg_data, customAttr, JSON_TOK_STRING),
};

static const struct json_obj_descr json_cfg_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct hawkbit_cfg, mode, JSON_TOK_STRING),
	JSON_OBJ_DESCR_OBJECT(struct hawkbit_cfg, data, json_cfg_data_descr),
};

int hawkbit_new_config_data_cb(const char *device_id, uint8_t *buffer, const size_t buffer_size)
{
	struct hawkbit_cfg cfg = {
		.mode = "merge",
		.data.VIN = device_id,
		.data.customAttr = "Hello World!",
	};

	return json_obj_encode_buf(json_cfg_descr, ARRAY_SIZE(json_cfg_descr), &cfg, buffer,
				   buffer_size);
}
#endif /* CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES */

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
#ifdef CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES
	ret = hawkbit_set_custom_data_cb(hawkbit_new_config_data_cb);
	if (ret < 0) {
		LOG_ERR("Failed to set custom data callback");
	}
#endif /* CONFIG_HAWKBIT_CUSTOM_ATTRIBUTES */

	ret = hawkbit_init();
	if (ret < 0) {
		LOG_ERR("Failed to init hawkBit");
	}

#ifdef CONFIG_HAWKBIT_SET_SETTINGS_RUNTIME
	hawkbit_set_server_addr(CONFIG_HAWKBIT_SERVER);
	hawkbit_set_server_port(CONFIG_HAWKBIT_PORT);
#endif

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
