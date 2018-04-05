/* usb_descriptor.h - header for common device descriptor */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_DESCRIPTOR_H__
#define __USB_DESCRIPTOR_H__

#ifdef CONFIG_USB_CDC_ACM
#define NUMOF_IFACES_CDC_ACM		2
#define NUMOF_ENDPOINTS_CDC_ACM		3
#else
#define NUMOF_IFACES_CDC_ACM		0
#define NUMOF_ENDPOINTS_CDC_ACM		0
#endif

#ifdef CONFIG_USB_MASS_STORAGE
#define NUMOF_IFACES_MASS		1
#define NUMOF_ENDPOINTS_MASS		2
#else
#define NUMOF_IFACES_MASS		0
#define NUMOF_ENDPOINTS_MASS		0
#endif

#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
#define NUMOF_IFACES_RNDIS		2
#define NUMOF_ENDPOINTS_RNDIS		3
#else
#define NUMOF_IFACES_RNDIS		0
#define NUMOF_ENDPOINTS_RNDIS		0
#endif

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
#define NUMOF_IFACES_CDC_ECM		2
#define NUMOF_ENDPOINTS_CDC_ECM		3
#else
#define NUMOF_IFACES_CDC_ECM		0
#define NUMOF_ENDPOINTS_CDC_ECM		0
#endif

#ifdef CONFIG_USB_DEVICE_HID
#define NUMOF_IFACES_HID		1
#define NUMOF_ENDPOINTS_HID		1
#else /* CONFIG_USB_DEVICE_HID */
#define NUMOF_IFACES_HID		0
#define NUMOF_ENDPOINTS_HID		0
#endif /* CONFIG_USB_DEVICE_HID */

#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
#define NUMOF_IFACES_CDC_EEM		1
#define NUMOF_ENDPOINTS_CDC_EEM		2
#else
#define NUMOF_IFACES_CDC_EEM		0
#define NUMOF_ENDPOINTS_CDC_EEM		0
#endif

#ifdef CONFIG_USB_DEVICE_BLUETOOTH
#define NUMOF_IFACES_BLUETOOTH		1
#define NUMOF_ENDPOINTS_BLUETOOTH	3
#else
#define NUMOF_IFACES_BLUETOOTH		0
#define NUMOF_ENDPOINTS_BLUETOOTH	0
#endif

#ifdef CONFIG_USB_DFU_CLASS
#define NUMOF_IFACES_DFU		1
#else
#define NUMOF_IFACES_DFU		0
#endif

#define NUMOF_IFACES	(NUMOF_IFACES_CDC_ACM + NUMOF_IFACES_MASS + \
			 NUMOF_IFACES_RNDIS + NUMOF_IFACES_CDC_ECM + \
			 NUMOF_IFACES_HID + NUMOF_IFACES_CDC_EEM + \
			 NUMOF_IFACES_BLUETOOTH + NUMOF_IFACES_DFU)
#define NUMOF_ENDPOINTS	(NUMOF_ENDPOINTS_CDC_ACM + NUMOF_ENDPOINTS_MASS + \
			 NUMOF_ENDPOINTS_RNDIS + NUMOF_ENDPOINTS_CDC_ECM + \
			 NUMOF_ENDPOINTS_HID + NUMOF_ENDPOINTS_CDC_EEM + \
			 NUMOF_ENDPOINTS_BLUETOOTH)

#define FIRST_IFACE_CDC_ACM		0
#define FIRST_IFACE_MASS_STORAGE	NUMOF_IFACES_CDC_ACM
#define FIRST_IFACE_RNDIS		(NUMOF_IFACES_CDC_ACM + \
					 NUMOF_IFACES_MASS)
#define FIRST_IFACE_CDC_ECM		(NUMOF_IFACES_CDC_ACM + \
					 NUMOF_IFACES_MASS + \
					 NUMOF_IFACES_RNDIS)
#define FIRST_IFACE_HID			(FIRST_IFACE_CDC_ECM + \
					 NUMOF_IFACES_CDC_ECM)
#define FIRST_IFACE_CDC_EEM		(FIRST_IFACE_HID + \
					 NUMOF_IFACES_HID)
#define FIRST_IFACE_BLUETOOTH		(FIRST_IFACE_CDC_EEM + \
					 NUMOF_IFACES_CDC_EEM)
#define FIRST_IFACE_DFU			(FIRST_IFACE_BLUETOOTH + \
					 NUMOF_IFACES_BLUETOOTH)

/*
 * The USB Unicode bString is encoded in UTF16LE, which means it takes up
 * twice the amount of bytes than the same string encoded in ASCII7.
 * Use this macro to determine the length of the bString array.
 *
 * bString length without null character:
 *   bString_length = (sizeof(initializer_string) - 1) * 2
 * or:
 *   bString_length = sizeof(initializer_string) * 2 - 2
 */
#define USB_BSTRING_LENGTH(s)		(sizeof(s) * 2 - 2)

/*
 * The length of the string descriptor (bLength) is calculated from the
 * size of the two octets bLength and bDescriptorType plus the
 * length of the UTF16LE string:
 *
 *   bLength = 2 + bString_length
 *   bLength = 2 + sizeof(initializer_string) * 2 - 2
 *   bLength = sizeof(initializer_string) * 2
 * Use this macro to determine the bLength of the string descriptor.
 */
#define USB_STRING_DESCRIPTOR_LENGTH(s)	(sizeof(s) * 2)

u8_t *usb_get_device_descriptor(void);

#endif /* __USB_DESCRIPTOR_H__ */
