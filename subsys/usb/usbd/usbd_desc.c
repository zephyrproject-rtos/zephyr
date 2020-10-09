/*
 * Copyright (c) 2017-2020 PHYTEC Messtechnik GmbH
 * Copyright (c) 2017, 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <sys/byteorder.h>
#include <usb/usbd.h>
#include <drivers/hwinfo.h>
#include "usbd_internal.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(usbd_descriptor, CONFIG_USBD_LOG_LEVEL);

extern struct usbd_contex usbd_ctx;

/*
 * The last index of the initializer_string without null character is:
 *   ascii_idx_max = bLength / 2 - 2
 * Use this macro to determine the last index of ASCII7 string.
 */
#define USB_BSTRING_ASCII_IDX_MAX(n)	(n / 2 - 2)

/*
 * The last index of the bString is:
 *   utf16le_idx_max = sizeof(initializer_string) * 2 - 2 - 1
 *   utf16le_idx_max = bLength - 2 - 1
 * Use this macro to determine the last index of UTF16LE string.
 */
#define USB_BSTRING_UTF16LE_IDX_MAX(n)	(n - 3)

/*
 * This function fixes bString by transforming the ASCII-7 string
 * into a UTF16-LE during runtime.
 */
static void ascii7_to_utf16le(void *descriptor)
{
	struct usb_string_descriptor *str_desc = descriptor;
	int idx_max = USB_BSTRING_UTF16LE_IDX_MAX(str_desc->bLength);
	int ascii_idx_max = USB_BSTRING_ASCII_IDX_MAX(str_desc->bLength);
	uint8_t *buf = (uint8_t *)&str_desc->bString;

	LOG_DBG("idx_max %d, ascii_idx_max %d, buf %p",
		idx_max, ascii_idx_max, buf);

	for (int i = idx_max; i >= 0; i -= 2) {
		LOG_DBG("char %c : %x, idx %d -> %d",
			buf[ascii_idx_max],
			buf[ascii_idx_max],
			ascii_idx_max, i);
		__ASSERT(buf[ascii_idx_max] > 0x1F && buf[ascii_idx_max] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		buf[i] = 0U;
		buf[i - 1] = buf[ascii_idx_max--];
	}
}

/*
 * Default USB Serial Number string descriptor will be derived from
 * Hardware Information Driver (HWINFO). Please note that the length
 * of the new Serial Number descriptor may not exceed the length
 * of the CONFIG_USBD_DEVICE_SN.
 */
static void gen_sn_desc_from_hwinfo(struct usb_sn_descriptor *desc)
{
	uint8_t hwid[sizeof(CONFIG_USBD_DEVICE_SN) / 2];
	uint8_t sn[sizeof(CONFIG_USBD_DEVICE_SN) + 1];
	const char hex[] = "0123456789ABCDEF";

	memset(hwid, 0, sizeof(hwid));
	memset(sn, 0, sizeof(sn));

	if (hwinfo_get_device_id(hwid, sizeof(hwid)) > 0) {
		LOG_HEXDUMP_DBG(hwid, sizeof(hwid), "Serial Number");
		for (int i = 0; i < sizeof(hwid); i++) {
			sn[i * 2] = hex[hwid[i] >> 4];
			sn[i * 2 + 1] = hex[hwid[i] & 0xF];
		}
	}

	memcpy(desc->bString, sn, strlen(sn));
}

/*
 * Validate endpoint address and update the endpoint descriptors at runtime,
 * the result depends on the capabilities of the driver and the number and
 * type of endpoints.
 */
static int validate_ep_cfg_data(struct usb_ep_descriptor *const ep_desc,
				struct usbd_class_ctx *const cctx,
				uint32_t *ep_bm)
{
	struct usb_dc_ep_cfg_data ep_cfg;

	ep_cfg.ep_type = (ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK);
	ep_cfg.ep_mps = ep_desc->wMaxPacketSize;
	ep_cfg.ep_addr = ep_desc->bEndpointAddress;

	for (uint8_t idx = 1; idx < 16; idx++) {

		if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
			if (*ep_bm & (1 << (idx + 16))) {
				LOG_DBG("EP 0x%02x occupied",
					USB_EP_DIR_IN | idx);
				continue;
			}

			ep_cfg.ep_addr = (USB_EP_DIR_IN | idx);
		} else {
			if (*ep_bm & (1 << idx)) {
				LOG_DBG("EP 0x%02x occupied", idx);
				continue;
			}

			ep_cfg.ep_addr = idx;
		}
		if (!usb_dc_ep_check_cap(&ep_cfg)) {
			LOG_INF("Change EP address %x -> %x | 0x%02x",
				ep_desc->bEndpointAddress,
				ep_cfg.ep_addr, *ep_bm);

			ep_desc->bEndpointAddress = ep_cfg.ep_addr;

			if (ep_cfg.ep_addr & USB_EP_DIR_IN) {
				*ep_bm |= (1 << (idx + 16));
				cctx->ep_bm |= (1 << (idx + 16));
			} else {
				*ep_bm |= (1 << idx);
				cctx->ep_bm |= (1 << idx);
			}

			return 0;
		}
	}

	LOG_ERR("Failed to validate endpoint");

	return -1;
}

/**
 * @brief Assign bInterfaceNumber to class instance
 *
 * The total number of interfaces is stored in the
 * configuration descriptor's value bNumInterfaces.
 * This value is reset at the beginning and
 * is increased according to the number of interfaces.
 * The respective bInterfaceNumber must be assigned to
 * all interfaces in registered instances.
 *
 * @param[in] cctx Class context struct of the class instance
 *
 * @return 0 on success, other values on fail.
 */
static int usbd_cctx_assign_iface_num(struct usbd_class_ctx *const cctx)
{
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_desc_header *head = cctx->class_desc;
	uint8_t *ptr = (uint8_t *)head;
	uint8_t n_if = usbd_ctx.cfg_desc.bNumInterfaces;

	while (head->bLength != 0) {

		switch (head->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (struct usb_if_descriptor *)ptr;

			if (if_desc->bAlternateSetting == 0) {
				if_desc->bInterfaceNumber = n_if;
				n_if++;
			} else {
				if_desc->bInterfaceNumber = n_if - 1;
			}

			LOG_WRN("Descriptor %p, bInterfaceNumber %u",
				if_desc, if_desc->bInterfaceNumber);
			break;
		default:
			break;
		}

		ptr += head->bLength;
		head = (struct usb_desc_header *)ptr;
	}

	usbd_ctx.cfg_desc.bNumInterfaces = n_if;

	return 0;
}

/**
 * @brief Assign address to all endpoints of a class instance
 *
 * Like bInterfaceNumber the endpoint addresses must be assigned
 * for all registered instances.
 * The occupied endpoint addresses are saved in a bitmap.
 * The IN endpoints are mapped in the upper nibble.
 * Bitmap is available globally in the device context for all
 * existing endpoints and locally in the respective class context
 * for the endpoints of an instance.
 *
 * @param[in] cctx Class context struct of the class instance
 *
 * @return 0 on success, other values on fail.
 */
static int usbd_cctx_assign_ep_addr(struct usbd_class_ctx *const cctx)
{
	struct usb_desc_header *head = cctx->class_desc;
	uint8_t *ptr = (uint8_t *)head;
	struct usb_if_descriptor *if_desc = NULL;
	struct usb_ep_descriptor *ep_desc = NULL;
	uint32_t tmp_ep_bm = usbd_ctx.ep_bm;
	uint32_t class_ep_bm = usbd_ctx.ep_bm;

	while (head->bLength != 0) {

		switch (head->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (struct usb_if_descriptor *)ptr;
			LOG_DBG("Interface descriptor %p", ptr);

			if (if_desc->bAlternateSetting == 0) {
				usbd_ctx.ep_bm = class_ep_bm;
			}

			class_ep_bm |= tmp_ep_bm;
			tmp_ep_bm = usbd_ctx.ep_bm;
			break;

		case USB_DESC_ENDPOINT:
			LOG_DBG("Endpoint descriptor %p", ptr);
			ep_desc = (struct usb_ep_descriptor *)ptr;
			if (validate_ep_cfg_data(ep_desc,
						 cctx,
						 &tmp_ep_bm)) {
				return -1;
			}

			break;

		default:
			break;
		}

		ptr += head->bLength;
		head = (struct usb_desc_header *)ptr;
	}

	class_ep_bm |= tmp_ep_bm;
	usbd_ctx.ep_bm = class_ep_bm;
	LOG_INF("EP bitmap 0x%02x, cctx bitmap 0x%02x",
		usbd_ctx.ep_bm, cctx->ep_bm);

	return 0;
}

const static uint8_t mfr_str[] = {CONFIG_USBD_DEVICE_MANUFACTURER};
const static uint8_t product_str[] = {CONFIG_USBD_DEVICE_PRODUCT};

/**
 * @brief Initialize descriptors
 *
 * Initialize device, configuration, string,
 * and all class descriptors.
 *
 * @return 0 on success, other values on fail.
 */
int usbd_init_desc(void)
{
	struct usbd_class_ctx *cctx;
	size_t cfg_len = 0;
	int ret;

	/* Workaround bString reinitialization */
	memcpy(&usbd_ctx.mfr_desc.bString, mfr_str, sizeof(mfr_str));
	memcpy(&usbd_ctx.product_desc.bString, product_str, sizeof(product_str));

	gen_sn_desc_from_hwinfo(&usbd_ctx.sn_desc);
	ascii7_to_utf16le(&usbd_ctx.mfr_desc);
	ascii7_to_utf16le(&usbd_ctx.product_desc);
	ascii7_to_utf16le(&usbd_ctx.sn_desc);

	/* FIXME: find better way to reinit */
	usbd_ctx.ep_bm = BIT(16) | BIT(0);
	usbd_ctx.cfg_desc.bNumInterfaces = 0;

	SYS_SLIST_FOR_EACH_CONTAINER(&usbd_ctx.class_list, cctx, node) {
		if (cctx->class_desc == NULL) {
			continue;
		}

		LOG_INF("New cctx %p, descriptor length %u",
			cctx, usbd_cctx_desc_len(cctx));

		ret = usbd_cctx_assign_iface_num(cctx);
		if (ret != 0) {
			LOG_ERR("Failed to assign interface numbers");
			return ret;
		}

		ret = usbd_cctx_assign_ep_addr(cctx);
		if (ret != 0) {
			LOG_ERR("Failed to assign endpoint addresses");
			return ret;
		}

		if (cctx->ops.init != NULL) {
			ret = cctx->ops.init(cctx);
			if (ret != 0) {
				return ret;
			}
		}

		cfg_len += usbd_cctx_desc_len(cctx);
	}

	sys_put_le16(sizeof(usbd_ctx.cfg_desc) + cfg_len,
		     (uint8_t *)&usbd_ctx.cfg_desc.wTotalLength);

	LOG_WRN("bNumInterfaces %u wTotalLength %u",
		usbd_ctx.cfg_desc.bNumInterfaces,
		usbd_ctx.cfg_desc.wTotalLength);

	return 0;
}
