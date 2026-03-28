/* test_callback_register.c - Test bt_bap_broadcast_source_register and unregister */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/fff.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "bap_broadcast_source.h"

#define FFF_GLOBALS

static void mock_init_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	mock_bap_broadcast_source_init();
}

ZTEST_RULE(mock_rule, mock_init_rule_before, NULL);

static void bap_broadcast_source_test_cb_register_suite_after(void *f)
{
	bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
}

ZTEST_SUITE(bap_broadcast_source_test_cb_register_suite, NULL, NULL, NULL,
	    bap_broadcast_source_test_cb_register_suite_after, NULL);

static ZTEST(bap_broadcast_source_test_cb_register_suite, test_broadcast_source_register_cb)
{
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST(bap_broadcast_source_test_cb_register_suite,
	     test_broadcast_source_register_cb_inval_param_null)
{
	int err;

	err = bt_bap_broadcast_source_register_cb(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(bap_broadcast_source_test_cb_register_suite,
	     test_broadcast_source_register_cb_inval_double_register)
{
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, -EEXIST, "Unexpected return value %d", err);
}

static ZTEST(bap_broadcast_source_test_cb_register_suite, test_broadcast_source_unregister_cb)
{
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);
}

static ZTEST(bap_broadcast_source_test_cb_register_suite,
	     test_broadcast_source_unregister_cb_inval_param_null)
{
	int err;

	err = bt_bap_broadcast_source_unregister_cb(NULL);
	zassert_equal(err, -EINVAL, "Unexpected return value %d", err);
}

static ZTEST(bap_broadcast_source_test_cb_register_suite,
	     test_broadcast_source_unregister_cb_inval_double_unregister)
{
	int err;

	err = bt_bap_broadcast_source_register_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, 0, "Unexpected return value %d", err);

	err = bt_bap_broadcast_source_unregister_cb(&mock_bap_broadcast_source_cb);
	zassert_equal(err, -ENOENT, "Unexpected return value %d", err);
}
