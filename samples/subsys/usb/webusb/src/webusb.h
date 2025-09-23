/*
 * Copyright (c) 2016-2019 Intel Corporation
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_WEBUSB_DESCRIPTOR_H
#define ZEPHYR_INCLUDE_WEBUSB_DESCRIPTOR_H

/*
 * WebUSB platform capability and WebUSB URL descriptor.
 * See https://wicg.github.io/webusb for reference.
 */

#define WEBUSB_REQ_GET_URL		0x02U
#define WEBUSB_DESC_TYPE_URL		0x03U
#define WEBUSB_URL_PREFIX_HTTP		0x00U
#define WEBUSB_URL_PREFIX_HTTPS		0x01U

#define SAMPLE_WEBUSB_VENDOR_CODE	0x01U
#define SAMPLE_WEBUSB_LANDING_PAGE	0x01U

struct usb_bos_webusb_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_webusb cap;
} __packed;

static const struct usb_bos_webusb_desc bos_cap_webusb = {
	/* WebUSB Platform Capability Descriptor:
	 * https://wicg.github.io/webusb/#webusb-platform-capability-descriptor
	 */
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			 + sizeof(struct usb_bos_capability_webusb),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/* WebUSB Platform Capability UUID
		 * 3408b638-09a9-47a0-8bfd-a0768815b665
		 */
		.PlatformCapabilityUUID = {
			0x38, 0xB6, 0x08, 0x34,
			0xA9, 0x09,
			0xA0, 0x47,
			0x8B, 0xFD,
			0xA0, 0x76, 0x88, 0x15, 0xB6, 0x65,
		},
	},
	.cap = {
		.bcdVersion = sys_cpu_to_le16(0x0100),
		.bVendorCode = SAMPLE_WEBUSB_VENDOR_CODE,
		.iLandingPage = SAMPLE_WEBUSB_LANDING_PAGE
	}
};

/* WebUSB URL Descriptor, see https://wicg.github.io/webusb/#webusb-descriptors */
static const uint8_t webusb_origin_url[] = {
	/* bLength, bDescriptorType, bScheme, UTF-8 encoded URL */
	0x11, WEBUSB_DESC_TYPE_URL, WEBUSB_URL_PREFIX_HTTP,
	'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', ':', '8', '0', '0', '0'
};

static int webusb_to_host_cb(const struct usbd_context *const ctx,
			     const struct usb_setup_packet *const setup,
			     struct net_buf *const buf)
{
	LOG_INF("Vendor callback to host");

	if (setup->wIndex == WEBUSB_REQ_GET_URL) {
		uint8_t index = USB_GET_DESCRIPTOR_INDEX(setup->wValue);

		if (index != SAMPLE_WEBUSB_LANDING_PAGE) {
			return -ENOTSUP;
		}

		LOG_INF("Get URL request, index %u", index);
		net_buf_add_mem(buf, &webusb_origin_url,
				MIN(net_buf_tailroom(buf), sizeof(webusb_origin_url)));

		return 0;
	}

	return -ENOTSUP;
}

USBD_DESC_BOS_VREQ_DEFINE(bos_vreq_webusb, sizeof(bos_cap_webusb), &bos_cap_webusb,
			  SAMPLE_WEBUSB_VENDOR_CODE, webusb_to_host_cb, NULL);

#endif /* ZEPHYR_INCLUDE_WEBUSB_DESCRIPTOR_H */
