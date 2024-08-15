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
#include <zephyr/logging/log.h>

#include "bstests.h"
#include "common.h"

#ifdef CONFIG_BT_CCP_CALL_CONTROL_SERVER
LOG_MODULE_REGISTER(ccp_call_control_server, CONFIG_LOG_DEFAULT_LEVEL);

extern enum bst_result_t bst_result;

static struct bt_ccp_call_control_server_bearer
	*bearers[CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT];

CREATE_FLAG(is_connected);
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	LOG_DBG("Connected to %s", addr);

	default_conn = bt_conn_ref(conn);
	SET_FLAG(is_connected);
}

static void init(void)
{
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};

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
		FAIL("Bluetooth enable failed (err %d)\n", err);

		return;
	}

	LOG_DBG("Bluetooth initialized");

	err = bt_conn_cb_register(&conn_callbacks);
	if (err != 0) {
		FAIL("Failed to register conn CBs (err %d)\n", err);

		return;
	}

	err = bt_le_scan_cb_register(&common_scan_cb);
	if (err != 0) {
		FAIL("Failed to register scan CBs (err %d)\n", err);

		return;
	}

	err = bt_ccp_call_control_server_register_bearer(&gtbs_param, &bearers[0]);
	if (err < 0) {
		FAIL("Failed to register GTBS (err %d)\n", err);

		return;
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
			FAIL("Failed to register bearer[%d]: %d\n", i, err);

			return;
		}

		LOG_INF("Registered bearer[%d]", i);
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err != 0) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}
}

static void unregister_bearers(void)
{
	for (int i = 0; i < CONFIG_BT_CCP_CALL_CONTROL_SERVER_BEARER_COUNT; i++) {
		if (bearers[i] != NULL) {
			const int err = bt_ccp_call_control_server_unregister_bearer(bearers[i]);

			if (err < 0) {
				FAIL("Failed to unregister bearer[%d]: %d\n", i, err);

				return;
			}

			LOG_DBG("Unregistered bearer[%d]", i);

			bearers[i] = NULL;
		}
	}
}

static void test_main(void)
{
	init();

	WAIT_FOR_FLAG(flag_connected);

	WAIT_FOR_FLAG(flag_disconnected);

	unregister_bearers();

	PASS("CCP Call Control Server Passed\n");
}

static const struct bst_test_instance test_ccp_call_control_server[] = {
	{
		.test_id = "ccp_call_control_server",
		.test_pre_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_ccp_call_control_server_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_ccp_call_control_server);
}
#else
struct bst_test_list *test_ccp_call_control_server_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_TBS */
