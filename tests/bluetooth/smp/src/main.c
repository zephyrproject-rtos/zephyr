/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <../subsys/bluetooth/host/smp.h>

ZTEST_SUITE(test_smp, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_smp, test_bt_smp_err_to_str)
{
	/* Test a couple of entries */
	zassert_str_equal(bt_smp_err_to_str(0x00),
			  "BT_SMP_ERR_SUCCESS");
	zassert_str_equal(bt_smp_err_to_str(0x0a),
			  "BT_SMP_ERR_INVALID_PARAMS");
	zassert_str_equal(bt_smp_err_to_str(0x0F),
			  "BT_SMP_ERR_KEY_REJECTED");

	/* Test entries that are not used */
	zassert_mem_equal(bt_smp_err_to_str(0x10),
			  "(unknown)", strlen("(unknown)"));
	zassert_mem_equal(bt_smp_err_to_str(0xFF),
			  "(unknown)", strlen("(unknown)"));

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		zassert_not_null(bt_smp_err_to_str(i), ": %d", i);
	}
}

ZTEST(test_smp, test_bt_security_err_to_str)
{
	/* Test a couple of entries */
	zassert_str_equal(bt_security_err_to_str(BT_SECURITY_ERR_AUTH_FAIL),
			  "BT_SECURITY_ERR_AUTH_FAIL");
	zassert_str_equal(bt_security_err_to_str(BT_SECURITY_ERR_KEY_REJECTED),
			  "BT_SECURITY_ERR_KEY_REJECTED");
	zassert_str_equal(bt_security_err_to_str(BT_SECURITY_ERR_UNSPECIFIED),
			  "BT_SECURITY_ERR_UNSPECIFIED");

	/* Test outside range */
	zassert_str_equal(bt_security_err_to_str(BT_SECURITY_ERR_UNSPECIFIED + 1),
			  "(unknown)");

	for (uint16_t i = 0; i <= UINT8_MAX; i++) {
		zassert_not_null(bt_security_err_to_str(i), ": %d", i);
	}
}

static void auth_cb_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)bt_conn_auth_cb_register(NULL);
}

ZTEST_SUITE(test_auth_cb_register, NULL, NULL, NULL, auth_cb_after, NULL);

static const struct bt_conn_auth_cb auth_cb_a;
static const struct bt_conn_auth_cb auth_cb_b;

ZTEST(test_auth_cb_register, test_register_same_cb_returns_eexist)
{
	zassert_ok(bt_conn_auth_cb_register(&auth_cb_a));
	zassert_equal(bt_conn_auth_cb_register(&auth_cb_a), -EEXIST);
}

ZTEST(test_auth_cb_register, test_register_other_cb_returns_ealready)
{
	zassert_ok(bt_conn_auth_cb_register(&auth_cb_a));
	zassert_equal(bt_conn_auth_cb_register(&auth_cb_b), -EALREADY);
}

ZTEST(test_auth_cb_register, test_register_after_unregister)
{
	zassert_ok(bt_conn_auth_cb_register(&auth_cb_a));
	zassert_ok(bt_conn_auth_cb_register(NULL));
	zassert_ok(bt_conn_auth_cb_register(&auth_cb_b));
}
