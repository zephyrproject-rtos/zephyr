/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>
#include <stdint.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_CCP_CALL_CONTROL_CLIENT
LOG_MODULE_REGISTER(ccp_call_control_client, CONFIG_LOG_DEFAULT_LEVEL);

extern enum bst_result_t bst_result;

CREATE_FLAG(flag_discovery_complete);

static struct bt_ccp_call_control_client *inst;

static void ccp_call_control_client_discover_cb(struct bt_ccp_call_control_client *client, int err,
						struct bt_ccp_call_control_client_bearers *bearers)
{
	if (err != 0) {
		FAIL("Failed to discover TBS: %d\n", err);
		return;
	}

	LOG_INF("Discovery completed with %s%u TBS bearers",
		bearers->gtbs_bearer != NULL ? "GTBS and " : "", bearers->tbs_count);

	if (IS_ENABLED(CONFIG_BT_TBS_CLIENT_TBS) && bearers->gtbs_bearer == NULL) {
		FAIL("Failed to discover GTBS");
		return;
	}

	SET_FLAG(flag_discovery_complete);
}

static void discover_tbs(void)
{
	int err;

	UNSET_FLAG(flag_discovery_complete);

	err = bt_ccp_call_control_client_discover(default_conn, &inst);
	if (err) {
		FAIL("Failed to discover TBS: %d", err);
		return;
	}

	WAIT_FOR_FLAG(flag_discovery_complete);
}

static void init(void)
{
	static struct bt_ccp_call_control_client_cb ccp_call_control_client_cbs = {
		.discover = ccp_call_control_client_discover_cb,
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth discover failed (err %d)\n", err);
		return;
	}

	err = bt_ccp_call_control_client_register_cb(&ccp_call_control_client_cbs);
	if (err != 0) {
		FAIL("Failed to register CCP Call Control Client cbs (err %d)\n", err);
		return;
	}
}

static void test_main(void)
{
	struct bt_le_ext_adv *ext_adv;
	int err;

	init();

	setup_connectable_adv(&ext_adv);

	printk("Advertising successfully started\n");

	WAIT_FOR_FLAG(flag_connected);

	discover_tbs();
	discover_tbs(); /* test that we can discover twice */

	err = bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		FAIL("Failed to disconnect: %d\n", err);
	}

	WAIT_FOR_FLAG(flag_disconnected);

	PASS("CCP Call Control Client Passed\n");
}

static const struct bst_test_instance test_ccp_call_control_client[] = {
	{
		.test_id = "ccp_call_control_client",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_ccp_call_control_client_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ccp_call_control_client);
}

#else
struct bst_test_list *test_ccp_call_control_client_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CCP_CALL_CONTROL_CLIENT */
