/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh.h>

#include "usbh_ch9.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_host.h"
#include "usbh_class.h"

#include "test_descriptor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_DBG);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

#define TEST_VID 0x2FE3
#define TEST_PID 0x0002

static const uint8_t test_hub_descriptor[] = {
	TEST_HUB_DESCRIPTOR
};

struct usb_device test_udev = {
	.cfg_desc = (void *)test_hub_descriptor,
};

/* Private class data, here just an integer but usually a custom struct. */
struct test_class_priv {
	enum {
		/* Test value stored before the class is initialized */
		TEST_CLASS_PRIV_INACTIVE,
		/* Test value stored after the class is initialized */
		TEST_CLASS_PRIV_IDLE,
		/* Test value stored after the class is probed */
		TEST_CLASS_PRIV_ENABLED,
		/* Test value stored after the class is initialized */
		TEST_CLASS_PRIV_INITIALIZED,
		/* Test value stored after the class is suspended */
		TEST_CLASS_PRIV_SUSPENDED,
	} state;
};

static struct test_class_priv test_class_priv = {
	.state = TEST_CLASS_PRIV_INACTIVE,
};

static int test_class_init(struct usbh_class_data *const c_data,
			   struct usbh_context *const uhs_ctx)
{
	struct test_class_priv *priv = c_data->priv;

	LOG_DBG("initializing %p, priv value 0x%x", c_data, *priv);

	zassert_equal(priv->state, TEST_CLASS_PRIV_INACTIVE,
		      "Class should be initialized only once");

	priv->state = TEST_CLASS_PRIV_IDLE;

	return 0;
}

static int test_class_completion_cb(struct usbh_class_data *const c_data,
				    struct uhc_transfer *const xfer)
{
	struct test_class_priv *priv = c_data->priv;

	LOG_DBG("completion callback for %p, transfer %p", c_data, xfer);

	zassert_equal(priv->state, TEST_CLASS_PRIV_ENABLED);

	return -ENOTSUP;
}

static int test_class_probe(struct usbh_class_data *const c_data,
			    struct usb_device *const udev,
			    const uint8_t iface)
{
	struct test_class_priv *priv = c_data->priv;
	const struct usb_desc_header *desc = (const struct usb_desc_header *)test_hub_descriptor;
	const void *const desc_end = test_hub_descriptor + sizeof(test_hub_descriptor);
	const struct usb_if_descriptor *if_desc;

	zassert_equal(priv->state, TEST_CLASS_PRIV_IDLE);

	desc = usbh_desc_get_by_iface(desc, desc_end, iface);
	if (desc == NULL) {
		return -ENOENT;
	}

	if (desc->bDescriptorType != USB_DESC_INTERFACE) {
		return -ENOTSUP;
	}

	if_desc = (const struct usb_if_descriptor *)desc;
	if (if_desc->bInterfaceClass != USB_HUB_CLASSCODE ||
	    if_desc->bInterfaceSubClass != 0x00) {
		return -ENOTSUP;
	}

	priv->state = TEST_CLASS_PRIV_ENABLED;

	return 0;
}

static int test_class_removed(struct usbh_class_data *const c_data)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_ENABLED);

	priv->state = TEST_CLASS_PRIV_IDLE;

	return 0;
}

static int test_class_suspended(struct usbh_class_data *const c_data)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_ENABLED);

	priv->state = TEST_CLASS_PRIV_SUSPENDED;

	return 0;
}

static int test_class_resumed(struct usbh_class_data *const c_data)
{
	struct test_class_priv *priv = c_data->priv;

	zassert_equal(priv->state, TEST_CLASS_PRIV_SUSPENDED);

	priv->state = TEST_CLASS_PRIV_ENABLED;

	return 0;
}

static struct usbh_class_api test_class_api = {
	.init = &test_class_init,
	.completion_cb = &test_class_completion_cb,
	.probe = &test_class_probe,
	.removed = &test_class_removed,
	.suspended = &test_class_suspended,
	.resumed = &test_class_resumed,
};

struct usbh_class_filter test_filters[] = {
	{
		.class = 9,
		.sub = 0,
		.proto = 1,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{
		.vid = TEST_VID,
		.pid = TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

struct usbh_class_filter test_empty_filters[] = {
	{0},
};

/* Define a class used in the tests */
USBH_DEFINE_CLASS(test_class, &test_class_api, &test_class_priv, test_filters);

ZTEST(host_class, test_class_matchiing)
{
	struct usb_device *udev = &test_udev;
	const struct usb_desc_header *desc = usbh_desc_get_cfg(udev);
	const void *const desc_end = usbh_desc_get_cfg_end(udev);
	struct usbh_class_filter info_ok = {
		.vid = TEST_VID,
		.pid = TEST_PID,
	};
	struct usbh_class_filter info_wrong_vid = {
		.vid = TEST_VID + 1,
		.pid = TEST_PID,
	};
	struct usbh_class_filter info_wrong_pid = {
		.vid = TEST_VID,
		.pid = TEST_PID + 1,
	};
	int ret;

	zassert(usbh_class_is_matching(test_filters, &info_ok),
		"Filtering on valid VID:PID should match");

	zassert(usbh_class_is_matching(NULL, &info_ok),
		"Filtering on NULL rules should match");

	zassert(!usbh_class_is_matching(test_empty_filters, &info_ok),
		"Filtering on NULL rules should not match");

	zassert(!usbh_class_is_matching(test_filters, &info_wrong_vid),
		"Filtering on invalid VID only should not match");

	zassert(!usbh_class_is_matching(test_filters, &info_wrong_pid),
		"Filtering on invalid PID only should not match");

	desc = usbh_desc_get_next_function(desc, desc_end);
	zassert_not_null(desc, "There should be at least a function descriptor");

	ret = usbh_desc_get_iface_info(desc, &info_ok, 0);
	zassert_ok(ret, "Expecting the class info to be found");

	ret = usbh_desc_get_iface_info(desc, &info_wrong_pid, 0);
	zassert_ok(ret, "Expecting the class info to be found");

	ret = usbh_desc_get_iface_info(desc, &info_wrong_vid, 0);
	zassert_ok(ret, "Expecting the class info to be found");

	zassert(usbh_class_is_matching(test_filters, &info_ok),
		"Filteringon valid VID:PID and code triple should match");

	zassert(usbh_class_is_matching(test_filters, &info_wrong_vid),
		"Filtering on code triple only should match");

	zassert(usbh_class_is_matching(test_filters, &info_wrong_pid),
		"Filtering on code triple only should match");
}

ZTEST(host_class, test_class_fake_device)
{
	struct usbh_class_data *c_data = test_class.c_data;
	struct usb_device *udev = &test_udev;
	struct test_class_priv *priv = c_data->priv;
	int ret;

	zassert_equal(priv->state, TEST_CLASS_PRIV_IDLE,
		      "The class should have been initialized by usbh_init()");

	ret = usbh_class_probe(c_data, udev, 1);
	zassert_equal(ret, -ENOENT, "There is no interface 1 so should be rejected");
	zassert_equal(priv->state, TEST_CLASS_PRIV_IDLE,
		      "The class should not be enabled if probing failed");

	ret = usbh_class_probe(c_data, udev, 0);
	zassert_ok(ret, "The interface 0 should match in this example class (%s)", strerror(-ret));
	zassert_equal(priv->state, TEST_CLASS_PRIV_ENABLED,
		      "The class should be enabled if probing succeeded");

	ret = usbh_class_suspended(c_data);
	zassert_ok(ret, "Susupending the class while it is running should succeed");
	zassert_equal(priv->state, TEST_CLASS_PRIV_SUSPENDED,
		      "The class private state should have been updated");

	ret = usbh_class_resumed(c_data);
	zassert_ok(ret, "Resuming the class after suspending should succeed");
	zassert_equal(priv->state, TEST_CLASS_PRIV_ENABLED,
		      "The class private state should have been updated");

	ret = usbh_class_removed(c_data);
	zassert_ok(ret, "Removing the class after probing it should succeed");
	zassert_equal(priv->state,  TEST_CLASS_PRIV_IDLE,
		      "The class should be back to inactive ");

	ret = usbh_class_probe(c_data, udev, 0);
	zassert_ok(ret, "Probing the class again should succeed");
	zassert_equal(priv->state,  TEST_CLASS_PRIV_ENABLED,
		      "The class should be back to active ");
}

static void *usb_test_enable(void)
{
	int ret;

	ret = usbh_init(&uhs_ctx);
	zassert_ok(ret, "Failed to initialize USB host (%s)", strerror(-ret));

	ret = usbh_enable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable USB host (%s)", strerror(-ret));

	ret = uhc_bus_reset(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus reset (%s)", strerror(-ret));

	ret = uhc_bus_resume(uhs_ctx.dev);
	zassert_ok(ret, "Failed to signal bus resume (%s)", strerror(-ret));

	ret = uhc_sof_enable(uhs_ctx.dev);
	zassert_ok(ret, "Failed to enable SoF generator (%s)", strerror(-ret));

	LOG_INF("Host controller enabled");

	/* Allow the host time to reset the host. */
	k_msleep(200);

	return NULL;
}

static void usb_test_shutdown(void *f)
{
	int ret;

	ret = usbh_disable(&uhs_ctx);
	zassert_ok(ret, "Failed to enable host support (%s)", strerror(-ret));

	ret = usbh_shutdown(&uhs_ctx);
	zassert_ok(ret, "Failed to shutdown host support (%s)", strerror(-ret));

	LOG_INF("Host controller disabled");
}

ZTEST_SUITE(host_class, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
