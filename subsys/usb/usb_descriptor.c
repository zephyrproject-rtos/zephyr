/* usb_descriptor.c - USB common device descriptor definition */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 * Copyright (c) 2017, 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/byteorder.h>
#include <misc/__assert.h>
#include <usb/usbstruct.h>
#include <usb/usb_device.h>
#include <usb/usb_common.h>
#include <usb/class/usb_msc.h>
#include <usb/class/usb_cdc.h>
#include <usb/class/usb_hid.h>
#include <usb/class/usb_dfu.h>
#include "usb_descriptor.h"

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#include <logging/sys_log.h>

/*
 * The USB Unicode bString is encoded in UTF16LE, which means it takes up
 * twice the amount of bytes than the same string encoded in ASCII7.
 *
 * without null character:
 *   utf16_length = (sizeof(initializer_string) - 1) * 2
 * or:
 *   utf16_length = sizeof(initializer_string) * 2 - 2
 * the last index of the bString is:
 *   idx_max = sizeof(initializer_string) * 2 - 2 - 1
 * and the last index of the initializer_string without null character is:
 *   asci_idx_max = sizeof(initializer_string) - 1 - 1
 *
 *
 * The length of the string descriptor is calculated from the
 * size of the two octets bLength and bDescriptorType plus the
 * length of the UTF16LE string:
 *   descr_length = 2 + utf16_length
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
#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
	struct usb_rndis_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		struct usb_association_descriptor iad;
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
	} __packed rndis_cfg;
#endif /* CONFIG_USB_DEVICE_NETWORK_RNDIS */
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
#ifdef CONFIG_USB_DEVICE_HID
	struct usb_hid_config {
		struct usb_if_descriptor if0;
		struct usb_hid_descriptor if0_hid;
		struct usb_ep_descriptor if0_int_ep;
	} __packed hid_cfg;
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
	struct usb_cdc_eem_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		struct usb_association_descriptor iad;
#endif
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_in_ep;
		struct usb_ep_descriptor if0_out_ep;
	} __packed cdc_eem_cfg;
#endif
#ifdef CONFIG_USB_DEVICE_BLUETOOTH
	struct usb_bluetooth_config {
		struct usb_if_descriptor if0;
		struct usb_ep_descriptor if0_int_ep;
		struct usb_ep_descriptor if0_out_ep;
		struct usb_ep_descriptor if0_in_ep;
	} __packed bluetooth_cfg;
#endif
#ifdef CONFIG_USB_DFU_CLASS
	struct usb_dfu_config {
		struct usb_if_descriptor if0;
		struct dfu_runtime_descriptor dfu_descr;
	} __packed dfu_cfg;
#endif
	struct usb_string_desription {
		struct usb_string_descriptor lang_descr;
		struct usb_mfr_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[MFR_DESC_LENGTH - 2];
		} __packed utf16le_mfr;

		struct usb_product_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[PRODUCT_DESC_LENGTH - 2];
		} __packed utf16le_product;

		struct usb_sn_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[SN_DESC_LENGTH - 2];
		} __packed utf16le_sn;
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
		struct usb_cdc_ecm_mac_descriptor {
			u8_t bLength;
			u8_t bDescriptorType;
			u8_t bString[ECM_MAC_DESC_LENGTH - 2];
		} __packed utf16le_mac;
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
		.wTotalLength = sys_cpu_to_le16(
			sizeof(struct dev_common_descriptor)
			- sizeof(struct usb_device_descriptor)
			- sizeof(struct usb_string_desription)
			- sizeof(struct usb_desc_header)),
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
#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
	.rndis_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.iad = {
			.bLength = sizeof(struct usb_association_descriptor),
			.bDescriptorType = USB_ASSOCIATION_DESC,
			.bFirstInterface = FIRST_IFACE_RNDIS,
			.bInterfaceCount = 0x02,
			.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
			.bFunctionSubClass = 6,
			.bFunctionProtocol = 0,
			.iFunction = 0,
		},
#endif
		/* Interface descriptor 0 */
		/* CDC Communication interface */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_RNDIS,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
			.bInterfaceSubClass = ACM_SUBCLASS,
			.bInterfaceProtocol = ACM_VENDOR_PROTOCOL,
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
			.bmCapabilities = 0x00,
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
			.bmCapabilities = 0x00,
		},
		/* Union Functional Descriptor */
		.if0_union = {
			.bFunctionLength = sizeof(struct cdc_union_descriptor),
			.bDescriptorType = CS_INTERFACE,
			.bDescriptorSubtype = UNION_FUNC_DESC,
			.bControlInterface = 0,
			.bSubordinateInterface0 = 1,
		},
		/* Notification EP Descriptor */
		.if0_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_RNDIS_INT_EP_ADDR,
			.bmAttributes = USB_DC_EP_INTERRUPT,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_RNDIS_INTERRUPT_EP_MPS),
			.bInterval = 0x09,
		},

		/* Interface descriptor 1 */
		/* CDC Data Interface */
		.if1 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_RNDIS + 1,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		/* Data Endpoint IN */
		.if1_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_RNDIS_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_RNDIS_BULK_EP_MPS),
			.bInterval = 0x00,
		},
		/* Data Endpoint OUT */
		.if1_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_RNDIS_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_RNDIS_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
#endif /* CONFIG_USB_DEVICE_NETWORK_RNDIS */
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	.cdc_ecm_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.iad = {
			.bLength = sizeof(struct usb_association_descriptor),
			.bDescriptorType = USB_ASSOCIATION_DESC,
			.bFirstInterface = FIRST_IFACE_CDC_ECM,
			.bInterfaceCount = 0x02,
			.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
			.bFunctionSubClass = ECM_SUBCLASS,
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
			.bInterfaceSubClass = ECM_SUBCLASS,
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
			.bInterfaceSubClass = ECM_SUBCLASS,
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
#endif /* CONFIG_USB_MASS_STORAGE */
#ifdef CONFIG_USB_DEVICE_HID
	.hid_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_HID,
			.bAlternateSetting = 0,
			.bNumEndpoints = 1,
			.bInterfaceClass = HID_CLASS,
			.bInterfaceSubClass = 0,
			.bInterfaceProtocol = 0,
			.iInterface = 0,
		},
		.if0_hid = {
			.bLength = sizeof(struct usb_hid_descriptor),
			.bDescriptorType = USB_HID_DESC,
			.bcdHID = sys_cpu_to_le16(USB_1_1),
			.bCountryCode = 0,
			.bNumDescriptors = 1,
			.subdesc[0] = {
				.bDescriptorType = USB_HID_REPORT_DESC,
				/*
				 * descriptor length needs to be set
				 * after initialization
				 */
				.wDescriptorLength = 0,
			},
		},
		.if0_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_HID_INT_EP_ADDR,
			.bmAttributes = USB_DC_EP_INTERRUPT,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_HID_INTERRUPT_EP_MPS),
			.bInterval = 0x09,
		},
	},
#endif /* CONFIG_USB_DEVICE_HID */

#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
	.cdc_eem_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
		.iad = {
			.bLength = sizeof(struct usb_association_descriptor),
			.bDescriptorType = USB_ASSOCIATION_DESC,
			.bFirstInterface = FIRST_IFACE_CDC_EEM,
			.bInterfaceCount = 0x01,
			.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
			.bFunctionSubClass = EEM_SUBCLASS,
			.bFunctionProtocol = 0,
			.iFunction = 0,
		},
#endif

		/* Interface descriptor 0 */
		/* CDC Communication interface */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_CDC_EEM,
			.bAlternateSetting = 0,
			.bNumEndpoints = 2,
			.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
			.bInterfaceSubClass = EEM_SUBCLASS,
			.bInterfaceProtocol = EEM_PROTOCOL,
			.iInterface = 0,
		},

		/* Data Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_EEM_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_EEM_BULK_EP_MPS),
			.bInterval = 0x00,
		},

		/* Data Endpoint OUT */
		.if0_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_CDC_EEM_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_CDC_EEM_BULK_EP_MPS),
			.bInterval = 0x00,
		},
	},
#endif /* CONFIG_USB_DEVICE_NETWORK_EEM */
#ifdef CONFIG_USB_DEVICE_BLUETOOTH
	.bluetooth_cfg = {
		/* Interface descriptor 0 */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_BLUETOOTH,
			.bAlternateSetting = 0,
			.bNumEndpoints = 3,
			.bInterfaceClass = WIRELESS_DEVICE_CLASS,
			.bInterfaceSubClass = RF_SUBCLASS,
			.bInterfaceProtocol = BLUETOOTH_PROTOCOL,
			.iInterface = 0,
		},

		/* Interrupt Endpoint */
		.if0_int_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_BLUETOOTH_INT_EP_ADDR,
			.bmAttributes = USB_DC_EP_INTERRUPT,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_BLUETOOTH_INT_EP_MPS),
			.bInterval = 0x01,
		},

		/* Data Endpoint OUT */
		.if0_out_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_BLUETOOTH_OUT_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_BLUETOOTH_BULK_EP_MPS),
			.bInterval = 0x01,
		},

		/* Data Endpoint IN */
		.if0_in_ep = {
			.bLength = sizeof(struct usb_ep_descriptor),
			.bDescriptorType = USB_ENDPOINT_DESC,
			.bEndpointAddress = CONFIG_BLUETOOTH_IN_EP_ADDR,
			.bmAttributes = USB_DC_EP_BULK,
			.wMaxPacketSize =
				sys_cpu_to_le16(
				CONFIG_BLUETOOTH_BULK_EP_MPS),
			.bInterval = 0x01,
		},
	},
#endif /* CONFIG_USB_DEVICE_BLUETOOTH */
#ifdef CONFIG_USB_DFU_CLASS
	.dfu_cfg = {
		/* Interface descriptor */
		.if0 = {
			.bLength = sizeof(struct usb_if_descriptor),
			.bDescriptorType = USB_INTERFACE_DESC,
			.bInterfaceNumber = FIRST_IFACE_DFU,
			.bAlternateSetting = 0,
			.bNumEndpoints = 0,
			.bInterfaceClass = DFU_DEVICE_CLASS,
			.bInterfaceSubClass = DFU_SUBCLASS,
			.bInterfaceProtocol = DFU_RT_PROTOCOL,
			.iInterface = 0,
		},
		.dfu_descr = {
			.bLength = sizeof(struct dfu_runtime_descriptor),
			.bDescriptorType = DFU_FUNC_DESC,
			.bmAttributes = DFU_ATTR_CAN_DNLOAD |
					DFU_ATTR_CAN_UPLOAD |
					DFU_ATTR_MANIFESTATION_TOLERANT,
			.wDetachTimeOut =
				sys_cpu_to_le16(CONFIG_USB_DFU_DETACH_TIMEOUT),
			.wTransferSize =
				sys_cpu_to_le16(CONFIG_USB_DFU_MAX_XFER_SIZE),
			.bcdDFUVersion =
				sys_cpu_to_le16(DFU_VERSION),
		},
	},
#endif /* CONFIG_USB_DFU_CLASS */
	.string_descr = {
		.lang_descr = {
			.bLength = sizeof(struct usb_string_descriptor),
			.bDescriptorType = USB_STRING_DESC,
			.bString = sys_cpu_to_le16(0x0409),
		},
		/* Manufacturer String Descriptor */
		.utf16le_mfr = {
			.bLength = MFR_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_MANUFACTURER,
		},
		/* Product String Descriptor */
		.utf16le_product = {
			.bLength = PRODUCT_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_PRODUCT,
		},
		/* Serial Number String Descriptor */
		.utf16le_sn = {
			.bLength = SN_DESC_LENGTH,
			.bDescriptorType = USB_STRING_DESC,
			.bString = CONFIG_USB_DEVICE_SN,
		},
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
		.utf16le_mac = {
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


/*
 * Utility function: Inline conversion an ASCII-7 string of length asci_idx_max
 * into a UTF16-LE string of idx_max bytes.
 */
void ascii7_to_utf16le(int idx_max, int asci_idx_max, u8_t *buf)
{
	for (int i = idx_max; i >= 0; i -= 2) {
		SYS_LOG_DBG("char %c : %x, idx %d -> %d",
			    buf[asci_idx_max],
			    buf[asci_idx_max],
			    asci_idx_max, i);
		__ASSERT(buf[asci_idx_max] > 0x1F && buf[asci_idx_max] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		buf[i] = 0;
		buf[i - 1] = buf[asci_idx_max--];
	}
}

u8_t *usb_get_device_descriptor(void)
{
	ascii7_to_utf16le(MFR_UC_IDX_MAX, MFR_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.utf16le_mfr.bString);

	ascii7_to_utf16le(PRODUCT_UC_IDX_MAX, PRODUCT_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.utf16le_product.bString);

	ascii7_to_utf16le(SN_UC_IDX_MAX, SN_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.utf16le_sn.bString);

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	ascii7_to_utf16le(ECM_MAC_UC_IDX_MAX, ECM_STRING_IDX_MAX,
		(u8_t *)common_desc.string_descr.utf16le_mac.bString);
#endif

	return (u8_t *) &common_desc;
}

#ifdef CONFIG_USB_DEVICE_HID
void usb_set_hid_report_size(u16_t report_desc_size)
{
	common_desc.hid_cfg.if0_hid.subdesc[0].wDescriptorLength =
		sys_cpu_to_le16(report_desc_size);
}
#endif
