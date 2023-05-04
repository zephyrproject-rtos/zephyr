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

#include "usbd_device.h"
#include "usbd_desc.h"
#include "usbd_ch9.h"
#include "usbd_config.h"
#include "usbd_class.h"
#include "usbd_class_api.h"
#include "usbd_interface.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_ch9, CONFIG_USBD_LOG_LEVEL);

#define CTRL_AWAIT_SETUP_DATA		0
#define CTRL_AWAIT_STATUS_STAGE		1

static bool reqtype_is_to_host(const struct usb_setup_packet *const setup)
{
	return setup->wLength && USB_REQTYPE_GET_DIR(setup->bmRequestType);
}

static bool reqtype_is_to_device(const struct usb_setup_packet *const setup)
{
	return !reqtype_is_to_host(setup);
}

static void ch9_set_ctrl_type(struct usbd_contex *const uds_ctx,
				   const int type)
{
	uds_ctx->ch9_data.ctrl_type = type;
}

static int ch9_get_ctrl_type(struct usbd_contex *const uds_ctx)
{
	return uds_ctx->ch9_data.ctrl_type;
}

static int set_address_after_status_stage(struct usbd_contex *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	int ret;

	ret = udc_set_address(uds_ctx->dev, setup->wValue);
	if (ret) {
		LOG_ERR("Failed to set device address 0x%x", setup->wValue);
		return ret;
	}

	uds_ctx->ch9_data.new_address = false;

	return ret;
}

static int sreq_set_address(struct usbd_contex *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wValue > 127 || setup->wLength) {
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

	uds_ctx->ch9_data.new_address = true;
	if (usbd_state_is_address(uds_ctx) && setup->wValue == 0) {
		uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;
	} else {
		uds_ctx->ch9_data.state = USBD_STATE_ADDRESS;
	}


	return 0;
}

static int sreq_set_configuration(struct usbd_contex *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
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

	if (setup->wValue && !usbd_config_exist(uds_ctx, setup->wValue)) {
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

	return ret;
}

static int sreq_set_interface(struct usbd_contex *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t cur_alt;
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

	if (usbd_get_alt_value(uds_ctx, setup->wIndex, &cur_alt)) {
		errno = -ENOTSUP;
		return 0;
	}

	LOG_INF("Set Interfaces %u, alternate %u -> %u",
		setup->wIndex, cur_alt, setup->wValue);

	if (setup->wValue == cur_alt) {
		return 0;
	}

	ret = usbd_interface_set(uds_ctx, setup->wIndex, setup->wValue);
	if (ret == -ENOENT) {
		LOG_INF("Interface alternate does not exist");
		errno = ret;
		ret = 0;
	}

	return ret;
}

static void sreq_feature_halt_notify(struct usbd_contex *const uds_ctx,
				     const uint8_t ep, const bool halted)
{
	struct usbd_class_node *c_nd = usbd_class_get_by_ep(uds_ctx, ep);

	if (c_nd != NULL) {
		usbd_class_feature_halt(c_nd, ep, halted);
	}
}

static int sreq_clear_feature(struct usbd_contex *const uds_ctx)
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

static int sreq_set_feature(struct usbd_contex *const uds_ctx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t ep = setup->wIndex;
	int ret = 0;

	/* Not specified if wLength is non-zero, treat as error */
	if (setup->wLength) {
		errno = -ENOTSUP;
		return 0;
	}

	/*
	 * TEST_MODE is not supported yet, other are not specified
	 * in default state, treat as error.
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

static int std_request_to_device(struct usbd_contex *const uds_ctx,
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

static int sreq_get_status(struct usbd_contex *const uds_ctx,
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

static int sreq_get_desc_cfg(struct usbd_contex *const uds_ctx,
			     struct net_buf *const buf,
			     const uint8_t idx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usb_cfg_descriptor *cfg_desc;
	struct usbd_config_node *cfg_nd;
	struct usbd_class_node *c_nd;
	uint16_t len;

	cfg_nd = usbd_config_get(uds_ctx, idx + 1);
	if (cfg_nd == NULL) {
		LOG_ERR("Configuration descriptor %u not found", idx + 1);
		errno = -ENOTSUP;
		return 0;
	}

	cfg_desc = cfg_nd->desc;
	len = MIN(setup->wLength, net_buf_tailroom(buf));
	net_buf_add_mem(buf, cfg_desc, MIN(len, cfg_desc->bLength));

	SYS_SLIST_FOR_EACH_CONTAINER(&cfg_nd->class_list, c_nd, node) {
		struct usb_desc_header *head = c_nd->data->desc;
		size_t desc_len = usbd_class_desc_len(c_nd);

		len = MIN(setup->wLength, net_buf_tailroom(buf));
		net_buf_add_mem(buf, head, MIN(len, desc_len));
	}

	LOG_DBG("Get Configuration descriptor %u, len %u", idx, buf->len);

	return 0;
}

static int sreq_get_desc(struct usbd_contex *const uds_ctx,
			 struct net_buf *const buf,
			 const uint8_t type, const uint8_t idx)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	struct usb_desc_header *head;
	size_t len;

	if (type == USB_DESC_DEVICE) {
		head = uds_ctx->desc;
	} else {
		head = usbd_get_descriptor(uds_ctx, type, idx);
	}

	if (head == NULL) {
		errno = -ENOTSUP;
		return 0;
	}

	len = MIN(setup->wLength, net_buf_tailroom(buf));
	net_buf_add_mem(buf, head, MIN(len, head->bLength));

	return 0;
}

static int sreq_get_descriptor(struct usbd_contex *const uds_ctx,
			       struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	uint8_t desc_type = USB_GET_DESCRIPTOR_TYPE(setup->wValue);
	uint8_t desc_idx = USB_GET_DESCRIPTOR_INDEX(setup->wValue);

	LOG_DBG("Get Descriptor request type %u index %u",
		desc_type, desc_idx);

	switch (desc_type) {
	case USB_DESC_DEVICE:
		return sreq_get_desc(uds_ctx, buf, USB_DESC_DEVICE, 0);
	case USB_DESC_CONFIGURATION:
		return sreq_get_desc_cfg(uds_ctx, buf, desc_idx);
	case USB_DESC_STRING:
		return sreq_get_desc(uds_ctx, buf, USB_DESC_STRING, desc_idx);
	case USB_DESC_INTERFACE:
	case USB_DESC_ENDPOINT:
	case USB_DESC_OTHER_SPEED:
	default:
		break;
	}

	errno = -ENOTSUP;
	return 0;
}

static int sreq_get_configuration(struct usbd_contex *const uds_ctx,
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

static int sreq_get_interface(struct usbd_contex *const uds_ctx,
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

	cfg_nd = usbd_config_get_current(uds_ctx);
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

static int std_request_to_host(struct usbd_contex *const uds_ctx,
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

static int nonstd_request(struct usbd_contex *const uds_ctx,
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
			ret = usbd_class_control_to_dev(c_nd, setup, dbuf);
		} else {
			ret = usbd_class_control_to_host(c_nd, setup, dbuf);
		}
	} else {
		errno = -ENOTSUP;
	}

	return ret;
}

static int handle_setup_request(struct usbd_contex *const uds_ctx,
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

static int ctrl_xfer_get_setup(struct usbd_contex *const uds_ctx,
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

int usbd_handle_ctrl_xfer(struct usbd_contex *const uds_ctx,
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
			if (unlikely(uds_ctx->ch9_data.new_address)) {
				return set_address_after_status_stage(uds_ctx);
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

int usbd_init_control_pipe(struct usbd_contex *const uds_ctx)
{
	uds_ctx->ch9_data.state = USBD_STATE_DEFAULT;
	ch9_set_ctrl_type(uds_ctx, CTRL_AWAIT_SETUP_DATA);

	return 0;
}
