/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Giancarlo Stasi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/bluetooth/addr.h>

BUILD_ASSERT(IS_ENABLED(CONFIG_BT_HAS_HCI_VS),
	     "This app requires Zephyr-specific HCI vendor extensions");

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LENGTH (sizeof(DEVICE_NAME) - 1)

/* Advertising Interval: the longer, the less energy consumption.
 * Units: 0.625 milliseconds.
 * The Minimum Advertising Interval and Maximum Advertising Interval should not be the same value
 * (as stated in Bluetooth Core Spec 5.2, section 7.8.5)
 */
#define ADV_MIN_INTERVAL        BT_GAP_ADV_SLOW_INT_MIN
#define ADV_MAX_INTERVAL        BT_GAP_ADV_SLOW_INT_MAX

#define ADV_OPTIONS             (BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ)

static uint8_t scan_data[] = {'V', 'S', ' ', 'S', 'a', 'm', 'p', 'l', 'e'};

static const struct bt_le_adv_param parameters = {
	.options = ADV_OPTIONS,
	.interval_min = ADV_MIN_INTERVAL,
	.interval_max = ADV_MAX_INTERVAL,
};

static const struct bt_data adv_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LENGTH),
};

static const struct bt_data scan_rsp_data[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_MANUFACTURER_DATA, scan_data, sizeof(scan_data)),
};

static const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, str, sizeof(str));

	return str;
}

/* Bluetooth specification doesn't allow the scan request event with legacy advertisements.
 * Ref: Bluetooth Core Specification v5.4, section 7.7.65.19 "LE Scan Request Received event" :
 *      "This event shall only be generated if advertising was enabled using the
 *       HCI_LE_Set_Extended_Advertising_Enable command."
 * Added a Vendor Specific command to add this feature and save RAM.
 */
static void enable_legacy_adv_scan_request_event(bool enable)
{
	struct bt_hci_cp_vs_set_scan_req_reports *cp;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_create(BT_HCI_OP_VS_SET_SCAN_REQ_REPORTS, sizeof(*cp));
	if (!buf) {
		printk("%s: Unable to allocate HCI command buffer\n", __func__);
		return;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->enable = (uint8_t) enable;

	err = bt_hci_cmd_send(BT_HCI_OP_VS_SET_SCAN_REQ_REPORTS, buf);
	if (err) {
		printk("Set legacy cb err: %d\n", err);
		return;
	}
}

static bool vs_scanned(struct net_buf_simple *buf)
{
	struct bt_hci_evt_vs_scan_req_rx *evt;
	struct bt_hci_evt_vs *vs;

	vs = net_buf_simple_pull_mem(buf, sizeof(*vs));
	evt = (void *)buf->data;

	printk("%s subevent 0x%02x peer %s rssi %d\n", __func__,
	       vs->subevent, bt_addr_le_str(&evt->addr), evt->rssi);

	return true;
}

static int start_advertising(void)
{
	int err;

	err = bt_hci_register_vnd_evt_cb(vs_scanned);
	if (err) {
		printk("VS user callback register err %d\n", err);
		return err;
	}

	enable_legacy_adv_scan_request_event(true);
	err = bt_le_adv_start(&parameters, adv_data, ARRAY_SIZE(adv_data),
			      scan_rsp_data, ARRAY_SIZE(scan_rsp_data));
	if (err) {
		printk("Start legacy adv err %d\n", err);
		return err;
	}

	printk("Advertising successfully started (%s)\n", CONFIG_BT_DEVICE_NAME);

	return 0;
}

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = start_advertising();

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Vendor-Specific Scan Request sample started\n");
}

int main(void)
{
	int err;

	printk("Starting Vendor-Specific Scan Request sample\n");

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	printk("Main function end, leave stack running for scans\n");

	return 0;
}
