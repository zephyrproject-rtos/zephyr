/*
 * Copyright (c) 2016-2018 Intel Corporation.
 * Copyright (c) 2018-2021 Nordic Semiconductor ASA
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

static const struct device *hdev;
static struct k_work report_send;
static ATOMIC_DEFINE(hid_ep_in_busy, 1);

#define HID_EP_BUSY_FLAG	0
#define REPORT_PERIOD		K_SECONDS(2)

static struct report {
	uint8_t id;
	uint8_t value;
} __packed report_1 = {
	.id = REPORT_ID_1,
	.value = 0,
};

static void report_event_handler(struct k_timer *dummy);
static K_TIMER_DEFINE(event_timer, report_event_handler, NULL);

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
	int ret, wrote;

	if (!atomic_test_and_set_bit(hid_ep_in_busy, HID_EP_BUSY_FLAG)) {
		ret = hid_int_ep_write(hdev, (uint8_t *)&report_1,
				       sizeof(report_1), &wrote);
		if (ret != 0) {
			/*
			 * Do nothing and wait until host has reset the device
			 * and hid_ep_in_busy is cleared.
			 */
			LOG_ERR("Failed to submit report");
		} else {
			LOG_DBG("Report submitted");
		}
	} else {
		LOG_DBG("HID IN endpoint busy");
	}
}

static void int_in_ready_cb(const struct device *dev)
{
	ARG_UNUSED(dev);
	if (!atomic_test_and_clear_bit(hid_ep_in_busy, HID_EP_BUSY_FLAG)) {
		LOG_WRN("IN endpoint callback without preceding buffer write");
	}
}

/*
 * On Idle callback is available here as an example even if actual use is
 * very limited. In contrast to report_event_handler(),
 * report value is not incremented here.
 */
static void on_idle_cb(const struct device *dev, uint16_t report_id)
{
	LOG_DBG("On idle callback");
	k_work_submit(&report_send);
}

static void report_event_handler(struct k_timer *dummy)
{
	/* Increment reported data */
	report_1.value++;
	k_work_submit(&report_send);
}

static void protocol_cb(const struct device *dev, uint8_t protocol)
{
	LOG_DBG("New protocol: %s", protocol == HID_PROTOCOL_BOOT ?
		"boot" : "report");
}

static const struct hid_ops ops = {
	.int_in_ready = int_in_ready_cb,
	.on_idle = on_idle_cb,
	.protocol_change = protocol_cb,
};

static void status_cb(enum usb_dc_status_code status, const uint8_t *param)
{
	switch (status) {
	case USB_DC_CONFIGURED:
		int_in_ready_cb(hdev);
		break;
	case USB_DC_SOF:
		break;
	default:
		LOG_DBG("status %u unhandled", status);
		break;
	}
}

void main(void)
{
	int ret;

	LOG_DBG("Starting application");

	ret = usb_enable(status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	k_work_init(&report_send, send_report);
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

	atomic_set_bit(hid_ep_in_busy, HID_EP_BUSY_FLAG);
	k_timer_start(&event_timer, REPORT_PERIOD, REPORT_PERIOD);

	return usb_hid_init(hdev);
}

SYS_INIT(composite_pre_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
