/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>

ZTEST_SUITE(test_bluetooth, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_bluetooth, test_ctrl_sw_privacy)
{
	zassert_false(bt_enable(NULL), "%s failed", __func__);
}
