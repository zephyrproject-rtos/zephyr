/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

#include "cmsis_dap.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap_usb, CONFIG_DAP_LOG_LEVEL);

/*
 * This file implements CMSIS DAP USB backend function using bulk endpoints.
 */

static uint8_t response_buf[512];

NET_BUF_POOL_FIXED_DEFINE(dap_func_pool,
			  1, 0, sizeof(struct udc_buf_info), NULL);

UDC_STATIC_BUF_DEFINE(dap_func_buf, 512);

struct dap_func_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

#define SAMPLE_FUNCTION_ENABLED		0

struct dap_func_data {
	struct dap_func_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct usbd_desc_node *const iface_str_desc_nd;
	atomic_t state;
};

static uint8_t dap_func_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t dap_func_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static int dap_func_request_handler(struct usbd_class_data *c_data,
				  struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = NULL;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	LOG_DBG("Transfer finished %p -> ep 0x%02x, len %u, err %d",
		(void *)c_data, bi->ep, buf->len, err);

	if (atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED) && err == 0) {
		uint8_t ep = bi->ep;
		size_t len;

		memset(bi, 0, sizeof(struct udc_buf_info));
		if (ep == dap_func_get_bulk_in(c_data)) {
			bi->ep = dap_func_get_bulk_out(c_data);
			net_buf_reset(buf);
		} else {
			bi->ep = dap_func_get_bulk_in(c_data);

			len = dap_execute_cmd(buf->data, response_buf);
			net_buf_reset(buf);
			LOG_DBG("response length %u, starting with [0x%02X, 0x%02X]",
				len, response_buf[0], response_buf[1]);
			net_buf_add_mem(buf, response_buf, MIN(len, net_buf_tailroom(buf)));
		}

		if (usbd_ep_enqueue(c_data, buf)) {
			LOG_ERR("Failed to enqueue buffer");
			usbd_ep_buf_free(uds_ctx, buf);
		}
	} else {
		LOG_ERR("Function is disabled or transfer failed");
		usbd_ep_buf_free(uds_ctx, buf);
	}

	return 0;
}

static void *dap_func_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

struct net_buf *dap_func_buf_alloc(struct usbd_class_data *const c_data,
				const uint8_t ep)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;
	size_t size;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		size = 512U;
	} else {
		size = 64U;
	}

	buf = net_buf_alloc_with_data(&dap_func_pool, dap_func_buf, size, K_NO_WAIT);
	net_buf_reset(buf);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}

static void dap_func_enable(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;

	LOG_INF("Configuration enabled");

	if (!atomic_test_and_set_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
		if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
			dap_update_pkt_size(512);
		} else {
			dap_update_pkt_size(64);
		}

		buf = dap_func_buf_alloc(c_data, dap_func_get_bulk_out(c_data));
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer");
			return;
		}

		if (usbd_ep_enqueue(c_data, buf)) {
			LOG_ERR("Failed to enqueue buffer");
			usbd_ep_buf_free(uds_ctx, buf);
		}
	}
}

static void dap_func_disable(struct usbd_class_data *const c_data)
{
	struct dap_func_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, SAMPLE_FUNCTION_ENABLED);
	LOG_INF("Configuration disabled");
}

static int dap_func_init(struct usbd_class_data *c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct dap_func_data *data = usbd_class_get_private(c_data);
	struct dap_func_desc *desc = data->desc;

	LOG_DBG("Init class instance %p", (void *)c_data);

	if (usbd_add_descriptor(uds_ctx, data->iface_str_desc_nd)) {
		LOG_ERR("Failed to add interface string descriptor");
	} else {
		desc->if0.iInterface = usbd_str_desc_get_idx(data->iface_str_desc_nd);
	}

	return 0;
}

struct usbd_class_api dap_func_api = {
	.request = dap_func_request_handler,
	.get_desc = dap_func_get_desc,
	.enable = dap_func_enable,
	.disable = dap_func_disable,
	.init = dap_func_init,
};

#define DAP_FUNC_DESCRIPTOR_DEFINE(n, _)					\
static struct dap_func_desc dap_func_desc_##n = {				\
	/* Interface descriptor 0 */						\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_VENDOR,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	/* Endpoint OUT */							\
	.if0_out_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Endpoint IN */							\
	.if0_in_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* High-speed Endpoint OUT */						\
	.if0_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* High-speed Endpoint IN */						\
	.if0_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512),				\
		.bInterval = 0x00,						\
	},									\
										\
	/* Termination descriptor */						\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *dap_func_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0,			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_out_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_in_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.nil_desc,			\
};										\
										\
const static struct usb_desc_header *dap_func_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0,			\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_hs_out_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.if0_hs_in_ep,		\
	(struct usb_desc_header *) &dap_func_desc_##n.nil_desc,			\
};


#define DAP_FUNC_FUNCTION_DATA_DEFINE(n, _)					\
	USBD_DESC_STRING_DEFINE(iface_str_desc_nd_##n,				\
				"CMSIS-DAP v2",					\
				USBD_DUT_STRING_INTERFACE);			\
										\
	static struct dap_func_data dap_func_data_##n = {			\
		.desc = &dap_func_desc_##n,					\
		.fs_desc = dap_func_fs_desc_##n,				\
		.hs_desc = dap_func_hs_desc_##n,				\
		.iface_str_desc_nd = &iface_str_desc_nd_##n,			\
	};									\
										\
	USBD_DEFINE_CLASS(dap_func_##n, &dap_func_api, &dap_func_data_##n, NULL);

LISTIFY(1, DAP_FUNC_DESCRIPTOR_DEFINE, ())
LISTIFY(1, DAP_FUNC_FUNCTION_DATA_DEFINE, ())
