/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"
#include "usbd_class.h"
#include "usbd_ch9.h"
#include "usbd_desc.h"
#include "usbd_endpoint.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_ep, CONFIG_USBD_LOG_LEVEL);

int usbd_ep_enable(const struct device *dev,
		   const struct usb_ep_descriptor *const ed,
		   uint32_t *const ep_bm)
{
	int ret;

	ret = udc_ep_enable(dev, ed->bEndpointAddress, ed->bmAttributes,
			    ed->wMaxPacketSize, ed->bInterval);
	if (ret == 0) {
		usbd_ep_bm_set(ep_bm, ed->bEndpointAddress);
	}

	return ret;
}

int usbd_ep_disable(const struct device *dev,
		    const uint8_t ep,
		    uint32_t *const ep_bm)
{
	int ret;

	ret = udc_ep_disable(dev, ep);
	if (ret) {
		return ret;
	}

	usbd_ep_bm_clear(ep_bm, ep);

	ret = udc_ep_dequeue(dev, ep);
	if (ret) {
		return ret;
	}

	k_yield();

	return ret;
}

static void usbd_ep_ctrl_set_zlp(struct usbd_contex *const uds_ctx,
				 struct net_buf *const buf)
{
	struct usb_setup_packet *setup = usbd_get_setup_pkt(uds_ctx);
	size_t min_len = MIN(setup->wLength, buf->len);

	if (buf->len == 0) {
		return;
	}

	/*
	 * Set ZLP flag when host asks for a bigger length and the
	 * last chunk is wMaxPacketSize long, to indicate the last
	 * packet.
	 */
	if (setup->wLength > min_len && !(min_len % USB_CONTROL_EP_MPS)) {
		/*
		 * Transfer length is less as requested by wLength and
		 * is multiple of wMaxPacketSize.
		 */
		LOG_DBG("add ZLP, wLength %u buf length %zu",
			setup->wLength, min_len);
		udc_ep_buf_set_zlp(buf);

	}
}

/*
 * All the functions below are part of public USB device support API.
 */

struct net_buf *usbd_ep_ctrl_buf_alloc(struct usbd_contex *const uds_ctx,
				       const uint8_t ep, const size_t size)
{
	if (USB_EP_GET_IDX(ep)) {
		/* Not a control endpoint */
		return NULL;
	}

	return udc_ep_buf_alloc(uds_ctx->dev, ep, size);
}

int usbd_ep_ctrl_enqueue(struct usbd_contex *const uds_ctx,
			 struct net_buf *const buf)
{
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	if (USB_EP_GET_IDX(bi->ep)) {
		/* Not a control endpoint */
		return -EINVAL;
	}

	if (USB_EP_DIR_IS_IN(bi->ep)) {
		if (usbd_is_suspended(uds_ctx)) {
			LOG_ERR("device is suspended");
			return -EPERM;
		}

		usbd_ep_ctrl_set_zlp(uds_ctx, buf);
	}

	return udc_ep_enqueue(uds_ctx->dev, buf);
}

struct net_buf *usbd_ep_buf_alloc(const struct usbd_class_node *const c_nd,
				  const uint8_t ep, const size_t size)
{
	struct usbd_contex *uds_ctx = usbd_class_get_ctx(c_nd);

	return udc_ep_buf_alloc(uds_ctx->dev, ep, size);
}

int usbd_ep_enqueue(const struct usbd_class_node *const c_nd,
		    struct net_buf *const buf)
{
	struct usbd_contex *uds_ctx = usbd_class_get_ctx(c_nd);
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	if (USB_EP_DIR_IS_IN(bi->ep)) {
		if (usbd_is_suspended(uds_ctx)) {
			return -EPERM;
		}
	}

	bi->owner = (void *)c_nd;

	return udc_ep_enqueue(uds_ctx->dev, buf);
}

int usbd_ep_buf_free(struct usbd_contex *const uds_ctx, struct net_buf *buf)
{
	return udc_ep_buf_free(uds_ctx->dev, buf);
}

int usbd_ep_dequeue(struct usbd_contex *const uds_ctx, const uint8_t ep)
{
	return udc_ep_dequeue(uds_ctx->dev, ep);
}

int usbd_ep_set_halt(struct usbd_contex *const uds_ctx, const uint8_t ep)
{
	struct usbd_ch9_data *ch9_data = &uds_ctx->ch9_data;
	int ret;

	ret = udc_ep_set_halt(uds_ctx->dev, ep);
	if (ret) {
		LOG_WRN("Set halt 0x%02x failed", ep);
		return ret;
	}

	usbd_ep_bm_set(&ch9_data->ep_halt, ep);

	return ret;
}

int usbd_ep_clear_halt(struct usbd_contex *const uds_ctx, const uint8_t ep)
{
	struct usbd_ch9_data *ch9_data = &uds_ctx->ch9_data;
	int ret;

	ret = udc_ep_clear_halt(uds_ctx->dev, ep);
	if (ret) {
		LOG_WRN("Clear halt 0x%02x failed", ep);
		return ret;
	}

	usbd_ep_bm_clear(&ch9_data->ep_halt, ep);

	return ret;
}

bool usbd_ep_is_halted(struct usbd_contex *const uds_ctx, const uint8_t ep)
{
	struct usbd_ch9_data *ch9_data = &uds_ctx->ch9_data;

	return usbd_ep_bm_is_set(&ch9_data->ep_halt, ep);
}
