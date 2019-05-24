/*
 * Copyright (c) 2015 - 2016, Freescale Semiconductor, Inc.
 * Copyright 2016 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __USB_SPEC_H__
#define __USB_SPEC_H__

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* USB speed (the value cannot be changed because EHCI QH use the value directly)*/
#define USB_SPEED_FULL (0x00U)
#define USB_SPEED_LOW (0x01U)
#define USB_SPEED_HIGH (0x02U)
#define USB_SPEED_SUPER (0x04U)

/* Set up packet structure */
typedef struct _usb_setup_struct
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_struct_t;

/* USB  standard descriptor endpoint type */
#define USB_ENDPOINT_CONTROL (0x00U)
#define USB_ENDPOINT_ISOCHRONOUS (0x01U)
#define USB_ENDPOINT_BULK (0x02U)
#define USB_ENDPOINT_INTERRUPT (0x03U)

/* USB  standard descriptor transfer direction (cannot change the value because iTD use the value directly) */
#define USB_OUT (0U)
#define USB_IN (1U)

/* USB standard descriptor length */
#define USB_DESCRIPTOR_LENGTH_DEVICE (0x12U)
#define USB_DESCRIPTOR_LENGTH_CONFIGURE (0x09U)
#define USB_DESCRIPTOR_LENGTH_INTERFACE (0x09U)
#define USB_DESCRIPTOR_LENGTH_ENDPOINT (0x07U)
#define USB_DESCRIPTOR_LENGTH_ENDPOINT_COMPANION (0x06U)
#define USB_DESCRIPTOR_LENGTH_DEVICE_QUALITIER (0x0AU)
#define USB_DESCRIPTOR_LENGTH_OTG_DESCRIPTOR (5U)
#define USB_DESCRIPTOR_LENGTH_BOS_DESCRIPTOR (5U)
#define USB_DESCRIPTOR_LENGTH_DEVICE_CAPABILITY_USB20_EXTENSION (0x07U)
#define USB_DESCRIPTOR_LENGTH_DEVICE_CAPABILITY_SUPERSPEED (0x0AU)

/* USB Device Capability Type Codes */
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY_WIRELESS (0x01U)
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY_USB20_EXTENSION (0x02U)
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY_SUPERSPEED (0x03U)

/* USB standard descriptor type */
#define USB_DESCRIPTOR_TYPE_DEVICE (0x01U)
#define USB_DESCRIPTOR_TYPE_CONFIGURE (0x02U)
#define USB_DESCRIPTOR_TYPE_STRING (0x03U)
#define USB_DESCRIPTOR_TYPE_INTERFACE (0x04U)
#define USB_DESCRIPTOR_TYPE_ENDPOINT (0x05U)
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALITIER (0x06U)
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION (0x07U)
#define USB_DESCRIPTOR_TYPE_INTERFAACE_POWER (0x08U)
#define USB_DESCRIPTOR_TYPE_OTG (0x09U)
#define USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION (0x0BU)
#define USB_DESCRIPTOR_TYPE_BOS (0x0F)
#define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY (0x10)

#define USB_DESCRIPTOR_TYPE_HID (0x21U)
#define USB_DESCRIPTOR_TYPE_HID_REPORT (0x22U)
#define USB_DESCRIPTOR_TYPE_HID_PHYSICAL (0x23U)

#define USB_DESCRIPTOR_TYPE_ENDPOINT_COMPANION (0x30U)

/* USB standard request type */
#define USB_REQUEST_TYPE_DIR_MASK (0x80U)
#define USB_REQUEST_TYPE_DIR_SHIFT (7U)
#define USB_REQUEST_TYPE_DIR_OUT (0x00U)
#define USB_REQUEST_TYPE_DIR_IN (0x80U)

#define USB_REQUEST_TYPE_TYPE_MASK (0x60U)
#define USB_REQUEST_TYPE_TYPE_SHIFT (5U)
#define USB_REQUEST_TYPE_TYPE_STANDARD (0U)
#define USB_REQUEST_TYPE_TYPE_CLASS (0x20U)
#define USB_REQUEST_TYPE_TYPE_VENDOR (0x40U)

#define USB_REQUEST_TYPE_RECIPIENT_MASK (0x1FU)
#define USB_REQUEST_TYPE_RECIPIENT_SHIFT (0U)
#define USB_REQUEST_TYPE_RECIPIENT_DEVICE (0x00U)
#define USB_REQUEST_TYPE_RECIPIENT_INTERFACE (0x01U)
#define USB_REQUEST_TYPE_RECIPIENT_ENDPOINT (0x02U)
#define USB_REQUEST_TYPE_RECIPIENT_OTHER (0x03U)

/* USB standard request */
#define USB_REQUEST_STANDARD_GET_STATUS (0x00U)
#define USB_REQUEST_STANDARD_CLEAR_FEATURE (0x01U)
#define USB_REQUEST_STANDARD_SET_FEATURE (0x03U)
#define USB_REQUEST_STANDARD_SET_ADDRESS (0x05U)
#define USB_REQUEST_STANDARD_GET_DESCRIPTOR (0x06U)
#define USB_REQUEST_STANDARD_SET_DESCRIPTOR (0x07U)
#define USB_REQUEST_STANDARD_GET_CONFIGURATION (0x08U)
#define USB_REQUEST_STANDARD_SET_CONFIGURATION (0x09U)
#define USB_REQUEST_STANDARD_GET_INTERFACE (0x0AU)
#define USB_REQUEST_STANDARD_SET_INTERFACE (0x0BU)
#define USB_REQUEST_STANDARD_SYNCH_FRAME (0x0CU)

/* USB standard request GET Status */
#define USB_REQUEST_STANDARD_GET_STATUS_DEVICE_SELF_POWERED_SHIFT (0U)
#define USB_REQUEST_STANDARD_GET_STATUS_DEVICE_REMOTE_WARKUP_SHIFT (1U)

#define USB_REQUEST_STANDARD_GET_STATUS_ENDPOINT_HALT_MASK (0x01U)
#define USB_REQUEST_STANDARD_GET_STATUS_ENDPOINT_HALT_SHIFT (0U)

#define USB_REQUEST_STANDARD_GET_STATUS_OTG_STATUS_SELECTOR (0xF000U)

/* USB standard request CLEAR/SET feature */
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_ENDPOINT_HALT (0U)
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP (1U)
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_DEVICE_TEST_MODE (2U)
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_B_HNP_ENABLE (3U)
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_A_HNP_SUPPORT (4U)
#define USB_REQUEST_STANDARD_FEATURE_SELECTOR_A_ALT_HNP_SUPPORT (5U)

/* USB standard descriptor configure bmAttributes */
#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK (0x80U)
#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_SHIFT (7U)

#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_MASK (0x40U)
#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT (6U)

#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_MASK (0x20U)
#define USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT (5U)

/* USB standard descriptor endpoint bmAttributes */
#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_MASK (0x80U)
#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT (7U)
#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_OUT (0U)
#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_IN (0x80U)

#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_MASK (0x0FU)
#define USB_DESCRIPTOR_ENDPOINT_ADDRESS_NUMBER_SHFIT (0U)

#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_TYPE_MASK (0x03U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_NUMBER_SHFIT (0U)

#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_MASK (0x0CU)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_SHFIT (2U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_NO_SYNC (0x00U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_ASYNC (0x04U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_ADAPTIVE (0x08U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_SYNC_TYPE_SYNC (0x0CU)

#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_USAGE_TYPE_MASK (0x30U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_USAGE_TYPE_SHFIT (4U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_USAGE_TYPE_DATA_ENDPOINT (0x00U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_USAGE_TYPE_FEEDBACK_ENDPOINT (0x10U)
#define USB_DESCRIPTOR_ENDPOINT_ATTRIBUTE_USAGE_TYPE_IMPLICIT_FEEDBACK_DATA_ENDPOINT (0x20U)

#define USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_SIZE_MASK (0x07FFu)
#define USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_MULT_TRANSACTIONS_MASK (0x1800u)
#define USB_DESCRIPTOR_ENDPOINT_MAXPACKETSIZE_MULT_TRANSACTIONS_SHFIT (11U)

/* USB standard descriptor otg bmAttributes */
#define USB_DESCRIPTOR_OTG_ATTRIBUTES_SRP_MASK (0x01u)
#define USB_DESCRIPTOR_OTG_ATTRIBUTES_HNP_MASK (0x02u)
#define USB_DESCRIPTOR_OTG_ATTRIBUTES_ADP_MASK (0x04u)

/* USB standard descriptor device capability usb20 extension bmAttributes */
#define USB_DESCRIPTOR_DEVICE_CAPABILITY_USB20_EXTENSION_LPM_MASK (0x02U)
#define USB_DESCRIPTOR_DEVICE_CAPABILITY_USB20_EXTENSION_LPM_SHIFT (1U)
#define USB_DESCRIPTOR_DEVICE_CAPABILITY_USB20_EXTENSION_BESL_MASK (0x04U)
#define USB_DESCRIPTOR_DEVICE_CAPABILITY_USB20_EXTENSION_BESL_SHIFT (2U)


/* Language structure */
typedef struct _usb_language
{
    uint8_t **string;    /* The Strings descriptor array */
    uint32_t *length;    /* The strings descriptor length array */
    uint16_t languageId; /* The language id of current language */
} usb_language_t;

typedef struct _usb_language_list
{
    uint8_t *languageString;      /* The String 0U pointer */
    uint32_t stringLength;        /* The String 0U Length */
    usb_language_t *languageList; /* The language list */
    uint8_t count;                /* The language count */
} usb_language_list_t;

typedef struct _usb_descriptor_common
{
    uint8_t bLength;         /* Size of this descriptor in bytes */
    uint8_t bDescriptorType; /* DEVICE Descriptor Type */
    uint8_t bData[1];        /* Data */
} usb_descriptor_common_t;

typedef struct _usb_descriptor_device
{
    uint8_t bLength;            /* Size of this descriptor in bytes */
    uint8_t bDescriptorType;    /* DEVICE Descriptor Type */
    uint8_t bcdUSB[2];          /* UUSB Specification Release Number in Binary-Coded Decimal, e.g. 0x0200U */
    uint8_t bDeviceClass;       /* Class code */
    uint8_t bDeviceSubClass;    /* Sub-Class code */
    uint8_t bDeviceProtocol;    /* Protocol code */
    uint8_t bMaxPacketSize0;    /* Maximum packet size for endpoint zero */
    uint8_t idVendor[2];        /* Vendor ID (assigned by the USB-IF) */
    uint8_t idProduct[2];       /* Product ID (assigned by the manufacturer) */
    uint8_t bcdDevice[2];       /* Device release number in binary-coded decimal */
    uint8_t iManufacturer;      /* Index of string descriptor describing manufacturer */
    uint8_t iProduct;           /* Index of string descriptor describing product */
    uint8_t iSerialNumber;      /* Index of string descriptor describing the device serial number */
    uint8_t bNumConfigurations; /* Number of possible configurations */
} usb_descriptor_device_t;

typedef struct _usb_descriptor_configuration
{
    uint8_t bLength;             /* Descriptor size in bytes = 9U */
    uint8_t bDescriptorType;     /* CONFIGURATION type = 2U or 7U */
    uint8_t wTotalLength[2];     /* Length of concatenated descriptors */
    uint8_t bNumInterfaces;      /* Number of interfaces, this configuration. */
    uint8_t bConfigurationValue; /* Value to set this configuration. */
    uint8_t iConfiguration;      /* Index to configuration string */
    uint8_t bmAttributes;        /* Configuration characteristics */
    uint8_t bMaxPower;           /* Maximum power from bus, 2 mA units */
} usb_descriptor_configuration_t;

typedef struct _usb_descriptor_interface
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_descriptor_interface_t;

typedef struct _usb_descriptor_endpoint
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint8_t wMaxPacketSize[2];
    uint8_t bInterval;
} usb_descriptor_endpoint_t;

typedef struct _usb_descriptor_endpoint_companion
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bMaxBurst;
    uint8_t bmAttributes;
    uint8_t wBytesPerInterval[2];
} usb_descriptor_endpoint_companion_t;

typedef struct _usb_descriptor_binary_device_object_store
{
    uint8_t bLength;            /* Descriptor size in bytes = 5U */
    uint8_t bDescriptorType;    /* BOS Descriptor type = 0FU*/
    uint8_t wTotalLength[2];    /*Length of this descriptor and all of its sub descriptors*/
    uint8_t bNumDeviceCaps;     /*The number of separate device capability descriptors in the BOS*/
} usb_descriptor_bos_t;

typedef struct _usb_descriptor_usb20_extension
{
    uint8_t bLength;            /* Descriptor size in bytes = 7U */
    uint8_t bDescriptorType;    /* DEVICE CAPABILITY Descriptor type = 0x10U*/
    uint8_t bDevCapabilityType;  /*Length of this descriptor and all of its sub descriptors*/
    uint8_t bmAttributes[4];     /*Bitmap encoding of supported device level features.*/
} usb_descriptor_usb20_extension_t;
typedef struct _usb_descriptor_super_speed_device_capability
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bDevCapabilityType;
    uint8_t bmAttributes;
    uint8_t wSpeedsSupported[2];
    uint8_t bFunctionalitySupport;
    uint8_t bU1DevExitLat;
    uint8_t wU2DevExitLat[2];
} usb_bos_device_capability_susperspeed_desc_t;
typedef union _usb_descriptor_union
{
    usb_descriptor_common_t common;               /* Common descriptor */
    usb_descriptor_device_t device;               /* Device descriptor */
    usb_descriptor_configuration_t configuration; /* Configuration descriptor */
    usb_descriptor_interface_t interface;         /* Interface descriptor */
    usb_descriptor_endpoint_t endpoint;           /* Endpoint descriptor */
    usb_descriptor_endpoint_companion_t endpointCompanion; /* Endpoint companion descriptor */
} usb_descriptor_union_t;

#endif /* __USB_SPEC_H__ */
