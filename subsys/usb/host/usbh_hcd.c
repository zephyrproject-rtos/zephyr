/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh_hcd.h>
#include <zephyr/drivers/usb/uhc.h>

int usbh_ep_sync_after_clear_feature(struct usb_device *udev)
{
	const struct device *hcd = usbh_hcd_dev(udev);

	return uhc_ep_sync_after_clear_feature(hcd, udev);
}

int usbh_eps_verify_steady(struct usb_device *udev)
{
	const struct device *hcd = usbh_hcd_dev(udev);

	return uhc_eps_verify_steady(hcd, udev);
}

bool usbh_post_configure_steady(const struct usb_device *udev)
{
	const struct device *hcd = usbh_hcd_dev(udev);

	return uhc_post_configure_steady(hcd);
}
