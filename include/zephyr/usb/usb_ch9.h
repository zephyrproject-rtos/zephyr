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

#include <zephyr/version.h>
#include <zephyr/sys/util.h>
#include <zephyr/math/ilog2.h>
#include <zephyr/usb/class/usb_hub.h>

#ifndef ZEPHYR_INCLUDE_USB_CH9_H_
#define ZEPHYR_INCLUDE_USB_CH9_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Representation of the bmRequestType bit field.
 */
struct usb_req_type_field {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Request recipient. */
	uint8_t recipient : 5;
	/** Request type. */
	uint8_t type : 2;
	/** Data transfer direction. */
	uint8_t direction : 1;
#else
	/** Data transfer direction. */
	uint8_t direction : 1;
	/** Request type. */
	uint8_t type : 2;
	/** Request recipient. */
	uint8_t recipient : 5;
#endif
} __packed;

/** USB Setup Data packet. See Table 9-2 of the specification. */
struct usb_setup_packet {
	/** Request type union providing raw and structured access. */
	union {
		/** Raw request type value. */
		uint8_t bmRequestType;
		/** Request type bit fields. */
		struct usb_req_type_field RequestType;
	} __packed;
	/** Request code. */
	uint8_t bRequest;
	/** Request-specific value. */
	uint16_t wValue;
	/** Request-specific index. */
	uint16_t wIndex;
	/** Expected data length. */
	uint16_t wLength;
} __packed;

/**
 * @name USB Setup packet RequestType Direction values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_dir
 * @{
 */
/** Host-to-device data direction. */
#define USB_REQTYPE_DIR_TO_DEVICE	0
/** Device-to-host data direction. */
#define USB_REQTYPE_DIR_TO_HOST		1
/** @} */

/**
 * @name USB Setup packet RequestType Type values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_type
 * @{
 */
/** Standard USB request. */
#define USB_REQTYPE_TYPE_STANDARD	0
/** Class-specific request. */
#define USB_REQTYPE_TYPE_CLASS		1
/** Vendor-specific request. */
#define USB_REQTYPE_TYPE_VENDOR		2
/** Reserved request type. */
#define USB_REQTYPE_TYPE_RESERVED	3
/** @} */

/**
 * @name USB Setup packet RequestType Recipient values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_recipient
 * @{
 */
/** Device recipient. */
#define USB_REQTYPE_RECIPIENT_DEVICE	0
/** Interface recipient. */
#define USB_REQTYPE_RECIPIENT_INTERFACE	1
/** Endpoint recipient. */
#define USB_REQTYPE_RECIPIENT_ENDPOINT	2
/** Other recipient. */
#define USB_REQTYPE_RECIPIENT_OTHER	3
/** @} */

/**
 * @brief Get data transfer direction from bmRequestType.
 *
 * @param bmRequestType Encoded request type.
 *
 * @return 0 for host-to-device, 1 for device-to-host (see @ref usb_reqtype_dir).
 */
#define USB_REQTYPE_GET_DIR(bmRequestType) (((bmRequestType) >> 7) & 0x01U)

/**
 * @brief Get request type from bmRequestType.
 *
 * @param bmRequestType Encoded request type.
 *
 * @return Encoded request type value (see @ref usb_reqtype_type).
 */
#define USB_REQTYPE_GET_TYPE(bmRequestType) (((bmRequestType) >> 5) & 0x03U)

/**
 * @brief Get request recipient from bmRequestType.
 *
 * @param bmRequestType Encoded request type.
 *
 * @return Encoded recipient value (see @ref usb_reqtype_recipient).
 */
#define USB_REQTYPE_GET_RECIPIENT(bmRequestType) ((bmRequestType) & 0x1FU)

/**
 * @brief Check if request transfer direction is to host.
 *
 * @param setup Pointer to USB Setup packet.
 *
 * @retval true Transfer direction is device-to-host.
 * @retval false Transfer direction is host-to-device.
 */
static inline bool usb_reqtype_is_to_host(const struct usb_setup_packet *setup)
{
	return setup->RequestType.direction == USB_REQTYPE_DIR_TO_HOST;
}

/**
 * @brief Check if request transfer direction is to device.
 *
 * @param setup Pointer to USB Setup packet.
 *
 * @retval true Transfer direction is host-to-device.
 * @retval false Transfer direction is device-to-host.
 */
static inline bool usb_reqtype_is_to_device(const struct usb_setup_packet *setup)
{
	return setup->RequestType.direction == USB_REQTYPE_DIR_TO_DEVICE;
}

/**
 * @name USB Standard Request Codes. See Table 9-4 of the specification.
 * @{
 */
/** Get Status request. */
#define USB_SREQ_GET_STATUS		0x00
/** Clear Feature request. */
#define USB_SREQ_CLEAR_FEATURE		0x01
/** Set Feature request. */
#define USB_SREQ_SET_FEATURE		0x03
/** Set Address request. */
#define USB_SREQ_SET_ADDRESS		0x05
/** Get Descriptor request. */
#define USB_SREQ_GET_DESCRIPTOR		0x06
/** Set Descriptor request. */
#define USB_SREQ_SET_DESCRIPTOR		0x07
/** Get Configuration request. */
#define USB_SREQ_GET_CONFIGURATION	0x08
/** Set Configuration request. */
#define USB_SREQ_SET_CONFIGURATION	0x09
/** Get Interface request. */
#define USB_SREQ_GET_INTERFACE		0x0A
/** Set Interface request. */
#define USB_SREQ_SET_INTERFACE		0x0B
/** Synch Frame request. */
#define USB_SREQ_SYNCH_FRAME		0x0C
/** @} */

/**
 * @name Descriptor Types. See Table 9-5 of the specification.
 * @{
 */
/** Device descriptor type. */
#define USB_DESC_DEVICE			1
/** Configuration descriptor type. */
#define USB_DESC_CONFIGURATION		2
/** String descriptor type. */
#define USB_DESC_STRING			3
/** Interface descriptor type. */
#define USB_DESC_INTERFACE		4
/** Endpoint descriptor type. */
#define USB_DESC_ENDPOINT		5
/** Device qualifier descriptor type. */
#define USB_DESC_DEVICE_QUALIFIER	6
/** Other speed configuration descriptor type. */
#define USB_DESC_OTHER_SPEED		7
/** Interface power descriptor type. */
#define USB_DESC_INTERFACE_POWER	8
/** @} */

/**
 * @name Additional Descriptor Types. See Table 9-5 of the specification.
 * @{
 */
/** OTG descriptor type. */
#define USB_DESC_OTG			9
/** Debug descriptor type. */
#define USB_DESC_DEBUG			10
/** Interface association descriptor type. */
#define USB_DESC_INTERFACE_ASSOC	11
/** Binary object store descriptor type. */
#define USB_DESC_BOS			15
/** Device capability descriptor type. */
#define USB_DESC_DEVICE_CAPABILITY	16
/** @} */

/**
 * @name Class-Specific Descriptor Types defined by USB Common Class Specification
 * @{
 */
/** Class-specific device descriptor type. */
#define USB_DESC_CS_DEVICE		0x21
/** Class-specific configuration descriptor type. */
#define USB_DESC_CS_CONFIGURATION	0x22
/** Class-specific string descriptor type. */
#define USB_DESC_CS_STRING		0x23
/** Class-specific interface descriptor type. */
#define USB_DESC_CS_INTERFACE		0x24
/** Class-specific endpoint descriptor type. */
#define USB_DESC_CS_ENDPOINT		0x25
/** @} */

/**
 * @name USB Standard Feature Selectors. See Table 9-6 of the specification.
 * @{
 */
/** Endpoint halt feature selector. */
#define USB_SFS_ENDPOINT_HALT		0x00
/** Remote wakeup feature selector. */
#define USB_SFS_REMOTE_WAKEUP		0x01
/** Test mode feature selector. */
#define USB_SFS_TEST_MODE		0x02
/** @} */

/**
 * @name USB Test Mode Selectors. See Table 9-7 of the specification.
 * @{
 */
/** Test_J test mode. */
#define USB_SFS_TEST_MODE_J		0x01
/** Test_K test mode. */
#define USB_SFS_TEST_MODE_K		0x02
/** Test_SE0_NAK test mode. */
#define USB_SFS_TEST_MODE_SE0_NAK	0x03
/** Test_Packet test mode. */
#define USB_SFS_TEST_MODE_PACKET	0x04
/** Test_Force_Enable test mode. */
#define USB_SFS_TEST_MODE_FORCE_ENABLE	0x05
/** @} */

/**
 * @name Bits used for GetStatus response. See Figure 9-4.
 * @{
 */
/** Self-powered status bit. */
#define USB_GET_STATUS_SELF_POWERED	BIT(0)
/** Remote wakeup status bit. */
#define USB_GET_STATUS_REMOTE_WAKEUP	BIT(1)
/** @} */

/**
 * Header of a USB descriptor.
 */
struct usb_desc_header {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
} __packed;

/** Header of an USB class-specific descriptor */
struct usb_cs_desc_header {
	/** Length of the descriptor in bytes */
	uint8_t bLength;
	/** Type of the descriptor */
	uint8_t bDescriptorType;
	/** Class-specific type of the descriptor */
	uint8_t bDescriptorSubtype;
} __packed;

/**
 * USB Standard Device Descriptor. See Table 9-8 of the specification.
 */
struct usb_device_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** USB specification release number. */
	uint16_t bcdUSB;
	/** Device class. */
	uint8_t bDeviceClass;
	/** Device subclass. */
	uint8_t bDeviceSubClass;
	/** Device protocol. */
	uint8_t bDeviceProtocol;
	/** Maximum packet size for endpoint zero. */
	uint8_t bMaxPacketSize0;
	/** Vendor ID. */
	uint16_t idVendor;
	/** Product ID. */
	uint16_t idProduct;
	/** Device release number. */
	uint16_t bcdDevice;
	/** Manufacturer string index. */
	uint8_t iManufacturer;
	/** Product string index. */
	uint8_t iProduct;
	/** Serial number string index. */
	uint8_t iSerialNumber;
	/** Number of configurations. */
	uint8_t bNumConfigurations;
} __packed;

/**
 * USB Device Qualifier Descriptor. See Table 9-9 of the specification.
 */
struct usb_device_qualifier_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** USB specification release number. */
	uint16_t bcdUSB;
	/** Device class. */
	uint8_t bDeviceClass;
	/** Device subclass. */
	uint8_t bDeviceSubClass;
	/** Device protocol. */
	uint8_t bDeviceProtocol;
	/** Maximum packet size for endpoint zero. */
	uint8_t bMaxPacketSize0;
	/** Number of configurations. */
	uint8_t bNumConfigurations;
	/** Reserved field. */
	uint8_t bReserved;
} __packed;

/** USB Standard Configuration Descriptor. See Table 9-10 of the specification. */
struct usb_cfg_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** Total length of configuration. */
	uint16_t wTotalLength;
	/** Number of interfaces. */
	uint8_t bNumInterfaces;
	/** Configuration value. */
	uint8_t bConfigurationValue;
	/** Configuration string index. */
	uint8_t iConfiguration;
	/** Configuration characteristics. */
	uint8_t bmAttributes;
	/** Maximum power consumption. */
	uint8_t bMaxPower;
} __packed;

/**
 * USB Standard Interface Descriptor. See Table 9-12 of the specification.
 */
struct usb_if_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** Interface number. */
	uint8_t bInterfaceNumber;
	/** Alternate setting value. */
	uint8_t bAlternateSetting;
	/** Number of endpoints. */
	uint8_t bNumEndpoints;
	/** Interface class. */
	uint8_t bInterfaceClass;
	/** Interface subclass. */
	uint8_t bInterfaceSubClass;
	/** Interface protocol. */
	uint8_t bInterfaceProtocol;
	/** Interface string index. */
	uint8_t iInterface;
} __packed;

/**
 * Endpoint attribute bit fields. See Table 9-13 of the specification.
 */
struct usb_ep_desc_bmattr {
#ifdef CONFIG_LITTLE_ENDIAN
	/** Transfer type. */
	uint8_t transfer : 2;
	/** Synchronization type. */
	uint8_t synch : 2;
	/** Usage type. */
	uint8_t usage : 2;
	/** Reserved bits. */
	uint8_t reserved : 2;
#else
	/** Reserved bits. */
	uint8_t reserved : 2;
	/** Usage type. */
	uint8_t usage : 2;
	/** Synchronization type. */
	uint8_t synch : 2;
	/** Transfer type. */
	uint8_t transfer : 2;
#endif
} __packed;

/**
 * USB Standard Endpoint Descriptor. See Table 9-13 of the specification.
 */
struct usb_ep_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** Endpoint address. */
	uint8_t bEndpointAddress;
	union {
		/** Raw endpoint attributes. */
		uint8_t bmAttributes;
		/** Structured endpoint attributes. */
		struct usb_ep_desc_bmattr Attributes;
	};
	/** Maximum packet size. */
	uint16_t wMaxPacketSize;
	/** Polling interval. */
	uint8_t bInterval;
} __packed;

/**
 * USB Unicode (UTF16LE) String Descriptor. See Table 9-15 of the specification.
 */
struct usb_string_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** UTF-16LE encoded character. */
	uint16_t bString;
} __packed;

/**
 * USB Association Descriptor defined in USB 3 spec. See Table 9-16 of the specification.
 */
struct usb_association_descriptor {
	/** Descriptor length. */
	uint8_t bLength;
	/** Descriptor type. */
	uint8_t bDescriptorType;
	/** First interface number. */
	uint8_t bFirstInterface;
	/** Number of associated interfaces. */
	uint8_t bInterfaceCount;
	/** Function class. */
	uint8_t bFunctionClass;
	/** Function subclass. */
	uint8_t bFunctionSubClass;
	/** Function protocol. */
	uint8_t bFunctionProtocol;
	/** Function string index. */
	uint8_t iFunction;
} __packed;

/**
 * @name USB Standard Configuration Descriptor Characteristics. See Table 9-10 of the specification.
 * @{
 */
/** Reserved configuration bit. */
#define USB_SCD_RESERVED	BIT(7)
/** Self-powered configuration bit. */
#define USB_SCD_SELF_POWERED	BIT(6)
/** Remote wakeup configuration bit. */
#define USB_SCD_REMOTE_WAKEUP	BIT(5)
/** @} */

/**
 * @name USB Defined Base Class Codes
 * @{
 */
/** Audio device class. */
#define USB_BCC_AUDIO			0x01
/** CDC control device class. */
#define USB_BCC_CDC_CONTROL		0x02
/** HID device class. */
#define USB_BCC_HID			0x03
/** Mass storage device class. */
#define USB_BCC_MASS_STORAGE		0x08
/** CDC data device class. */
#define USB_BCC_CDC_DATA		0x0A
/** Video device class. */
#define USB_BCC_VIDEO			0x0E
/** MCTP device class. */
#define USB_BCC_MCTP			0x14
/** Wireless controller device class. */
#define USB_BCC_WIRELESS_CONTROLLER	0xE0
/** Miscellaneous device class. */
#define USB_BCC_MISCELLANEOUS		0xEF
/** Application-specific device class. */
#define USB_BCC_APPLICATION		0xFE
/** Vendor-specific device class. */
#define USB_BCC_VENDOR			0xFF
/** @} */

/**
 * @name USB Specification Release Numbers (bcdUSB Descriptor field)
 * @{
 */
/** USB 1.1 specification release number. */
#define USB_SRN_1_1			0x0110
/** USB 2.0 specification release number. */
#define USB_SRN_2_0			0x0200
/** USB 2.0.1 specification release number. */
#define USB_SRN_2_0_1			0x0201
/** USB 2.1 specification release number. */
#define USB_SRN_2_1			0x0210
/** @} */

/**
 * Convert a decimal value to BCD representation.
 *
 * @param dec Decimal value.
 *
 * @return Value encoded as BCD.
 */
#define USB_DEC_TO_BCD(dec) ((((dec) / 10) << 4) | ((dec) % 10))

/** USB Device release number (bcdDevice Descriptor field) */
#define USB_BCD_DRN		(USB_DEC_TO_BCD(KERNEL_VERSION_MAJOR) << 8 | \
				 USB_DEC_TO_BCD(KERNEL_VERSION_MINOR))

/**
 * Obtain descriptor type from USB_SREQ_GET_DESCRIPTOR request value.
 *
 * @param wValue GET_DESCRIPTOR request wValue field.
 *
 * @return Descriptor type encoded in @p wValue.
 */
#define USB_GET_DESCRIPTOR_TYPE(wValue)		((uint8_t)((wValue) >> 8))

/**
 * Obtain descriptor index from USB_SREQ_GET_DESCRIPTOR request value.
 *
 * @param wValue GET_DESCRIPTOR request wValue field.
 *
 * @return Descriptor index encoded in @p wValue.
 */
#define USB_GET_DESCRIPTOR_INDEX(wValue)	((uint8_t)(wValue))

/**
 * USB Control Endpoints maximum packet size (MPS)
 *
 * This value may not be correct for devices operating at speeds other than
 * high speed.
 */
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

/**
 * Get endpoint index (number) from endpoint address.
 *
 * @param ep Endpoint address.
 *
 * @return Endpoint number.
 */
#define USB_EP_GET_IDX(ep)		((ep) & ~USB_EP_DIR_MASK)

/**
 * Get direction based on endpoint address.
 *
 * @param ep Endpoint address.
 *
 * @return Direction mask.
 */
#define USB_EP_GET_DIR(ep)		((ep) & USB_EP_DIR_MASK)

/**
 * Get endpoint address from endpoint index and direction.
 *
 * @param idx Endpoint index.
 * @param dir Endpoint direction (@ref USB_EP_DIR_IN or @ref USB_EP_DIR_OUT).
 *
 * @return Encoded endpoint address.
 */
#define USB_EP_GET_ADDR(idx, dir)	((idx) | ((dir) & USB_EP_DIR_MASK))

/**
 * Determine if an endpoint address refers to an IN endpoint.
 *
 * @param ep Endpoint address.
 *
 * @retval true Endpoint is an IN endpoint.
 * @retval false Endpoint is an OUT endpoint.
 */
#define USB_EP_DIR_IS_IN(ep)		(USB_EP_GET_DIR(ep) == USB_EP_DIR_IN)

/**
 * Determine if an endpoint address refers to an OUT endpoint.
 *
 * @param ep Endpoint address.
 *
 * @retval true Endpoint is an OUT endpoint.
 * @retval false Endpoint is an IN endpoint.
 */
#define USB_EP_DIR_IS_OUT(ep)		(USB_EP_GET_DIR(ep) == USB_EP_DIR_OUT)

/** USB Control Endpoints OUT address */
#define USB_CONTROL_EP_OUT		(USB_EP_DIR_OUT | 0U)

/** USB Control Endpoints IN address */
#define USB_CONTROL_EP_IN		(USB_EP_DIR_IN | 0U)

/**
 * @name USB endpoint transfer types
 * @{
 */
/** Mask for endpoint transfer type. */
#define USB_EP_TRANSFER_TYPE_MASK	0x3U
/** Control transfer endpoint. */
#define USB_EP_TYPE_CONTROL		0U
/** Isochronous transfer endpoint. */
#define USB_EP_TYPE_ISO			1U
/** Bulk transfer endpoint. */
#define USB_EP_TYPE_BULK		2U
/** Interrupt transfer endpoint. */
#define USB_EP_TYPE_INTERRUPT		3U
/** @} */

/**
 * Calculate full speed interrupt endpoint bInterval from a value in microseconds.
 *
 * @param us Polling interval in microseconds.
 *
 * @return Encoded @c bInterval value.
 */
#define USB_FS_INT_EP_INTERVAL(us)	CLAMP(((us) / 1000U), 1U, 255U)

/**
 * Calculate high speed interrupt endpoint bInterval from microseconds.
 *
 * @param us Polling interval in microseconds.
 *
 * @return Encoded @c bInterval value.
 */
#define USB_HS_INT_EP_INTERVAL(us)	CLAMP((ilog2((us) / 125U) + 1U), 1U, 16U)

/**
 * Calculate full speed isochronous endpoint bInterval from microseconds.
 *
 * @param us Polling interval in microseconds.
 *
 * @return Encoded @c bInterval value.
 */
#define USB_FS_ISO_EP_INTERVAL(us)	CLAMP((ilog2((us) / 1000U) + 1U), 1U, 16U)

/**
 * Calculate high speed isochronous endpoint bInterval from microseconds.
 *
 * @param us Polling interval in microseconds.
 *
 * @return Encoded @c bInterval value.
 */
#define USB_HS_ISO_EP_INTERVAL(us)	CLAMP((ilog2((us) / 125U) + 1U), 1U, 16U)

/**
 * Get endpoint size field from Max Packet Size value.
 *
 * @param mps Encoded Max Packet Size.
 *
 * @return Endpoint size field.
 */
#define USB_MPS_EP_SIZE(mps)		((mps) & BIT_MASK(11))

/**
 * Get number of additional transactions per microframe from MPS.
 *
 * @param mps Encoded Max Packet Size.
 *
 * @return Additional transactions per microframe.
 */
#define USB_MPS_ADDITIONAL_TRANSACTIONS(mps) (((mps) & 0x1800) >> 11)

/**
 * Calculate total payload length from Max Packet Size value.
 *
 * @param mps Encoded Max Packet Size.
 *
 * @return Total payload length in bytes.
 */
#define USB_MPS_TO_TPL(mps)	\
	((1 + USB_MPS_ADDITIONAL_TRANSACTIONS(mps)) * USB_MPS_EP_SIZE(mps))

/**
 * Calculate Max Packet Size value from total payload length.
 *
 * @param tpl Total payload length in bytes.
 *
 * @return Encoded Max Packet Size value.
 */
#define USB_TPL_TO_MPS(tpl)				\
	(((tpl) > 2048) ? ((2 << 11) | ((tpl) / 3)) :	\
	 ((tpl) > 1024) ? ((1 << 11) | ((tpl) / 2)) :	\
	 (tpl))

/**
 * Round up total payload length to the next valid value.
 *
 * @param tpl Total payload length in bytes.
 *
 * @return Rounded payload length.
 */
#define USB_TPL_ROUND_UP(tpl)				\
	(((tpl) > 2048) ? ROUND_UP(tpl, 3) :		\
	 ((tpl) > 1024) ? ROUND_UP(tpl, 2) :		\
	 (tpl))

/**
 * Determine whether total payload length value is valid according to USB 2.0.
 *
 * @param tpl Total payload length in bytes.
 *
 * @retval true Payload length is valid.
 * @retval false Payload length is invalid.
 */
#define USB_TPL_IS_VALID(tpl)				\
	(((tpl) > 3072) ? false :			\
	 ((tpl) > 2048) ? ((tpl) % 3 == 0) :		\
	 ((tpl) > 1024) ? ((tpl) % 2 == 0) :		\
	 ((tpl) >= 0))

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CH9_H_ */
