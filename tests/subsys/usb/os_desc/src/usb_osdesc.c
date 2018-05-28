/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>

#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <os_desc.h>

#define MSOS_STRING_LENGTH	18
static struct string_desc {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bString[MSOS_STRING_LENGTH - 4];
	u8_t bMS_VendorCode;
	u8_t bPad;
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

static struct usb_os_descriptor os_desc = {
	.string = (u8_t *)&msosv1_string_descriptor,
	.string_len = sizeof(msosv1_string_descriptor),
	.vendor_code = 0x03,
	.compat_id = NULL,
	.compat_id_len = 0,
};

void test_register_osdesc(void)
{
	TC_PRINT("%s\n", __func__);

	usb_register_os_desc(&os_desc);
}

bool test_handle_os_desc(void)
{
	struct usb_setup_packet setup;
	s32_t len = 0;
	u8_t *data = NULL;
	int ret;

	setup.wValue = (DESC_STRING & 0xFF) << 8;
	setup.wValue |= 0xEE;

	ret = usb_handle_os_desc(&setup, &len, &data);

	TC_PRINT("%s: ret %d len %u data %p\n", __func__, ret, len, data);

	zassert_true(!ret, "Return code failed");
	zassert_equal(len, sizeof(msosv1_string_descriptor), "Wrong length");
	zassert_true(!memcmp(data, &msosv1_string_descriptor, len),
		     "Wrong data");

	return true;
}

void test_osdesc_string(void)
{
	test_register_osdesc();
	test_handle_os_desc();

	zassert_true(true, "failed");
}

void test_osdesc_feature(void)
{
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_osdesc,
			 ztest_unit_test(test_osdesc_string),
			 ztest_unit_test(test_osdesc_feature));
	ztest_run_test_suite(test_osdesc);
}
