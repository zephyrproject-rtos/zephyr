/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/drivers/hwinfo.h>

#include "usbd_device.h"
#include "usbd_desc.h"
#include "usbd_ch9.h"
#include "usbd_config.h"
#include "usbd_class.h"
#include "usbd_class_api.h"
#include "usbd_interface.h"
#include "usbd_msg.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_ch9, CONFIG_USBD_LOG_LEVEL);

#define CTRL_AWAIT_SETUP_DATA		0
#define CTRL_AWAIT_STATUS_STAGE		1

#define SF_TEST_MODE_SELECTOR(wIndex)		((uint8_t)((wIndex) >> 8))
#define SF_TEST_LOWER_BYTE(wIndex)		((uint8_t)(wIndex))

static int nonstd_request(struct usbd_context *const uds_ctx,
			  struct net_buf *const dbuf);

static bool reqtype_is_to_host(const struct usb_setup_packet *const setup)
{
	return setup->wLength && USB_REQTYPE_GET_DIR(setup->bmRequestType);
}

static bool reqtype_is_to_device(const struct usb_setup_packet *const setup)
{
	return !reqtype_is_to_host(setup);
}

static void ch9_set_ctrl_type(struct usbd_context *const uds_ctx,
				   const int type)
{
	uds_ctx->ch9_data.ctrl_type = type;
}

static int ch9_get_ctrl_type(struct usbd_context *const uds_ctx)
{
	return uds_ctx->ch9_data.ctrl_type;
}

static int post_status_stage(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret = 0;

	if (setup->bRequest == USB_SREQ_SET_ADDRESS) {
		ret = udc_set_address(uds_ctx->dev, setup->wValue);
		if (ret) {
			LOG_ERR("Failed to set device address 0x%x", setup->wValue);
		}
	}

	if (setup->bRequest == USB_SREQ_SET_FEATURE &&
	    setup->wValue == USB_SFS_TEST_MODE) {
		uint8_t mode = SF_TEST_MODE_SELECTOR(setup->wIndex);

		ret = udc_test_mode(uds_ctx->dev, mode, false);
		if (ret) {
			LOG_ERR("Failed to enable TEST_MODE %u", mode);
		}
	}

	uds_ctx->ch9_data.post_status = false;

	return ret;
}

static int sreq_set_address(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct udc_device_caps caps = udc_caps(uds_ctx->dev);

	/* Not specified if wIndex or wLength is non-zero, treat as error */
	if (setup->wValue > 127 || setup->wIndex || setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (usbd_state_is_configured(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (caps.addr_before_status) {
		int ret;

		ret = udc_set_address(uds_ctx->dev, setup->wValue);
		if (ret) {
			LOG_ERR("Failed to set device address 0x%x", setup->wValue);
			return ret;
		}
	} else {
		uds_ctx->ch9_data.post_status = true;
	}

	if (usbd_state_is_address(uds_ctx) && setup->wValue == 0) {
		uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;
	} else {
		uds_ctx->ch9_data.state = USBD_STATE_ADDRESS;
	}


	return 0;
}

static int sreq_set_configuration(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	const enum usbd_speed speed = usbd_bus_speed(uds_ctx);
	int ret;

	LOG_INF("Set Configuration Request value %u", setup->wValue);

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wValue > UINT8_MAX || setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		errno = -ENOTSUP;
		return 0;
	}

	if (usbd_state_is_default(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (setup->wValue && !usbd_config_exist(uds_ctx, speed, setup->wValue)) {
		errno = -EPERM;
		return 0;
	}

	if (setup->wValue == usbd_get_config_value(uds_ctx)) {
		LOG_DBG("Already in the configuration %u", setup->wValue);
		return 0;
	}

	ret = usbd_config_set(uds_ctx, setup->wValue);
	if (ret) {
		LOG_ERR("Failed to set configuration %u, %d",
			setup->wValue, ret);
		return ret;
	}

	if (setup->wValue == 0) {
		/* Enter address state */
		uds_ctx->ch9_data.state = USBD_STATE_ADDRESS;
	} else {
		uds_ctx->ch9_data.state = USBD_STATE_CONFIGURED;
	}

	if (ret == 0) {
		usbd_msg_pub_simple(uds_ctx, USBD_MSG_CONFIGURATION, setup->wValue);
	}

	return ret;
}

static int sreq_set_interface(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret;

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE) {
		errno = -ENOTSUP;
		return 0;
	}

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	if (setup->wValue > UINT8_MAX || setup->wIndex > UINT8_MAX) {
		errno = -ENOTSUP;
		return 0;
	}

	if (!usbd_state_is_configured(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	ret = usbd_interface_set(uds_ctx, setup->wIndex, setup->wValue);
	if (ret == -ENOENT) {
		LOG_INF("Interface or alternate does not exist");
		errno = ret;
		ret = 0;
	}

	return ret;
}

static void sreq_feature_halt_notify(struct usbd_context *const uds_ctx,
				     const uint8_t ep, const bool halted)
{
	struct usbd_class_node *c_nd = usbd_class_get_by_ep(uds_ctx, ep);

	if (c_nd != NULL) {
		usbd_class_feature_halt(c_nd->c_data, ep, halted);
	}
}

static int sreq_clear_feature(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t ep = setup->wIndex;
	int ret = 0;

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	/* Not specified in default state, treat as error */
	if (usbd_state_is_default(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (usbd_state_is_address(uds_ctx) && setup->wIndex) {
		errno = -EPERM;
		return 0;
	}

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		if (setup->wIndex != 0) {
			errno = -EPERM;
			return 0;
		}

		if (setup->wValue == USB_SFS_REMOTE_WAKEUP) {
			LOG_DBG("Clear feature remote wakeup");
			uds_ctx->status.rwup = false;
		}
		break;
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		if (setup->wValue == USB_SFS_ENDPOINT_HALT) {
			/* UDC checks if endpoint is enabled */
			errno = usbd_ep_clear_halt(uds_ctx, ep);
			ret = (errno == -EPERM) ? errno : 0;
			if (ret == 0) {
				/* Notify class instance */
				sreq_feature_halt_notify(uds_ctx, ep, false);
			}
			break;
		}
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
	default:
		break;
	}

	return ret;
}

static int set_feature_test_mode(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t mode = SF_TEST_MODE_SELECTOR(setup->wIndex);

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE ||
	    SF_TEST_LOWER_BYTE(setup->wIndex) != 0) {
		errno = -ENOTSUP;
		return 0;
	}

	if (udc_test_mode(uds_ctx->dev, mode, true) != 0) {
		errno = -ENOTSUP;
		return 0;
	}

	uds_ctx->ch9_data.post_status = true;

	return 0;
}

static int sreq_set_feature(struct usbd_context *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t ep = setup->wIndex;
	int ret = 0;

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	if (unlikely(setup->wValue == USB_SFS_TEST_MODE)) {
		return set_feature_test_mode(uds_ctx);
	}

	/*
	 * Other request behavior is not specified in the default state, treat
	 * as an error.
	 */
	if (usbd_state_is_default(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (usbd_state_is_address(uds_ctx) && setup->wIndex) {
		errno = -EPERM;
		return 0;
	}

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		if (setup->wIndex != 0) {
			errno = -EPERM;
			return 0;
		}

		if (setup->wValue == USB_SFS_REMOTE_WAKEUP) {
			LOG_DBG("Set feature remote wakeup");
			uds_ctx->status.rwup = true;
		}
		break;
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		if (setup->wValue == USB_SFS_ENDPOINT_HALT) {
			/* UDC checks if endpoint is enabled */
			errno = usbd_ep_set_halt(uds_ctx, ep);
			ret = (errno == -EPERM) ? errno : 0;
			if (ret == 0) {
				/* Notify class instance */
				sreq_feature_halt_notify(uds_ctx, ep, true);
			}
			break;
		}
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
	default:
		break;
	}

	return ret;
}

static int std_request_to_device(struct usbd_context *const uds_ctx,
				 struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_SET_ADDRESS:
		ret = sreq_set_address(uds_ctx);
		break;
	case USB_SREQ_SET_CONFIGURATION:
		ret = sreq_set_configuration(uds_ctx);
		break;
	case USB_SREQ_SET_INTERFACE:
		ret = sreq_set_interface(uds_ctx);
		break;
	case USB_SREQ_CLEAR_FEATURE:
		ret = sreq_clear_feature(uds_ctx);
		break;
	case USB_SREQ_SET_FEATURE:
		ret = sreq_set_feature(uds_ctx);
		break;
	default:
		errno = -ENOTSUP;
		ret = 0;
		break;
	}

	return ret;
}

static int sreq_get_status(struct usbd_context *const uds_ctx,
			   struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t ep = setup->wIndex;
	uint16_t response = 0;

	if (setup->wLength != sizeof(response)) {
		errno = -ENOTSUP;
		return 0;
	}

	/* Not specified in default state, treat as error */
	if (usbd_state_is_default(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (usbd_state_is_address(uds_ctx) && setup->wIndex) {
		errno = -EPERM;
		return 0;
	}

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_DEVICE:
		if (setup->wIndex != 0) {
			errno = -EPERM;
			return 0;
		}

		response = uds_ctx->status.rwup ?
			   USB_GET_STATUS_REMOTE_WAKEUP : 0;
		break;
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		response = usbd_ep_is_halted(uds_ctx, ep) ? BIT(0) : 0;
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
		/* Response is always reset to zero.
		 * TODO: add check if interface exist?
		 */
		break;
	default:
		break;
	}

	if (net_buf_tailroom(buf) < setup->wLength) {
		errno = -ENOMEM;
		return 0;
	}

	LOG_DBG("Get Status response 0x%04x", response);
	net_buf_add_le16(buf, response);

	return 0;
}

/*
 * This function handles configuration and USB2.0 other-speed-configuration
 * descriptor type requests.
 */
static int sreq_get_desc_cfg(struct usbd_context *const uds_ctx,
			     struct net_buf *const buf,
			     const uint8_t idx,
			     const bool other_cfg)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	enum usbd_speed speed = usbd_bus_speed(uds_ctx);
	struct usb_cfg_descriptor other_desc;
	struct usb_cfg_descriptor *cfg_desc;
	struct usbd_config_node *cfg_nd;
	enum usbd_speed get_desc_speed;
	struct usbd_class_node *c_nd;
	uint16_t len;

	/*
	 * If the other-speed-configuration-descriptor is requested and the
	 * controller does not support high speed, respond with an error.
	 */
	if (other_cfg && usbd_caps_speed(uds_ctx) != USBD_SPEED_HS) {
		errno = -ENOTSUP;
		return 0;
	}

	if (other_cfg) {
		if (speed == USBD_SPEED_FS) {
			get_desc_speed = USBD_SPEED_HS;
		} else {
			get_desc_speed = USBD_SPEED_FS;
		}
	} else {
		get_desc_speed = speed;
	}

	cfg_nd = usbd_config_get(uds_ctx, get_desc_speed, idx + 1);
	if (cfg_nd == NULL) {
		LOG_ERR("Configuration descriptor %u not found", idx + 1);
		errno = -ENOTSUP;
		return 0;
	}

	if (other_cfg) {
		/* Copy the configuration descriptor and update the type */
		memcpy(&other_desc, cfg_nd->desc, sizeof(other_desc));
		other_desc.bDescriptorType = USB_DESC_OTHER_SPEED;
		cfg_desc = &other_desc;
	} else {
		cfg_desc = cfg_nd->desc;
	}

	net_buf_add_mem(buf, cfg_desc, MIN(net_buf_tailroom(buf), cfg_desc->bLength));

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		struct usb_desc_header **dhp;

		dhp = usbd_class_get_desc(c_nd->c_data, get_desc_speed);
		if (dhp == NULL) {
			continue;
		}

		while (*dhp != NULL && (*dhp)->bLength != 0) {
			len = MIN(net_buf_tailroom(buf), (*dhp)->bLength);
			net_buf_add_mem(buf, *dhp, len);
			dhp++;
		}
	}

	if (buf->len > setup->wLength) {
		net_buf_remove_mem(buf, buf->len - setup->wLength);
	}

	LOG_DBG("Get Configuration descriptor %u, len %u", idx, buf->len);

	return 0;
}

#define USBD_HWID_SN_MAX 32U

/* Generate valid USB device serial number from hwid */
static ssize_t get_sn_from_hwid(uint8_t sn[static USBD_HWID_SN_MAX])
{
	static const char hex[] = "0123456789ABCDEF";
	uint8_t hwid[USBD_HWID_SN_MAX / 2U];
	ssize_t hwid_len = -ENOSYS;

	if (IS_ENABLED(CONFIG_HWINFO)) {
		hwid_len = hwinfo_get_device_id(hwid, sizeof(hwid));
	}

	if (hwid_len < 0) {
		if (hwid_len == -ENOSYS) {
			LOG_ERR("HWINFO not implemented or enabled");
		}

		return hwid_len;
	}

	for (ssize_t i = 0; i < hwid_len; i++) {
		sn[i * 2] = hex[hwid[i] >> 4];
		sn[i * 2 + 1] = hex[hwid[i] & 0xF];
	}

	return hwid_len * 2;
}

/* Copy and convert ASCII-7 string descriptor to UTF16-LE */
static void string_ascii7_to_utf16le(struct usbd_desc_node *const dn,
				     struct net_buf *const buf, const uint16_t wLength)
{
	uint8_t hwid_sn[USBD_HWID_SN_MAX];
	struct usb_desc_header head = {
		.bDescriptorType = dn->bDescriptorType,
	};
	uint8_t *ascii7_str;
	size_t len;
	size_t i;

	if (dn->str.utype == USBD_DUT_STRING_SERIAL_NUMBER && dn->str.use_hwinfo) {
		ssize_t hwid_len = get_sn_from_hwid(hwid_sn);

		if (hwid_len < 0) {
			errno = -ENOTSUP;
			return;
		}

		head.bLength = sizeof(head) + hwid_len * 2;
		ascii7_str = hwid_sn;
	} else {
		head.bLength = dn->bLength;
		ascii7_str = (uint8_t *)dn->ptr;
	}

	LOG_DBG("wLength %u, bLength %u, tailroom %u",
		wLength, head.bLength, net_buf_tailroom(buf));

	len = MIN(net_buf_tailroom(buf), MIN(head.bLength,  wLength));

	/* Add bLength and bDescriptorType */
	net_buf_add_mem(buf, &head, MIN(len, sizeof(head)));
	len -= MIN(len, sizeof(head));

	for (i = 0; i < len / 2; i++) {
		__ASSERT(ascii7_str[i] > 0x1F && ascii7_str[i] < 0x7F,
			 "Only printable ascii-7 characters are allowed in USB "
			 "string descriptors");
		net_buf_add_le16(buf, ascii7_str[i]);
	}

	if (len & 1) {
		net_buf_add_u8(buf, ascii7_str[i]);
	}
}

static int sreq_get_desc_dev(struct usbd_context *const uds_ctx,
			     struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usb_desc_header *head;
	size_t len;

	len = MIN(setup->wLength, net_buf_tailroom(buf));

	switch (usbd_bus_speed(uds_ctx)) {
	case USBD_SPEED_FS:
		head = uds_ctx->fs_desc;
		break;
	case USBD_SPEED_HS:
		head = uds_ctx->hs_desc;
		break;
	default:
		errno = -ENOTSUP;
		return 0;
	}

	net_buf_add_mem(buf, head, MIN(len, head->bLength));

	return 0;
}

static int sreq_get_desc_str(struct usbd_context *const uds_ctx,
			     struct net_buf *const buf, const uint8_t idx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usbd_desc_node *d_nd;
	size_t len;

	/* Get string descriptor */
	d_nd = usbd_get_descriptor(uds_ctx, USB_DESC_STRING, idx);
	if (d_nd == NULL) {
		errno = -ENOTSUP;
		return 0;
	}

	if (usbd_str_desc_get_idx(d_nd) == 0U) {
		/* Language ID string descriptor */
		struct usb_string_descriptor langid = {
			.bLength = d_nd->bLength,
			.bDescriptorType = d_nd->bDescriptorType,
			.bString =  *(uint16_t *)d_nd->ptr,
		};

		len = MIN(setup->wLength, net_buf_tailroom(buf));
		net_buf_add_mem(buf, &langid, MIN(len, langid.bLength));
	} else {
		/* String descriptors in ASCII7 format */
		string_ascii7_to_utf16le(d_nd, buf, setup->wLength);
	}

	return 0;
}

static int sreq_get_dev_qualifier(struct usbd_context *const uds_ctx,
				  struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	/* At Full-Speed we want High-Speed descriptor and vice versa */
	struct usb_device_descriptor *d_desc =
		usbd_bus_speed(uds_ctx) == USBD_SPEED_FS ?
		uds_ctx->hs_desc : uds_ctx->fs_desc;
	struct usb_device_qualifier_descriptor q_desc = {
		.bLength = sizeof(struct usb_device_qualifier_descriptor),
		.bDescriptorType = USB_DESC_DEVICE_QUALIFIER,
		.bcdUSB = d_desc->bcdUSB,
		.bDeviceClass = d_desc->bDeviceClass,
		.bDeviceSubClass = d_desc->bDeviceSubClass,
		.bDeviceProtocol = d_desc->bDeviceProtocol,
		.bMaxPacketSize0 = d_desc->bMaxPacketSize0,
		.bNumConfigurations = d_desc->bNumConfigurations,
		.bReserved = 0U,
	};
	size_t len;

	/*
	 * If the Device Qualifier descriptor is requested and the controller
	 * does not support high speed, respond with an error.
	 */
	if (usbd_caps_speed(uds_ctx) != USBD_SPEED_HS) {
		errno = -ENOTSUP;
		return 0;
	}

	LOG_DBG("Get Device Qualifier");
	len = MIN(setup->wLength, net_buf_tailroom(buf));
	net_buf_add_mem(buf, &q_desc, MIN(len, q_desc.bLength));

	return 0;
}

static void desc_fill_bos_root(struct usbd_context *const uds_ctx,
			       struct usb_bos_descriptor *const root)
{
	struct usbd_desc_node *desc_nd;

	root->bLength = sizeof(struct usb_bos_descriptor);
	root->bDescriptorType = USB_DESC_BOS;
	root->wTotalLength = root->bLength;
	root->bNumDeviceCaps = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(&uds_ctx->descriptors, desc_nd, node) {
		if (desc_nd->bDescriptorType == USB_DESC_BOS) {
			root->wTotalLength += desc_nd->bLength;
			root->bNumDeviceCaps++;
		}
	}
}

static int sreq_get_desc_bos(struct usbd_context *const uds_ctx,
			     struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usb_device_descriptor *dev_dsc;
	struct usb_bos_descriptor bos;
	struct usbd_desc_node *desc_nd;
	size_t len;

	switch (usbd_bus_speed(uds_ctx)) {
	case USBD_SPEED_FS:
		dev_dsc = uds_ctx->fs_desc;
		break;
	case USBD_SPEED_HS:
		dev_dsc = uds_ctx->hs_desc;
		break;
	default:
		errno = -ENOTSUP;
		return 0;
	}

	if (sys_le16_to_cpu(dev_dsc->bcdUSB) < 0x0201U) {
		errno = -ENOTSUP;
		return 0;
	}

	desc_fill_bos_root(uds_ctx, &bos);
	len = MIN(net_buf_tailroom(buf), MIN(setup->wLength, bos.wTotalLength));

	LOG_DBG("wLength %u, bLength %u, wTotalLength %u, tailroom %u",
		setup->wLength, bos.bLength, bos.wTotalLength, net_buf_tailroom(buf));

	net_buf_add_mem(buf, &bos, MIN(len, bos.bLength));

	len -= MIN(len, sizeof(bos));
	if (len == 0) {
		return 0;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&uds_ctx->descriptors, desc_nd, node) {
		if (desc_nd->bDescriptorType == USB_DESC_BOS) {
			LOG_DBG("bLength %u, len %u, tailroom %u",
				desc_nd->bLength, len, net_buf_tailroom(buf));
			net_buf_add_mem(buf, desc_nd->ptr, MIN(len, desc_nd->bLength));

			len -= MIN(len, desc_nd->bLength);
			if (len == 0) {
				break;
			}
		}
	}

	return 0;
}

static int sreq_get_descriptor(struct usbd_context *const uds_ctx,
			       struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t desc_type = USB_GET_DESCRIPTOR_TYPE(setup->wValue);
	uint8_t desc_idx = USB_GET_DESCRIPTOR_INDEX(setup->wValue);

	LOG_DBG("Get Descriptor request type %u index %u",
		desc_type, desc_idx);

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_DEVICE) {
		/*
		 * If the recipient is not the device then it is probably a
		 * class specific  request where wIndex is the interface
		 * number or endpoint and not the language ID. e.g. HID
		 * Class Get Descriptor request.
		 */
		return nonstd_request(uds_ctx, buf);
	}

	switch (desc_type) {
	case USB_DESC_DEVICE:
		return sreq_get_desc_dev(uds_ctx, buf);
	case USB_DESC_CONFIGURATION:
		return sreq_get_desc_cfg(uds_ctx, buf, desc_idx, false);
	case USB_DESC_OTHER_SPEED:
		return sreq_get_desc_cfg(uds_ctx, buf, desc_idx, true);
	case USB_DESC_STRING:
		return sreq_get_desc_str(uds_ctx, buf, desc_idx);
	case USB_DESC_DEVICE_QUALIFIER:
		return sreq_get_dev_qualifier(uds_ctx, buf);
	case USB_DESC_BOS:
		return sreq_get_desc_bos(uds_ctx, buf);
	case USB_DESC_INTERFACE:
	case USB_DESC_ENDPOINT:
	default:
		break;
	}

	errno = -ENOTSUP;
	return 0;
}

static int sreq_get_configuration(struct usbd_context *const uds_ctx,
				  struct net_buf *const buf)

{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t cfg = usbd_get_config_value(uds_ctx);

	/* Not specified in default state, treat as error */
	if (usbd_state_is_default(uds_ctx)) {
		errno = -EPERM;
		return 0;
	}

	if (setup->wLength != sizeof(cfg)) {
		errno = -ENOTSUP;
		return 0;
	}

	if (net_buf_tailroom(buf) < setup->wLength) {
		errno = -ENOMEM;
		return 0;
	}

	net_buf_add_u8(buf, cfg);

	return 0;
}

static int sreq_get_interface(struct usbd_context *const uds_ctx,
			      struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usb_cfg_descriptor *cfg_desc;
	struct usbd_config_node *cfg_nd;
	uint8_t cur_alt;

	if (setup->RequestType.recipient != USB_REQTYPE_RECIPIENT_INTERFACE) {
		errno = -EPERM;
		return 0;
	}

	/* Treat as error in default (not specified) and addressed states. */
	cfg_nd = usbd_config_get_current(uds_ctx);
	if (cfg_nd == NULL) {
		errno = -EPERM;
		return 0;
	}

	cfg_desc = cfg_nd->desc;

	if (setup->wIndex > UINT8_MAX ||
	    setup->wIndex > cfg_desc->bNumInterfaces) {
		errno = -ENOTSUP;
		return 0;
	}

	if (usbd_get_alt_value(uds_ctx, setup->wIndex, &cur_alt)) {
		errno = -ENOTSUP;
		return 0;
	}

	LOG_DBG("Get Interfaces %u, alternate %u",
		setup->wIndex, cur_alt);

	if (setup->wLength != sizeof(cur_alt)) {
		errno = -ENOTSUP;
		return 0;
	}

	if (net_buf_tailroom(buf) < setup->wLength) {
		errno = -ENOMEM;
		return 0;
	}

	net_buf_add_u8(buf, cur_alt);

	return 0;
}

static int std_request_to_host(struct usbd_context *const uds_ctx,
			       struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret;

	switch (setup->bRequest) {
	case USB_SREQ_GET_STATUS:
		ret = sreq_get_status(uds_ctx, buf);
		break;
	case USB_SREQ_GET_DESCRIPTOR:
		ret = sreq_get_descriptor(uds_ctx, buf);
		break;
	case USB_SREQ_GET_CONFIGURATION:
		ret = sreq_get_configuration(uds_ctx, buf);
		break;
	case USB_SREQ_GET_INTERFACE:
		ret = sreq_get_interface(uds_ctx, buf);
		break;
	default:
		errno = -ENOTSUP;
		ret = 0;
		break;
	}

	return ret;
}

static int vendor_device_request(struct usbd_context *const uds_ctx,
				 struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usbd_vreq_node *vreq_nd;

	vreq_nd = usbd_device_get_vreq(uds_ctx, setup->bRequest);
	if (vreq_nd == NULL) {
		errno = -ENOTSUP;
		return 0;
	}

	if (reqtype_is_to_device(setup) && vreq_nd->to_dev != NULL) {
		LOG_DBG("Vendor request 0x%02x to device", setup->bRequest);
		errno = vreq_nd->to_dev(uds_ctx, setup, buf);
		return 0;
	}

	if (reqtype_is_to_host(setup) && vreq_nd->to_host != NULL) {
		LOG_DBG("Vendor request 0x%02x to host", setup->bRequest);
		errno = vreq_nd->to_host(uds_ctx, setup, buf);
		return 0;
	}

	errno = -ENOTSUP;
	return 0;
}

static int nonstd_request(struct usbd_context *const uds_ctx,
			  struct net_buf *const dbuf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usbd_class_node *c_nd = NULL;
	int ret = 0;

	switch (setup->RequestType.recipient) {
	case USB_REQTYPE_RECIPIENT_ENDPOINT:
		c_nd = usbd_class_get_by_ep(uds_ctx, setup->wIndex);
		break;
	case USB_REQTYPE_RECIPIENT_INTERFACE:
		c_nd = usbd_class_get_by_iface(uds_ctx, setup->wIndex);
		break;
	case USB_REQTYPE_RECIPIENT_DEVICE:
		c_nd = usbd_class_get_by_req(uds_ctx, setup->bRequest);
		break;
	default:
		break;
	}

	if (c_nd != NULL) {
		if (reqtype_is_to_device(setup)) {
			ret = usbd_class_control_to_dev(c_nd->c_data, setup, dbuf);
		} else {
			ret = usbd_class_control_to_host(c_nd->c_data, setup, dbuf);
		}
	} else {
		return vendor_device_request(uds_ctx, dbuf);
	}

	return ret;
}

static int handle_setup_request(struct usbd_context *const uds_ctx,
				struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret;

	errno = 0;

	switch (setup->RequestType.type) {
	case USB_REQTYPE_TYPE_STANDARD:
		if (reqtype_is_to_device(setup)) {
			ret = std_request_to_device(uds_ctx, buf);
		} else {
			ret = std_request_to_host(uds_ctx, buf);
		}
		break;
	case USB_REQTYPE_TYPE_CLASS:
	case USB_REQTYPE_TYPE_VENDOR:
		ret = nonstd_request(uds_ctx, buf);
		break;
	default:
		errno = -ENOTSUP;
		ret = 0;
	}

	if (errno) {
		LOG_INF("protocol error:");
		LOG_HEXDUMP_INF(setup, sizeof(*setup), "setup:");
		if (errno == -ENOTSUP) {
			LOG_INF("not supported");
		}
		if (errno == -EPERM) {
			LOG_INF("not permitted in device state %u",
				uds_ctx->ch9_data.state);
		}
	}

	return ret;
}

static int ctrl_xfer_get_setup(struct usbd_context *const uds_ctx,
			       struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct net_buf *buf_b;
	struct udc_buf_info *bi, *bi_b;

	if (buf->len < sizeof(struct usb_setup_packet)) {
		return -EINVAL;
	}

	memcpy(setup, buf->data, sizeof(struct usb_setup_packet));

	setup->wValue = sys_le16_to_cpu(setup->wValue);
	setup->wIndex = sys_le16_to_cpu(setup->wIndex);
	setup->wLength = sys_le16_to_cpu(setup->wLength);

	bi = udc_get_buf_info(buf);

	buf_b = buf->frags;
	if (buf_b == NULL) {
		LOG_ERR("Buffer for data|status is missing");
		return -ENODATA;
	}

	bi_b = udc_get_buf_info(buf_b);

	if (reqtype_is_to_device(setup)) {
		if (setup->wLength) {
			if (!bi_b->data) {
				LOG_ERR("%p is not data", buf_b);
				return -EINVAL;
			}
		} else {
			if (!bi_b->status) {
				LOG_ERR("%p is not status", buf_b);
				return -EINVAL;
			}
		}
	} else {
		if (!setup->wLength) {
			LOG_ERR("device-to-host with wLength zero");
			return -ENOTSUP;
		}

		if (!bi_b->data) {
			LOG_ERR("%p is not data", buf_b);
			return -EINVAL;
		}

	}

	return 0;
}

static struct net_buf *spool_data_out(struct net_buf *const buf)
{
	struct net_buf *next_buf = buf;
	struct udc_buf_info *bi;

	while (next_buf) {
		LOG_INF("spool %p", next_buf);
		next_buf = net_buf_frag_del(NULL, next_buf);
		if (next_buf) {
			bi = udc_get_buf_info(next_buf);
			if (bi->status) {
				return next_buf;
			}
		}
	}

	return NULL;
}

int usbd_handle_ctrl_xfer(struct usbd_context *const uds_ctx,
			  struct net_buf *const buf, const int err)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct udc_buf_info *bi;
	int ret = 0;

	bi = udc_get_buf_info(buf);
	if (USB_EP_GET_IDX(bi->ep)) {
		LOG_ERR("Can only handle control requests");
		return -EIO;
	}

	if (err && err != -ENOMEM && !bi->setup) {
		if (err == -ECONNABORTED) {
			LOG_INF("Transfer 0x%02x aborted (bus reset?)", bi->ep);
			net_buf_unref(buf);
			return 0;
		}

		LOG_ERR("Control transfer for 0x%02x has error %d, halt",
			bi->ep, err);
		net_buf_unref(buf);
		return err;
	}

	LOG_INF("Handle control %p ep 0x%02x, len %u, s:%u d:%u s:%u",
		buf, bi->ep, buf->len, bi->setup, bi->data, bi->status);

	if (bi->setup && bi->ep == USB_CONTROL_EP_OUT) {
		struct net_buf *next_buf;

		if (ctrl_xfer_get_setup(uds_ctx, buf)) {
			LOG_ERR("Malformed setup packet");
			net_buf_unref(buf);
			goto ctrl_xfer_stall;
		}

		/* Remove setup packet buffer from the chain */
		next_buf = net_buf_frag_del(NULL, buf);
		if (next_buf == NULL) {
			LOG_ERR("Buffer for data|status is missing");
			goto ctrl_xfer_stall;
		}

		/*
		 * Handle request and data stage, next_buf is either
		 * data+status or status buffers.
		 */
		ret = handle_setup_request(uds_ctx, next_buf);
		if (ret) {
			net_buf_unref(next_buf);
			return ret;
		}

		if (errno) {
			/*
			 * Halt, only protocol errors are recoverable.
			 * Free data stage and linked status stage buffer.
			 */
			net_buf_unref(next_buf);
			goto ctrl_xfer_stall;
		}

		ch9_set_ctrl_type(uds_ctx, CTRL_AWAIT_STATUS_STAGE);
		if (reqtype_is_to_device(setup) && setup->wLength) {
			/* Enqueue STATUS (IN) buffer */
			next_buf = spool_data_out(next_buf);
			if (next_buf == NULL) {
				LOG_ERR("Buffer for status is missing");
				goto ctrl_xfer_stall;
			}

			ret = usbd_ep_ctrl_enqueue(uds_ctx, next_buf);
		} else {
			/* Enqueue DATA (IN) or STATUS (OUT) buffer */
			ret = usbd_ep_ctrl_enqueue(uds_ctx, next_buf);
		}

		return ret;
	}

	if (bi->status && bi->ep == USB_CONTROL_EP_OUT) {
		if (ch9_get_ctrl_type(uds_ctx) == CTRL_AWAIT_STATUS_STAGE) {
			LOG_INF("s-in-status finished");
		} else {
			LOG_WRN("Awaited s-in-status not finished");
		}

		net_buf_unref(buf);

		return 0;
	}

	if (bi->status && bi->ep == USB_CONTROL_EP_IN) {
		net_buf_unref(buf);

		if (ch9_get_ctrl_type(uds_ctx) == CTRL_AWAIT_STATUS_STAGE) {
			LOG_INF("s-(out)-status finished");
			if (unlikely(uds_ctx->ch9_data.post_status)) {
				ret = post_status_stage(uds_ctx);
			}
		} else {
			LOG_WRN("Awaited s-(out)-status not finished");
		}

		return ret;
	}

ctrl_xfer_stall:
	/*
	 * Halt only the endpoint over which the host expects
	 * data or status stage. This facilitates the work of the drivers.
	 *
	 * In the case there is -ENOMEM for data OUT stage halt
	 * control OUT endpoint.
	 */
	if (reqtype_is_to_host(setup)) {
		ret = udc_ep_set_halt(uds_ctx->dev, USB_CONTROL_EP_IN);
	} else if (setup->wLength) {
		uint8_t ep = (err == -ENOMEM) ? USB_CONTROL_EP_OUT : USB_CONTROL_EP_IN;

		ret = udc_ep_set_halt(uds_ctx->dev, ep);
	} else {
		ret = udc_ep_set_halt(uds_ctx->dev, USB_CONTROL_EP_IN);
	}

	ch9_set_ctrl_type(uds_ctx, CTRL_AWAIT_SETUP_DATA);

	return ret;
}

int usbd_init_control_pipe(struct usbd_context *const uds_ctx)
{
	uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;
	ch9_set_ctrl_type(uds_ctx, CTRL_AWAIT_SETUP_DATA);

	return 0;
}
