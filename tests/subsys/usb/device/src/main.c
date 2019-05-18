/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>

#include <misc/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>

/* Max packet size for endpoints */
#define BULK_EP_MPS		64

#define ENDP_BULK_IN		0x81

#define VALID_EP		ENDP_BULK_IN
#define INVALID_EP		0x20

static const struct dev_common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor configuration_descr;
	struct usb_device_config {
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_in_ep;
	} __packed device_configuration;
	/*
	 * String descriptors not enabled at the moment
	 */
} __packed desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_1_1),
		.bDeviceClass = CUSTOM_CLASS,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = USB_MAX_CTRL_MPS,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 0,
		.iProduct = 0,
		.iSerialNumber = 0,
		.bNumConfigurations = 1,
	},

	/* Configuration descriptor */
	.configuration_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		.wTotalLength = sizeof(struct dev_common_descriptor)
			      - sizeof(struct usb_device_descriptor),
		.bNumInterfaces = 1,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},

	/* Device configuration */
	.device_configuration = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = 0,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = CUSTOM_CLASS,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},

		/* Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = ENDP_BULK_IN,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
};

struct usb_desc_header *__usb_descriptor_start = (void *)&desc;

static void status_cb(struct usb_cfg_data *cfg,
		      enum usb_dc_status_code status,
		      const u8_t *param)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(status);
	ARG_UNUSED(param);
}

/* EP Bulk IN handler, used to send data to the Host */
static void bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data device_ep[] = {
	{
		.ep_cb = bulk_in,
		.ep_addr = ENDP_BULK_IN
	},
};

static struct usb_cfg_data device_config = {
	.usb_device_description = (u8_t *)&desc,
	.cb_usb_status = status_cb,
	.interface = {
		.vendor_handler = NULL,
		.class_handler = NULL,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(device_ep),
	.endpoint = device_ep,
};

static int device_init(void)
{
	int ret;

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&device_config);
	zassert_equal(ret, TC_PASS, "usb_set_config() failed");

	/* Enable USB driver */
	ret = usb_enable(&device_config);
	zassert_equal(ret, TC_PASS, "usb_enable() failed");

	return ret;
}

static void test_usb_setup(void)
{
	zassert_equal(device_init(), TC_PASS, "init failed");
}

static void test_usb_disable(void)
{
	zassert_equal(usb_disable(), TC_PASS, "usb_disable() failed");
}

static void test_usb_deconfig(void)
{
	zassert_equal(usb_deconfig(), TC_PASS, "usb_deconfig() failed");
}

/* Test USB Device Cotnroller API */
static void test_usb_dc_api(void)
{
	/* Control endpoins are configured */
	zassert_equal(usb_dc_ep_mps(0x0), 64,
		      "usb_dc_ep_mps(0x00) failed");
	zassert_equal(usb_dc_ep_mps(0x80), 64,
		      "usb_dc_ep_mps(0x80) failed");

	/* Bulk EP is not configured yet */
	zassert_equal(usb_dc_ep_mps(ENDP_BULK_IN), 0,
		      "usb_dc_ep_mps(ENDP_BULK_IN) not configured");

	zassert_equal(usb_dc_set_address(0x01), TC_PASS,
		      "usb_dc_set_address(0x01)");
}

/* Test USB Device Cotnroller API for invalid parameters */
static void test_usb_dc_api_invalid(void)
{
	size_t size;
	u8_t byte;

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

static void test_usb_dc_api_read_write(void)
{
	size_t size;
	u8_t byte;

	/* Read invalid EP */
	zassert_not_equal(usb_read(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_read(INVALID_EP)");

	/* Write to invalid EP */
	zassert_not_equal(usb_write(INVALID_EP, &byte, sizeof(byte), &size),
			  TC_PASS, "usb_write(INVALID_EP)");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_device,
			 /* Test API for not USB attached state */
			 ztest_unit_test(test_usb_dc_api_invalid),
			 ztest_unit_test(test_usb_disable),
			 ztest_unit_test(test_usb_setup),
			 ztest_unit_test(test_usb_dc_api),
			 ztest_unit_test(test_usb_dc_api_read_write),
			 ztest_unit_test(test_usb_dc_api_invalid),
			 ztest_unit_test(test_usb_deconfig),
			 ztest_unit_test(test_usb_disable));

	ztest_run_test_suite(test_device);
}
