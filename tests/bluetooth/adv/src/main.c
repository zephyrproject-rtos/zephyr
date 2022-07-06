/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include <ztest.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define TIMEOUT_MS 300000 /* 5 minutes */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

ZTEST_SUITE(adv_tests, NULL, NULL, NULL, NULL, NULL);

ZTEST(adv_tests, test_adv_fast_ad_data_update)
{
	int err;

	printk("Starting Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	zassert_equal(err, 0, "Bluetooth init failed (err %d)\n", err);

	printk("Bluetooth initialized\n");

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	zassert_equal(err, 0, "Advertising failed to start (err %d)\n", err);

	printk("Advertising started\n");

	while (k_uptime_get() < TIMEOUT_MS) {
		err = bt_le_adv_update_data(ad, ARRAY_SIZE(ad),
					    sd, ARRAY_SIZE(sd));
		zassert_equal(err, 0, "Update adv data failed (err %d)\n", err);
	}
}
