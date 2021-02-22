/*
 * Copyright (c) 2016-2018 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <init.h>

#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>

#define LOG_LEVEL LOG_LEVEL_DBG
LOG_MODULE_REGISTER(main);

#define REPORT_ID_1	0x01
#define REPORT_ID_2	0x02

static struct k_delayed_work delayed_report_send;

static const struct device *hdev;

#define REPORT_TIMEOUT K_SECONDS(2)

/* Some HID sample Report Descriptor */
static const uint8_t hid_report_desc[] = {
	/* 0x05, 0x01,		USAGE_PAGE (Generic Desktop)		*/
	HID_USAGE_PAGE(HID_USAGE_GEN_DESKTOP),
	/* 0x09, 0x00,		USAGE (Undefined)			*/
	HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
	/* 0xa1, 0x01,		COLLECTION (Application)		*/
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	/* 0x15, 0x00,			LOGICAL_MINIMUM one-byte (0)	*/
	HID_LOGICAL_MIN8(0x00),
	/* 0x26, 0xff, 0x00,		LOGICAL_MAXIMUM two-bytes (255)	*/
	HID_LOGICAL_MAX16(0xFF, 0x00),
	/* 0x85, 0x01,			REPORT_ID (1)			*/
	HID_REPORT_ID(REPORT_ID_1),
	/* 0x75, 0x08,			REPORT_SIZE (8) in bits		*/
	HID_REPORT_SIZE(8),
	/* 0x95, 0x01,			REPORT_COUNT (1)		*/
	HID_REPORT_COUNT(1),
	/* 0x09, 0x00,			USAGE (Undefined)		*/
	HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
	/* v0x81, 0x82,			INPUT (Data,Var,Abs,Vol)	*/
	HID_INPUT(0x02),
	/* 0x85, 0x02,			REPORT_ID (2)			*/
	HID_REPORT_ID(REPORT_ID_2),
	/* 0x75, 0x08,			REPORT_SIZE (8) in bits		*/
	HID_REPORT_SIZE(8),
	/* 0x95, 0x01,			REPORT_COUNT (1)		*/
	HID_REPORT_COUNT(1),
	/* 0x09, 0x00,			USAGE (Undefined)		*/
	HID_USAGE(HID_USAGE_GEN_DESKTOP_UNDEFINED),
	/* 0x91, 0x82,			OUTPUT (Data,Var,Abs,Vol)	*/
	HID_OUTPUT(0x82),
	/* 0xc0			END_COLLECTION			*/
	HID_END_COLLECTION,
};

static void send_report(struct k_work *work)
{
	static uint8_t report_1[2] = { REPORT_ID_1, 0x00 };
	int ret, wrote;

	ret = hid_int_ep_write(hdev, report_1, sizeof(report_1), &wrote);

	LOG_DBG("Wrote %d bytes with ret %d", wrote, ret);

	/* Increment reported data */
	report_1[1]++;
}

static void in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);

	k_delayed_work_submit(&delayed_report_send, REPORT_TIMEOUT);
}

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	switch (status) {
	case USB_DC_CONFIGURED:
		in_ready_cb(hdev);
		break;
	case USB_DC_SOF:
		break;
	default:
		LOG_DBG("status %u unhandled", status);
		break;
	}
}

static void idle_cb(const struct device *dev, uint16_t report_id)
{
	static uint8_t report_1[2] = { 0x00, 0xEB };
	int ret, wrote;

	ret = hid_int_ep_write(dev, report_1, sizeof(report_1), &wrote);

	LOG_DBG("Idle callback: wrote %d bytes with ret %d", wrote, ret);
}

static void protocol_cb(const struct device *dev, uint8_t protocol)
{
	LOG_DBG("New protocol: %s", protocol == HID_PROTOCOL_BOOT ?
		"boot" : "report");
}

static const struct hid_ops ops = {
	.int_in_ready = in_ready_cb,
	.on_idle = idle_cb,
	.protocol_change = protocol_cb,
};

void main(void)
{
	int ret;

	LOG_DBG("Starting application");

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	k_delayed_work_init(&delayed_report_send, send_report);
}

static int composite_pre_init(const struct device *dev)
{
	hdev = device_get_binding("HID_0");
	if (hdev == NULL) {
		LOG_ERR("Cannot get USB HID Device");
		return -ENODEV;
	}

	LOG_DBG("HID Device: dev %p", hdev);

	usb_hid_register_device(hdev, hid_report_desc, sizeof(hid_report_desc),
				&ops);

	return usb_hid_init(hdev);
}

SYS_INIT(composite_pre_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
