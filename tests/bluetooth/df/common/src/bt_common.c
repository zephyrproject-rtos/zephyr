/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stddef.h>
#include <ztest.h>

#include <bluetooth/bluetooth.h>

#include "bt_common.h"

void *ut_bt_setup(void)
{
	int err;

	/* Initialize bluetooth subsystem. It's okay if it's already on. */
	err = bt_enable(NULL);
	zassert_true((err == 0 || err == -EALREADY),
		"Bluetooth subsystem initialization failed (%d)", err);

	return NULL;
}
