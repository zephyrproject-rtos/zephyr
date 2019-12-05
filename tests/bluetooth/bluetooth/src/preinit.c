/*
 * Copyright (c) 2019 ZhongYao Luo
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

#include <errno.h>
#include <tc_util.h>
#include <ztest.h>

#include <bluetooth/bluetooth.h>
#include <drivers/bluetooth/hci_driver.h>

#define EXPECTED_ERROR -ENOSYS

static int driver_open(void)
{
	TC_PRINT("driver: %s\n", __func__);

	/* Indicate that there is no real Bluetooth device */
	return 0;
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

static int bluetooth_preinit(void *data)
{
	TC_PRINT("preinit: %s\n", __func__);
	return EXPECTED_ERROR;
}

void test_bluetooth_preinit_entry(void)
{
	driver_init();

	zassert_true((bt_enable(NULL, bluetooth_preinit, NULL) == EXPECTED_ERROR),
			"bt_enable failed");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_bluetooth_preinit,
			ztest_unit_test(test_bluetooth_preinit_entry));
	ztest_run_test_suite(test_bluetooth_preinit);
}
