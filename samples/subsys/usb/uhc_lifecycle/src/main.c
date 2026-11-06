/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_ch9.h>

#include "uhc_common.h"

static int ep_request_events;

static int sample_uhc_event_cb(const struct device *dev, const struct uhc_event *const event)
{
	ARG_UNUSED(dev);

	if (event->type == UHC_EVT_EP_REQUEST) {
		ep_request_events++;
	}

	return 0;
}

static const struct device *sample_uhc_dev(void)
{
	return DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0));
}

int main(void)
{
	const struct device *uhc = sample_uhc_dev();
	struct usb_device udev;
	struct usb_device_descriptor desc = {.bMaxPacketSize0 = 64};
	struct uhc_transfer *xfer;
	uint8_t addr;
	int events_before;
	int err;

	printk("UHC lifecycle sample\n");

	if (!device_is_ready(uhc)) {
		printk("Virtual UHC not ready\n");
		return 1;
	}

	if (!uhc_is_initialized(uhc)) {
		err = uhc_init(uhc, sample_uhc_event_cb, NULL);
		if (err != 0) {
			printk("uhc_init failed: %d\n", err);
			return 1;
		}
	}

	if (!uhc_is_enabled(uhc)) {
		err = uhc_enable(uhc);
		if (err != 0) {
			printk("uhc_enable failed: %d\n", err);
			return 1;
		}
	}

	memset(&udev, 0, sizeof(udev));

	err = uhc_add_endpoints(uhc, &udev);
	if (err != 0) {
		printk("uhc_add_endpoints failed: %d\n", err);
		return 1;
	}

	err = uhc_eps_verify_steady(uhc, &udev);
	if (err != 0) {
		printk("uhc_eps_verify_steady failed: %d\n", err);
		return 1;
	}

	if (!uhc_post_configure_steady(uhc)) {
		printk("uhc_post_configure_steady unexpected\n");
		return 1;
	}

	if (uhc_assign_address(uhc, &udev, &addr) != -ENOTSUP) {
		printk("uhc_assign_address should return -ENOTSUP\n");
		return 1;
	}

	udev.dev_desc = desc;
	xfer = uhc_xfer_alloc(uhc, USB_CONTROL_EP_OUT, &udev, NULL, NULL);
	if (xfer == NULL) {
		printk("uhc_xfer_alloc failed\n");
		return 1;
	}

	err = uhc_xfer_append(uhc, xfer);
	if (err != 0) {
		printk("uhc_xfer_append failed: %d\n", err);
		return 1;
	}

	events_before = ep_request_events;
	uhc_xfer_return(uhc, xfer, -ECONNRESET);
	if (ep_request_events != events_before + 1) {
		printk("uhc_xfer_return did not complete queued transfer\n");
		return 1;
	}

	uhc_release_device(uhc, &udev);
	uhc_free_dev(uhc);

	printk("UHC lifecycle OK\n");

	return 0;
}
