/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "usb_host_dump.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usbh_msc.h>

LOG_MODULE_REGISTER(usb_host_dump, LOG_LEVEL_INF);

void usb_host_dump_cfg_interfaces(struct usb_device *udev)
{
	if (udev->cfg_desc == NULL) {
		LOG_INF("usb_host_dump: no configuration descriptor");
		return;
	}

	if (udev->state != USB_STATE_CONFIGURED) {
		LOG_INF("usb_host_dump: device not configured (state=%d)", udev->state);
	}

	const uint8_t *raw = udev->cfg_desc;
	const struct usb_cfg_descriptor *cfg = (const void *)raw;
	const uint16_t total = cfg->wTotalLength;

	LOG_INF("Configuration[%u]: interfaces (parsed from cfg_desc, %u bytes)",
		cfg->bConfigurationValue, total);

	for (uint16_t off = 0U; off + 2U <= total;) {
		const uint8_t len = raw[off];
		const uint8_t type = raw[off + 1U];

		if (len < 2U || off + len > total) {
			break;
		}

		if (type == USB_DESC_INTERFACE && len >= sizeof(struct usb_if_descriptor)) {
			const struct usb_if_descriptor *ifd = (const void *)(raw + off);

			LOG_INF("  if %u alt %u: class 0x%02x sub 0x%02x proto 0x%02x "
				"(%u endpoint(s))",
				ifd->bInterfaceNumber, ifd->bAlternateSetting, ifd->bInterfaceClass,
				ifd->bInterfaceSubClass, ifd->bInterfaceProtocol,
				ifd->bNumEndpoints);
		}

		off += len;
	}
}

void usb_host_try_print_msc_capacity(struct usb_device *udev)
{
	struct usbh_msc_iface msc;
	int ret;

	if (udev->cfg_desc == NULL || udev->state != USB_STATE_CONFIGURED) {
		return;
	}

	ret = usbh_msc_find_bulk_interface(udev, &msc);
	if (ret != 0) {
		return;
	}

	LOG_INF("MSC probe: if%u alt%u sub 0x%02x proto 0x%02x bulk IN=0x%02x OUT=0x%02x "
		"mps %u/%u BBB=%d",
		msc.iface_num, msc.alt_setting, msc.subclass, msc.protocol, msc.ep_in_addr,
		msc.ep_out_addr, msc.mps_in, msc.mps_out, msc.bulk_only ? 1 : 0);
}

int usb_host_msc_storage_bringup(struct usb_device *udev, const struct device *uhc_dev)
{
	return usbh_msc_storage_bringup(udev, uhc_dev, NULL);
}
