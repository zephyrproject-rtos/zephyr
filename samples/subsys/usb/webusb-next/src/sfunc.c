/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sfunc, LOG_LEVEL_INF);

#include <zephyr/usb/usbd.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/sys/byteorder.h>

/*
 * This file implements a simple USB function that echoes received data back to
 * the host using bulk endpoints.
 */

NET_BUF_POOL_FIXED_DEFINE(sfunc_pool,
			  1, 0, sizeof(struct udc_buf_info), NULL);

UDC_STATIC_BUF_DEFINE(sfunc_buf, 512);

struct sfunc_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_out_ep;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

#define SAMPLE_FUNCTION_ENABLED		0

struct sfunc_data {
	struct sfunc_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	atomic_t state;
};

static uint8_t sfunc_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct sfunc_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct sfunc_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_out_ep.bEndpointAddress;
	}

	return desc->if0_out_ep.bEndpointAddress;
}

static uint8_t sfunc_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct sfunc_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct sfunc_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_in_ep.bEndpointAddress;
	}

	return desc->if0_in_ep.bEndpointAddress;
}

static int sfunc_request_handler(struct usbd_class_data *c_data,
				  struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct sfunc_data *data = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = NULL;

	bi = (struct udc_buf_info *)net_buf_user_data(buf);
	LOG_INF("Transfer finished %p -> ep 0x%02x, len %u, err %d",
		(void *)c_data, bi->ep, buf->len, err);

	if (atomic_test_bit(&data->state, SAMPLE_FUNCTION_ENABLED) && err == 0) {
		uint8_t ep = bi->ep;

		memset(bi, 0, sizeof(struct udc_buf_info));

		if (ep == sfunc_get_bulk_in(c_data)) {
			bi->ep = sfunc_get_bulk_out(c_data);
			net_buf_reset(buf);
		} else {
			bi->ep = sfunc_get_bulk_in(c_data);
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

static void *sfunc_get_desc(struct usbd_class_data *const c_data,
			     const enum usbd_speed speed)
{
	struct sfunc_data *data = usbd_class_get_private(c_data);

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

struct net_buf *sfunc_buf_alloc(struct usbd_class_data *const c_data,
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

	buf = net_buf_alloc_with_data(&sfunc_pool, sfunc_buf, size, K_NO_WAIT);
	net_buf_reset(buf);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static void sfunc_enable(struct usbd_class_data *const c_data)
{
	struct sfunc_data *data = usbd_class_get_private(c_data);
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	struct net_buf *buf;

	LOG_INF("Configuration enabled");

	if (!atomic_test_and_set_bit(&data->state, SAMPLE_FUNCTION_ENABLED)) {
		buf = sfunc_buf_alloc(c_data, sfunc_get_bulk_out(c_data));
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

static void sfunc_disable(struct usbd_class_data *const c_data)
{
	struct sfunc_data *data = usbd_class_get_private(c_data);

	atomic_clear_bit(&data->state, SAMPLE_FUNCTION_ENABLED);
	LOG_INF("Configuration disabled");
}

static int sfunc_init(struct usbd_class_data *c_data)
{
	LOG_DBG("Init class instance %p", (void *)c_data);

	return 0;
}

struct usbd_class_api sfunc_api = {
	.request = sfunc_request_handler,
	.get_desc = sfunc_get_desc,
	.enable = sfunc_enable,
	.disable = sfunc_disable,
	.init = sfunc_init,
};

#define SFUNC_DESCRIPTOR_DEFINE(n, _)						\
static struct sfunc_desc sfunc_desc_##n = {					\
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
const static struct usb_desc_header *sfunc_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &sfunc_desc_##n.if0,				\
	(struct usb_desc_header *) &sfunc_desc_##n.if0_in_ep,			\
	(struct usb_desc_header *) &sfunc_desc_##n.if0_out_ep,			\
	(struct usb_desc_header *) &sfunc_desc_##n.nil_desc,			\
};										\
										\
const static struct usb_desc_header *sfunc_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &sfunc_desc_##n.if0,				\
	(struct usb_desc_header *) &sfunc_desc_##n.if0_hs_in_ep,		\
	(struct usb_desc_header *) &sfunc_desc_##n.if0_hs_out_ep,		\
	(struct usb_desc_header *) &sfunc_desc_##n.nil_desc,			\
};


#define SFUNC_FUNCTION_DATA_DEFINE(n, _)					\
	static struct sfunc_data sfunc_data_##n = {				\
		.desc = &sfunc_desc_##n,					\
		.fs_desc = sfunc_fs_desc_##n,					\
		.hs_desc = sfunc_hs_desc_##n,					\
	};									\
										\
	USBD_DEFINE_CLASS(sfunc_##n, &sfunc_api, &sfunc_data_##n, NULL);

LISTIFY(1, SFUNC_DESCRIPTOR_DEFINE, ())
LISTIFY(1, SFUNC_FUNCTION_DATA_DEFINE, ())
