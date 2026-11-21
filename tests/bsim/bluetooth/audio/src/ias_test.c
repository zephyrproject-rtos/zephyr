/*
 * Copyright (c) 2022 Codecoup
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/ias.h>
#include <zephyr/logging/log.h>
#include <zephyr/types.h>

#include "bstests.h"
#include "common.h"

LOG_MODULE_REGISTER(ias_test);

#ifdef CONFIG_BT_IAS
extern enum bst_result_t bst_result;

CREATE_FLAG(g_high_alert_received);
CREATE_FLAG(g_mild_alert_received);
CREATE_FLAG(g_stop_alert_received);

static void high_alert_cb(void)
{
	SET_FLAG(g_high_alert_received);
}

static void mild_alert_cb(void)
{
	SET_FLAG(g_mild_alert_received);
}

static void no_alert_cb(void)
{
	SET_FLAG(g_stop_alert_received);
}

BT_IAS_CB_DEFINE(ias_callbacks) = {
	.high_alert = high_alert_cb,
	.mild_alert = mild_alert_cb,
	.no_alert = no_alert_cb,
};

static void test_main(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_FLAG(g_high_alert_received);
	LOG_INF("High alert received");

	err = bt_ias_local_alert_stop();
	if (err != 0) {
		FAIL("Failed to locally stop alert: %d\n", err);
		return;
	}
	WAIT_FOR_FLAG(g_stop_alert_received);

	WAIT_FOR_FLAG(g_mild_alert_received);
	LOG_INF("Mild alert received");

	WAIT_FOR_FLAG(g_stop_alert_received);
	LOG_INF("Stop alert received");

	PASS("IAS test passed\n");
}

static void test_two_clients(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth init failed (err %d)\n", err);
		return;
	}

	setup_connectable_adv(&ext_adv);

	WAIT_FOR_FLAG(flag_connected);
	UNSET_FLAG(flag_connected);

	/* The first connection stopped the advertising set */
	err = bt_le_ext_adv_start(ext_adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err != 0) {
		FAIL("Failed to restart advertising (err %d)\n", err);
		return;
	}

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_FLAG(g_high_alert_received);
	LOG_INF("High alert received");

	/* The first client lowers its level to Mild Alert after the second client wrote an
	 * invalid level, which must not count as a request of any level.
	 */
	WAIT_FOR_COND(TEST_FLAG(g_mild_alert_received) || TEST_FLAG(g_stop_alert_received));
	if (TEST_FLAG(g_stop_alert_received)) {
		FAIL("Alert stopped while a client requests Mild Alert\n");
		return;
	}

	PASS("IAS two clients test passed\n");

	/* Both clients passed before the last write reached this device */
	bs_trace_silent_exit(0);
}

static const struct bst_test_instance test_ias[] = {
	{
		.test_id = "ias",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,

	},
	{
		.test_id = "ias_two_clients",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_two_clients,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_ias_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ias);
}

#endif /* CONFIG_BT_IAS */
