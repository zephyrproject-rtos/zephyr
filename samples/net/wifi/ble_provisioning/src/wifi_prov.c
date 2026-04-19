/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_credentials.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/atomic.h>

#include "wifi_prov.h"

LOG_MODULE_REGISTER(wifi_prov, LOG_LEVEL_INF);

#define WIFI_MGMT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)

static struct net_mgmt_event_callback mgmt_cb;
static wifi_prov_result_cb result_cb;
static atomic_t connect_in_progress;

static enum wifi_security_type map_security(uint8_t s, size_t psk_len)
{
	if (s == WIFI_PROV_SECURITY_AUTO) {
		return psk_len ? WIFI_SECURITY_TYPE_PSK : WIFI_SECURITY_TYPE_NONE;
	}

	return (enum wifi_security_type)s;
}

static void wifi_event(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
		       struct net_if *iface)
{
	const struct wifi_status *st = (const struct wifi_status *)cb->info;

	ARG_UNUSED(iface);

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		if (!atomic_cas(&connect_in_progress, 1, 0)) {
			break;
		}

		if (st && st->status == 0) {
			LOG_INF("Wi-Fi connected");
			if (result_cb) {
				result_cb(true);
			}
		} else {
			LOG_ERR("Wi-Fi connect failed (%d)", st ? st->status : -1);
			if (result_cb) {
				result_cb(false);
			}
		}
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("Wi-Fi disconnected");
		break;
	default:
		break;
	}
}

int wifi_prov_init(wifi_prov_result_cb cb)
{
	result_cb = cb;

	net_mgmt_init_event_callback(&mgmt_cb, wifi_event, WIFI_MGMT_EVENTS);
	net_mgmt_add_event_callback(&mgmt_cb);

	return 0;
}

bool wifi_prov_creds_valid(const struct wifi_prov_creds *creds)
{
	return creds && creds->ssid[0] != '\0';
}

bool wifi_prov_has_stored_creds(void)
{
	return !wifi_credentials_is_empty();
}

int wifi_prov_creds_save(const struct wifi_prov_creds *creds)
{
	enum wifi_security_type type;
	size_t ssid_len;
	size_t psk_len;

	if (!wifi_prov_creds_valid(creds)) {
		return -EINVAL;
	}

	ssid_len = strlen(creds->ssid);
	psk_len = strlen(creds->psk);
	type = map_security(creds->security, psk_len);

	if (type == WIFI_SECURITY_TYPE_NONE && psk_len > 0) {
		LOG_WRN("PSK provided but security is open; dropping PSK");
		psk_len = 0;
	} else if (type != WIFI_SECURITY_TYPE_NONE && psk_len == 0) {
		LOG_ERR("security=%u requires a non-empty PSK", type);
		return -EINVAL;
	}

	/* Single-slot provisioning: clear previous entries before storing a new one. */
	(void)wifi_credentials_delete_all();

	return wifi_credentials_set_personal(creds->ssid, ssid_len, type, NULL, 0, creds->psk,
					     psk_len, 0, 0, 0);
}

int wifi_prov_creds_erase(void)
{
	return wifi_credentials_delete_all();
}

int wifi_prov_connect_stored(void)
{
	struct net_if *iface = net_if_get_wifi_sta();
	int err;

	if (!iface) {
		LOG_ERR("no Wi-Fi STA interface");
		return -ENODEV;
	}

	if (wifi_credentials_is_empty()) {
		return -ENOENT;
	}

	LOG_INF("connecting using stored credentials");

	atomic_set(&connect_in_progress, 1);

	err = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
	if (err) {
		atomic_set(&connect_in_progress, 0);
	}

	return err;
}
