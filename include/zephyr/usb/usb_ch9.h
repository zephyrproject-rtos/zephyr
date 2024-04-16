/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Chapter 9 structures and definitions
 *
 * This file contains the USB Chapter 9 structures definitions
 * and follows, with few exceptions, the USB Specification 2.0.
 */

#include <version.h>
#include <zephyr/sys/util.h>
#include <zephyr/usb/class/usb_hub.h>

#ifndef ZEPHYR_INCLUDE_USB_CH9_H_
#define ZEPHYR_INCLUDE_USB_CH9_H_

#ifdef __cplusplus
extern "C" {
#endif

struct usb_req_type_field {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t recipient : 5;
	uint8_t type : 2;
	uint8_t direction : 1;
#else
	uint8_t direction : 1;
	uint8_t type : 2;
	uint8_t recipient : 5;
#endif
} __packed;

/** USB Setup Data packet defined in spec. Table 9-2 */
struct usb_setup_packet {
	union {
		uint8_t bmRequestType;
		struct usb_req_type_field RequestType;
	};
	uint8_t bRequest;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
};

/** USB Setup packet RequestType Direction values (from Table 9-2) */
#define USB_REQTYPE_DIR_TO_DEVICE	0
#define USB_REQTYPE_DIR_TO_HOST		1

/** USB Setup packet RequestType Type values (from Table 9-2) */
#define USB_REQTYPE_TYPE_STANDARD	0
#define USB_REQTYPE_TYPE_CLASS		1
#define USB_REQTYPE_TYPE_VENDOR		2
#define USB_REQTYPE_TYPE_RESERVED	3

/** USB Setup packet RequestType Recipient values (from Table 9-2) */
#define USB_REQTYPE_RECIPIENT_DEVICE	0
#define USB_REQTYPE_RECIPIENT_INTERFACE	1
#define USB_REQTYPE_RECIPIENT_ENDPOINT	2
#define USB_REQTYPE_RECIPIENT_OTHER	3

/** Get data transfer direction from bmRequestType */
#define USB_REQTYPE_GET_DIR(bmRequestType) (((bmRequestType) >> 7) & 0x01U)
/** Get request type from bmRequestType */
#define USB_REQTYPE_GET_TYPE(bmRequestType) (((bmRequestType) >> 5) & 0x03U)
/** Get request recipient from bmRequestType */
#define USB_REQTYPE_GET_RECIPIENT(bmRequestType) ((bmRequestType) & 0x1FU)

/**
 * @brief Check if request transfer direction is to host.
 *
 * @param setup Pointer to USB Setup packet
 * @return true If transfer direction is to host
 */
static inline bool usb_reqtype_is_to_host(const struct usb_setup_packet *setup)
{
	return setup->RequestType.direction == USB_REQTYPE_DIR_TO_HOST;
}

/**
 * @brief Check if request transfer direction is to device.
 *
 * @param setup Pointer to USB Setup packet
 * @return true If transfer direction is to device
 */
static inline bool usb_reqtype_is_to_device(const struct usb_setup_packet *setup)
{
	return setup->RequestType.direction == USB_REQTYPE_DIR_TO_DEVICE;
}

/** USB Standard Request Codes defined in spec. Table 9-4 */
#define USB_SREQ_GET_STATUS		0x00
#define USB_SREQ_CLEAR_FEATURE		0x01
#define USB_SREQ_SET_FEATURE		0x03
#define USB_SREQ_SET_ADDRESS		0x05
#define USB_SREQ_GET_DESCRIPTOR		0x06
#define USB_SREQ_SET_DESCRIPTOR		0x07
#define USB_SREQ_GET_CONFIGURATION	0x08
#define USB_SREQ_SET_CONFIGURATION	0x09
#define USB_SREQ_GET_INTERFACE		0x0A
#define USB_SREQ_SET_INTERFACE		0x0B
#define USB_SREQ_SYNCH_FRAME		0x0C

/** Descriptor Types defined in spec. Table 9-5 */
#define USB_DESC_DEVICE			1
#define USB_DESC_CONFIGURATION		2
#define USB_DESC_STRING			3
#define USB_DESC_INTERFACE		4
#define USB_DESC_ENDPOINT		5
#define USB_DESC_DEVICE_QUALIFIER	6
#define USB_DESC_OTHER_SPEED		7
#define USB_DESC_INTERFACE_POWER	8
/** Additional Descriptor Types defined in USB 3 spec. Table 9-5 */
#define USB_DESC_OTG			9
#define USB_DESC_DEBUG			10
#define USB_DESC_INTERFACE_ASSOC	11
#define USB_DESC_BOS			15
#define USB_DESC_DEVICE_CAPABILITY	16

/** Class-Specific Descriptor Types as defined by
 *  USB Common Class Specification
 */
#define USB_DESC_CS_DEVICE		0x21
#define USB_DESC_CS_CONFIGURATION	0x22
#define USB_DESC_CS_STRING		0x23
#define USB_DESC_CS_INTERFACE		0x24
#define USB_DESC_CS_ENDPOINT		0x25

/** USB Standard Feature Selectors defined in spec. Table 9-6 */
#define USB_SFS_ENDPOINT_HALT		0x00
#define USB_SFS_REMOTE_WAKEUP		0x01
#define USB_SFS_TEST_MODE		0x02

/** Bits used for GetStatus response defined in spec. Figure 9-4 */
#define USB_GET_STATUS_SELF_POWERED	BIT(0)
#define USB_GET_STATUS_REMOTE_WAKEUP	BIT(1)

/** Header of an USB descriptor */
struct usb_desc_header {
	uint8_t bLength;
	uint8_t bDescriptorType;
} __packed;

/** USB Standard Device Descriptor defined in spec. Table 9-8 */
struct usb_device_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} __packed;

/** USB Standard Configuration Descriptor defined in spec. Table 9-10 */
struct usb_cfg_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} __packed;

/** USB Standard Interface Descriptor defined in spec. Table 9-12 */
struct usb_if_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} __packed;

struct usb_ep_desc_bmattr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t transfer : 2;
	uint8_t synch: 2;
	uint8_t usage: 2;
	uint8_t reserved: 2;
#else
	uint8_t reserved: 2;
	uint8_t usage : 2;
	uint8_t synch : 2;
	uint8_t transfer : 2;
#endif
} __packed;

/** USB Standard Endpoint Descriptor defined in spec. Table 9-13 */
struct usb_ep_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	union {
		uint8_t bmAttributes;
		struct usb_ep_desc_bmattr Attributes;
	};
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} __packed;

/** USB Unicode (UTF16LE) String Descriptor defined in spec. Table 9-15 */
struct usb_string_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bString;
} __packed;

/** USB Association Descriptor defined in USB 3 spec. Table 9-16 */
struct usb_association_descriptor {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bFirstInterface;
	uint8_t bInterfaceCount;
	uint8_t bFunctionClass;
	uint8_t bFunctionSubClass;
	uint8_t bFunctionProtocol;
	uint8_t iFunction;
} __packed;

/** USB Standard Configuration Descriptor Characteristics from Table 9-10 */
#define USB_SCD_RESERVED	BIT(7)
#define USB_SCD_SELF_POWERED	BIT(6)
#define USB_SCD_REMOTE_WAKEUP	BIT(5)

/** USB Defined Base Class Codes from https://www.usb.org/defined-class-codes */
#define USB_BCC_AUDIO			0x01
#define USB_BCC_CDC_CONTROL		0x02
#define USB_BCC_HID			0x03
#define USB_BCC_MASS_STORAGE		0x08
#define USB_BCC_CDC_DATA		0x0A
#define USB_BCC_VIDEO			0x0E
#define USB_BCC_WIRELESS_CONTROLLER	0xE0
#define USB_BCC_MISCELLANEOUS		0xEF
#define USB_BCC_APPLICATION		0xFE
#define USB_BCC_VENDOR			0xFF

/** USB Specification Release Numbers (bcdUSB Descriptor field) */
#define USB_SRN_1_1			0x0110
#define USB_SRN_2_0			0x0200
#define USB_SRN_2_1			0x0210

#define USB_DEC_TO_BCD(dec)	((((dec) / 10) << 4) | ((dec) % 10))

/** USB Device release number (bcdDevice Descriptor field) */
#define USB_BCD_DRN		(USB_DEC_TO_BCD(KERNEL_VERSION_MAJOR) << 8 | \
				 USB_DEC_TO_BCD(KERNEL_VERSION_MINOR))

/** Macro to obtain descriptor type from USB_SREQ_GET_DESCRIPTOR request */
#define USB_GET_DESCRIPTOR_TYPE(wValue)		((uint8_t)((wValue) >> 8))

/** Macro to obtain descriptor index from USB_SREQ_GET_DESCRIPTOR request */
#define USB_GET_DESCRIPTOR_INDEX(wValue)	((uint8_t)(wValue))

/** USB Control Endpoints maximum packet size (MPS) */
#define USB_CONTROL_EP_MPS		64U

/** USB endpoint direction mask */
#define USB_EP_DIR_MASK			(uint8_t)BIT(7)

/** USB IN endpoint direction */
#define USB_EP_DIR_IN			(uint8_t)BIT(7)

/** USB OUT endpoint direction */
#define USB_EP_DIR_OUT			0U

/*
 * REVISE: this should actually be (ep) & 0x0F, but is causes
 * many regressions in the current device support that are difficult
 * to handle.
 */
/** Get endpoint index (number) from endpoint address */
#define USB_EP_GET_IDX(ep)		((ep) & ~USB_EP_DIR_MASK)

/** Get direction based on endpoint address */
#define USB_EP_GET_DIR(ep)		((ep) & USB_EP_DIR_MASK)

/** Get endpoint address from endpoint index and direction */
#define USB_EP_GET_ADDR(idx, dir)	((idx) | ((dir) & USB_EP_DIR_MASK))

/** True if the endpoint is an IN endpoint */
#define USB_EP_DIR_IS_IN(ep)		(USB_EP_GET_DIR(ep) == USB_EP_DIR_IN)

/** True if the endpoint is an OUT endpoint */
#define USB_EP_DIR_IS_OUT(ep)		(USB_EP_GET_DIR(ep) == USB_EP_DIR_OUT)

/** USB Control Endpoints OUT address */
#define USB_CONTROL_EP_OUT		(USB_EP_DIR_OUT | 0U)

/** USB Control Endpoints IN address */
#define USB_CONTROL_EP_IN		(USB_EP_DIR_IN | 0U)

/** USB endpoint transfer type mask */
#define USB_EP_TRANSFER_TYPE_MASK	0x3U

/** USB endpoint transfer type control */
#define USB_EP_TYPE_CONTROL		0U

/** USB endpoint transfer type isochronous */
#define USB_EP_TYPE_ISO			1U

/** USB endpoint transfer type bulk */
#define USB_EP_TYPE_BULK		2U

/** USB endpoint transfer type interrupt */
#define USB_EP_TYPE_INTERRUPT		3U

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CH9_H_ */
