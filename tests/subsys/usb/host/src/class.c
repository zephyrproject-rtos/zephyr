/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/logging/log.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_class_api.h"
#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_host.h"

#include "usbh_test_common.h"

LOG_MODULE_REGISTER(test_class, LOG_LEVEL_DBG);

/* Private class data, here just an integer but usually a custom struct. */
struct usbh_foo_priv {
	enum {
		/* Test value stored before the class is initialized */
		FOO_CLASS_PRIV_INACTIVE,
		/* Test value stored after the class is initialized */
		FOO_CLASS_PRIV_IDLE,
		/* Test value stored after the class is probed */
		FOO_CLASS_PRIV_ENABLED,
		/* Test value stored after the class is initialized */
		FOO_CLASS_PRIV_INITIALIZED,
		/* Test value stored after the class is suspended */
		FOO_CLASS_PRIV_SUSPENDED,
	} state;
};

static struct usbh_foo_priv usbh_foo_priv = {
	.state = FOO_CLASS_PRIV_INACTIVE,
};

static int usbh_foo_init(struct usbh_class_data *const c_data)
{
	struct usbh_foo_priv *priv = c_data->priv;

	LOG_DBG("initializing %p, priv value 0x%x", c_data, *priv);

	zassert_equal(priv->state, FOO_CLASS_PRIV_INACTIVE,
		      "Class should be initialized only once");

	priv->state = FOO_CLASS_PRIV_IDLE;

	return 0;
}

static int usbh_foo_completion_cb(struct usbh_class_data *const c_data,
				  struct uhc_transfer *const xfer)
{
	struct usbh_foo_priv *priv = c_data->priv;

	LOG_DBG("completion callback for %p, transfer %p", c_data, xfer);

	zassert_equal(priv->state, FOO_CLASS_PRIV_ENABLED);

	return -ENOTSUP;
}

static int usbh_foo_probe(struct usbh_class_data *const c_data,
			  struct usb_device *const udev,
			  const uint8_t iface)
{
	struct usbh_foo_priv *const priv = c_data->priv;
	const struct usb_desc_header *desc;
	const struct usb_if_descriptor *if_desc;

	LOG_DBG("Probing class %s", c_data->name);

	zassert_equal(priv->state, FOO_CLASS_PRIV_IDLE);

	desc = usbh_desc_get_iface(udev, iface);
	if (desc == NULL) {
		LOG_WRN("Could not get interface %d", iface);
		return -ENOENT;
	}

	if (desc->bDescriptorType != USB_DESC_INTERFACE) {
		LOG_ERR("Not an interface descriptor");
		return -ENOTSUP;
	}

	if_desc = (const struct usb_if_descriptor *)desc;
	if (if_desc->bInterfaceClass != USB_BCC_VENDOR) {
		LOG_ERR("Unexpected class code");
		return -ENOTSUP;
	}

	priv->state = FOO_CLASS_PRIV_ENABLED;

	return 0;
}

static int usbh_foo_removed(struct usbh_class_data *const c_data)
{
	struct usbh_foo_priv *const priv = c_data->priv;

	LOG_INF("Removed class %s", c_data->name);

	zassert_equal(priv->state, FOO_CLASS_PRIV_ENABLED);

	priv->state = FOO_CLASS_PRIV_IDLE;

	return 0;
}

static int usbh_foo_suspended(struct usbh_class_data *const c_data)
{
	struct usbh_foo_priv *const priv = c_data->priv;

	zassert_equal(priv->state, FOO_CLASS_PRIV_ENABLED);

	priv->state = FOO_CLASS_PRIV_SUSPENDED;

	return 0;
}

static int usbh_foo_resumed(struct usbh_class_data *const c_data)
{
	struct usbh_foo_priv *const priv = c_data->priv;

	zassert_equal(priv->state, FOO_CLASS_PRIV_SUSPENDED);

	priv->state = FOO_CLASS_PRIV_ENABLED;

	return 0;
}

static struct usbh_class_api usbh_foo_api = {
	.init = &usbh_foo_init,
	.completion_cb = &usbh_foo_completion_cb,
	.probe = &usbh_foo_probe,
	.removed = &usbh_foo_removed,
	.suspended = &usbh_foo_suspended,
	.resumed = &usbh_foo_resumed,
};

const struct usbh_class_filter filter_rules_vid_pid[] = {
	{
		.vid = FOO_TEST_VID,
		.pid = FOO_TEST_PID,
		.flags = USBH_CLASS_MATCH_VID_PID,
	},
	{0},
};

const struct usbh_class_filter filter_rules_triple[] = {
	{
		.class = FOO_TEST_CLASS,
		.sub = FOO_TEST_SUB,
		.proto = FOO_TEST_PROTO,
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
	},
	{0},
};

const struct usbh_class_filter filter_rules_either[] = {
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

const struct usbh_class_filter filter_rules_empty[] = {
	{0},
};

/* Define a class used in the tests */
USBH_DEFINE_CLASS(foo, &usbh_foo_api, &usbh_foo_priv, filter_rules_triple);
