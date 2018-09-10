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

#define WPANUSB_SUBCLASS	0
#define WPANUSB_PROTOCOL	0

/* Max packet size for endpoints */
#define WPANUSB_BULK_EP_MPS		64

/* Max Bluetooth command data size */
#define WPANUSB_CLASS_MAX_DATA_SIZE	100

#define WPANUSB_ENDP_BULK_IN		0x81

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
} __packed wpanusb_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_1_1),
		.bDeviceClass = CUSTOM_CLASS,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
		.bMaxPacketSize0 = MAX_PACKET_SIZE0,
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
			.bInterfaceSubClass = WPANUSB_SUBCLASS,
			.bInterfaceProtocol = WPANUSB_PROTOCOL,
			.iInterface = 0,
		},

		/* Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = WPANUSB_ENDP_BULK_IN,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize = sys_cpu_to_le16(WPANUSB_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
};

struct usb_desc_header *__usb_descriptor_start = (void *)&wpanusb_desc;

static void wpanusb_status_cb(enum usb_dc_status_code status, const u8_t *param)
{
}

/* EP Bulk IN handler, used to send data to the Host */
static void wpanusb_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
}

/* Describe EndPoints configuration */
static struct usb_ep_cfg_data wpanusb_ep[] = {
	{
		.ep_cb = wpanusb_bulk_in,
		.ep_addr = WPANUSB_ENDP_BULK_IN
	},
};

static struct usb_cfg_data wpanusb_config = {
	.usb_device_description = (u8_t *)&wpanusb_desc,
	.cb_usb_status = wpanusb_status_cb,
	.interface = {
		.vendor_handler = NULL,
		.class_handler = NULL,
		.custom_handler = NULL,
	},
	.num_endpoints = ARRAY_SIZE(wpanusb_ep),
	.endpoint = wpanusb_ep,
};

static int wpanusb_init(void)
{
	int ret;

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&wpanusb_config);
	zassert_equal(ret, 0, "usb_set_config() failed");

	/* Enable USB driver */
	ret = usb_enable(&wpanusb_config);
	zassert_equal(ret, 0, "usb_enable() failed");

	return 0;
}

static void test_device_setup(void)
{
	int ret;

	ret = wpanusb_init();
	zassert_equal(ret, 0, "init failed");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_device,
			 ztest_unit_test(test_device_setup));

	ztest_run_test_suite(test_device);
}
