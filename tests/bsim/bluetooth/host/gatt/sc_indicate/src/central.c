/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test_central, LOG_LEVEL_DBG);

#include "bs_bt_utils.h"

DEFINE_FLAG(flag_discovered);
DEFINE_FLAG(flag_subscribed);
DEFINE_FLAG(flag_indicated);

enum GATT_HANDLES {
	SC,
	CCC,
	NUM_HANDLES,
};

static uint16_t gatt_handles[NUM_HANDLES] = {0};

static struct bt_gatt_subscribe_params subscribe_params;

static void sc_subscribed(struct bt_conn *conn,
			  uint8_t err,
			  struct bt_gatt_subscribe_params *params)
{
	LOG_DBG("subscribed");
	SET_FLAG(flag_subscribed);
}

static uint8_t sc_indicated(struct bt_conn *conn,
			    struct bt_gatt_subscribe_params *params,
			    const void *data,
			    uint16_t length)
{
	LOG_DBG("indication received");

	SET_FLAG(flag_indicated);

	return BT_GATT_ITER_CONTINUE;
}

static void subscribe(void)
{
	int err;

	subscribe_params.ccc_handle = gatt_handles[CCC];
	subscribe_params.value_handle = gatt_handles[SC];
	subscribe_params.value = BT_GATT_CCC_INDICATE;
	subscribe_params.subscribe = sc_subscribed;
	subscribe_params.notify = sc_indicated;

	err = bt_gatt_subscribe(get_g_conn(), &subscribe_params);
	BSIM_ASSERT(!err, "bt_gatt_subscribe failed (%d)\n", err);

	WAIT_FOR_FLAG(flag_subscribed);
}

static uint8_t discover_func(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     struct bt_gatt_discover_params *params,
			     int err)
{
	if (attr == NULL) {
		for (size_t i = 0U; i < ARRAY_SIZE(gatt_handles); i++) {
			LOG_DBG("handle[%d] = 0x%x", i, gatt_handles[i]);
			BSIM_ASSERT(gatt_handles[i] != 0, "did not find all handles\n");
		}

		(void)memset(params, 0, sizeof(*params));
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		const struct bt_gatt_chrc *chrc = (struct bt_gatt_chrc *)attr->user_data;
		static const struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

		if (bt_uuid_cmp(chrc->uuid, BT_UUID_GATT_SC) == 0) {
			int err;

			LOG_DBG("found sc");
			gatt_handles[SC] = chrc->value_handle;

			params->uuid = &ccc_uuid.uuid;
			params->start_handle = attr->handle + 2;
			params->type = BT_GATT_DISCOVER_DESCRIPTOR;

			err = bt_gatt_discover(conn, params);
			BSIM_ASSERT(!err, "bt_gatt_discover failed (%d)\n", err);

			return BT_GATT_ITER_STOP;
		}

	} else if (params->type == BT_GATT_DISCOVER_DESCRIPTOR &&
		   bt_uuid_cmp(params->uuid, BT_UUID_GATT_CCC) == 0) {
		LOG_DBG("found ccc");
		gatt_handles[CCC] = attr->handle;
		SET_FLAG(flag_discovered);

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static void gatt_discover(void)
{
	int err;
	static struct bt_gatt_discover_params discover_params;

	discover_params.uuid = NULL;
	discover_params.func = discover_func;
	discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(get_g_conn(), &discover_params);
	BSIM_ASSERT(!err, "bt_gatt_discover failed (%d)\n", err);

	WAIT_FOR_FLAG(flag_discovered);

	LOG_DBG("sc handle: %d", gatt_handles[SC]);
	LOG_DBG("ccc handle: %d", gatt_handles[CCC]);
}

void central(void)
{
	/*
	 * test goal: check that service changed indication is sent on
	 * reconnection when the server's GATT database has been updated since
	 * last connection
	 *
	 * the central will connect, bond with the peripheral and then
	 * disconnect after doing that, the central will try to connect again,
	 * this time it will not elevate the security
	 *
	 * to pass the test, the central will wait to receive the service
	 * changed indication
	 */

	int err;
	struct bt_conn_auth_info_cb bt_conn_auth_info_cb = {
		.pairing_failed = pairing_failed,
		.pairing_complete = pairing_complete,
	};

	err = bt_enable(NULL);
	BSIM_ASSERT(!err, "bt_enable failed (%d)\n", err);

	err = bt_conn_auth_info_cb_register(&bt_conn_auth_info_cb);
	BSIM_ASSERT(!err, "bt_conn_auth_info_cb_register failed.\n");

	err = settings_load();
	BSIM_ASSERT(!err, "settings_load failed (%d)\n", err);

	scan_connect_to_first_result();
	wait_connected();

	set_security(BT_SECURITY_L2);

	TAKE_FLAG(flag_pairing_complete);
	TAKE_FLAG(flag_bonded);

	/* subscribe to the service changed indication */
	gatt_discover();
	subscribe();

	disconnect();
	wait_disconnected();
	clear_g_conn();

	scan_connect_to_first_result();
	wait_connected();

	/* wait for service change indication */
	WAIT_FOR_FLAG(flag_indicated);

	PASS("PASS\n");
}
