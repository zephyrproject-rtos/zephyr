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
#include <zephyr/drivers/bluetooth.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

struct driver_data {
};

#define EXPECTED_ERROR -ENOSYS

static int driver_open(const struct device *dev, bt_hci_recv_t recv)
{
	TC_PRINT("driver: %s\n", __func__);

	/* Indicate that there is no real Bluetooth device */
	return EXPECTED_ERROR;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	return 0;
}

static const struct bt_hci_driver_api driver_api = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst) \
	static struct driver_data driver_data_##inst = { \
	}; \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, NULL, \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

ZTEST_SUITE(test_bluetooth, NULL, NULL, NULL, NULL, NULL);

ZTEST(test_bluetooth, test_bluetooth_entry)
{
	zassert_true((bt_enable(NULL) == EXPECTED_ERROR),
			"bt_enable failed");
}
