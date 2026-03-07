/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>

#include "usbh_device.h"

LOG_MODULE_REGISTER(udc_test, LOG_LEVEL_INF);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

ZTEST(usbh_test, test_device)
{
	struct usb_device *udev;
	int ret;

	ret = usbh_init(&uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host");

	ret = usbh_enable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host");

	LOG_INF("Host controller enabled");

	/* Give time for any USB Host stack operation to complete */
	k_sleep(K_MSEC(500));

	udev = usbh_device_get_any(&uhs_ctx);
	zassert_not_null(udev, "Expecting a device to be connected and detected");
	zassert_not_equal(udev->addr, 0, "Could not complete enumeration up to SET_ADDRESS");
	zassert_not_null(udev->cfg_desc, "Could not read configuration descriptor header");
	zassert_equal(udev->state, USB_STATE_CONFIGURED, "Could not complete configuration");

	ret = usbh_disable(&uhs_ctx);
	zassert_ok(ret, "Failed to disable USB host");

	ret = usbh_shutdown(&uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support");

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(usbh_test, NULL, NULL, NULL, NULL, NULL);
