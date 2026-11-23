/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/atomic.h>

#include "gatt_svc.h"
#include "wifi_prov.h"

LOG_MODULE_REGISTER(wifi_ble_prov, LOG_LEVEL_INF);

#if defined(CONFIG_WIFI_BLE_PROV_SECURITY_AUTH)
#define PROV_SECURITY_ENABLED 1
#define PROV_MIN_SEC          BT_SECURITY_L4
#elif defined(CONFIG_WIFI_BLE_PROV_SECURITY_ENCRYPT)
#define PROV_SECURITY_ENABLED 1
#define PROV_MIN_SEC          BT_SECURITY_L2
#else
#define PROV_SECURITY_ENABLED 0
#endif

#define ADV_RESTART_DELAY_MS 200

static struct wifi_prov_creds pending;
static K_MUTEX_DEFINE(pending_lock);
static struct k_work connect_work;
static struct k_work erase_work;
static struct k_work_delayable adv_restart_work;
static atomic_t adv_stopped_for_provisioning;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("connection to %s failed (0x%02x %s)", bt_conn_dst_str(conn), err,
			bt_hci_err_to_str(err));
		return;
	}

	LOG_INF("%s: %s", __func__, bt_conn_dst_str(conn));

#if PROV_SECURITY_ENABLED
	{
		int sec_err = bt_conn_set_security(conn, PROV_MIN_SEC);

		if (sec_err && sec_err != -EBUSY) {
			LOG_WRN("bt_conn_set_security (%d)", sec_err);
		}
	}
#endif
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("%s: %s (reason 0x%02x %s)", __func__, bt_conn_dst_str(conn), reason,
		bt_hci_err_to_str(reason));

	/* Clear pending staging so partial SSID/PSK from a disconnected
	 * central doesn't mix with the next peer's writes.
	 */
	k_mutex_lock(&pending_lock, K_FOREVER);
	memset(&pending, 0, sizeof(pending));
	pending.security = WIFI_PROV_SECURITY_AUTO;
	k_mutex_unlock(&pending_lock);

	if (atomic_get(&adv_stopped_for_provisioning)) {
		return;
	}

	/* Zephyr stops advertising on connect. Restart it on the work
	 * queue after the host has released the connection resources,
	 * otherwise bt_le_adv_start() can return -ENOMEM.
	 */
	(void)k_work_schedule(&adv_restart_work, K_MSEC(ADV_RESTART_DELAY_MS));
}

#if PROV_SECURITY_ENABLED
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err) {
		LOG_ERR("security failed: %s level %u err %d", bt_conn_dst_str(conn), level, err);
	} else {
		LOG_INF("security changed: %s level %u", bt_conn_dst_str(conn), level);
	}
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
#if PROV_SECURITY_ENABLED
	.security_changed = security_changed,
#endif
};

#if defined(CONFIG_WIFI_BLE_PROV_SECURITY_AUTH)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	LOG_INF("passkey for %s: %06u", bt_conn_dst_str(conn), passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	LOG_WRN("pairing cancelled: %s", bt_conn_dst_str(conn));
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("paired with %s (bonded=%d)", bt_conn_dst_str(conn), bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	LOG_ERR("pairing failed with %s (reason %u)", bt_conn_dst_str(conn), reason);
}

static const struct bt_conn_auth_cb auth_cb = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb auth_info_cb = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};
#endif /* CONFIG_WIFI_BLE_PROV_SECURITY_AUTH */

static void try_connect(void)
{
	struct wifi_prov_creds snapshot;
	int err;

	k_mutex_lock(&pending_lock, K_FOREVER);
	snapshot = pending;
	k_mutex_unlock(&pending_lock);

	if (!wifi_prov_creds_valid(&snapshot)) {
		LOG_WRN("SSID missing; cannot connect");
		gatt_svc_set_status(GATT_SVC_STATUS_FAILED);
		return;
	}

	err = wifi_prov_creds_save(&snapshot);
	if (err) {
		LOG_ERR("save creds failed (%d)", err);
		gatt_svc_set_status(GATT_SVC_STATUS_FAILED);
		return;
	}

	gatt_svc_set_status(GATT_SVC_STATUS_CONNECTING);

	err = wifi_prov_connect_stored();
	if (err) {
		LOG_ERR("connect dispatch failed (%d)", err);
		gatt_svc_set_status(GATT_SVC_STATUS_FAILED);
	}
}

static void connect_work_handler(struct k_work *w)
{
	ARG_UNUSED(w);
	try_connect();
}

static void erase_work_handler(struct k_work *w)
{
	int err;

	ARG_UNUSED(w);

	wifi_prov_creds_erase();

	k_mutex_lock(&pending_lock, K_FOREVER);
	memset(&pending, 0, sizeof(pending));
	pending.security = WIFI_PROV_SECURITY_AUTO;
	k_mutex_unlock(&pending_lock);

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err && err != -ENOENT) {
		LOG_WRN("bt_unpair (%d)", err);
	}

	/* If advertising was stopped after a successful Wi-Fi connect
	 * (KEEP_BLE_AFTER_CONNECT=n), re-enable it so the device stays
	 * reachable for re-provisioning.
	 */
	if (atomic_cas(&adv_stopped_for_provisioning, 1, 0)) {
		err = gatt_svc_adv_start();
		if (err) {
			LOG_ERR("adv restart failed (%d)", err);
		}
	}

	LOG_INF("credentials erased");

	gatt_svc_set_status(GATT_SVC_STATUS_IDLE);
}

static void adv_restart_work_handler(struct k_work *w)
{
	ARG_UNUSED(w);
	(void)gatt_svc_adv_start();
}

static void on_ssid_written(const uint8_t *data, size_t len)
{
	if (len == 0 || len > WIFI_PROV_SSID_MAX_LEN) {
		LOG_WRN("SSID length out of range (%zu)", len);
		return;
	}

	k_mutex_lock(&pending_lock, K_FOREVER);
	memset(pending.ssid, 0, sizeof(pending.ssid));
	memcpy(pending.ssid, data, len);
	k_mutex_unlock(&pending_lock);

	LOG_INF("SSID set (%zu B)", len);
}

static void on_password_written(const uint8_t *data, size_t len)
{
	if (len > WIFI_PROV_PSK_MAX_LEN) {
		LOG_WRN("PSK length out of range (%zu)", len);
		return;
	}

	k_mutex_lock(&pending_lock, K_FOREVER);
	memset(pending.psk, 0, sizeof(pending.psk));
	if (len > 0) {
		memcpy(pending.psk, data, len);
	}
	k_mutex_unlock(&pending_lock);

	LOG_INF("PSK set (%zu B)", len);
}

static void on_security_written(uint8_t security)
{
	k_mutex_lock(&pending_lock, K_FOREVER);
	pending.security = security;
	k_mutex_unlock(&pending_lock);

	LOG_INF("security set to %u", security);
}

static void on_control_written(uint8_t cmd)
{
	switch (cmd) {
	case GATT_SVC_CTRL_CONNECT:
		k_work_submit(&connect_work);
		break;
	case GATT_SVC_CTRL_ERASE:
		k_work_submit(&erase_work);
		break;
	default:
		LOG_WRN("unknown control command 0x%02x", cmd);
		break;
	}
}

static void wifi_result(bool connected_ok)
{
	gatt_svc_set_status(connected_ok ? GATT_SVC_STATUS_CONNECTED : GATT_SVC_STATUS_FAILED);

	if (connected_ok && !IS_ENABLED(CONFIG_WIFI_BLE_PROV_KEEP_BLE_AFTER_CONNECT)) {
		atomic_set(&adv_stopped_for_provisioning, 1);
		(void)k_work_cancel_delayable(&adv_restart_work);
		(void)gatt_svc_adv_stop();
	}
}

int main(void)
{
	const struct gatt_svc_callbacks cbs = {
		.ssid_written = on_ssid_written,
		.password_written = on_password_written,
		.security_written = on_security_written,
		.control_written = on_control_written,
	};
	int err;

	LOG_INF("Wi-Fi BLE provisioning starting");

	pending.security = WIFI_PROV_SECURITY_AUTO;

	k_work_init(&connect_work, connect_work_handler);
	k_work_init(&erase_work, erase_work_handler);
	k_work_init_delayable(&adv_restart_work, adv_restart_work_handler);

	err = wifi_prov_init(wifi_result);
	if (err) {
		LOG_ERR("wifi_prov_init (%d)", err);
		return err;
	}

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable (%d)", err);
		return err;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		(void)settings_load();
	}

#if defined(CONFIG_WIFI_BLE_PROV_SECURITY_AUTH)
	err = bt_conn_auth_cb_register(&auth_cb);
	if (err) {
		LOG_ERR("bt_conn_auth_cb_register (%d)", err);
		return err;
	}
	err = bt_conn_auth_info_cb_register(&auth_info_cb);
	if (err) {
		LOG_ERR("bt_conn_auth_info_cb_register (%d)", err);
		return err;
	}

	LOG_INF("security: authenticated pairing (passkey)");
#elif defined(CONFIG_WIFI_BLE_PROV_SECURITY_ENCRYPT)
	LOG_INF("security: encrypted link");
#else
	LOG_INF("security: none (plaintext)");
#endif

	gatt_svc_init(&cbs);

	err = gatt_svc_adv_start();
	if (err) {
		LOG_ERR("initial advertising failed (%d)", err);
	}

	if (wifi_prov_has_stored_creds()) {
		LOG_INF("stored credentials found, connecting");
		gatt_svc_set_status(GATT_SVC_STATUS_CONNECTING);

		err = wifi_prov_connect_stored();
		if (err) {
			LOG_ERR("auto-connect failed (%d)", err);
			gatt_svc_set_status(GATT_SVC_STATUS_FAILED);
		}
	} else {
		LOG_INF("no stored credentials; waiting for provisioning over BLE");
		gatt_svc_set_status(GATT_SVC_STATUS_IDLE);
	}

	return 0;
}
