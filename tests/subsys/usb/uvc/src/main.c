/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_uvc.h>
#include <zephyr/ztest.h>

#include "../../../../../drivers/video/video_common.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

/*
 * Minimal USB Host initialization.
 */

USBH_CONTROLLER_DEFINE(app_usbh,
		       DEVICE_DT_GET(DT_NODELABEL(virtual_uhc0)));

struct usbh_context *app_usbh_init_controller(void)
{
	int err;

	err = usbh_init(&app_usbh);
	if (err) {
		LOG_ERR("Failed to initialize host support");
		return NULL;
	}

	return &app_usbh;
}

USBD_DEVICE_DEFINE(app_usbd, DEVICE_DT_GET(DT_NODELABEL(virtual_udc0)), 0x2fe3, 0x9999);
USBD_DESC_LANG_DEFINE(app_lang);
USBD_DESC_MANUFACTURER_DEFINE(app_mfr, "Nordic");
USBD_DESC_PRODUCT_DEFINE(app_product, "Virtual UVC device");
USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_CONFIGURATION_DEFINE(app_fs_config, 0, 200, &fs_cfg_desc);

/*
 * Minimal USB Device initialization.
 */

struct usbd_context *app_usbd_init_device(void)
{
	int ret;

	ret = usbd_add_descriptor(&app_usbd, &app_lang);
	if (ret != 0) {
		LOG_ERR("Failed to initialize language descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_descriptor(&app_usbd, &app_mfr);
	if (ret != 0) {
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_descriptor(&app_usbd, &app_product);
	if (ret != 0) {
		LOG_ERR("Failed to initialize product descriptor (%d)", ret);
		return NULL;
	}

	ret = usbd_add_configuration(&app_usbd, USBD_SPEED_FS, &app_fs_config);
	if (ret != 0) {
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}

	ret = usbd_register_all_classes(&app_usbd, USBD_SPEED_FS, 1, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to add register classes");
		return NULL;
	}

	usbd_device_set_code_triple(&app_usbd, USBD_SPEED_FS, USB_BCC_MISCELLANEOUS, 0x02, 0x01);

	usbd_self_powered(&app_usbd, USB_SCD_SELF_POWERED);

	ret = usbd_init(&app_usbd);
	if (ret != 0) {
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}

	return &app_usbd;
}

/*
 * Test using this host connected to this device via UVB
 */

const struct video_format test_formats[] = {
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 640, .height = 480},
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 320, .height = 240},
	{.pixelformat = VIDEO_PIX_FMT_YUYV, .width = 160, .height = 120},
};

const struct device *const uvc_dev = DEVICE_DT_GET(DT_NODELABEL(uvc_device));
const struct device *const video_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_camera));

ZTEST(usb_uvc, test_virtual_device_virtual_host)
{
	struct usbd_context *usbd_ctx;
	struct usbh_context *usbh_ctx;
	int ret;

	uvc_set_video_dev(uvc_dev, video_dev);

	for (size_t i = 0; i < ARRAY_SIZE(test_formats); i++) {
		struct video_format fmt = test_formats[i];

		ret = video_estimate_fmt_size(&fmt);
		zassert_ok(ret);

		ret = uvc_add_format(uvc_dev, &fmt);
		zassert_ok(ret);
	}

	usbd_ctx = app_usbd_init_device();
	zassert_not_null(usbd_ctx, "USB device initialization must succeed");

	usbh_ctx = app_usbh_init_controller();
	zassert_not_null(usbh_ctx, "USB host initialization must succeed");

	ret = usbd_enable(usbd_ctx);
	zassert_ok(ret, "USB device error code %d must be 0", ret);

	ret = usbh_enable(usbh_ctx);
	zassert_ok(ret, "USB host enable error code %d must be 0", ret);

	k_sleep(K_MSEC(500));

	/* TODO: test the video devices here. */

	ret = usbh_disable(usbh_ctx);
	zassert_ok(ret, "USB host disable error code %d must be 0", ret);

	ret = usbd_disable(usbd_ctx);
	zassert_ok(ret, "USB device disable error code %d must be 0", ret);

	ret = usbh_shutdown(usbh_ctx);
	zassert_ok(ret, "USB host shutdown error code %d must be 0", ret);

	ret = usbd_shutdown(usbd_ctx);
	zassert_ok(ret, "USB device shutdown error code %d must be 0", ret);
}

ZTEST_SUITE(usb_uvc, NULL, NULL, NULL, NULL, NULL);
