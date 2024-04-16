/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/ead.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>

#include "data.h"

LOG_MODULE_REGISTER(ead_peripheral_sample, CONFIG_BT_EAD_LOG_LEVEL);

static struct bt_le_ext_adv_cb adv_cb;
static struct bt_conn_cb peripheral_cb;
static struct bt_conn_auth_cb peripheral_auth_cb;

static struct bt_conn *default_conn;

static struct k_poll_signal disconn_signal;
static struct k_poll_signal passkey_enter_signal;

static ssize_t read_key_material(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
				 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
				 sizeof(struct key_material));
}

BT_GATT_SERVICE_DEFINE(key_mat, BT_GATT_PRIMARY_SERVICE(BT_UUID_CUSTOM_SERVICE),
		       BT_GATT_CHARACTERISTIC(BT_UUID_GATT_EDKM, BT_GATT_CHRC_READ,
					      BT_GATT_PERM_READ_AUTHEN, read_key_material, NULL,
					      &mk));

static int update_ad_data(struct bt_le_ext_adv *adv)
{
	int err;
	size_t offset;

	/* Encrypt ad structure 1 */

	size_t size_ad_1 = BT_DATA_SERIALIZED_SIZE(ad[1].data_len);
	uint8_t ad_1[size_ad_1];
	size_t size_ead_1 = BT_EAD_ENCRYPTED_PAYLOAD_SIZE(size_ad_1);
	uint8_t ead_1[size_ead_1];

	bt_data_serialize(&ad[1], ad_1);

	err = bt_ead_encrypt(mk.session_key, mk.iv, ad_1, size_ad_1, ead_1);
	if (err != 0) {
		LOG_ERR("Error during first encryption");
		return -1;
	}

	/* Encrypt ad structures 3 and 4 */

	size_t size_ad_3_4 =
		BT_DATA_SERIALIZED_SIZE(ad[3].data_len) + BT_DATA_SERIALIZED_SIZE(ad[4].data_len);
	uint8_t ad_3_4[size_ad_3_4];
	size_t size_ead_2 = BT_EAD_ENCRYPTED_PAYLOAD_SIZE(size_ad_3_4);
	uint8_t ead_2[size_ead_2];

	offset = bt_data_serialize(&ad[3], &ad_3_4[0]);
	bt_data_serialize(&ad[4], &ad_3_4[offset]);

	err = bt_ead_encrypt(mk.session_key, mk.iv, ad_3_4, size_ad_3_4, ead_2);
	if (err != 0) {
		LOG_ERR("Error during second encryption");
		return -1;
	}

	/* Concatenate and update the advertising data */
	struct bt_data ad_structs[] = {
		ad[0],
		BT_DATA(BT_DATA_ENCRYPTED_AD_DATA, ead_1, size_ead_1),
		ad[2],
		BT_DATA(BT_DATA_ENCRYPTED_AD_DATA, ead_2, size_ead_2),
	};

	LOG_INF("Advertising data size: %zu", bt_data_get_len(ad_structs, ARRAY_SIZE(ad_structs)));

	err = bt_le_ext_adv_set_data(adv, ad_structs, ARRAY_SIZE(ad_structs), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data (%d)", err);
		return -1;
	}

	LOG_DBG("Advertising Data Updated");

	return 0;
}

static int set_ad_data(struct bt_le_ext_adv *adv)
{
	return update_ad_data(adv);
}

static bool rpa_expired_cb(struct bt_le_ext_adv *adv)
{
	LOG_DBG("RPA expired");

	/* The Bluetooth Core Specification say that the Randomizer and thus the
	 * Advertising Data shall be updated each time the address is changed.
	 *
	 * ref:
	 * Supplement to the Bluetooth Core Specification | v11, Part A 1.23.4
	 */
	update_ad_data(adv);

	return true;
}

static int create_adv(struct bt_le_ext_adv **adv)
{
	int err;
	struct bt_le_adv_param params;

	memset(&params, 0, sizeof(struct bt_le_adv_param));

	params.options |= BT_LE_ADV_OPT_CONNECTABLE;
	params.options |= BT_LE_ADV_OPT_EXT_ADV;

	params.id = BT_ID_DEFAULT;
	params.sid = 0;
	params.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
	params.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;

	adv_cb.rpa_expired = rpa_expired_cb;

	err = bt_le_ext_adv_create(&params, &adv_cb, adv);
	if (err) {
		LOG_ERR("Failed to create advertiser (%d)", err);
		return -1;
	}

	return 0;
}

static int start_adv(struct bt_le_ext_adv *adv)
{
	int err;
	int32_t timeout = 0;
	uint8_t num_events = 0;

	struct bt_le_ext_adv_start_param start_params;

	start_params.timeout = timeout;
	start_params.num_events = num_events;

	err = bt_le_ext_adv_start(adv, &start_params);
	if (err) {
		LOG_ERR("Failed to start advertiser (%d)", err);
		return -1;
	}

	LOG_DBG("Advertiser started");

	return 0;
}

static int stop_and_delete_adv(struct bt_le_ext_adv *adv)
{
	int err;

	err = bt_le_ext_adv_stop(adv);
	if (err) {
		LOG_ERR("Failed to stop advertiser (err %d)", err);
		return -1;
	}

	err = bt_le_ext_adv_delete(adv);
	if (err) {
		LOG_ERR("Failed to delete advertiser (err %d)", err);
		return -1;
	}

	return 0;
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		LOG_ERR("Failed to connect to %s (%u)", addr, err);
		return;
	}

	LOG_DBG("Connected to %s", addr);

	default_conn = conn;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Disconnected from %s (reason 0x%02x)", addr, reason);

	k_poll_signal_raise(&disconn_signal, 0);
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		LOG_DBG("Security changed: %s level %u", addr, level);
	} else {
		LOG_DBG("Security failed: %s level %u err %d", addr, level, err);
	}
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	char passkey_str[7];
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);

	printk("Passkey for %s: %s\n", addr, passkey_str);

	k_poll_signal_raise(&passkey_enter_signal, 0);
}

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char passkey_str[7];
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	snprintk(passkey_str, ARRAY_SIZE(passkey_str), "%06u", passkey);

	LOG_DBG("Passkey for %s: %s", addr, passkey_str);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_DBG("Pairing cancelled: %s", addr);
}

static int init_bt(void)
{
	int err;

	k_poll_signal_init(&disconn_signal);
	k_poll_signal_init(&passkey_enter_signal);

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return -1;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		LOG_ERR("Unpairing failed (err %d)", err);
	}

	peripheral_cb.connected = connected;
	peripheral_cb.disconnected = disconnected;
	peripheral_cb.security_changed = security_changed;

	bt_conn_cb_register(&peripheral_cb);

	peripheral_auth_cb.pairing_confirm = NULL;
	peripheral_auth_cb.passkey_confirm = auth_passkey_confirm;
	peripheral_auth_cb.passkey_display = auth_passkey_display;
	peripheral_auth_cb.passkey_entry = NULL;
	peripheral_auth_cb.oob_data_request = NULL;
	peripheral_auth_cb.cancel = auth_cancel;

	err = bt_conn_auth_cb_register(&peripheral_auth_cb);
	if (err) {
		LOG_ERR("Failed to register bt_conn_auth_cb (err %d)", err);
		return -1;
	}

	return 0;
}

int run_peripheral_sample(int get_passkey_confirmation(struct bt_conn *conn))
{
	int err;
	struct bt_le_ext_adv *adv = NULL;

	err = init_bt();
	if (err) {
		return -1;
	}

	/* Setup advertiser */
	err = create_adv(&adv);
	if (err) {
		return -2;
	}

	err = start_adv(adv);
	if (err) {
		return -3;
	}

	err = set_ad_data(adv);
	if (err) {
		return -4;
	}

	/* Wait for the peer to update security */
	await_signal(&passkey_enter_signal);

	err = get_passkey_confirmation(default_conn);
	if (err) {
		LOG_ERR("Failure during security update");
		return -5;
	}

	/* Wait for the peer to disconnect */
	await_signal(&disconn_signal);

	/* Restart advertising */
	err = start_adv(adv);
	if (err) {
		return -3;
	}

	err = set_ad_data(adv);
	if (err) {
		return -4;
	}

	/* Wait 10s before stopping and deleting the advertiser */
	k_sleep(K_SECONDS(10));

	err = stop_and_delete_adv(adv);
	if (err) {
		return -6;
	}

	return 0;
}
