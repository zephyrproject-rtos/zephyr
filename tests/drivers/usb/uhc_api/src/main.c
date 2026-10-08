/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>

#include "uhc_common.h"

static int ep_request_events;

static int test_uhc_event_cb(const struct device *dev, const struct uhc_event *const event)
{
	ARG_UNUSED(dev);

	if (event->type == UHC_EVT_EP_REQUEST) {
		ep_request_events++;
	}

	return 0;
}

static const struct device *test_uhc_dev(void)
{
	return DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0));
}

static void *uhc_api_setup(void)
{
	const struct device *uhc = test_uhc_dev();
	int err;

	if (!device_is_ready(uhc)) {
		zassert_true(false, "Virtual UHC device not ready");
	}

	if (!uhc_is_initialized(uhc)) {
		err = uhc_init(uhc, test_uhc_event_cb, NULL);
		zassert_ok(err, "uhc_init failed");
	}

	if (!uhc_is_enabled(uhc)) {
		err = uhc_enable(uhc);
		zassert_ok(err, "uhc_enable failed");
	}

	return NULL;
}

ZTEST_SUITE(uhc_api, NULL, uhc_api_setup, NULL, NULL, NULL);

ZTEST(uhc_api, test_optional_wrappers_without_hcd_hooks)
{
	const struct device *uhc = test_uhc_dev();
	struct usb_device udev;
	uint8_t addr;

	memset(&udev, 0, sizeof(udev));

	zassert_ok(uhc_add_endpoints(uhc, &udev));
	zassert_ok(uhc_eps_verify_steady(uhc, &udev));
	zassert_ok(uhc_ep_sync_after_clear_feature(uhc, &udev));
	zassert_true(uhc_post_configure_steady(uhc));
	zassert_ok(uhc_prepare_enum(uhc, &udev));
	zassert_equal(uhc_attach_device(uhc, &udev), -ENOTSUP);
	zassert_equal(uhc_assign_address(uhc, &udev, &addr), -ENOTSUP);

	uhc_release_device(uhc, &udev);
	uhc_free_dev(uhc);

	zassert_equal(uhc_add_endpoints(NULL, &udev), -EINVAL);
	zassert_equal(uhc_add_endpoints(uhc, NULL), -EINVAL);
}

ZTEST(uhc_api, test_xfer_return_ignores_duplicate_completion)
{
	const struct device *uhc = test_uhc_dev();
	struct uhc_transfer xfer;
	int events_before;

	memset(&xfer, 0, sizeof(xfer));
	sys_ref_init(&xfer.ref);
	sys_ref_get(&xfer.ref);

	events_before = ep_request_events;

	uhc_xfer_return(uhc, &xfer, -EIO);
	zassert_equal(ep_request_events, events_before,
		      "never-queued completion must not emit EP_REQUEST");

	uhc_xfer_return(uhc, &xfer, -EIO);
	zassert_equal(ep_request_events, events_before, "duplicate xfer_return must be ignored");
}

ZTEST(uhc_api, test_xfer_return_completes_queued_transfer_once)
{
	const struct device *uhc = test_uhc_dev();
	struct usb_device udev;
	struct usb_device_descriptor desc = {.bMaxPacketSize0 = 64};
	struct uhc_data *data = uhc->data;
	struct uhc_transfer *xfer;
	int events_before;

	udev.dev_desc = desc;

	xfer = uhc_xfer_alloc(uhc, USB_CONTROL_EP_OUT, &udev, NULL, NULL);
	zassert_not_null(xfer);

	sys_dlist_append(&data->ctrl_xfers, &xfer->node);
	xfer->queued = 1;

	events_before = ep_request_events;
	uhc_xfer_return(uhc, xfer, -ECONNRESET);
	zassert_equal(ep_request_events, events_before + 1);
}
