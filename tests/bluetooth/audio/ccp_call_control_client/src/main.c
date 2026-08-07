/* main.c - Application main entry point */

/*
 * Copyright (c) 2024-2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/ccp.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

struct ccp_call_control_client_test_suite_fixture {
	struct bt_ccp_call_control_client_cb client_cbs;
	struct bt_ccp_call_control_client *client;
	struct bt_conn conn;

	/* Callback values */
	/** Need 1 additional bearer than the max to trigger some corner cases */
	struct bt_ccp_call_control_client_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT + 1];
};

static void discover_cb(struct bt_ccp_call_control_client *client, int err,
			struct bt_ccp_call_control_client_bearers *bearers, void *user_data)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = user_data;

	zassert_not_null(client);
	zassert_equal(err, 0);
	zassert_not_null(bearers);
	zassert_not_null(user_data);
	zassert_is_null(fixture->bearers[0]); /* expect only a single call */

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	zassert_not_null(bearers->gtbs_bearer);
	fixture->bearers[0] = bearers->gtbs_bearer;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	zassert_equal(CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES, bearers->tbs_count);
	zassert_not_null(bearers->tbs_bearers);
	for (size_t i = 0U; i < bearers->tbs_count; i++) {
		zassert_not_null(bearers->tbs_bearers[i]);
		fixture->bearers[i + IS_ENABLED(CONFIG_BT_TBS_CLIENT_GTBS)] =
			bearers->tbs_bearers[i];
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static void *ccp_call_control_client_test_suite_setup(void)
{
	struct ccp_call_control_client_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void ccp_call_control_client_test_suite_before(void *f)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = f;

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);

	fixture->client_cbs.discover = discover_cb;
	fixture->client_cbs.user_data = fixture;
}

static void ccp_call_control_client_test_suite_after(void *f)
{
	struct ccp_call_control_client_test_suite_fixture *fixture = f;

	(void)bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void ccp_call_control_client_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(ccp_call_control_client_test_suite, NULL, ccp_call_control_client_test_suite_setup,
	    ccp_call_control_client_test_suite_before, ccp_call_control_client_test_suite_after,
	    ccp_call_control_client_test_suite_teardown);

static ZTEST_F(ccp_call_control_client_test_suite, test_ccp_call_control_client_register_cb)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_register_cb_inval_param_null)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_register_cb_inval_double_register)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(-EEXIST, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite, test_ccp_call_control_client_unregister_cb)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_unregister_cb_inval_param_null)
{
	int err;

	err = bt_ccp_call_control_client_unregister_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_unregister_cb_inval_double_unregister)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite, test_ccp_call_control_client_discover)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_discover_inval_param_null_conn)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(NULL, &fixture->client);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite,
	       test_ccp_call_control_client_discover_inval_param_null_client)
{
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_test_suite, test_ccp_call_control_client_get_bearers)
{
	struct bt_ccp_call_control_client_bearers bearers;
	int err;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_get_bearers(fixture->client, &bearers);
	zassert_equal(0, err, "Unexpected return value %d", err);

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	zassert_not_null(bearers.gtbs_bearer);
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	zassert_equal(CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES, bearers.tbs_count);
	zassert_not_null(bearers.tbs_bearers);
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}
