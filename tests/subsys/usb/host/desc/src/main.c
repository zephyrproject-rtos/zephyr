/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_device.h"
#include "usbh_host.h"

#include "test_class.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_INF);

#define TEST_DEFAULT_INTERFACE		0
#define TEST_DEFAULT_ALTERNATE		1

USBD_CONFIGURATION_DEFINE(test_fs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200, NULL);

USBD_CONFIGURATION_DEFINE(test_hs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200, NULL);

USBD_DESC_LANG_DEFINE(test_lang);
USBD_DESC_STRING_DEFINE(test_mfg, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(test_product, "Zephyr USB Test", 2);
USBD_DESC_STRING_DEFINE(test_sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(test_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

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

ZTEST(host_class_api, test_class_matching)
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

	/* valid code triple */

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

static void *usb_test_enable(void)
{
	int ret;

	ret = usbh_init(&uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	ret = uhc_bus_reset(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus reset");

	ret = uhc_bus_resume(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus resume");

	ret = uhc_sof_enable(uhs_ctx.dev);
	zassert_ok(ret, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	ret = usbd_add_descriptor(&test_usbd, &test_lang);
	zassert_ok(ret, "Failed to initialize descriptor (%d)", ret);

	ret = usbd_add_descriptor(&test_usbd, &test_mfg);
	zassert_ok(ret, "Failed to initialize descriptor (%d)", ret);

	ret = usbd_add_descriptor(&test_usbd, &test_product);
	zassert_ok(ret, "Failed to initialize descriptor (%d)", ret);

	ret = usbd_add_descriptor(&test_usbd, &test_sn);
	zassert_ok(ret, "Failed to initialize descriptor (%d)", ret);

	ret = usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
	zassert_ok(ret, "Failed to add configuration (%d)", ret);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		ret = usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config);
		zassert_equal(ret, 0, "Failed to add configuration (%d)", ret);
	}

	ret = usbd_register_all_classes(&test_usbd, USBD_SPEED_FS, 1, NULL);
	zassert_ok(ret, "Failed to register all instances (%d)", ret);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		ret = usbd_register_all_classes(&test_usbd, USBD_SPEED_HS, 1, NULL);
		zassert_equal(ret, 0, "Failed to unregister all instances (%d)", ret);
	}

	ret = usbd_init(&test_usbd);
	zassert_ok(ret, "Failed to initialize device support");

	ret = usbd_enable(&test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	LOG_INF("Device support enabled");

	/* Allow the host time to reset the device. */
	k_msleep(200);

	return NULL;
}

static void usb_test_shutdown(void *f)
{
	int ret;

	ret = usbd_disable(&test_usbd);
	zassert_ok(ret, "Failed to enable device support");

	ret = usbd_shutdown(&test_usbd);
	zassert_ok(ret, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	ret = usbh_disable(&uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(host_class_api, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
