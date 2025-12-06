/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>

#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/fff.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include "cap_handover.h"
#include "test_common.h"

static void cap_handover_callbacks_test_suite_after(void *f)
{
	bt_cap_handover_unregister_cb(&mock_cap_handover_cb);
}

ZTEST_SUITE(cap_handover_callbacks_test_suite, NULL, NULL, NULL,
	    cap_handover_callbacks_test_suite_after, NULL);

static ZTEST(cap_handover_callbacks_test_suite, test_handover_register_cb)
{
	int err;

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST(cap_handover_callbacks_test_suite, test_handover_register_cb_inval_param_null)
{
	int err;

	err = bt_cap_handover_register_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST(cap_handover_callbacks_test_suite, test_handover_register_cb_inval_double_register)
{
	int err;

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(-EALREADY, err, "Unexpected return value %d", err);
}

static ZTEST(cap_handover_callbacks_test_suite, test_handover_unregister_cb)
{
	int err;

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_handover_unregister_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);
}

static ZTEST(cap_handover_callbacks_test_suite, test_handover_unregister_cb_inval_param_null)
{
	int err;

	err = bt_cap_handover_unregister_cb(NULL);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}

static ZTEST(cap_handover_callbacks_test_suite, test_handover_unregister_cb_inval_double_unregister)
{
	int err;

	err = bt_cap_handover_register_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_handover_unregister_cb(&mock_cap_handover_cb);
	zassert_equal(0, err, "Unexpected return value %d", err);

	err = bt_cap_handover_unregister_cb(&mock_cap_handover_cb);
	zassert_equal(-EINVAL, err, "Unexpected return value %d", err);
}
