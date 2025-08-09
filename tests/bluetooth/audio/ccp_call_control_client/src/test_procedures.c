/* test_procedures.c - Testing of CCP procedures  */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
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
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/ztest_test.h>
#include <zephyr/ztest_assert.h>

#include "conn.h"
#include "ccp_call_control_client.h"
#include "expects_util.h"
#include "test_common.h"

struct ccp_call_control_client_procedures_test_suite_fixture {
	/** Need 1 additional bearer than the max to trigger some corner cases */
	struct bt_ccp_call_control_client_bearer
		*bearers[CONFIG_BT_CCP_CALL_CONTROL_CLIENT_BEARER_COUNT];
	struct bt_ccp_call_control_client *client;
	struct bt_conn conn;
};

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

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
	struct bt_ccp_call_control_client_bearers *bearers;
	size_t i = 0U;
	int err;

	memset(fixture, 0, sizeof(*fixture));
	test_conn_init(&fixture->conn);

	err = bt_ccp_call_control_client_register_cb(&mock_ccp_call_control_client_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_ccp_call_control_client_discover(&fixture->conn, &fixture->client);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_ccp_call_control_client_cb.discover", 1,
			   mock_ccp_call_control_client_discover_cb_fake.call_count);
	zassert_not_null(mock_ccp_call_control_client_discover_cb_fake.arg0_history[0]);
	zassert_equal(0, mock_ccp_call_control_client_discover_cb_fake.arg1_history[0]);
	bearers = mock_ccp_call_control_client_discover_cb_fake.arg2_history[0];
	zassert_not_null(bearers);

#if defined(CONFIG_BT_TBS_CLIENT_GTBS)
	zassert_not_null(bearers->gtbs_bearer);
	fixture->bearers[i++] = bearers->gtbs_bearer;
#endif /* CONFIG_BT_TBS_CLIENT_GTBS */

#if defined(CONFIG_BT_TBS_CLIENT_TBS)
	zassert_equal(CONFIG_BT_TBS_CLIENT_MAX_TBS_INSTANCES, bearers->tbs_count);
	zassert_not_null(bearers->tbs_bearers);
	for (; i < bearers->tbs_count; i++) {
		zassert_not_null(bearers->tbs_bearers[i]);
		fixture->bearers[i] = bearers->gtbs_bearer;
	}
#endif /* CONFIG_BT_TBS_CLIENT_TBS */
}

static void ccp_call_control_client_procedures_test_suite_after(void *f)
{
	struct ccp_call_control_client_procedures_test_suite_fixture *fixture = f;

	(void)bt_ccp_call_control_client_unregister_cb(&mock_ccp_call_control_client_cb);
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

	zexpect_call_count("bt_ccp_call_control_client_cb.bearer_provider_name", 1,
			   mock_ccp_call_control_client_bearer_provider_name_cb_fake.call_count);
	zassert_not_null(mock_ccp_call_control_client_bearer_provider_name_cb_fake
				 .arg0_history[0]); /* bearer */
	zassert_equal(0, mock_ccp_call_control_client_bearer_provider_name_cb_fake
				 .arg1_history[0]); /* err */
	zassert_not_null(mock_ccp_call_control_client_bearer_provider_name_cb_fake
				 .arg2_history[0]); /* name */
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
