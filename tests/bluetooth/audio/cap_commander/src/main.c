/* main.c - Application main entry point */

/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "cap_commander.h"
#include "conn.h"
#include "expects_util.h"
#include "test_common.h"

DEFINE_FFF_GLOBALS;

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_init();
}

static void mock_destroy_rule_after(const struct ztest_unit_test *test, void *fixture)
{
	test_mocks_cleanup();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, mock_destroy_rule_after);

struct cap_commander_test_suite_fixture {
	struct bt_conn conns[CONFIG_BT_MAX_CONN];
};

static void cap_commander_test_suite_fixture_init(struct cap_commander_test_suite_fixture *fixture)
{
	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		test_conn_init(&fixture->conns[i]);
	}
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
	struct cap_commander_test_suite_fixture *fixture = f;

	bt_cap_commander_unregister_cb(&mock_cap_commander_cb);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		mock_bt_conn_disconnected(&fixture->conns[i], BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}
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

	for (size_t i = 0; i < ARRAY_SIZE(fixture->conns); i++) {
		err = bt_cap_commander_discover(&fixture->conns[i]);
		zassert_equal(0, err, "Unexpected return value %d", err);
	}

	zexpect_call_count("bt_cap_commander_cb.discovery_complete", ARRAY_SIZE(fixture->conns),
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
