/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_uvc.h>
#include <zephyr/ztest.h>
#include <sample_usbd.h>

#include "../../../../../drivers/video/video_common.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

const struct video_format test_formats[] = {
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 640, .height = 480},
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 320, .height = 240},
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 160, .height = 120},
};

const struct device *const uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc_device));
const struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

ZTEST(uvc_test, test_virtual_device_virtual_host)
{
	const struct device *uvc_dev;

	uvc_dev = device_get_binding("usbh_uvc_0");
	zassert_not_null(uvc_dev, "No USB host UVC instance available");
	LOG_INF("%s", uvc_dev->name);

	/* TODO: test the video devices here. */
}

static struct usbd_context *test_usbd;

USBH_CONTROLLER_DEFINE(test_uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

struct usbh_context *const uhs_ctx = &test_uhs_ctx;

void *uvc_test_enable(void)
{
	int ret;

	uvc_set_video_dev(uvc_dev, video_dev);

	for (size_t i = 0; i < ARRAY_SIZE(test_formats); i++) {
		struct video_format fmt = test_formats[i];

		ret = video_estimate_fmt_size(&fmt);
		zassert_ok(ret);

		ret = uvc_add_format(uvc_dev, &fmt);
		zassert_ok(ret);
	}

	k_sleep(K_MSEC(500));

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

void uvc_test_shutdown(void *f)
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

ZTEST_SUITE(uvc_test, NULL, uvc_test_enable, NULL, NULL, uvc_test_shutdown);
