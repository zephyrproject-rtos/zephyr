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
