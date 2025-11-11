/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/audio/tbs.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(ccp_call_control_server, CONFIG_LOG_DEFAULT_LEVEL);

#define SEM_TIMEOUT K_SECONDS(5)

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_UUID_GTBS_VAL)),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16, BT_UUID_16_ENCODE(BT_UUID_GTBS_VAL)),
	IF_ENABLED(CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT > 1,
		   (BT_DATA_BYTES(BT_DATA_UUID16_SOME, BT_UUID_16_ENCODE(BT_UUID_TBS_VAL)),
		    BT_DATA_BYTES(BT_DATA_SVC_DATA16, BT_UUID_16_ENCODE(BT_UUID_TBS_VAL))))};

static struct bt_le_ext_adv *adv;
static struct bt_conn *peer_conn;
static struct bt_ccp_call_control_server_bearer
	*bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT];

static K_SEM_DEFINE(sem_state_change, 0, 1);

static void connected_cb(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);

	peer_conn = bt_conn_ref(conn);
	k_sem_give(&sem_state_change);
}

static void disconnected_cb(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	if (conn != peer_conn) {
		return;
	}

	(void)bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s (reason 0x%02x)", addr, reason);

	bt_conn_unref(peer_conn);
	peer_conn = NULL;
	k_sem_give(&sem_state_change);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected_cb,
	.disconnected = disconnected_cb,
};

static int advertise(void)
{
	int err;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	if (err) {
		LOG_ERR("Failed to create advertising set: %d", err);

		return err;
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("Failed to set advertising data: %d", err);

		return err;
	}

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		LOG_ERR("Failed to start advertising set: %d", err);

		return err;
	}

	LOG_INF("Advertising successfully started");

	/* Wait for connection*/
	err = k_sem_take(&sem_state_change, K_FOREVER);
	if (err != 0) {
		LOG_ERR("Failed to take sem_state_change: err %d", err);

		return err;
	}

	return 0;
}

static int reset_ccp_call_control_server(void)
{
	int err;

	LOG_INF("Resetting");

	if (peer_conn != NULL) {
		err = bt_conn_disconnect(peer_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		if (err != 0) {
			return err;
		}

		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_state_change: %d", err);
			return err;
		}
	}

	if (adv != NULL) {
		err = bt_le_ext_adv_stop(adv);
		if (err != 0) {
			LOG_ERR("Failed to stop advertiser: %d", err);
			return err;
		}

		err = bt_le_ext_adv_delete(adv);
		if (err != 0) {
			LOG_ERR("Failed to delete advertiser: %d", err);
			return err;
		}

		adv = NULL;
	}

	k_sem_reset(&sem_state_change);

	return 0;
}

static int init_ccp_call_control_server(void)
{
	const struct bt_tbs_register_param gtbs_param = {
		.provider_name = "Generic TBS",
		.uci = "un000",
		.uri_schemes_supported = "tel,skype",
		.gtbs = true,
		.authorization_required = false,
		.technology = BT_TBS_TECHNOLOGY_3G,
		.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		LOG_ERR("Bluetooth enable failed (err %d)", err);

		return err;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_ccp_call_control_server_register_bearer(&gtbs_param, &bearers[0]);
	if (err < 0) {
		LOG_ERR("Failed to register GTBS (err %d)", err);

		return err;
	}

	LOG_INF("Registered GTBS bearer");

	for (int i = 1; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		char prov_name[22]; /* Enough to store "Telephone Bearer #255" */
		const struct bt_tbs_register_param tbs_param = {
			.provider_name = prov_name,
			.uci = "un000",
			.uri_schemes_supported = "tel,skype",
			.gtbs = false,
			.authorization_required = false,
			/* Set different technologies per bearer */
			.technology = (i % BT_TBS_TECHNOLOGY_WCDMA) + 1,
			.supported_features = CONFIG_BT_TBS_SUPPORTED_FEATURES,
		};

		snprintf(prov_name, sizeof(prov_name), "Telephone Bearer #%d", i);

		err = bt_ccp_call_control_server_register_bearer(&tbs_param, &bearers[i]);
		if (err < 0) {
			LOG_ERR("Failed to register bearer[%d]: %d", i, err);

			return err;
		}

		LOG_INF("Registered bearer[%d]", i);
	}

	return 0;
}

int main(void)
{
	int err;

	err = init_ccp_call_control_server();
	if (err != 0) {
		return 0;
	}

	LOG_INF("CCP Call Control Server initialized");

	while (true) {
		err = reset_ccp_call_control_server();
		if (err != 0) {
			LOG_ERR("Failed to reset");

			break;
		}

		/* Start advertising as a CCP Call Control Server, which includes setting the
		 * required advertising data based on the roles we support.
		 */
		err = advertise();
		if (err != 0) {
			continue;
		}

		/* After advertising we expect CCP Call Control Clients to connect to us and
		 * eventually disconnect again. As a CCP Call Control Server we just react to their
		 * requests and not do anything else.
		 */

		/* Reset if disconnected */
		err = k_sem_take(&sem_state_change, K_FOREVER);
		if (err != 0) {
			LOG_ERR("Failed to take sem_state_change: err %d", err);

			break;
		}
	}

	return 0;
}
