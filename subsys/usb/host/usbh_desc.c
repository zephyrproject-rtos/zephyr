/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>

#include "usbh_class.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_desc, CONFIG_USBH_LOG_LEVEL);

bool usbh_desc_is_valid(const void *const desc, const size_t size, const uint8_t type)
{
	const struct usb_desc_header *const head = desc;

	if (size < sizeof(struct usb_desc_header)) {
		return false;
	}

	/* Avoid too short bLength field or nil descriptor */
	if (head->bLength < size) {
		return false;
	}

	/* Expect the correct type */
	if (type != 0 && type != head->bDescriptorType) {
		return false;
	}

	return true;
}

bool usbh_desc_is_valid_interface(const void *const desc)
{
	return usbh_desc_is_valid(desc, sizeof(struct usb_if_descriptor), USB_DESC_INTERFACE);
}

bool usbh_desc_is_valid_association(const void *const desc)
{
	return usbh_desc_is_valid(desc, sizeof(struct usb_association_descriptor),
				  USB_DESC_INTERFACE_ASSOC);
}

bool usbh_desc_is_valid_configuration(const void *const desc)
{
	return usbh_desc_is_valid(desc, sizeof(struct usb_cfg_descriptor), USB_DESC_CONFIGURATION);
}

bool usbh_desc_is_valid_endpoint(const void *const desc)
{
	return usbh_desc_is_valid(desc, sizeof(struct usb_ep_descriptor), USB_DESC_ENDPOINT);
}

bool usbh_desc_is_valid_string(const void *const desc)
{
	return usbh_desc_is_valid(desc, sizeof(struct usb_string_descriptor), USB_DESC_STRING);
}

const void *usbh_desc_get_next(const void *const desc)
{
	const struct usb_desc_header *const head = desc;
	const void *next;

	if (!usbh_desc_is_valid(desc, sizeof(const struct usb_desc_header), 0)) {
		return NULL;
	}

	next = (const uint8_t *)desc + head->bLength;

	if (!usbh_desc_is_valid(next, sizeof(const struct usb_desc_header), 0)) {
		return NULL;
	}

	return next;
}

const void *usbh_desc_get_next_alt_setting(const void *const desc)
{
	const struct usb_desc_header *head = desc;

	/* Skip the current interface descriptor */
	head = usbh_desc_get_next(desc);

	/* Seek to the next alternate setting for this interface */
	for (; head != NULL; head = usbh_desc_get_next(head)) {
		struct usb_if_descriptor *if_d = (void *)head;

		if (head->bDescriptorType != USB_DESC_INTERFACE) {
			continue;
		}

		/* Non-zero Alternate Setting */
		if (usbh_desc_is_valid_interface(desc) && if_d->bAlternateSetting != 0) {
			return head;
		}

		/* Do not continue to the next interface */
		return NULL;
	}

	return NULL;
}

const void *usbh_desc_get_iface(const struct usb_device *const udev, const uint8_t iface)
{
	const struct usb_cfg_descriptor *const c_desc = udev->cfg_desc;

	for (unsigned int i = 0; i < c_desc->bNumInterfaces; i++) {
		const struct usb_host_interface *const host_iface = &udev->ifaces[i];
		const struct usb_if_descriptor *const if_d = (void *)host_iface->dhp;

		if (if_d->bInterfaceNumber == iface) {
			return host_iface->dhp;
		}
	}

	return NULL;
}

const void *usbh_desc_get_iad(const struct usb_device *const udev, const uint8_t iface)
{
	const struct usb_cfg_descriptor *const c_desc = udev->cfg_desc;

	for (unsigned int i = 0; i < c_desc->bNumInterfaces; i++) {
		const struct usb_host_interface *const host_iface = &udev->ifaces[i];
		const struct usb_if_descriptor *const if_d = (void *)host_iface->dhp;

		if (if_d->bInterfaceNumber == iface && host_iface->iad != NULL) {
			return host_iface->iad;
		}
	}

	return NULL;
}

const void *usbh_desc_get_endpoint(const struct usb_device *const udev, const uint8_t ep)
{
	uint8_t idx = USB_EP_GET_IDX(ep) & 0xf;

	return USB_EP_DIR_IS_IN(ep) ? udev->ep_in[idx].desc : udev->ep_out[idx].desc;
}

int usbh_desc_fill_filter(const struct usb_desc_header *const desc,
			  struct usbh_class_filter *const filter, uint8_t *const iface)
{
	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		const struct usb_association_descriptor *ia_desc = (const void *)desc;

		filter->class = ia_desc->bFunctionClass;
		filter->sub = ia_desc->bFunctionSubClass;
		filter->proto = ia_desc->bFunctionProtocol;
		if (iface != NULL) {
			*iface = ia_desc->bFirstInterface;
		}
		return 0;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE) {
		const struct usb_if_descriptor *if_desc = (const void *)desc;

		filter->class = if_desc->bInterfaceClass;
		filter->sub = if_desc->bInterfaceSubClass;
		filter->proto = if_desc->bInterfaceProtocol;
		if (iface != NULL) {
			*iface = if_desc->bInterfaceNumber;
		}
		return 0;
	}

	return -EINVAL;
}

const void *usbh_desc_get_next_function(const void *const desc)
{
	const struct usb_desc_header *head = desc;
	const struct usb_association_descriptor *const ass_d = desc;
	const struct usb_if_descriptor *if_d;
	uint8_t skip_num = 0;

	/* Skip all interfaces the Association descriptor contains */
	if (usbh_desc_is_valid_association(head)) {
		skip_num = ass_d->bInterfaceCount;
	}

	/* Skip the interface if the head is interface */
	if (usbh_desc_is_valid_interface(head)) {
		skip_num = 1;
	}

	while (true) {
		/* If already on an Interface Association or Interface, this will skip it */
		head = usbh_desc_get_next(head);
		if (head == NULL) {
			break;
		}

		if_d = (const void *)head;

		/* Association descriptor: this is always a new function */
		if (usbh_desc_is_valid_association(head)) {
			return head;
		}

		/* Only count the first Alternate Setting of an Interface */
		if (usbh_desc_is_valid_interface(head) && if_d->bAlternateSetting == 0) {
			if (skip_num == 0) {
				return head;
			}

			skip_num--;
		}
	}

	return NULL;
}

int usbh_desc_get_supported_langs(struct usb_device *const udev, uint16_t *const lang_ids,
				  size_t max_langs)
{
	struct net_buf *desc_buf;
	const struct usb_string_descriptor *str_desc;
	const uint16_t *lang_id;
	unsigned int str_desc_length, supported_langs;
	int ret;

	desc_buf = usbh_xfer_buf_alloc(udev, sizeof(struct usb_string_descriptor));
	if (desc_buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_str(udev, 0, 0, desc_buf);
	if (ret != 0) {
		goto done;
	}

	if (!usbh_desc_is_valid_string(desc_buf->data)) {
		ret = -EBADMSG;
		goto done;
	}

	str_desc = (struct usb_string_descriptor *)desc_buf->data;
	if (str_desc->bLength > sizeof(struct usb_string_descriptor)) {
		str_desc_length = str_desc->bLength;
		usbh_xfer_buf_free(udev, desc_buf);

		desc_buf = usbh_xfer_buf_alloc(udev, str_desc_length);
		if (desc_buf == NULL) {
			ret = -ENOMEM;
			goto done;
		}

		ret = usbh_req_desc_str(udev, 0, 0, desc_buf);
		if (ret != 0) {
			goto done;
		}

		if (!usbh_desc_is_valid_string(desc_buf->data)) {
			ret = -EBADMSG;
			goto done;
		}

		str_desc = (struct usb_string_descriptor *)desc_buf->data;
	}

	supported_langs = (str_desc->bLength - 2) / 2;
	lang_id = (uint16_t *)&str_desc->bString;
	for (unsigned int i = 0; i < MIN(supported_langs, max_langs); i++) {
		lang_ids[i] = sys_le16_to_cpu(lang_id[i]);
	}

	ret = supported_langs;
	if (supported_langs >= INT_MAX) {
		ret = -ENOTSUP;
	}

done:
	if (desc_buf != NULL) {
		usbh_xfer_buf_free(udev, desc_buf);
	}

	return ret;
}

int usbh_desc_str_utfle16_to_ascii(const struct net_buf *const desc_buf, char *const str,
				   size_t len)
{
	const struct usb_string_descriptor *str_desc;
	const uint16_t *utf16le_char;
	unsigned int utf16le_str_len, cp_len;
	uint16_t utf16le_code;
	bool has_non_ascii = false;

	if (desc_buf == NULL || str == NULL || len == 0) {
		return -EINVAL;
	}

	if (desc_buf->frags != NULL) {
		return -EINVAL;
	}

	if (!usbh_desc_is_valid_string(desc_buf->data)) {
		return -EINVAL;
	}

	str_desc = (struct usb_string_descriptor *)desc_buf->data;
	utf16le_str_len = (str_desc->bLength - 2) / 2;
	if (utf16le_str_len == 0) {
		str[0] = '\0';
		return 0;
	}

	utf16le_char = (uint16_t *)&str_desc->bString;

	for (cp_len = 0; cp_len < MIN(utf16le_str_len, len - 1); cp_len++) {
		utf16le_code = sys_le16_to_cpu(utf16le_char[cp_len]);
		if (utf16le_code == 0) {
			break;
		} else if (utf16le_code > 0x7F) {
			LOG_WRN("Non-ASCII character 0x%04X at index %u", utf16le_code, cp_len);
			str[cp_len] = '?';
			has_non_ascii = true;
		} else {
			str[cp_len] = (char)utf16le_code;
		}
	}
	str[cp_len] = '\0';

	if (cp_len == len - 1 && utf16le_str_len > cp_len) {
		LOG_WRN("String truncated: %u/%u characters copied", cp_len, utf16le_str_len);
	}

	return has_non_ascii ? -ENOTSUP : (int)cp_len;
}
