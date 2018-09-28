/* usb_descriptor.h - header for common device descriptor */

/*
 * Copyright (c) 2017 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_DESCRIPTOR_H__
#define __USB_DESCRIPTOR_H__

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

int usb_get_str_descriptor_idx(void *ptr);

u8_t *usb_update_sn_string_descriptor(void);
u8_t *usb_get_device_descriptor(void);

#endif /* __USB_DESCRIPTOR_H__ */
