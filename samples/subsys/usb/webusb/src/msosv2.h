/*
 * Copyright (c) 2016-2019 Intel Corporation
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MSOSV2_DESCRIPTOR_H
#define ZEPHYR_INCLUDE_MSOSV2_DESCRIPTOR_H

/*
 * Microsoft OS 2.0 platform capability and Microsoft OS 2.0 descriptor set.
 * See Microsoft OS 2.0 Descriptors Specification for reference.
 */

#define SAMPLE_MSOS2_VENDOR_CODE	0x02U
/* Windows version (10)*/
#define SAMPLE_MSOS2_OS_VERSION		0x0A000000UL

/* random GUID {FA611CC3-7057-42EE-9D82-4919639562B3} */
#define WEBUSB_DEVICE_INTERFACE_GUID \
	'{', 0x00, 'F', 0x00, 'A', 0x00, '6', 0x00, '1', 0x00, '1', 0x00, \
	'C', 0x00, 'C', 0x00, '3', 0x00, '-', 0x00, '7', 0x00, '0', 0x00, \
	'5', 0x00, '7', 0x00, '-', 0x00, '4', 0x00, '2', 0x00, 'E', 0x00, \
	'E', 0x00, '-', 0x00, '9', 0x00, 'D', 0x00, '8', 0x00, '2', 0x00, \
	'-', 0x00, '4', 0x00, '9', 0x00, '1', 0x00, '9', 0x00, '6', 0x00, \
	'3', 0x00, '9', 0x00, '5', 0x00, '6', 0x00, '2', 0x00, 'B', 0x00, \
	'3', 0x00, '}', 0x00, 0x00, 0x00, 0x00, 0x00

#define CDC_ACM_DESCRIPTOR_LENGTH 160

struct msosv2_descriptor {
	struct msosv2_descriptor_set_header header;
#if defined(CONFIG_USBD_CDC_ACM_CLASS)
	/*
	 * The composition of this descriptor is specific to the WebUSB example
	 * in its default configuration. If you use it for your application or
	 * change the configuration, you may need to modify this descriptor for
	 * your USB device. The following only covers the case where the CDC
	 * ACM implementation is enabled, and there is only one CDC ACM UART
	 * instance, and the CDC ACM communication interface is the first in
	 * the configuration.
	 */
	struct msosv2_function_subset_header subset_header;
#endif
	struct msosv2_compatible_id compatible_id;
	struct msosv2_guids_property guids_property;
} __packed;

static struct msosv2_descriptor msosv2_desc = {
	.header = {
		.wLength = sizeof(struct msosv2_descriptor_set_header),
		.wDescriptorType = MS_OS_20_SET_HEADER_DESCRIPTOR,
		.dwWindowsVersion = sys_cpu_to_le32(SAMPLE_MSOS2_OS_VERSION),
		.wTotalLength = sizeof(msosv2_desc),
	},
#if defined(CONFIG_USBD_CDC_ACM_CLASS)
	.subset_header = {
		.wLength = sizeof(struct msosv2_function_subset_header),
		.wDescriptorType = MS_OS_20_SUBSET_HEADER_FUNCTION,
		.bFirstInterface = 0,
		.wSubsetLength = CDC_ACM_DESCRIPTOR_LENGTH,
	},
#endif
	.compatible_id = {
		.wLength = sizeof(struct msosv2_compatible_id),
		.wDescriptorType = MS_OS_20_FEATURE_COMPATIBLE_ID,
		.CompatibleID = {'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00},
	},
	.guids_property = {
		.wLength = sizeof(struct msosv2_guids_property),
		.wDescriptorType = MS_OS_20_FEATURE_REG_PROPERTY,
		.wPropertyDataType = MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ,
		.wPropertyNameLength = 42,
		.PropertyName = {DEVICE_INTERFACE_GUIDS_PROPERTY_NAME},
		.wPropertyDataLength = 80,
		.bPropertyData = {WEBUSB_DEVICE_INTERFACE_GUID},
	},
};

struct bos_msosv2_descriptor {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed;

struct bos_msosv2_descriptor bos_msosv2_desc = {
	/*
	 * Microsoft OS 2.0 Platform Capability Descriptor,
	 * see Microsoft OS 2.0 Descriptors Specification
	 */
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor)
			 + sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.bReserved = 0,
		/* Microsoft OS 2.0 descriptor platform capability UUID
		 * D8DD60DF-4589-4CC7-9CD2-659D9E648A9F
		 */
		.PlatformCapabilityUUID = {
			0xDF, 0x60, 0xDD, 0xD8,
			0x89, 0x45,
			0xC7, 0x4C,
			0x9C, 0xD2,
			0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		.dwWindowsVersion = sys_cpu_to_le32(SAMPLE_MSOS2_OS_VERSION),
		.wMSOSDescriptorSetTotalLength = sys_cpu_to_le16(sizeof(msosv2_desc)),
		.bMS_VendorCode = SAMPLE_MSOS2_VENDOR_CODE,
		.bAltEnumCode = 0x00
	},
};

static int msosv2_to_host_cb(const struct usbd_context *const ctx,
			     const struct usb_setup_packet *const setup,
			     struct net_buf *const buf)
{
	LOG_INF("Vendor callback to host");

	if (setup->bRequest == SAMPLE_MSOS2_VENDOR_CODE &&
	    setup->wIndex == MS_OS_20_DESCRIPTOR_INDEX) {
		LOG_INF("Get MS OS 2.0 Descriptor Set");

		net_buf_add_mem(buf, &msosv2_desc,
				MIN(net_buf_tailroom(buf), sizeof(msosv2_desc)));

		return 0;
	}

	return -ENOTSUP;
}

USBD_DESC_BOS_VREQ_DEFINE(bos_vreq_msosv2, sizeof(bos_msosv2_desc), &bos_msosv2_desc,
			  SAMPLE_MSOS2_VENDOR_CODE, msosv2_to_host_cb, NULL);

#endif /* ZEPHYR_INCLUDE_MSOSV2_DESCRIPTOR_H */
