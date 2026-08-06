/*
 * Copyright (c) 2023 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(csip_notify_server_test);

extern enum bst_result_t bst_result;

static struct bt_csip_set_member_svc_inst *svc_inst;

static bool is_peer_subscribed(struct bt_conn *conn)
{
	struct bt_gatt_attr *attr;

	attr = bt_gatt_find_by_uuid(NULL, 0, BT_UUID_CSIS_SET_LOCK);
	if (!attr) {
		LOG_INF("No BT_UUID_CSIS_SET_LOCK attribute found");
		return false;
	}

	return bt_gatt_is_subscribed(conn, attr, BT_GATT_CCC_NOTIFY);
}

static void csip_set_member_lock_changed_cb(struct bt_conn *conn,
					    struct bt_csip_set_member_svc_inst *svc_inst,
					    bool locked)
{
	ARG_UNUSED(svc_inst);

	LOG_INF("Client %p %s the lock", conn, locked ? "locked" : "released");
}

static struct bt_csip_set_member_cb csip_cb = {
	.lock_changed  = csip_set_member_lock_changed_cb,
};

static void test_main(void)
{
	int err;
	struct bt_csip_set_member_register_param csip_params = {
		.set_size = 1,
		.rank = 1,
		.lockable = true,
		.cb = &csip_cb,
	};
	struct bt_le_ext_adv *ext_adv;

	LOG_INF("Enabling Bluetooth");
	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	LOG_INF("Registering CSIP Set Member");

	err = bt_cap_acceptor_register(&csip_params, &svc_inst);
	if (err != 0) {
		LOG_ERR("Failed to register csip");
		return;
	}

	setup_connectable_adv(&ext_adv);

	LOG_INF("Waiting to be connected");
	WAIT_FOR_FLAG(flag_connected);
	LOG_INF("Connected");
	LOG_INF("Waiting to be subscribed");

	while (!is_peer_subscribed(default_conn)) {
		(void)k_sleep(K_MSEC(10U));
	}
	LOG_INF("Subscribed");

	err = bt_csip_set_member_lock(svc_inst, true, false);
	if (err != 0) {
		FAIL("Failed to set lock (err %d)\n", err);
		return;
	}

	/* Now wait for client to disconnect */
	LOG_INF("Wait for client disconnect");
	WAIT_FOR_UNSET_FLAG(flag_connected);
	LOG_INF("Client disconnected");

	/* Trigger changes while device is disconnected */
	err = bt_csip_set_member_lock(svc_inst, false, false);
	if (err != 0) {
		FAIL("Failed to set lock (err %d)\n", err);
		return;
	}

	LOG_INF("Start Advertising");
	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to start advertising set (err %d)\n", err);

		err = bt_le_ext_adv_delete(ext_adv);
		if (err != 0) {
			FAIL("Failed to delete extended advertising set (err %d)\n", err);
		}

		return;
	}

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_UNSET_FLAG(flag_connected);

	PASS("CSIP Notify Server passed\n");
}

static const struct bst_test_instance test_csip_notify_server[] = {
	{
		.test_id = "csip_notify_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_csip_notify_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_csip_notify_server);
}
