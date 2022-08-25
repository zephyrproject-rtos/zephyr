/* bluetooth.c - Bluetooth smoke test */

/*
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <errno.h>
#include <zephyr/tc_util.h>
#include <zephyr/ztest.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/drivers/bluetooth/hci_driver.h>

#define EXPECTED_ERROR -ENOSYS

static int driver_open(void)
{
	TC_PRINT("driver: %s\n", __func__);

	/* Indicate that there is no real Bluetooth device */
	return EXPECTED_ERROR;
}

static int driver_send(struct net_buf *buf)
{
	return 0;
}

static const struct bt_hci_driver drv = {
	.name         = "test",
	.bus          = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open         = driver_open,
	.send         = driver_send,
};

static void driver_init(void)
{
	bt_hci_driver_register(&drv);
}

ZTEST_SUITE(test_bluetooth, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_bluetooth, test_bluetooth_entry)
{
	driver_init();

	zassert_true((bt_enable(NULL) == EXPECTED_ERROR),
			"bt_enable failed");
}
