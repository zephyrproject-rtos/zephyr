/*
 * Copyright (c) 2016-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for WebUSB enabled custom class driver.
 *
 * Sample app for WebUSB enabled custom class driver. The received
 * data is echoed back to the WebUSB based application running in
 * the browser at host.
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/bos.h>

#include "webusb.h"

/* Predefined response to control commands related to MS OS 2.0 descriptors */
static const uint8_t msos2_descriptor[] = {
	/* MS OS 2.0 set header descriptor   */
	0x0A, 0x00,             /* Descriptor size (10 bytes)                 */
	0x00, 0x00,             /* MS_OS_20_SET_HEADER_DESCRIPTOR             */
	0x00, 0x00, 0x03, 0x06, /* Windows version (8.1) (0x06030000)         */
	(0x0A + 0x14 + 0x08), 0x00, /* Length of the MS OS 2.0 descriptor set */

	/* MS OS 2.0 function subset ID descriptor
	 * This means that the descriptors below will only apply to one
	 * set of interfaces
	 */
	0x08, 0x00, /* Descriptor size (8 bytes) */
	0x02, 0x00, /* MS_OS_20_SUBSET_HEADER_FUNCTION */
	0x02,       /* Index of first interface this subset applies to. */
	0x00,       /* reserved */
	(0x08 + 0x14), 0x00, /* Length of the MS OS 2.0 descriptor subset */

	/* MS OS 2.0 compatible ID descriptor */
	0x14, 0x00, /* Descriptor size                */
	0x03, 0x00, /* MS_OS_20_FEATURE_COMPATIBLE_ID */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_webusb_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_webusb cap;
} __packed bos_cap_webusb = {
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
		.bVendorCode = 0x01,
		.iLandingPage = 0x01
	}
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_msosv2_desc {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed bos_cap_msosv2 = {
	/* Microsoft OS 2.0 Platform Capability Descriptor
	 * See https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/
	 * microsoft-defined-usb-descriptors
	 * Adapted from the source:
	 * https://github.com/sowbug/weblight/blob/master/firmware/webusb.c
	 * (BSD-2) Thanks http://janaxelson.com/files/ms_os_20_descriptors.c
	 */
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			+ sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		.PlatformCapabilityUUID = {
			/**
			 * MS OS 2.0 Platform Capability ID
			 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
			 */
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		/* Windows version (8.1) (0x06030000) */
		.dwWindowsVersion = sys_cpu_to_le32(0x06030000),
		.wMSOSDescriptorSetTotalLength =
			sys_cpu_to_le16(sizeof(msos2_descriptor)),
		.bMS_VendorCode = 0x02,
		.bAltEnumCode = 0x00
	},
};

USB_DEVICE_BOS_DESC_DEFINE_CAP struct usb_bos_capability_lpm bos_cap_lpm = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
	/**
	 * BIT(1) - LPM support
	 * BIT(2) - BESL support
	 */
	.bmAttributes = BIT(1) | BIT(2),
};

/* WebUSB Device Requests */
static const uint8_t webusb_allowed_origins[] = {
	/* Allowed Origins Header:
	 * https://wicg.github.io/webusb/#get-allowed-origins
	 */
	0x05, 0x00, 0x0D, 0x00, 0x01,

	/* Configuration Subset Header:
	 * https://wicg.github.io/webusb/#configuration-subset-header
	 */
	0x04, 0x01, 0x01, 0x01,

	/* Function Subset Header:
	 * https://wicg.github.io/webusb/#function-subset-header
	 */
	0x04, 0x02, 0x02, 0x01
};

/* Number of allowed origins */
#define NUMBER_OF_ALLOWED_ORIGINS   1

/* URL Descriptor: https://wicg.github.io/webusb/#url-descriptor */
static const uint8_t webusb_origin_url[] = {
	/* Length, DescriptorType, Scheme */
	0x11, 0x03, 0x00,
	'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', ':', '8', '0', '0', '0'
};

/* Predefined response to control commands related to MS OS 1.0 descriptors
 * Please note that this code only defines "extended compat ID OS feature
 * descriptors" and not "extended properties OS features descriptors"
 */
#define MSOS_STRING_LENGTH	18
static struct string_desc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bString[MSOS_STRING_LENGTH];

} __packed msos1_string_descriptor = {
	.bLength = MSOS_STRING_LENGTH,
	.bDescriptorType = USB_DESC_STRING,
	/* Signature MSFT100 */
	.bString = {
		'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
		'1', 0x00, '0', 0x00, '0', 0x00,
		0x03, /* Vendor Code, used for a control request */
		0x00, /* Padding byte for VendorCode looks like UTF16 */
	},
};

static const uint8_t msos1_compatid_descriptor[] = {
	/* See https://github.com/pbatard/libwdi/wiki/WCID-Devices */
	/* MS OS 1.0 header section */
	0x28, 0x00, 0x00, 0x00, /* Descriptor size (40 bytes)          */
	0x00, 0x01,             /* Version 1.00                        */
	0x04, 0x00,             /* Type: Extended compat ID descriptor */
	0x01,                   /* Number of function sections         */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,       /* reserved    */

	/* MS OS 1.0 function section */
	0x02,     /* Index of interface this section applies to. */
	0x01,     /* reserved */
	/* 8-byte compatible ID string, then 8-byte sub-compatible ID string */
	'W',  'I',  'N',  'U',  'S',  'B',  0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /* reserved */
};

/**
 * @brief Custom handler for standard requests in
 *        order to catch the request and return the
 *        WebUSB Platform Capability Descriptor.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail
 */
int custom_handle_req(struct usb_setup_packet *pSetup,
		      int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		return -ENOTSUP;
	}

	if (USB_GET_DESCRIPTOR_TYPE(pSetup->wValue) == USB_DESC_STRING &&
	    USB_GET_DESCRIPTOR_INDEX(pSetup->wValue) == 0xEE) {
		*data = (uint8_t *)(&msos1_string_descriptor);
		*len = sizeof(msos1_string_descriptor);

		LOG_DBG("Get MS OS Descriptor v1 string");

		return 0;
	}

	return -EINVAL;
}

/**
 * @brief Handler called for vendor specific commands. This includes
 *        WebUSB allowed origins and MS OS 1.0 and 2.0 descriptors.
 *
 * @param pSetup    Information about the request to execute.
 * @param len       Size of the buffer.
 * @param data      Buffer containing the request result.
 *
 * @return  0 on success, negative errno code on fail.
 */
int vendor_handle_req(struct usb_setup_packet *pSetup,
		      int32_t *len, uint8_t **data)
{
	if (usb_reqtype_is_to_device(pSetup)) {
		return -ENOTSUP;
	}

	/* Get Allowed origins request */
	if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x01) {
		*data = (uint8_t *)(&webusb_allowed_origins);
		*len = sizeof(webusb_allowed_origins);

		LOG_DBG("Get webusb_allowed_origins");

		return 0;
	} else if (pSetup->bRequest == 0x01 && pSetup->wIndex == 0x02) {
		/* Get URL request */
		uint8_t index = USB_GET_DESCRIPTOR_INDEX(pSetup->wValue);

		if (index == 0U || index > NUMBER_OF_ALLOWED_ORIGINS) {
			return -ENOTSUP;
		}

		*data = (uint8_t *)(&webusb_origin_url);
		*len = sizeof(webusb_origin_url);

		LOG_DBG("Get webusb_origin_url");

		return 0;
	} else if (pSetup->bRequest == 0x02 && pSetup->wIndex == 0x07) {
		/* Get MS OS 2.0 Descriptors request */
		/* 0x07 means "MS_OS_20_DESCRIPTOR_INDEX" */
		*data = (uint8_t *)(&msos2_descriptor);
		*len = sizeof(msos2_descriptor);

		LOG_DBG("Get MS OS Descriptors v2");

		return 0;
	} else if (pSetup->bRequest == 0x03 && pSetup->wIndex == 0x04) {
		/* Get MS OS 1.0 Descriptors request */
		/* 0x04 means "Extended compat ID".
		 * Use 0x05 instead for "Extended properties".
		 */
		*data = (uint8_t *)(&msos1_compatid_descriptor);
		*len = sizeof(msos1_compatid_descriptor);

		LOG_DBG("Get MS OS Descriptors CompatibleID");

		return 0;
	}

	return -ENOTSUP;
}

/* Custom and Vendor request handlers */
static struct webusb_req_handlers req_handlers = {
	.custom_handler = custom_handle_req,
	.vendor_handler = vendor_handle_req,
};

int main(void)
{
	int ret;

	LOG_DBG("");

	usb_bos_register_cap((void *)&bos_cap_webusb);
	usb_bos_register_cap((void *)&bos_cap_msosv2);
	usb_bos_register_cap((void *)&bos_cap_lpm);

	/* Set the custom and vendor request handlers */
	webusb_register_request_handlers(&req_handlers);

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}
	return 0;
}
