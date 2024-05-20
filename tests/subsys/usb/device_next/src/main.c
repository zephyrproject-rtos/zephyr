/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>

#include "usbh_ch9.h"
#include "usbh_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_INF);

#define TEST_DEFAULT_INTERFACE		0
#define TEST_DEFAULT_ALTERNATE		1

USBD_CONFIGURATION_DEFINE(test_fs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200);

USBD_CONFIGURATION_DEFINE(test_hs_config,
			  USB_SCD_SELF_POWERED | USB_SCD_REMOTE_WAKEUP,
			  200);


USBD_DESC_LANG_DEFINE(test_lang);
USBD_DESC_STRING_DEFINE(test_mfg, "ZEPHYR", 1);
USBD_DESC_STRING_DEFINE(test_product, "Zephyr USB Test", 2);
USBD_DESC_STRING_DEFINE(test_sn, "0123456789ABCDEF", 3);

USBD_DEVICE_DEFINE(test_usbd,
		   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
		   0x2fe3, 0xffff);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

/* Get Configuration request test */
ZTEST(device_next, test_get_configuration)
{
	struct usb_device *udev;
	uint8_t cfg = 0;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	err = usbh_req_get_cfg(udev, &cfg);

	switch (udev->state) {
	case USB_STATE_DEFAULT:
		/* Not specified, expect protocol error */
		zassert_equal(err, -EPIPE, "Transfer status is not a protocol error");
		break;
	case USB_STATE_ADDRESSED:
		/* TODO: Expect zero value */
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_equal(cfg, 0, "Device not in address state");
		break;
	case USB_STATE_CONFIGURED:
		/* TODO: Expect non-zero valid configuration value */
		zassert_equal(err, 0, "Transfer status is an error");
		zassert_not_equal(cfg, 0, "Device not in configured state");
		break;
	default:
		break;
	}
}

/* Set Interface request test */
ZTEST(device_next, test_set_interface)
{
	struct usb_device *udev;
	int err;

	udev = usbh_device_get_any(&uhs_ctx);
	err = usbh_req_set_alt(udev, TEST_DEFAULT_INTERFACE,
			       TEST_DEFAULT_ALTERNATE);

	switch (udev->state) {
	case USB_STATE_DEFAULT:
		/* Not specified, expect protocol error */
	case USB_STATE_ADDRESSED:
		/* Expect protocol error */
		zassert_equal(err, -EPIPE, "Transfer status is not a protocol error");
		break;
	case USB_STATE_CONFIGURED:
		/* TODO */
	default:
		break;
	}
}

static void *usb_test_enable(void)
{
	struct usb_device *udev;
	int err;

	err = usbh_init(&uhs_ctx);
	zassert_equal(err, 0, "Failed to initialize USB host");

	err = usbh_enable(&uhs_ctx);
	zassert_equal(err, 0, "Failed to enable USB host");

	err = uhc_bus_reset(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to signal bus reset");

	err = uhc_bus_resume(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to signal bus resume");

	err = uhc_sof_enable(uhs_ctx.dev);
	zassert_equal(err, 0, "Failed to enable SoF generator");

	LOG_INF("Host controller enabled");

	err = usbd_add_descriptor(&test_usbd, &test_lang);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_mfg);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_product);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	err = usbd_add_descriptor(&test_usbd, &test_sn);
	zassert_equal(err, 0, "Failed to initialize descriptor (%d)", err);

	if (usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config);
		zassert_equal(err, 0, "Failed to add configuration (%d)");
	}

	err = usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config);
	zassert_equal(err, 0, "Failed to add configuration (%d)");

	if (usbd_caps_speed(&test_usbd) == USBD_SPEED_HS) {
		err = usbd_register_class(&test_usbd, "loopback_0", USBD_SPEED_HS, 1);
		zassert_equal(err, 0, "Failed to register loopback_0 class (%d)");
	}

	err = usbd_register_class(&test_usbd, "loopback_0", USBD_SPEED_FS, 1);
	zassert_equal(err, 0, "Failed to register loopback_0 class (%d)");

	err = usbd_init(&test_usbd);
	zassert_equal(err, 0, "Failed to initialize device support");

	err = usbd_enable(&test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	LOG_INF("Device support enabled");
	udev = usbh_device_get_any(&uhs_ctx);
	udev->state = USB_STATE_DEFAULT;

	return NULL;
}

static void usb_test_shutdown(void *f)
{
	int err;

	err = usbd_disable(&test_usbd);
	zassert_equal(err, 0, "Failed to enable device support");

	err = usbd_shutdown(&test_usbd);
	zassert_equal(err, 0, "Failed to shutdown device support");

	LOG_INF("Device support disabled");

	err = usbh_disable(&uhs_ctx);
	zassert_equal(err, 0, "Failed to disable USB host");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(device_next, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
