/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <ztest.h>

#include <zephyr/bluetooth/bluetooth.h>

void test_ctrl_sw_privacy(void)
{
	zassert_false(bt_enable(NULL), "%s failed", __func__);
}

/* test case main entry */
void test_main(void)
{
	ztest_test_suite(test_bluetooth,
			ztest_unit_test(test_ctrl_sw_privacy));
	ztest_run_test_suite(test_bluetooth);
}
