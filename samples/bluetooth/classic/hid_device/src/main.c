/* main.c - Bluetooth Classic HID keyboard device sample */

/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/classic/hid.h>

#include "report_desc.h"

#define HID_KEY_A 0x04U

struct hid_keyboard_report {
	uint8_t modifiers;
	uint8_t reserved;
	uint8_t keys[6];
} __packed;

static struct bt_hid_device *active_hid;
static struct k_work_delayable key_work;
static bool key_pressed;

static int hid_get_report(struct bt_hid_device *hid, enum bt_hid_report_type type,
			  uint8_t report_id, uint8_t *data, size_t *len)
{
	struct hid_keyboard_report report = {0};

	ARG_UNUSED(hid);
	ARG_UNUSED(report_id);

	if (type != BT_HID_REPORT_INPUT) {
		return -ENOTSUP;
	}

	if (*len < sizeof(report)) {
		return -ENOMEM;
	}

	memcpy(data, &report, sizeof(report));
	*len = sizeof(report);

	return 0;
}

static void hid_connected(struct bt_conn *conn, struct bt_hid_device *hid)
{
	ARG_UNUSED(conn);

	printk("HID control channel connected\n");
	active_hid = hid;
}

static void hid_disconnected(struct bt_hid_device *hid)
{
	ARG_UNUSED(hid);

	printk("HID disconnected\n");
	active_hid = NULL;
	k_work_cancel_delayable(&key_work);
}

static void hid_intr_connected(struct bt_hid_device *hid)
{
	printk("HID interrupt channel connected, starting demo keystrokes\n");
	k_work_schedule(&key_work, K_SECONDS(2));
}

static void send_keyboard_report(struct bt_hid_device *hid,
				 const struct hid_keyboard_report *report)
{
	int err;

	err = bt_hid_device_input_report_send(hid, 0U, (const uint8_t *)report,
					      sizeof(*report));
	if (err != 0) {
		printk("Failed to send HID report (%d)\n", err);
	}
}

static void key_work_handler(struct k_work *work)
{
	struct hid_keyboard_report report = {0};
	struct bt_hid_device *hid = active_hid;

	ARG_UNUSED(work);

	if (hid == NULL) {
		return;
	}

	if (!key_pressed) {
		report.keys[0] = HID_KEY_A;
		key_pressed = true;
		printk("Sending key press ('a')\n");
	} else {
		key_pressed = false;
		printk("Sending key release\n");
	}

	send_keyboard_report(hid, &report);
	k_work_schedule(&key_work, K_SECONDS(2));
}

static struct bt_hid_device_cb hid_cb = {
	.connected = hid_connected,
	.disconnected = hid_disconnected,
	.intr_connected = hid_intr_connected,
	.get_report = hid_get_report,
};

static struct bt_hid_device_register_param hid_param;

static void bt_ready(int err)
{
	if (err != 0) {
		printk("Bluetooth init failed (%d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	hid_param.report_desc = hid_keyboard_report_desc;
	hid_param.report_desc_len = hid_keyboard_report_desc_len;
	hid_param.subclass = BT_HID_DEVICE_SUBCLASS_KEYBOARD;
	hid_param.country_code = 0U;
	hid_param.device_release_number = 0x0100U;
	hid_param.name = CONFIG_BT_DEVICE_NAME;
	hid_param.boot_device = true;
	hid_param.normally_connectable = true;
	hid_param.supervision_timeout = 0x2000U;

	err = bt_hid_device_register(&hid_param, &hid_cb);
	if (err != 0) {
		printk("HID register failed (%d)\n", err);
		return;
	}

	err = bt_br_set_discoverable(true, false);
	if (err != 0) {
		printk("Discoverable mode failed (%d)\n", err);
		return;
	}

	printk("HID keyboard ready and discoverable as \"%s\"\n", CONFIG_BT_DEVICE_NAME);
}

int main(void)
{
	int err;

	k_work_init_delayable(&key_work, key_work_handler);

	err = bt_enable(bt_ready);
	if (err != 0) {
		printk("Bluetooth enable failed (%d)\n", err);
	}

	return 0;
}
