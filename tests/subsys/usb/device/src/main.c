/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>

/* Max packet size for endpoints */
#if defined(CONFIG_USB_DC_HAS_HS_SUPPORT)
#define BULK_EP_MPS		512
#else
#define BULK_EP_MPS		64
#endif

#define ENDP_BULK_IN		0x81

#define VALID_EP		ENDP_BULK_IN
#define INVALID_EP		0x20

struct usb_device_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
} __packed;

#define INITIALIZER_IF							\
	{								\
		.bLength = sizeof(struct usb_if_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE,			\
		.bInterfaceNumber = 0,					\
		.bAlternateSetting = 0,					\
		.bNumEndpoints = 1,					\
		.bInterfaceClass = USB_BCC_VENDOR,			\
		.bInterfaceSubClass = 0,				\
		.bInterfaceProtocol = 0,				\
		.iInterface = 0,					\
	}

#define INITIALIZER_IF_EP(addr, attr, mps, interval)			\
	{								\
		.bLength = sizeof(struct usb_ep_descriptor),		\
		.bDescriptorType = USB_DESC_ENDPOINT,			\
		.bEndpointAddress = addr,				\
		.bmAttributes = attr,					\
		.wMaxPacketSize = sys_cpu_to_le16(mps),			\
		.bInterval = interval,					\
	}

USBD_CLASS_DESCR_DEFINE(primary, 0) struct usb_device_desc dev_desc = {
	.if0 = INITIALIZER_IF,
	.if0_in_ep = INITIALIZER_IF_EP(ENDP_BULK_IN, USB_DC_EP_BULK,
				       BULK_EP_MPS, 0),
};

static void status_cb(struct usb_cfg_data *cfg,
		      enum usb_dc_status_code status,
		      const uint8_t *param)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(status);
	ARG_UNUSED(param);
}

/* EP Bulk IN handler, used to send data to the Host */
static void bulk_in(uint8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data device_ep[] = {
	{
		.ep_cb = bulk_in,
		.ep_addr = ENDP_BULK_IN
	},
};

USBD_DEFINE_CFG_DATA(device_config) = {
	.usb_device_description = NULL,
	.interface_descriptor = &dev_desc.if0,
	.cb_usb_status = status_cb,
	.interface = {
		.vendor_handler = NULL,
		.class_handler = NULL,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(device_ep),
	.endpoint = device_ep,
};

ZTEST(device_usb, test_usb_disable)
{
	zassert_equal(usb_disable(), TC_PASS, "usb_disable() failed");
}

ZTEST(device_usb, test_usb_deconfig)
{
	zassert_equal(usb_deconfig(), TC_PASS, "usb_deconfig() failed");
}

/* Test USB Device Controller API */
ZTEST(device_usb, test_usb_dc_api)
{
	/* Control endpoints are configured */
	zassert_equal(usb_dc_ep_mps(0x0), 64,
		      "usb_dc_ep_mps(0x00) failed");
	zassert_equal(usb_dc_ep_mps(0x80), 64,
		      "usb_dc_ep_mps(0x80) failed");

	/* Bulk EP is not configured yet */
	zassert_equal(usb_dc_ep_mps(ENDP_BULK_IN), 0,
		      "usb_dc_ep_mps(ENDP_BULK_IN) not configured");
}

/* Test USB Device Controller API for invalid parameters */
ZTEST(device_usb, test_usb_dc_api_invalid)
{
	uint32_t size;
	uint8_t byte;

	/* Set stall to invalid EP */
	zassert_not_equal(usb_dc_ep_set_stall(INVALID_EP), TC_PASS,
			  "usb_dc_ep_set_stall(INVALID_EP)");

	/* Clear stall to invalid EP */
	zassert_not_equal(usb_dc_ep_clear_stall(INVALID_EP), TC_PASS,
			  "usb_dc_ep_clear_stall(INVALID_EP)");

	/* Check if the selected endpoint is stalled */
	zassert_not_equal(usb_dc_ep_is_stalled(INVALID_EP, &byte), TC_PASS,
			  "usb_dc_ep_is_stalled(INVALID_EP, stalled)");
	zassert_not_equal(usb_dc_ep_is_stalled(VALID_EP, NULL), TC_PASS,
			  "usb_dc_ep_is_stalled(VALID_EP, NULL)");

	/* Halt invalid EP */
	zassert_not_equal(usb_dc_ep_halt(INVALID_EP), TC_PASS,
			  "usb_dc_ep_halt(INVALID_EP)");

	/* Enable invalid EP */
	zassert_not_equal(usb_dc_ep_enable(INVALID_EP), TC_PASS,
			  "usb_dc_ep_enable(INVALID_EP)");

	/* Disable invalid EP */
	zassert_not_equal(usb_dc_ep_disable(INVALID_EP), TC_PASS,
			  "usb_dc_ep_disable(INVALID_EP)");

	/* Flush invalid EP */
	zassert_not_equal(usb_dc_ep_flush(INVALID_EP), TC_PASS,
			  "usb_dc_ep_flush(INVALID_EP)");

	/* Set callback to invalid EP */
	zassert_not_equal(usb_dc_ep_set_callback(INVALID_EP, NULL), TC_PASS,
			  "usb_dc_ep_set_callback(INVALID_EP, NULL)");

	/* Write to invalid EP */
	zassert_not_equal(usb_dc_ep_write(INVALID_EP, &byte, sizeof(byte),
					  &size),
			  TC_PASS, "usb_dc_ep_write(INVALID_EP)");

	/* Read invalid EP */
	zassert_not_equal(usb_dc_ep_read(INVALID_EP, &byte, sizeof(byte),
					 &size),
			  TC_PASS, "usb_dc_ep_read(INVALID_EP)");
	zassert_not_equal(usb_dc_ep_read_wait(INVALID_EP, &byte, sizeof(byte),
					      &size),
			  TC_PASS, "usb_dc_ep_read_wait(INVALID_EP)");
	zassert_not_equal(usb_dc_ep_read_continue(INVALID_EP), TC_PASS,
			  "usb_dc_ep_read_continue(INVALID_EP)");

	/* Get endpoint max packet size for invalid EP */
	zassert_not_equal(usb_dc_ep_mps(INVALID_EP), TC_PASS,
			  "usb_dc_ep_mps(INVALID_EP)");
}

ZTEST(device_usb, test_usb_dc_api_read_write)
{
	uint32_t size;
	uint8_t byte;

	/* Read invalid EP */
	zassert_not_equal(usb_read(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_read(INVALID_EP)");

	/* Write to invalid EP */
	zassert_not_equal(usb_write(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_write(INVALID_EP)");
}

/* test case main entry */
static void *device_usb_setup(void)
{
	int ret;

	ret = usb_enable(NULL);
	zassert_true(ret == 0, "Failed to enable USB");
	/*
	 *Judge failure whether is due to failing to enable USB.
	 */

	return NULL;
}
ZTEST_SUITE(device_usb, NULL, device_usb_setup, NULL, NULL, NULL);
