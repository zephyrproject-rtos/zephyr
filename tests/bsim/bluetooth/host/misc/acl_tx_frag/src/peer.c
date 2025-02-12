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
#include <zephyr/logging/log.h>

#include "testlib/adv.h"
#include "testlib/att_read.h"
#include "testlib/att_write.h"
#include "testlib/conn.h"
#include "testlib/log_utils.h"

#include "babblekit/flags.h"
#include "babblekit/testcase.h"

/* local includes */
#include "data.h"

LOG_MODULE_REGISTER(peer, LOG_LEVEL_DBG);

static DEFINE_FLAG(is_subscribed);
static DEFINE_FLAG(got_notification_1);

extern unsigned long runtime_log_level;

int find_characteristic(struct bt_conn *conn, const struct bt_uuid *svc, const struct bt_uuid *chrc,
			uint16_t *chrc_value_handle)
{
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_end_handle;
	int err;

	LOG_DBG("");

	err = bt_testlib_gatt_discover_primary(&svc_handle, &svc_end_handle, conn, svc,
					       BT_ATT_FIRST_ATTRIBUTE_HANDLE,
					       BT_ATT_LAST_ATTRIBUTE_HANDLE);
	if (err != 0) {
		LOG_ERR("Failed to discover service: %d", err);

		return err;
	}

	LOG_DBG("svc_handle: %u, svc_end_handle: %u", svc_handle, svc_end_handle);

	err = bt_testlib_gatt_discover_characteristic(chrc_value_handle, &chrc_end_handle, NULL,
						      conn, chrc, (svc_handle + 1), svc_end_handle);
	if (err != 0) {
		LOG_ERR("Failed to get value handle: %d", err);

		return err;
	}

	LOG_DBG("chrc_value_handle: %u, chrc_end_handle: %u", *chrc_value_handle, chrc_end_handle);

	return err;
}

static uint8_t received_notification(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				     const void *data, uint16_t length)
{
	if (length) {
		LOG_INF("RX notification");
		LOG_HEXDUMP_DBG(data, length, "payload");
		SET_FLAG(got_notification_1);

		TEST_ASSERT(length == GATT_PAYLOAD_SIZE, "Unexpected length: %d", length);
	}

	return BT_GATT_ITER_CONTINUE;
}

static void sub_cb(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

	TEST_ASSERT(params, "params is NULL");
	TEST_ASSERT(params->value, "Host shouldn't know we have unsubscribed");

	LOG_DBG("Subscribed to handle 0x%04x", params->value_handle);
	SET_FLAG(is_subscribed);
}

/* Subscription parameters have the same lifetime as a subscription.
 * That is the backing struct should stay valid until a call to
 * `bt_gatt_unsubscribe()` is made. Hence the `static`.
 */
static struct bt_gatt_subscribe_params sub_params;

/* This is "working memory" used by the `CONFIG_BT_GATT_AUTO_DISCOVER_CCC`
 * feature. It also has to stay valid until the end of the async call.
 */
static struct bt_gatt_discover_params ccc_disc_params;

static void subscribe(struct bt_conn *conn, uint16_t handle, bt_gatt_notify_func_t cb)
{
	int err;

	/* Subscribe to notifications */
	sub_params.notify = cb;
	sub_params.subscribe = sub_cb;
	sub_params.value = BT_GATT_CCC_INDICATE;
	sub_params.value_handle = handle;

	/* Set-up auto-discovery of the CCC handle */
	sub_params.ccc_handle = 0;
	sub_params.disc_params = &ccc_disc_params;
	sub_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;

	err = bt_gatt_subscribe(conn, &sub_params);
	TEST_ASSERT(!err, "Subscribe failed (err %d)", err);

	WAIT_FOR_FLAG(is_subscribed);
}

static uint16_t g_handle;

static void test_iteration(void)
{
	struct bt_conn *conn;
	int err;

	err = bt_testlib_adv_conn(&conn, BT_ID_DEFAULT, bt_get_name());
	TEST_ASSERT(!err, "Failed to start connectable advertising (err %d)", err);

	if (g_handle) {
		LOG_DBG("Re-use cached characteristic");
	} else {
		LOG_DBG("Discover test characteristic");
		err = find_characteristic(conn, test_service_uuid, test_characteristic_uuid,
					  &g_handle);
		TEST_ASSERT(!err, "Failed to find characteristic: %d", err);
	}

	LOG_DBG("Subscribe to test characteristic: handle 0x%04x", g_handle);
	subscribe(conn, g_handle, received_notification);

	WAIT_FOR_FLAG(got_notification_1);

	bt_testlib_wait_disconnected(conn);
	bt_testlib_conn_unref(&conn);
}

/* Read the comments on `entrypoint_dut()` first. */
void entrypoint_peer(void)
{
	int err;

	/* Mark test as in progress. */
	TEST_START("peer");

	/* Set the log level given by the `log_level` CLI argument */
	bt_testlib_log_level_set("peer", runtime_log_level);

	/* Initialize Bluetooth */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Can't enable Bluetooth (err %d)", err);

	LOG_DBG("Bluetooth initialized");

	for (int i = 0; i < TEST_ITERATIONS; i++) {
		LOG_INF("## Iteration %d", i);
		test_iteration();
	}

	TEST_PASS_AND_EXIT("peer");
}
