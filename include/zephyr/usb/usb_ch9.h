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
	uint8_t recipient : 5; /**< Request recipient. */
	uint8_t type : 2;      /**< Request type. */
	uint8_t direction : 1; /**< Data transfer direction. */
#else
	uint8_t direction : 1; /**< Data transfer direction. */
	uint8_t type : 2;      /**< Request type. */
	uint8_t recipient : 5; /**< Request recipient. */
#endif
} __packed;

/** USB Setup Data packet. See Table 9-2 of the specification. */
struct usb_setup_packet {
	/** Request type union providing raw and structured access. */
	union {
		uint8_t bmRequestType;                 /**< Raw request type value. */
		struct usb_req_type_field RequestType; /**< Request type bit fields. */
	} __packed;
	uint8_t bRequest; /**< Request code. */
	uint16_t wValue;  /**< Request-specific value. */
	uint16_t wIndex;  /**< Request-specific index. */
	uint16_t wLength; /**< Expected data length. */
} __packed;

/**
 * @name USB Setup packet RequestType Direction values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_dir
 * @{
 */
#define USB_REQTYPE_DIR_TO_DEVICE	0 /**< Host-to-device data direction. */
#define USB_REQTYPE_DIR_TO_HOST		1 /**< Device-to-host data direction. */
/** @} */

/**
 * @name USB Setup packet RequestType Type values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_type
 * @{
 */
#define USB_REQTYPE_TYPE_STANDARD	0 /**< Standard USB request. */
#define USB_REQTYPE_TYPE_CLASS		1 /**< Class-specific request. */
#define USB_REQTYPE_TYPE_VENDOR		2 /**< Vendor-specific request. */
#define USB_REQTYPE_TYPE_RESERVED	3 /**< Reserved request type. */
/** @} */

/**
 * @name USB Setup packet RequestType Recipient values. See Table 9-2 of the specification.
 * @anchor usb_reqtype_recipient
 * @{
 */
#define USB_REQTYPE_RECIPIENT_DEVICE	0 /**< Device recipient. */
#define USB_REQTYPE_RECIPIENT_INTERFACE	1 /**< Interface recipient. */
#define USB_REQTYPE_RECIPIENT_ENDPOINT	2 /**< Endpoint recipient. */
#define USB_REQTYPE_RECIPIENT_OTHER	3 /**< Other recipient. */
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
#define USB_SREQ_GET_STATUS		0x00 /**< Get Status request. */
#define USB_SREQ_CLEAR_FEATURE		0x01 /**< Clear Feature request. */
#define USB_SREQ_SET_FEATURE		0x03 /**< Set Feature request. */
#define USB_SREQ_SET_ADDRESS		0x05 /**< Set Address request. */
#define USB_SREQ_GET_DESCRIPTOR		0x06 /**< Get Descriptor request. */
#define USB_SREQ_SET_DESCRIPTOR		0x07 /**< Set Descriptor request. */
#define USB_SREQ_GET_CONFIGURATION	0x08 /**< Get Configuration request. */
#define USB_SREQ_SET_CONFIGURATION	0x09 /**< Set Configuration request. */
#define USB_SREQ_GET_INTERFACE		0x0A /**< Get Interface request. */
#define USB_SREQ_SET_INTERFACE		0x0B /**< Set Interface request. */
#define USB_SREQ_SYNCH_FRAME		0x0C /**< Synch Frame request. */
/** @} */

/**
 * @name Descriptor Types. See Table 9-5 of the specification.
 * @{
 */
#define USB_DESC_DEVICE			1 /**< Device descriptor type. */
#define USB_DESC_CONFIGURATION		2 /**< Configuration descriptor type. */
#define USB_DESC_STRING			3 /**< String descriptor type. */
#define USB_DESC_INTERFACE		4 /**< Interface descriptor type. */
#define USB_DESC_ENDPOINT		5 /**< Endpoint descriptor type. */
#define USB_DESC_DEVICE_QUALIFIER	6 /**< Device qualifier descriptor type. */
#define USB_DESC_OTHER_SPEED		7 /**< Other speed configuration descriptor type. */
#define USB_DESC_INTERFACE_POWER	8 /**< Interface power descriptor type. */
/** @} */

/**
 * @name Additional Descriptor Types. See Table 9-5 of the specification.
 * @{
 */
#define USB_DESC_OTG			9  /**< OTG descriptor type. */
#define USB_DESC_DEBUG			10 /**< Debug descriptor type. */
#define USB_DESC_INTERFACE_ASSOC	11 /**< Interface association descriptor type. */
#define USB_DESC_BOS			15 /**< Binary object store descriptor type. */
#define USB_DESC_DEVICE_CAPABILITY	16 /**< Device capability descriptor type. */
/** @} */

/**
 * @name Class-Specific Descriptor Types defined by USB Common Class Specification
 * @{
 */
#define USB_DESC_CS_DEVICE		0x21 /**< Class-specific device descriptor type. */
#define USB_DESC_CS_CONFIGURATION	0x22 /**< Class-specific configuration descriptor type. */
#define USB_DESC_CS_STRING		0x23 /**< Class-specific string descriptor type. */
#define USB_DESC_CS_INTERFACE		0x24 /**< Class-specific interface descriptor type. */
#define USB_DESC_CS_ENDPOINT		0x25 /**< Class-specific endpoint descriptor type. */
/** @} */

/**
 * @name USB Standard Feature Selectors. See Table 9-6 of the specification.
 * @{
 */
#define USB_SFS_ENDPOINT_HALT		0x00 /**< Endpoint halt feature selector. */
#define USB_SFS_REMOTE_WAKEUP		0x01 /**< Remote wakeup feature selector. */
#define USB_SFS_TEST_MODE		0x02 /**< Test mode feature selector. */
/** @} */

/**
 * @name USB Test Mode Selectors. See Table 9-7 of the specification.
 * @{
 */
#define USB_SFS_TEST_MODE_J		0x01 /**< Test_J test mode. */
#define USB_SFS_TEST_MODE_K		0x02 /**< Test_K test mode. */
#define USB_SFS_TEST_MODE_SE0_NAK	0x03 /**< Test_SE0_NAK test mode. */
#define USB_SFS_TEST_MODE_PACKET	0x04 /**< Test_Packet test mode. */
#define USB_SFS_TEST_MODE_FORCE_ENABLE	0x05 /**< Test_Force_Enable test mode. */
/** @} */

/**
 * @name Bits used for GetStatus response. See Figure 9-4.
 * @{
 */
#define USB_GET_STATUS_SELF_POWERED	BIT(0) /**< Self-powered status bit. */
#define USB_GET_STATUS_REMOTE_WAKEUP	BIT(1) /**< Remote wakeup status bit. */
/** @} */

/**
 * Header of a USB descriptor.
 */
struct usb_desc_header {
	uint8_t bLength;         /**< Descriptor length. */
	uint8_t bDescriptorType; /**< Descriptor type. */
} __packed;

/**
 * USB Standard Device Descriptor. See Table 9-8 of the specification.
 */
struct usb_device_descriptor {
	uint8_t bLength;            /**< Descriptor length. */
	uint8_t bDescriptorType;    /**< Descriptor type. */
	uint16_t bcdUSB;            /**< USB specification release number. */
	uint8_t bDeviceClass;       /**< Device class. */
	uint8_t bDeviceSubClass;    /**< Device subclass. */
	uint8_t bDeviceProtocol;    /**< Device protocol. */
	uint8_t bMaxPacketSize0;    /**< Maximum packet size for endpoint zero. */
	uint16_t idVendor;          /**< Vendor ID. */
	uint16_t idProduct;         /**< Product ID. */
	uint16_t bcdDevice;         /**< Device release number. */
	uint8_t iManufacturer;      /**< Manufacturer string index. */
	uint8_t iProduct;           /**< Product string index. */
	uint8_t iSerialNumber;      /**< Serial number string index. */
	uint8_t bNumConfigurations; /**< Number of configurations. */
} __packed;

/**
 * USB Device Qualifier Descriptor. See Table 9-9 of the specification.
 */
struct usb_device_qualifier_descriptor {
	uint8_t bLength;            /**< Descriptor length. */
	uint8_t bDescriptorType;    /**< Descriptor type. */
	uint16_t bcdUSB;            /**< USB specification release number. */
	uint8_t bDeviceClass;       /**< Device class. */
	uint8_t bDeviceSubClass;    /**< Device subclass. */
	uint8_t bDeviceProtocol;    /**< Device protocol. */
	uint8_t bMaxPacketSize0;    /**< Maximum packet size for endpoint zero. */
	uint8_t bNumConfigurations; /**< Number of configurations. */
	uint8_t bReserved;          /**< Reserved field. */
} __packed;

/** USB Standard Configuration Descriptor. See Table 9-10 of the specification. */
struct usb_cfg_descriptor {
	uint8_t bLength;             /**< Descriptor length. */
	uint8_t bDescriptorType;     /**< Descriptor type. */
	uint16_t wTotalLength;       /**< Total length of configuration. */
	uint8_t bNumInterfaces;      /**< Number of interfaces. */
	uint8_t bConfigurationValue; /**< Configuration value. */
	uint8_t iConfiguration;      /**< Configuration string index. */
	uint8_t bmAttributes;        /**< Configuration characteristics. */
	uint8_t bMaxPower;           /**< Maximum power consumption. */
} __packed;

/**
 * USB Standard Interface Descriptor. See Table 9-12 of the specification.
 */
struct usb_if_descriptor {
	uint8_t bLength;            /**< Descriptor length. */
	uint8_t bDescriptorType;    /**< Descriptor type. */
	uint8_t bInterfaceNumber;   /**< Interface number. */
	uint8_t bAlternateSetting;  /**< Alternate setting value. */
	uint8_t bNumEndpoints;      /**< Number of endpoints. */
	uint8_t bInterfaceClass;    /**< Interface class. */
	uint8_t bInterfaceSubClass; /**< Interface subclass. */
	uint8_t bInterfaceProtocol; /**< Interface protocol. */
	uint8_t iInterface;         /**< Interface string index. */
} __packed;

/**
 * Endpoint attribute bit fields. See Table 9-13 of the specification.
 */
struct usb_ep_desc_bmattr {
#ifdef CONFIG_LITTLE_ENDIAN
	uint8_t transfer : 2; /**< Transfer type. */
	uint8_t synch : 2;    /**< Synchronization type. */
	uint8_t usage : 2;    /**< Usage type. */
	uint8_t reserved : 2; /**< Reserved bits. */
#else
	uint8_t reserved : 2; /**< Reserved bits. */
	uint8_t usage : 2;    /**< Usage type. */
	uint8_t synch : 2;    /**< Synchronization type. */
	uint8_t transfer : 2; /**< Transfer type. */
#endif
} __packed;

/**
 * USB Standard Endpoint Descriptor. See Table 9-13 of the specification.
 */
struct usb_ep_descriptor {
	uint8_t bLength;          /**< Descriptor length. */
	uint8_t bDescriptorType;  /**< Descriptor type. */
	uint8_t bEndpointAddress; /**< Endpoint address. */
	union {
		uint8_t bmAttributes;                 /**< Raw endpoint attributes. */
		struct usb_ep_desc_bmattr Attributes; /**< Structured endpoint attributes. */
	};
	uint16_t wMaxPacketSize; /**< Maximum packet size. */
	uint8_t bInterval;       /**< Polling interval. */
} __packed;

/**
 * USB Unicode (UTF16LE) String Descriptor. See Table 9-15 of the specification.
 */
struct usb_string_descriptor {
	uint8_t bLength;         /**< Descriptor length. */
	uint8_t bDescriptorType; /**< Descriptor type. */
	uint16_t bString;        /**< UTF-16LE encoded character. */
} __packed;

/**
 * USB Association Descriptor defined in USB 3 spec. See Table 9-16 of the specification.
 */
struct usb_association_descriptor {
	uint8_t bLength;           /**< Descriptor length. */
	uint8_t bDescriptorType;   /**< Descriptor type. */
	uint8_t bFirstInterface;   /**< First interface number. */
	uint8_t bInterfaceCount;   /**< Number of associated interfaces. */
	uint8_t bFunctionClass;    /**< Function class. */
	uint8_t bFunctionSubClass; /**< Function subclass. */
	uint8_t bFunctionProtocol; /**< Function protocol. */
	uint8_t iFunction;         /**< Function string index. */
} __packed;

/**
 * @name USB Standard Configuration Descriptor Characteristics. See Table 9-10 of the specification.
 * @{
 */
#define USB_SCD_RESERVED	BIT(7) /**< Reserved configuration bit. */
#define USB_SCD_SELF_POWERED	BIT(6) /**< Self-powered configuration bit. */
#define USB_SCD_REMOTE_WAKEUP	BIT(5) /**< Remote wakeup configuration bit. */
/** @} */

/**
 * @name USB Defined Base Class Codes
 * @{
 */
#define USB_BCC_AUDIO			0x01 /**< Audio device class. */
#define USB_BCC_CDC_CONTROL		0x02 /**< CDC control device class. */
#define USB_BCC_HID			0x03 /**< HID device class. */
#define USB_BCC_MASS_STORAGE		0x08 /**< Mass storage device class. */
#define USB_BCC_CDC_DATA		0x0A /**< CDC data device class. */
#define USB_BCC_VIDEO			0x0E /**< Video device class. */
#define USB_BCC_MCTP			0x14 /**< MCTP device class. */
#define USB_BCC_WIRELESS_CONTROLLER	0xE0 /**< Wireless controller device class. */
#define USB_BCC_MISCELLANEOUS		0xEF /**< Miscellaneous device class. */
#define USB_BCC_APPLICATION		0xFE /**< Application-specific device class. */
#define USB_BCC_VENDOR			0xFF /**< Vendor-specific device class. */
/** @} */

/**
 * @name USB Specification Release Numbers (bcdUSB Descriptor field)
 * @{
 */
#define USB_SRN_1_1			0x0110 /**< USB 1.1 specification release number. */
#define USB_SRN_2_0			0x0200 /**< USB 2.0 specification release number. */
#define USB_SRN_2_0_1			0x0201 /**< USB 2.0.1 specification release number. */
#define USB_SRN_2_1			0x0210 /**< USB 2.1 specification release number. */
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
 * @param dir Endpoint direction.
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
#define USB_EP_TRANSFER_TYPE_MASK	0x3U /**< Mask for endpoint transfer type. */
#define USB_EP_TYPE_CONTROL		0U   /**< Control transfer endpoint. */
#define USB_EP_TYPE_ISO			1U   /**< Isochronous transfer endpoint. */
#define USB_EP_TYPE_BULK		2U   /**< Bulk transfer endpoint. */
#define USB_EP_TYPE_INTERRUPT		3U   /**< Interrupt transfer endpoint. */
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
