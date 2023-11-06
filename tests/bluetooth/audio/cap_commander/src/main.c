/* main.c - Application main entry point */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/fff.h>

#include "bluetooth.h"
#include "cap_commander.h"
#include "conn.h"
#include "expects_util.h"

DEFINE_FFF_GLOBALS;

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	mock_cap_commander_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	mock_cap_commander_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct cap_commander_test_suite_fixture {
	struct bt_conn conn;
};

static void test_conn_init(struct bt_conn *conn)
{
	conn->index = 0;
	conn->info.type = BT_CONN_TYPE_LE;
	conn->info.role = BT_CONN_ROLE_PERIPHERAL;
	conn->info.state = BT_CONN_STATE_CONNECTED;
	conn->info.security.level = BT_SECURITY_L2;
	conn->info.security.enc_key_size = BT_ENC_KEY_SIZE_MAX;
	conn->info.security.flags = BT_SECURITY_FLAG_OOB | BT_SECURITY_FLAG_SC;
}

static void cap_commander_test_suite_fixture_init(struct cap_commander_test_suite_fixture *fixture)
{
	test_conn_init(&fixture->conn);
}

static void *cap_commander_test_suite_setup(void)
{
	struct cap_commander_test_suite_fixture *fixture;

	fixture = malloc(sizeof(*fixture));
	zassert_not_null(fixture);

	return fixture;
}

static void cap_commander_test_suite_before(void *f)
{
	memset(f, 0, sizeof(struct cap_commander_test_suite_fixture));
	cap_commander_test_suite_fixture_init(f);
}

static void cap_commander_test_suite_after(void *f)
{
	bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
}

static void cap_commander_test_suite_teardown(void *f)
{
	free(f);
}

ZTEST_SUITE(cap_commander_test_suite, NULL, cap_commander_test_suite_setup,
	    cap_commander_test_suite_before, cap_commander_test_suite_after,
	    cap_commander_test_suite_teardown);

ZTEST_F(cap_commander_test_suite, test_commander_register_cb)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_register_cb_inval_param_null)
{
	int err;

	err = bt_cap_commander_register_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_register_cb_inval_double_register)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb_inval_param_null)
{
	int err;

	err = bt_cap_commander_unregister_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_unregister_cb_inval_double_unregister)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_unregister_cb(&mock_cap_commander_cb);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

ZTEST_F(cap_commander_test_suite, test_commander_discover)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_discover(&fixture->conn);
	zassert_equal(0, err, "Unexpected return value %d", err);

	zexpect_call_count("bt_cap_commander_cb.discovery_complete", 1,
			   mock_cap_commander_discovery_complete_cb_fake.call_count);
}

ZTEST_F(cap_commander_test_suite, test_commander_discover_inval_param_null)
{
	int err;

	err = bt_cap_commander_register_cb(&mock_cap_commander_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_commander_discover(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}
