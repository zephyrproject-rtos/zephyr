/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>

#include "bt_common.h"

void *ut_bt_setup(void)
{
	int err;

	/* Initialize bluetooth subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth subsystem initialization failed");

	return NULL;
}

void ut_bt_teardown(void *data)
{
	int err;

	err = bt_disable();
	zassert_equal(err, 0, "Bluetooth subsystem de-initialization failed");
}
