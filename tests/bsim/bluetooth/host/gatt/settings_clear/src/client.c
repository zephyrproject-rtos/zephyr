/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/settings/settings.h>

#include <zephyr/logging/log.h>

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/conn.h"

#include "babblekit/flags.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

#include "common.h"

LOG_MODULE_REGISTER(client, LOG_LEVEL_DBG);

static DEFINE_FLAG(client_security_changed_flag);
static DEFINE_FLAG(client_is_subscribed_flag);

/* Subscription parameters have the same lifetime as a subscription.
 * That is the backing struct should stay valid until a call to
 * `bt_gatt_unsubscribe()` is made. Hence the `static`.
 */
static struct bt_gatt_subscribe_params sub_params;

/* This is "working memory" used by the `CONFIG_BT_GATT_AUTO_DISCOVER_CCC`
 * feature. It also has to stay valid until the end of the async call.
 */
static struct bt_gatt_discover_params ccc_disc_params;

static struct bt_conn_cb client_conn_cb;

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	TEST_ASSERT(err == 0, "Security update failed: %s level %u err %d", addr_str, level, err);

	LOG_DBG("Security changed: %s level %u", addr_str, level);
	SET_FLAG(client_security_changed_flag);
}

void find_characteristic(struct bt_conn *conn,
			const struct bt_uuid *svc,
			const struct bt_uuid *chrc,
			uint16_t *chrc_value_handle)
{
	int err;
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc,
					       BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					       BT_ATT_LAST_ATTRIBUTE_HANDLE);
	TEST_ASSERT(err == 0, "Failed to discover service: %d", err);

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	err = bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle,
						      NULL, conn, chrc, (svc_handle + 1),
						      svc_end_handle);
	TEST_ASSERT(err == 0, "Failed to get value handle: %d", err);

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);
}

static uint8_t received_notification(struct bt_conn *conn,
				     struct bt_gatt_subscribe_params *params,
				     const void *data,
				     uint16_t length)
{
	if (length) {
		LOG_INF("RX notification");
		LOG_HEXDUMP_DBG(data, length, "payload");
	}

	return BT_GATT_ITER_CONTINUE;
}

static void sub_cb(struct bt_conn *conn,
		   uint8_t err,
		   struct bt_gatt_subscribe_params *params)
{
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

	TEST_ASSERT(params, "params is NULL");
	TEST_ASSERT(params->value, "Host shouldn't know we have unsubscribed");

	LOG_DBG("Subscribed to handle 0x%04x", params->value_handle);
	SET_FLAG(client_is_subscribed_flag);
}



static void subscribe(struct bt_conn *conn,
		      uint16_t handle,
		      bt_gatt_notify_func_t cb)
{
	int err;

	/* Subscribe to notifications */
	sub_params.notify = cb;
	sub_params.subscribe = sub_cb;
	sub_params.value = BT_GATT_CCC_NOTIFY;
	sub_params.value_handle = handle;
	sub_params.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE;
	sub_params.disc_params = &ccc_disc_params;
	sub_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_subscribe(conn, &sub_params);
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

	WAIT_FOR_FLAG(client_is_subscribed_flag);
}

void client_procedure(void)
{
	int err;
	struct bt_conn *conn;
	uint16_t handle;

	TEST_START("client");

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Cannot enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	err = settings_load();
	TEST_ASSERT(err == 0, "Failed to load settings (err %d)", err);

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	TEST_ASSERT(err == 0, "Failed to unpair (err %d)", err);

	client_conn_cb.connected = NULL;
	client_conn_cb.disconnected = NULL;
	client_conn_cb.security_changed = security_changed;
	client_conn_cb.identity_resolved = NULL;

	bt_conn_cb_register(&client_conn_cb);
	TEST_ASSERT(err == 0, "Failed to set server conn callbacks (err %d)", err);

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, ADVERTISER_NAME);
	TEST_ASSERT(err == 0, "Failed to start connectable advertising (err %d)", err);

	WAIT_FOR_FLAG(client_security_changed_flag);

	find_characteristic(conn, test_service_uuid, test_characteristic_uuid, &handle);

	subscribe(conn, handle, received_notification);

	TEST_PASS("client");
}
