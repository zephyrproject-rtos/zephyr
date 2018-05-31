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

#include <usb/bos.h>

/* Helpers */
static void hexdump(const u8_t *data, size_t len)
{
	int n = 0;

	while (len--) {
		if (n % 16 == 0) {
			printk("%08X ", n);
		}

		printk("%02X ", *data++);

		n++;
		if (n % 8 == 0) {
			if (n % 16 == 0) {
				printk("\n");
			} else {
				printk(" ");
			}
		}
	}

	if (n % 16) {
		printk("\n");
	}
}

/*
 * Compare old style USB BOS definition with section aligned
 */

static const u8_t dummy_descriptor[] = {
	0x00, 0x01, 0x02
};

static struct webusb_bos_desc {
	struct usb_bos_descriptor bos;
	struct usb_bos_platform_descriptor platform_webusb;
	struct usb_bos_capability_webusb capability_data_webusb;
	struct usb_bos_platform_descriptor platform_msos;
	struct usb_bos_capability_msos capability_data_msos;
} __packed webusb_bos_descriptor = {
	.bos = {
		.bLength = sizeof(struct usb_bos_descriptor),
		.bDescriptorType = USB_BINARY_OBJECT_STORE_DESC,
		.wTotalLength = sizeof(struct webusb_bos_desc),
		.bNumDeviceCaps = 2,
	},
	/* WebUSB Platform Capability Descriptor:
	 * https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
	 */
	.platform_webusb = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_webusb),
		.bDescriptorType = USB_DEVICE_CAPABILITY_DESC,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/* WebUSB Platform Capability UUID
		 * 3408b638-09a9-47a0-8bfd-a0768815b665
		 */
		.PlatformCapabilityUUID = {
			0x38, 0xB6, 0x08, 0x34,
			0xA9, 0x09,
			0xA0, 0x47,
			0x8B, 0xFD,
			0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
		},
	},
	.capability_data_webusb = {
		.bcdVersion = sys_cpu_to_le16(0x0100),
		.bVendorCode = 0x01,
		.iLandingPage = 0x01
	},
	/* Microsoft OS 2.0 Platform Capability Descriptor
	 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/
	 * microsoft-defined-usb-descriptors
	 * Adapted from the source:
	 * https://github.com/sowbug/weblight/blob/master/firmware/webusb.c
	 * (BSD-2) Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
	 */
	.platform_msos = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DEVICE_CAPABILITY_DESC,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		.PlatformCapabilityUUID = {
			/**
			 * MS OS 2.0 Platform Capability ID
			 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
			 */
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.capability_data_msos = {
		/* Windows version (8.1) (0x06030000) */
		.dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		.wMSOSDescriptorSetTotalLength =
			sys_cpu_to_le16(sizeof(dummy_descriptor)),
		.bMS_VendorCode = 0x02,
		.bAltEnumCode = 0x00
	}
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_webusb {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_webusb cap;
} __packed cap_webusb = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor) +
			sizeof(struct usb_bos_capability_webusb),
		.bDescriptorType = USB_DEVICE_CAPABILITY_DESC,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/* WebUSB Platform Capability UUID
		 * 3408b638-09a9-47a0-8bfd-a0768815b665
		 */
		.PlatformCapabilityUUID = {
			0x38, 0xB6, 0x08, 0x34,
			0xA9, 0x09,
			0xA0, 0x47,
			0x8B, 0xFD,
			0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
		},
	},
	.cap = {
		.bcdVersion = sys_cpu_to_le16(0x0100),
		.bVendorCode = 0x01,
		.iLandingPage = 0x01
	},
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_msosv2 {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed cap_msosv2 = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DEVICE_CAPABILITY_DESC,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		.PlatformCapabilityUUID = {
			/**
			 * MS OS 2.0 Platform Capability ID
			 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
			 */
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		/* Windows version (8.1) (0x06030000) */
		.dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		.wMSOSDescriptorSetTotalLength =
			sys_cpu_to_le16(sizeof(dummy_descriptor)),
		.bMS_VendorCode = 0x02,
		.bAltEnumCode = 0x00,
	},
};

static void test_usb_bos_macros(void)
{
	const struct usb_bos_descriptor *hdr = usb_bos_get_header();
	size_t len = usb_bos_get_length();

	TC_PRINT("length %u\n", len);

	usb_bos_register_cap((void *)&cap_webusb);
	usb_bos_register_cap((void *)&cap_msosv2);

	/* usb_bos_fix_total_length(); corrected with register */

	hexdump((void *)hdr, len);
	hexdump((void *)&webusb_bos_descriptor, len);

	zassert_true(len ==
		     sizeof(struct usb_bos_descriptor) +
		     sizeof(cap_webusb) +
		     sizeof(cap_msosv2),
		     "Incorrect calculated length");
	zassert_true(!memcmp(hdr, &webusb_bos_descriptor, len), "Wrong data");
}

static void test_usb_bos(void)
{
	struct usb_setup_packet setup;
	s32_t len = 0;
	u8_t *data = NULL;
	int ret;

	/* Already registered due to previous test */

	setup.wValue = (DESCRIPTOR_TYPE_BOS & 0xFF) << 8;

	ret = usb_handle_bos(&setup, &len, &data);

	TC_PRINT("%s: ret %d len %u data %p\n", __func__, ret, len, data);

	zassert_true(!ret, "Return code failed");
	zassert_equal(len, sizeof(webusb_bos_descriptor), "Wrong length");
	zassert_true(!memcmp(data, &webusb_bos_descriptor, len),
		     "Wrong data");
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_osdesc,
			 ztest_unit_test(test_usb_bos_macros),
			 ztest_unit_test(test_usb_bos));
	ztest_run_test_suite(test_osdesc);
}
