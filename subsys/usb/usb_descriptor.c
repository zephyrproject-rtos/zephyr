/* usb_descriptor.c - USB common device descriptor definition */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/byteorder.h>
#include <usb/usbstruct.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_msc.h>
#include <usb/class/usb_cdc.h>
#include "usb_descriptor.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_LEVEL
#include <logging/sys_log.h>

/*
 * The USB Unicode bString is twice as long as initializer_string
 * without null character:
 *   uc_length = (sizeof(initializer_string) - 1) * 2
 * or:
 *   uc_length = sizeof(initializer_string) * 2 - 2
 * the last index of the bString is:
 *   idx_max = sizeof(initializer_string) * 2 - 2 - 1
 * and the last index of the initializer_string without null character is:
 *   asci_idx_max = sizeof(initializer_string) - 1 - 1
 *
 *
 * The length of the string descriptor is calculated from the
 * size of the two octets bLength and bDescriptorType plus the
 * length of the Unicode string:
 *   descr_length = 2 + uc_length
 *   descr_length = 2 + sizeof(initializer-string) * 2 - 2
 *   descr_length = sizeof(initializer-string) * 2
 *
 */

/* Structure representing the global USB description */
struct dev_common_descriptor {
	struct usb_device_descriptor device_descriptor;
	struct usb_cfg_descriptor cfg_descr;
#ifdef CONFIG_USB_CDC_ACM
	struct usb_cdc_acm_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		struct usb_association_descriptor iad_cdc;
#endif
		struct usb_if_descriptor if0;
		struct cdc_header_descriptor if0_header;
		struct cdc_cm_descriptor if0_cm;
		struct cdc_acm_descriptor if0_acm;
		struct cdc_union_descriptor if0_union;
		struct usb_ep_descriptor if0_int_ep;

		struct usb_if_descriptor if1;
		struct usb_ep_descriptor if1_in_ep;
		struct usb_ep_descriptor if1_out_ep;
	} __packed cdc_acm_cfg;
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	struct usb_cdc_ecm_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		struct usb_association_descriptor iad;
#endif
		struct usb_if_descriptor if0;
		struct cdc_header_descriptor if0_header;
		struct cdc_union_descriptor if0_union;
		struct cdc_ecm_descriptor if0_netfun_ecm;
		struct usb_ep_descriptor if0_int_ep;

		struct usb_if_descriptor if1_0;

		struct usb_if_descriptor if1_1;
		struct usb_ep_descriptor if1_1_in_ep;
		struct usb_ep_descriptor if1_1_out_ep;
	} __packed cdc_ecm_cfg;
#endif
#ifdef CONFIG_USB_MASS_STORAGE
	struct usb_mass_config {
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_in_ep;
		struct usb_ep_descriptor if0_out_ep;
	} __packed mass_cfg;
#endif
	struct usb_string_desription {
		struct usb_string_descriptor lang_descr;
		struct usb_mfr_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[MFR_DESC_LENGTH - 2];
		} __packed unicode_mfr;

		struct usb_product_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[PRODUCT_DESC_LENGTH - 2];
		} __packed unicode_product;

		struct usb_sn_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[SN_DESC_LENGTH - 2];
		} __packed unicode_sn;
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
		struct usb_cdc_ecm_mac_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[ECM_MAC_DESC_LENGTH - 2];
		} __packed unicode_mac;
#endif /* CONFIG_USB_DEVICE_NETWORK_ECM */
	} __packed string_descr;
	struct usb_desc_header term_descr;
} __packed;

static struct dev_common_descriptor common_desc = {
	/* Device descriptor */
	.device_descriptor = {
		.bLength = sizeof(struct usb_device_descriptor),
		.bDescriptorType = USB_DEVICE_DESC,
		.bcdUSB = sys_cpu_to_le16(USB_1_1),
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.bDeviceClass = MISC_CLASS,
		.bDeviceSubClass = 0x02,
		.bDeviceProtocol = 0x01,
#else
		.bDeviceClass = 0,
		.bDeviceSubClass = 0,
		.bDeviceProtocol = 0,
#endif
		.bMaxPacketSize0 = MAX_PACKET_SIZE0,
		.idVendor = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_VID),
		.idProduct = sys_cpu_to_le16((u16_t)CONFIG_USB_DEVICE_PID),
		.bcdDevice = sys_cpu_to_le16(BCDDEVICE_RELNUM),
		.iManufacturer = 1,
		.iProduct = 2,
		.iSerialNumber = 3,
		.bNumConfigurations = 1,
	},
	/* Configuration descriptor */
	.cfg_descr = {
		.bLength = sizeof(struct usb_cfg_descriptor),
		.bDescriptorType = USB_CONFIGURATION_DESC,
		.wTotalLength = sizeof(struct dev_common_descriptor)
			      - sizeof(struct usb_device_descriptor)
			      - sizeof(struct usb_string_desription)
			      - sizeof(struct usb_desc_header),
		.bNumInterfaces = NUMOF_IFACES,
		.bConfigurationValue = 1,
		.iConfiguration = 0,
		.bmAttributes = USB_CONFIGURATION_ATTRIBUTES,
		.bMaxPower = MAX_LOW_POWER,
	},
#ifdef CONFIG_USB_CDC_ACM
	.cdc_acm_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.iad_cdc = {
			.bLength = sizeof(struct usb_association_descriptor),
			.bDescriptorType = USB_ASSOCIATION_DESC,
			.bFirstInterface = FIRST_IFACE_CDC_ACM,
			.bInterfaceCount = 0x02,
			.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
			.bFunctionSubClass = ACM_SUBCLASS,
			.bFunctionProtocol = 0,
			.iFunction = 0,
		},
#endif
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_ACM,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
			.bInterfaceSubClass = ACM_SUBCLASS,
			.bInterfaceProtocol = V25TER_PROTOCOL,
			.iInterface = 0,
		},
		/* Header Functional Descriptor */
		.if0_header = {
			.bFunctionLength = sizeof(struct cdc_header_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = HEADER_FUNC_DESC,
			.bcdCDC = sys_cpu_to_le16(USB_1_1),
		},
		/* Call Management Functional Descriptor */
		.if0_cm = {
			.bFunctionLength = sizeof(struct cdc_cm_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,
			.bmCapabilities = 0x02,
			.bDataInterface = 1,
		},
		/* ACM Functional Descriptor */
		.if0_acm = {
			.bFunctionLength = sizeof(struct cdc_acm_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = ACM_FUNC_DESC,
			/* Device supports the request combination of:
			 *	Set_Line_Coding,
			 *	Set_Control_Line_State,
			 *	Get_Line_Coding
			 *	and the notification Serial_State
			 */
			.bmCapabilities = 0x02,
		},
		/* Union Functional Descriptor */
		.if0_union = {
			.bFunctionLength = sizeof(struct cdc_union_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = UNION_FUNC_DESC,
			.bControlInterface = 0,
			.bSubordinateInterface0 = 1,
		},
		/* Endpoint INT */
		.if0_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ACM_INT_EP_ADDR,
			.bmAttributes = USB_DC_EP_INTERRUPT,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ACM_INTERRUPT_EP_MPS),
			.bInterval = 0x0A,
		},
		/* Interface descriptor */
		.if1 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_ACM + 1,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		/* First Endpoint IN */
		.if1_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ACM_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ACM_BULK_EP_MPS),
			.bInterval = 0x00,
		},
		/* Second Endpoint OUT */
		.if1_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ACM_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ACM_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	.cdc_ecm_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.iad = {
			.bLength = sizeof(struct usb_association_descriptor),
			.bDescriptorType = USB_ASSOCIATION_DESC,
			.bFirstInterface = FIRST_IFACE_CDC_ECM,
			.bInterfaceCount = 0x02,
			.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
			.bFunctionSubClass = CDC_ECM_SUBCLASS,
			.bFunctionProtocol = 0,
			.iFunction = 0,
		},
#endif

		/* Interface descriptor 0 */
		/* CDC Communication interface */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_ECM,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
			.bInterfaceSubClass = CDC_ECM_SUBCLASS,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		/* Header Functional Descriptor */
		.if0_header = {
			.bFunctionLength = sizeof(struct cdc_header_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = HEADER_FUNC_DESC,
			.bcdCDC = sys_cpu_to_le16(USB_1_1),
		},
		/* Union Functional Descriptor */
		.if0_union = {
			.bFunctionLength = sizeof(struct cdc_union_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = UNION_FUNC_DESC,
			.bControlInterface = 0,
			.bSubordinateInterface0 = 1,
		},
		/* Ethernet Networking Functional descriptor */
		.if0_netfun_ecm = {
			.bFunctionLength = sizeof(struct cdc_ecm_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = ETHERNET_FUNC_DESC,
			.iMACAddress = 4,
			.bmEthernetStatistics = sys_cpu_to_le32(0), /* None */
			.wMaxSegmentSize = sys_cpu_to_le16(1514),
			.wNumberMCFilters = sys_cpu_to_le16(0), /* None */
			.bNumberPowerFilters = 0, /* No wake up */
		},
		/* Notification EP Descriptor */
		.if0_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ECM_INT_EP_ADDR,
			.bmAttributes = USB_DC_EP_INTERRUPT,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ECM_INTERRUPT_EP_MPS),
			.bInterval = 0x09,
		},

		/* Interface descriptor 1/0 */
		/* CDC Data Interface */
		.if1_0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_ECM + 1,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},

		/* Interface descriptor 1/1 */
		/* CDC Data Interface */
		.if1_1 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_ECM + 1,
			.bAlternateSetting = 1,
			.bNumEndpoints = 2,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
			.bInterfaceSubClass = CDC_ECM_SUBCLASS,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		/* Data Endpoint IN */
		.if1_1_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ECM_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ECM_BULK_EP_MPS),
			.bInterval = 0x00,
		},
		/* Data Endpoint OUT */
		.if1_1_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_ECM_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_ECM_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
#endif
#ifdef CONFIG_USB_MASS_STORAGE
	.mass_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_MASS_STORAGE,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = MASS_STORAGE_CLASS,
			.bInterfaceSubClass = SCSI_TRANSPARENT_SUBCLASS,
			.bInterfaceProtocol = BULK_ONLY_PROTOCOL,
			.iInterface = 0,
		},
		/* First Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_MASS_STORAGE_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_MASS_STORAGE_BULK_EP_MPS),
			.bInterval = 0x00,
		},
		/* Second Endpoint OUT */
		.if0_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_MASS_STORAGE_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_MASS_STORAGE_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
#endif
	.string_descr = {
		.lang_descr = {
			.bLength = sizeof(struct usb_string_descriptor),
			.bDescriptorType = USB_STRING_DESC,
			.bString = sys_cpu_to_le16(0x0409),
		},
		/* Manufacturer String Descriptor */
		.unicode_mfr = {
			.bLength = MFR_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_MANUFACTURER,
		},
		/* Product String Descriptor */
		.unicode_product = {
			.bLength = PRODUCT_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_PRODUCT,
		},
		/* Serial Number String Descriptor */
		.unicode_sn = {
			.bLength = SN_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_SN,
		},
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
		.unicode_mac = {
			.bLength = ECM_MAC_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_NETWORK_ECM_MAC
		},
#endif
	},
	.term_descr = {
		.bLength = 0,
		.bDescriptorType = 0,
	},
};


void usb_fix_unicode_string(int idx_max, int asci_idx_max, u8_t *buf)
{
	for (int i = idx_max; i >= 0; i -= 2) {
		SYS_LOG_DBG("char %c : %x, idx %d -> %d",
			    buf[asci_idx_max],
			    buf[asci_idx_max],
			    asci_idx_max, i);
		buf[i] = 0;
		buf[i - 1] = buf[asci_idx_max--];
	}
}

u8_t *usb_get_device_descriptor(void)
{
	usb_fix_unicode_string(MFR_UC_IDX_MAX, MFR_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.unicode_mfr.bString);

	usb_fix_unicode_string(PRODUCT_UC_IDX_MAX, PRODUCT_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.unicode_product.bString);

	usb_fix_unicode_string(SN_UC_IDX_MAX, SN_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.unicode_sn.bString);

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	usb_fix_unicode_string(ECM_MAC_UC_IDX_MAX, ECM_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.unicode_mac.bString);
#endif

	return (u8_t *) &common_desc;
}
