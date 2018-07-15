/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_USB_DEVICE_OS_DESC)
#define USB_OSDESC_EXTENDED_COMPAT_ID	0x04

/*
 * Devices supporting Microsoft OS Descriptors store special string
 * descriptor at fixed index (0xEE). It is read when a new device is
 * attached to a computer for the first time.
 */
#define USB_OSDESC_STRING_DESC_INDEX	0xEE

struct usb_os_descriptor {
	u8_t *string;
	size_t string_len;
	u8_t vendor_code;

	u8_t *compat_id;
	size_t compat_id_len;
};

int usb_handle_os_desc(struct usb_setup_packet *setup,
		       s32_t *len, u8_t **data_buf);
int usb_handle_os_desc_feature(struct usb_setup_packet *setup,
			       s32_t *len, u8_t **data_buf);
void usb_register_os_desc(struct usb_os_descriptor *desc);
bool usb_os_desc_enabled(void);
#else
#define usb_os_desc_enabled(x)			false
#define usb_handle_os_desc(x, y, z)		-ENOTSUP
#define usb_handle_os_desc_feature(x, y, z)	-ENOTSUP
#define usb_register_os_desc(x)
#endif
