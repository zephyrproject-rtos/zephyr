/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh.h>

#include "usbh_ch9.h"
#include "usbh_host.h"
#include "usbh_class_api.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_DBG);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

/* Utility for USB descriptors */
#define _LE16(n) ((n) & 0xff), ((n) >> 8)

/* Private class data, here just an integer but usually a custom struct. */
static struct test_class_priv {
	enum {
		/* Test value stored before the class is initialized */
		TEST_CLASS_PRIV_NOT_INIT,
		/* Test value stored after the class is initialized */
		TEST_CLASS_PRIV_INIT_OK,
	} state;
} test_class_priv = {
	.state = TEST_CLASS_PRIV_NOT_INIT,
};

static int test_class_init(struct usbh_class_data *const c_data,
			   struct usbh_context *const uhs_ctx)
{
	struct test_class_priv *priv = c_data->priv;

	LOG_DBG("initializing %p, priv value 0x%x", c_data, *priv);

	zassert_equal(priv->state, TEST_CLASS_PRIV_NOT_INIT,
		      "Class should be initialized only once");

	priv->state = TEST_CLASS_PRIV_INIT_OK;

	return 0;
}

static int test_class_completion_cb(struct usbh_class_data *const c_data,
				    struct usb_device *const udev,
				    struct uhc_transfer *const xfer)
{
	struct test_class_priv *priv = c_data->priv;

	LOG_DBG("completion callback for %p, transfer %p", c_data, xfer);

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static int test_class_probe(struct usbh_class_data *const c_data,
			    struct usb_device *const udev,
			    void *const desc_beg,
			    void *const desc_end)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static int test_class_removed(struct usbh_class_data *const c_data,
			      struct usb_device *const udev)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static int test_class_rwup(struct usbh_class_data *const c_data,
			   struct usb_device *const udev)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static int test_class_suspended(struct usbh_class_data *const c_data,
			        struct usb_device *const udev)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static int test_class_resumed(struct usbh_class_data *const c_data,
			      struct usb_device *const udev)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_INIT_OK);

	return 0;
}

static struct usbh_class_api test_class_api = {
	.init = &test_class_init,
	.completion_cb = &test_class_completion_cb,
	.probe = &test_class_probe,
	.removed = &test_class_removed,
	.rwup = &test_class_rwup,
	.suspended = &test_class_suspended,
	.resumed = &test_class_resumed,
};

/* Define a class used in the tests */
USBH_DEFINE_CLASS(test_class, &test_class_api, &test_class_priv);

/* Obtained with lsusb then verified against the USB 2.0 standard's sample HUB descriptor */
const uint8_t test_hub_descriptor[] = {
	/* Device Descriptor: */
	18,		/* bLength */
	1,		/* bDescriptorType */
	_LE16(0x0200),	/* bcdUSB */
	0x09,		/* bDeviceClass */
	0x00,		/* bDeviceSubClass */
	0x02,		/* bDeviceProtocol */
	64,		/* bMaxPacketSize0 */
	_LE16(0x0bda),	/* idVendor */
	_LE16(0x5411),	/* idProduct */
	_LE16(0x0001),	/* bcdDevice */
	0,		/* iManufacturer */
	0,		/* iProduct */
	0,		/* iSerial */
	1,		/* bNumConfigurations */

	/* Device Qualifier Descriptor: */
	10,		/* bLength */
	6,		/* bDescriptorType */
	_LE16(0x0200),	/* bcdUSB */
	0x09,		/* bDeviceClass */
	0x00,		/* bDeviceSubClass */
	1,		/* bDeviceProtocol */
	64,		/* bMaxPacketSize0 */
	1,		/* bNumConfigurations */

	/* Configuration Descriptor: */
	9,		/* bLength */
	2,		/* bDescriptorType */
	_LE16(0x0029),	/* wTotalLength */
	1,		/* bNumInterfaces */
	1,		/* bConfigurationValue */
	0,		/* iConfiguration */
	0xe0,		/* bmAttributes */
	0,		/* MaxPower */

	/* Interface Descriptor: */
	9,		/* bLength */
	4,		/* bDescriptorType */
	0,		/* bInterfaceNumber */
	0,		/* bAlternateSetting */
	1,		/* bNumEndpoints */
	9,		/* bInterfaceClass */
	0,		/* bInterfaceSubClass */
	1,		/* bInterfaceProtocol */
	0,		/* iInterface */

	/* Endpoint Descriptor: */
	7,		/* bLength */
	5,		/* bDescriptorType */
	0x81,		/* bEndpointAddress */
	0x03,		/* bmAttributes */
	_LE16(1),	/* wMaxPacketSize */
	12,		/* bInterval */

	/* Interface Descriptor: */
	9,		/* bLength */
	4,		/* bDescriptorType */
	0,		/* bInterfaceNumber */
	1,		/* bAlternateSetting */
	1,		/* bNumEndpoints */
	9,		/* bInterfaceClass */
	0,		/* bInterfaceSubClass */
	2,		/* bInterfaceProtocol */
	0,		/* iInterface */

	/* Endpoint Descriptor: */
	7,		/* bLength */
	5,		/* bDescriptorType */
	0x81,		/* bEndpointAddress */
	0x03,		/* bmAttributes */
	_LE16(1),	/* wMaxPacketSize */
	12,		/* bInterval */
};

ZTEST(host, test_class_fake_device)
{
	zassert_equal(test_class_priv.state, TEST_CLASS_PRIV_INIT_OK,
		      "The class should have been initialized by usbh_init()");
}

static void *usb_test_enable(void)
{
	int ret;

	ret = usbh_init(&uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host (%d)", ret);

	ret = usbh_enable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host (%d)", ret);

	ret = uhc_bus_reset(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus reset (%d)", ret);

	ret = uhc_bus_resume(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus resume (%d)", ret);

	ret = uhc_sof_enable(uhs_ctx.dev);
	zassert_ok(ret, "Failed to enable SoF generator (%d)", ret);

	LOG_INF("Host controller enabled");

	/* Allow the host time to reset the host. */
	k_msleep(200);

	return NULL;
}

static void usb_test_shutdown(void *f)
{
	int ret;

	ret = usbh_disable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable host support (%d)", ret);

	ret = usbh_shutdown(&uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support (%d)", ret);

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(host, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
