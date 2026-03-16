/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>
#include <sample_usbd.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_device.h"
#include "usbh_host.h"

#include "usbh_test_common.h"

LOG_MODULE_REGISTER(usbh_test, LOG_LEVEL_INF);

static const struct usbh_class_filter filter_rules_vid_pid[] = {
	{
		.vid = FOO_TEST_VID,
		.pid = FOO_TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_triple[] = {
	{
		.class = FOO_TEST_CLASS,
		.sub = FOO_TEST_SUB,
		.proto = FOO_TEST_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_either[] = {
	{
		.class = FOO_TEST_CLASS,
		.sub = FOO_TEST_SUB,
		.proto = FOO_TEST_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{
		.vid = FOO_TEST_VID,
		.pid = FOO_TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

static const struct usbh_class_filter filter_rules_empty[] = {
	{0},
};

static const struct usbh_class_filter filter_invalid_triple = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID,
};

static const struct usbh_class_filter filter_invalid_vid_triple = {
	.vid = FOO_TEST_VID + 1,
	.pid = FOO_TEST_PID,
};

static const struct usbh_class_filter filter_invalid_pid_triple = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID + 1,
};

static const struct usbh_class_filter filter_valid = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

static const struct usbh_class_filter filter_invalid_vid = {
	.vid = FOO_TEST_VID + 1,
	.pid = FOO_TEST_PID,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

static const struct usbh_class_filter filter_invalid_pid = {
	.vid = FOO_TEST_VID,
	.pid = FOO_TEST_PID + 1,
	.class = FOO_TEST_CLASS,
	.sub = FOO_TEST_SUB,
	.proto = FOO_TEST_PROTO,
};

ZTEST(usbh_test, test_class_matching)
{
	/* Invalid code triple */

	zassert(usbh_class_is_matching(NULL, &filter_invalid_triple),
		"Filtering on NULL rules should match");

	zassert(!usbh_class_is_matching(filter_rules_empty, &filter_invalid_triple),
		"Filtering on empty rules should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_vid_triple),
		"Filtering on invalid VID + invalid code triple should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_pid_triple),
		"Filtering on invalid PID + invalid code triple should not match");

	zassert(usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_triple),
		"Filtering on valid VID:PID + invalid code triple (ignored) should match");

	zassert(!usbh_class_is_matching(filter_rules_triple, &filter_invalid_triple),
		"Filtering on valid VID:PID (ignored) + invalid code triple should not match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_invalid_triple),
		"Filtering on valid VID:PID + invalid code triple should match");

	zassert(!usbh_class_is_matching(filter_rules_either, &filter_invalid_pid_triple),
		"Filtering on invalid VID:PID + invalid code triple should not match");

	zassert(!usbh_class_is_matching(filter_rules_either, &filter_invalid_pid_triple),
		"Filtering on invalid VID:PID + invalid code triple should not match");

	/* Valid code triple */

	zassert(usbh_class_is_matching(filter_rules_vid_pid, &filter_valid),
		"Filtering on valid VID:PID + valid code triple (ignored) should match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_vid),
		"Filtering on invalid VID + valid code triple (ignored) should not match");

	zassert(!usbh_class_is_matching(filter_rules_vid_pid, &filter_invalid_pid),
		"Filtering on invalid PID + valid code triple (ignored) should not match");

	zassert(usbh_class_is_matching(filter_rules_triple, &filter_invalid_pid),
		"Filtering on invalid PID (ignored) + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_triple, &filter_invalid_vid),
		"Filtering on invalid VID (ignored) + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_invalid_pid),
		"Filtering on invalid PID + valid code triple should match");

	zassert(usbh_class_is_matching(filter_rules_either, &filter_valid),
		"Filtering on valid VID:PID + valid code triple should match");
}

ZTEST(usbh_test, test_get_next_desc)
{
	const struct usb_device *udev;
	const struct usb_desc_header *desc;

	udev = usbh_device_get_any(uhs_ctx);
	zassert_not_null(udev);

	desc = udev->cfg_desc;
	zassert_not_null(desc);

	/* #0 cfg */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_CONFIGURATION);
	desc = usbh_desc_get_next(desc);

	/* #1 iad */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE_ASSOC);
	desc = usbh_desc_get_next(desc);

	/* #2 if0 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #3 if0_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #4 if0_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #5 if1 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #6 if1_int_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #7 if1_int_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #8 if2_0 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #9 if2_0_iso_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #10 if2_0_iso_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #11 if2_1 */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	desc = usbh_desc_get_next(desc);

	/* #12 if2_1_iso_in_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #13 if2_1_iso_out_ep */
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	desc = usbh_desc_get_next(desc);

	/* #14 nil_desc */
	zassert_is_null(desc);
}

ZTEST(usbh_test, test_get_types)
{
	const struct usb_device *udev;
	const struct usb_desc_header *desc;

	udev = usbh_device_get_any(uhs_ctx);
	zassert_not_null(udev);

	/* #2 if0 */
	desc = usbh_desc_get_iface(udev, 0);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(((struct usb_if_descriptor *)desc)->bInterfaceNumber, 0);

	/* #3 if0_out_ep */
	desc = usbh_desc_get_endpoint(udev, 0x01);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(((struct usb_ep_descriptor *)desc)->bEndpointAddress, 0x01);

	/* #4 if0_in_ep */
	desc = usbh_desc_get_endpoint(udev, 0x81);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_ENDPOINT);
	zassert_equal(((struct usb_ep_descriptor *)desc)->bEndpointAddress, 0x81);

	/* #5 if1 */
	desc = usbh_desc_get_iface(udev, 1);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE);
	zassert_equal(((struct usb_if_descriptor *)desc)->bInterfaceNumber, 1);
	zassert_equal(((struct usb_if_descriptor *)desc)->bAlternateSetting, 0);
	desc = usbh_desc_get_next_alt_setting(desc);
	zassert_is_null(desc);
}

ZTEST(usbh_test, test_get_next_function)
{
	const struct usb_device *udev;
	const struct usb_desc_header *desc;

	udev = usbh_device_get_any(uhs_ctx);
	zassert_not_null(udev);

	desc = udev->cfg_desc;
	zassert_not_null(desc);

	/* #1 iad */
	desc = usbh_desc_get_next_function(desc);
	zassert_not_null(desc);
	zassert_equal(desc->bDescriptorType, USB_DESC_INTERFACE_ASSOC);

	/* end */
	desc = usbh_desc_get_next_function(desc);
	zassert_is_null(desc);
}

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

struct usbh_context *const uhs_ctx = &test_uhs_ctx;

void *usbh_test_enable(void)
{
	int ret;

	ret = usbh_init(uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_reset(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus reset");

	ret = uhc_bus_resume(uhs_ctx->dev);
	zassert_ok(ret, "Failed to signal bus resume");

	ret = uhc_sof_enable(uhs_ctx->dev);
	zassert_ok(ret, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	test_usbd = sample_usbd_setup_device(NULL);
	zassert_not_null(test_usbd, "Failed to setup USB device");

	ret = usbd_init(test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	return NULL;
}

void usbh_test_shutdown(void *f)
{
	int ret;

	ret = usbd_disable(test_usbd);
	zassert_ok(ret, "Failed to disable device support");

	ret = usbd_shutdown(test_usbd);
	zassert_ok(ret, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	ret = usbh_disable(uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(usbh_test, NULL, usbh_test_enable, NULL, NULL, usbh_test_shutdown);
