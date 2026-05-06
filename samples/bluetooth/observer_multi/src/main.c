/*
 * Copyright (C) 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Multi-controller observer test.
 * Enables two HCI controllers, starts scanning on both simultaneously,
 * then stops scanning.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>

static const struct device *hci1 = DEVICE_DT_GET(DT_ALIAS(bt_hci1));

static void device_found_0(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			   struct net_buf_simple *ad)
{
	printk("[hci0] %s RSSI %d type %u len %u\n", bt_addr_le_str(addr), rssi, type, ad->len);
}

static void device_found_1(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			   struct net_buf_simple *ad)
{
	printk("[hci1] %s RSSI %d type %u len %u\n", bt_addr_le_str(addr), rssi, type, ad->len);
}

int main(void)
{
	int err;

	printk("Multi-controller observer test\n");

	/* Enable default controller */
	err = bt_enable(NULL);
	if (err) {
		printk("bt_enable failed (err %d)\n", err);
		return 0;
	}
	printk("Default controller initialized\n");

	/* Enable second controller */
	err = bt_dev_enable(hci1, NULL);
	if (err) {
		printk("bt_dev_enable failed (err %d)\n", err);
		return 0;
	}
	printk("Second controller initialized\n");

	/* Start scan on hci0 */
	struct bt_le_scan_param param0 = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.hci_dev = NULL, /* default controller */
	};

	err = bt_le_scan_start(&param0, device_found_0);
	if (err) {
		printk("Scan start hci0 failed (err %d)\n", err);
		return 0;
	}
	printk("Scanning on hci0...\n");

	/* Start scan on hci1 */
	struct bt_le_scan_param param1 = {
		.type = BT_LE_SCAN_TYPE_PASSIVE,
		.options = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval = BT_GAP_SCAN_FAST_INTERVAL,
		.window = BT_GAP_SCAN_FAST_WINDOW,
		.hci_dev = hci1,
	};

	err = bt_le_scan_start(&param1, device_found_1);
	if (err) {
		printk("Scan start hci1 failed (err %d)\n", err);
		/* Continue with hci0 only */
	} else {
		printk("Scanning on hci1...\n");
	}

	/* Let both scan for 8 seconds */
	k_sleep(K_SECONDS(8));

	printk("Stopping all scans...\n");
	bt_le_scan_stop();

	/* Disable second controller */
	bt_dev_disable(hci1);
	printk("Second controller disabled\n");

	printk("Test complete\n");
	return 0;
}
