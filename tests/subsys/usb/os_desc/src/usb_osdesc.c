/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <tc_util.h>

#include <sys/byteorder.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <os_desc.h>

#define MSOS_STRING_LENGTH	18
static struct string_desc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[MSOS_STRING_LENGTH - 4];
	uint8_t bMS_VendorCode;
	uint8_t bPad;
} __packed msosv1_string_descriptor = {
	.bLength = MSOS_STRING_LENGTH,
	.bDescriptorType = USB_STRING_DESC,
	/* Signature MSFT100 */
	.bString = {
		'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
		'1', 0x00, '0', 0x00, '0', 0x00
	},
	.bMS_VendorCode = 0x03,	/* Vendor Code, used for a control request */
	.bPad = 0x00,		/* Padding byte for VendorCode look as UTF16 */
};

static struct compat_id_desc {
	/* MS OS 1.0 Header Section */
	uint32_t dwLength;
	uint16_t bcdVersion;
	uint16_t wIndex;
	uint8_t bCount;
	uint8_t Reserved[7];
	/* MS OS 1.0 Function Section */
	struct compat_id_func {
		uint8_t bFirstInterfaceNumber;
		uint8_t Reserved1;
		uint8_t compatibleID[8];
		uint8_t subCompatibleID[8];
		uint8_t Reserved2[6];
	} __packed func[1];
} __packed msosv1_compatid_descriptor = {
	.dwLength = sys_cpu_to_le32(40),
	.bcdVersion = sys_cpu_to_le16(0x0100),
	.wIndex = sys_cpu_to_le16(USB_OSDESC_EXTENDED_COMPAT_ID),
	.bCount = 0x01, /* One function section */
	.Reserved = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},

	.func = {
		{
			.bFirstInterfaceNumber = 0x00,
			.Reserved1 = 0x01,
			.compatibleID = {
				'R', 'N', 'D', 'I', 'S', 0x00, 0x00, 0x00
			},
			.subCompatibleID = {
				'5', '1', '6', '2', '0', '0', '1', 0x00
			},
			.Reserved2 = {
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			}
		},
	}
};

static struct usb_os_descriptor os_desc = {
	.string = (uint8_t *)&msosv1_string_descriptor,
	.string_len = sizeof(msosv1_string_descriptor),
	.vendor_code = 0x03,
	.compat_id = (uint8_t *)&msosv1_compatid_descriptor,
	.compat_id_len = sizeof(msosv1_compatid_descriptor),
};

void test_register_osdesc(void)
{
	TC_PRINT("%s\n", __func__);

	usb_register_os_desc(&os_desc);
}

static void test_handle_os_desc(void)
{
	struct usb_setup_packet setup;
	int32_t len = 0;
	uint8_t *data = NULL;
	int ret;

	setup.wValue = (USB_STRING_DESC & 0xFF) << 8;
	setup.wValue |= USB_OSDESC_STRING_DESC_INDEX;

	ret = usb_handle_os_desc(&setup, &len, &data);

	TC_PRINT("%s: ret %d len %u data %p\n", __func__, ret, len, data);

	zassert_true(!ret, "Return code failed");
	zassert_equal(len, sizeof(msosv1_string_descriptor), "Wrong length");
	zassert_true(!memcmp(data, &msosv1_string_descriptor, len),
		     "Wrong data");
}

static void test_handle_os_desc_feature(void)
{
	struct usb_setup_packet setup;
	int32_t len = 0;
	uint8_t *data = NULL;
	int ret;

	setup.bRequest = os_desc.vendor_code;
	setup.wIndex = USB_OSDESC_EXTENDED_COMPAT_ID;

	ret = usb_handle_os_desc_feature(&setup, &len, &data);

	TC_PRINT("%s: ret %d len %u data %p\n", __func__, ret, len, data);

	zassert_true(!ret, "Return code failed");
	zassert_equal(len, sizeof(msosv1_compatid_descriptor), "Wrong length");
	zassert_true(!memcmp(data, &msosv1_compatid_descriptor, len),
		     "Wrong data");
}

static void test_osdesc_string(void)
{
	test_register_osdesc();
	test_handle_os_desc();
}

static void test_osdesc_feature(void)
{
	test_register_osdesc();
	test_handle_os_desc_feature();
}

/* test case main entry */
void test_main(void)
{
	ztest_test_suite(test_osdesc,
			 ztest_unit_test(test_osdesc_string),
			 ztest_unit_test(test_osdesc_feature));
	ztest_run_test_suite(test_osdesc);
}
