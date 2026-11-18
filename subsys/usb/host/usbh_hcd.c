/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbh_hcd.h>

int usbh_endpoint_recover(struct usb_device *udev, uint8_t ep, enum uhc_ep_recovery_reason reason)
{
	const struct device *hcd = usbh_hcd_dev(udev);

	return uhc_endpoint_recover(hcd, udev, ep, reason);
}
