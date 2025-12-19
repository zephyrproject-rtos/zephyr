/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh.h>

#include "usbh_desc.h"
#include "usbh_device.h"
#include "usbh_class.h"

#include "test_descriptor.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usb_test, LOG_LEVEL_DBG);

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

ZTEST(host_desc, test_desc_browse)
{
	const struct usb_desc_header *desc;

	/* From "test_descriptor.h":
	 * #0 struct usb_cfg_descriptor
	 * #1 struct usb_association_descriptor
	 * #2 struct usb_if_descriptor
	 * #3 struct usb_ep_descriptor
	 * #4 struct usb_ep_descriptor
	 * #5 struct usb_if_descriptor
	 * #6 struct usb_if_descriptor
	 */
	desc = (const struct usb_desc_header *)&test_desc.cfg;

	/* desc at #0 struct usb_cfg_descriptor */

	zassert_mem_equal(desc, &test_desc.cfg,
			  sizeof(test_desc.cfg),
			  "needs to be at the config descriptor");

	/* desc at #0 struct usb_cfg_descriptor */

	desc = usbh_desc_get_iad(&test_udev0, 0);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.iad,
			  sizeof(test_desc.cfg.foo_func.iad),
			  "needs to return the interface 1 alt 1");

	/* desc at #1 struct usb_association_descriptor */

	desc = usbh_desc_get_iface(&test_udev0, 0);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.if0,
			  sizeof(test_desc.cfg.foo_func.if0),
			  "needs to return the interface 0 alt 0 descriptor");

	zassert_is_null(usbh_desc_get_next_alt_setting(desc),
			"only one AltSetting for interface 0");

	/* desc at #2 struct usb_if_descriptor */

	desc = usbh_desc_get_next(desc);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.if0_out_ep,
			  sizeof(test_desc.cfg.foo_func.if0_out_ep),
			  "needs to return the association descriptor");

	/* desc at #3 struct usb_ep_descriptor */

	desc = usbh_desc_get_endpoint(&test_udev0, 0x81);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.if0_in_ep,
			  sizeof(test_desc.cfg.foo_func.if0_in_ep),
			  "needs to return the interface 1 alt 0");

	/* desc at #4 struct usb_ep_descriptor */

	desc = usbh_desc_get_iface(&test_udev0, 1);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.if1_alt0,
			  sizeof(test_desc.cfg.foo_func.if1_alt0),
			  "needs to return the interface 1 alt 0");

	/* desc at #5 struct usb_if_descriptor */

	desc = usbh_desc_get_next_alt_setting(desc);
	zassert_not_null(desc);
	zassert_mem_equal(desc, &test_desc.cfg.foo_func.if1_alt1,
			  sizeof(test_desc.cfg.foo_func.if1_alt1),
			  "needs to return the interface 1 alt 1");

	/* desc at #6 struct usb_if_descriptor */

	zassert_is_null(usbh_desc_get_next_alt_setting(desc),
			"no more alt setting after interface 1 alt 1");

	zassert_is_null(usbh_desc_get_next(desc),
			"should be at the last descriptor");
}

ZTEST(host_desc, test_desc_query)
{
	struct usb_device *const udev = &test_udev0;
	const struct usb_if_descriptor *if_d;

	if_d = usbh_desc_get_iface(udev, 0);
	zassert_not_null(if_d, "should find interface #0");
	zassert_equal(if_d->bDescriptorType, USB_DESC_INTERFACE,
		   "should be type INTERFACE");
	zassert_equal(if_d->bInterfaceNumber, 0,
		   "interface #0 found should have interface number 0");
	zassert_equal(if_d->bAlternateSetting, 0,
		   "interface #0 found should have Alt Settings 0");

	if_d = usbh_desc_get_iface(udev, 2);
	zassert_is_null(if_d,
			"there is no interface #2 in this test");

}

static void *usb_test_enable(void)
{
	struct usb_device *const udev = &test_udev0;
	int ret;

	ret = usbh_device_parse_cfg_desc(udev);
	zassert_ok(ret, "Failed to parse configuration descriptor (%s)", strerror(-ret));

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

ZTEST_SUITE(host_desc, NULL, usb_test_enable, NULL, NULL, usb_test_shutdown);
