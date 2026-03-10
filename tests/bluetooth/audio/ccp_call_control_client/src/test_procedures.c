/* test_procedures.c - Testing of CCP procedures  */

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
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/util_utf8.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

#include "conn.h"
#include "test_common.h"

struct ccp_call_control_client_procedures_test_suite_fixture {
	struct bt_ccp_call_control_client_cb client_cbs;
	struct bt_ccp_call_control_client *client;
	struct bt_conn conn;

	/* Callback values */
	struct bt_ccp_call_control_client_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT];
	char bearer_name[CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH];
};

static void discover_cb(struct bt_ccp_call_control_client *client, int err,
			struct bt_ccp_call_control_client_bearers *bearers, void *user_data)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = user_data;

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

static void bearer_provider_name_cb(struct bt_ccp_call_control_client_bearer *bearer, int err,
				    const char *name, void *user_data)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = user_data;

	zassert_not_null(bearer);
	zassert_equal(err, 0);
	zassert_not_null(name);
	zassert_not_null(user_data);
	zassert_true(strlen(name) < CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
	zassert_true(strlen(fixture->bearer_name) == 0U); /* expect only a single call */
	utf8_lcpy(fixture->bearer_name, name, CONFIG_BT_TBS_MAX_PROVIDER_NAME_LENGTH);
}

static void *ccp_call_control_client_procedures_test_suite_setup(void)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void ccp_call_control_client_procedures_test_suite_before(void *f)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = f;
	int err;

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);

	fixture->client_cbs.discover = discover_cb;
	fixture->client_cbs.bearer_provider_name = bearer_provider_name_cb;
	fixture->client_cbs.user_data = fixture;

	err = bt_ccp_call_control_client_register_cb(&fixture->client_cbs);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zassert_not_null(fixture->bearers[0]);
}

static void ccp_call_control_client_procedures_test_suite_after(void *f)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = f;

	(void)bt_ccp_call_control_client_unregister_cb(&fixture->client_cbs);
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
}

static void ccp_call_control_client_procedures_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(ccp_call_control_client_procedures_test_suite, NULL,
	    ccp_call_control_client_procedures_test_suite_setup,
	    ccp_call_control_client_procedures_test_suite_before,
	    ccp_call_control_client_procedures_test_suite_after,
	    ccp_call_control_client_procedures_test_suite_teardown);

static ZTEST_F(ccp_call_control_client_procedures_test_suite,
	       test_ccp_call_control_client_read_bearer_provider_name)
{
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(fixture->bearers[0]);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	zassert_true(strlen(fixture->bearer_name) > 0U);
}

static ZTEST_F(ccp_call_control_client_procedures_test_suite,
	       test_ccp_call_control_client_read_bearer_provider_name_inval_null_bearer)
{
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_procedures_test_suite,
	       test_ccp_call_control_client_read_bearer_provider_name_inval_not_discovered)
{
	int err;

	/* Fake disconnection to clear the discovered value for the bearers*/
	mock_bt_conn_disconnected(&fixture->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	/* Mark as connected again but without discovering */
	test_conn_init(&fixture->conn);

	err = bt_ccp_call_control_client_read_bearer_provider_name(fixture->bearers[0]);
	zassert_equal(err, -EFAULT, "Unexpected return value %d", err);
}

static ZTEST_F(ccp_call_control_client_procedures_test_suite,
	       test_ccp_call_control_client_read_bearer_provider_name_inval_bearer)
{
	struct bt_ccp_call_control_client_bearer *invalid_bearer =
		(struct bt_ccp_call_control_client_bearer *)0xdeadbeefU;
	int err;

	err = bt_ccp_call_control_client_read_bearer_provider_name(invalid_bearer);
	zassert_equal(err, -EEXIST, "Unexpected return value %d", err);
}
