/*
 * Copyright (c) 2024 Intel Corporation
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/sys/__assert.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/drivers/usb/udc.h>
#include <zephyr/pmci/mctp/mctp_usb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mctp_usb, CONFIG_MCTP_LOG_LEVEL);

#include "libmctp-alloc.h"

#define MCTP_USB_DMTF_0 0x1A
#define MCTP_USB_DMTF_1 0xB4

#define MCTP_USB_ENABLED 0
#define MCTP_USB_NUM_INSTANCES CONFIG_MCTP_USB_CLASS_INSTANCES_COUNT

UDC_BUF_POOL_DEFINE(mctp_usb_ep_pool, MCTP_USB_NUM_INSTANCES * 2, USBD_MAX_BULK_MPS,
		    sizeof(struct udc_buf_info), NULL);

struct mctp_usb_class_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_fs_out_ep;
	struct usb_ep_descriptor if0_fs_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

struct mctp_usb_class_ctx {
	struct usbd_class_data *class_data;
	struct mctp_usb_class_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	struct mctp_usb_class_inst *inst;
	uint8_t inst_idx;
	struct net_buf *out_net_buf;
	uint8_t out_buf[USBD_MAX_BULK_MPS];
	struct k_work out_work;
	atomic_t state;
};

static struct net_buf *mctp_usb_class_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&mctp_usb_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static uint8_t mctp_usb_class_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(ctx->class_data)) == USBD_SPEED_HS) {
		return ctx->desc->if0_hs_in_ep.bEndpointAddress;
	}
#endif

	return ctx->desc->if0_fs_in_ep.bEndpointAddress;
}

static uint8_t mctp_usb_class_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(ctx->class_data)) == USBD_SPEED_HS) {
		return ctx->desc->if0_hs_out_ep.bEndpointAddress;
	}
#endif

	return ctx->desc->if0_fs_out_ep.bEndpointAddress;
}

static void mctp_usb_reset_rx_state(struct mctp_binding_usb *usb)
{
	if (usb->rx_pkt != NULL) {
		mctp_pktbuf_free(usb->rx_pkt);
		usb->rx_pkt = NULL;
	}

	usb->rx_data_idx = 0;
	usb->rx_state = STATE_WAIT_HDR_DMTF0;
}

int mctp_usb_tx(struct mctp_binding *binding, struct mctp_pktbuf *pkt)
{
	struct mctp_binding_usb *usb = CONTAINER_OF(binding, struct mctp_binding_usb, binding);
	struct usbd_class_data *c_data = usb->usb_class_data;
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);
	size_t len = mctp_pktbuf_size(pkt);
	struct net_buf *buf = NULL;
	int err;

	if (!atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
		return -EPERM;
	}

	if (len > MCTP_USB_MAX_PACKET_LENGTH) {
		return -E2BIG;
	}

	err = k_sem_take(&usb->tx_lock, K_MSEC(CONFIG_MCTP_USB_TX_TIMEOUT));
	if (err != 0) {
		LOG_ERR("Semaphore could not be obtained");
		return err;
	}

	usb->tx_buf[0] = MCTP_USB_DMTF_0;
	usb->tx_buf[1] = MCTP_USB_DMTF_1;
	usb->tx_buf[2] = 0;
	usb->tx_buf[3] = len;

	memcpy((void *)&usb->tx_buf[MCTP_USB_HEADER_SIZE], pkt->data, len);

	LOG_HEXDUMP_DBG(usb->tx_buf, len + MCTP_USB_HEADER_SIZE, "buf = ");

	if (usb->usb_class_data == NULL) {
		LOG_ERR("MCTP instance not found");
		return -ENODEV;
	}

	buf = mctp_usb_class_buf_alloc(mctp_usb_class_get_bulk_in(c_data));
	if (buf == NULL) {
		k_sem_give(&usb->tx_lock);
		LOG_ERR("Failed to allocate IN buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, usb->tx_buf, len + MCTP_USB_HEADER_SIZE);

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		k_sem_give(&usb->tx_lock);
		LOG_ERR("Failed to enqueue IN buffer");
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

int mctp_usb_start(struct mctp_binding *binding)
{
	struct mctp_binding_usb *usb = CONTAINER_OF(binding, struct mctp_binding_usb, binding);

	k_sem_init(&usb->tx_lock, 1, 1);
	mctp_binding_set_tx_enabled(binding, true);

	return 0;
}

static void mctp_usb_class_out_work(struct k_work *work)
{
	struct mctp_usb_class_ctx *ctx = CONTAINER_OF(work, struct mctp_usb_class_ctx, out_work);
	struct net_buf *buf;
	size_t buf_size = 0;

	if (!atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
		return;
	}

	/* Move data from net_buf to our ctx buffer so we can receive another USB packet */
	if (ctx->out_net_buf != NULL) {
		buf_size = ctx->out_net_buf->len;
		memcpy(ctx->out_buf, ctx->out_net_buf->data, ctx->out_net_buf->len);

		/* Free the current buffer and allocate another for OUT */
		net_buf_unref(ctx->out_net_buf);
	}

	buf = mctp_usb_class_buf_alloc(mctp_usb_class_get_bulk_out(ctx->class_data));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate OUT buffer");
		return;
	}

	if (usbd_ep_enqueue(ctx->class_data, buf)) {
		net_buf_unref(buf);
		LOG_ERR("Failed to enqueue OUT buffer");
	}

	/* Process the MCTP data */
	struct mctp_binding_usb *usb = (struct mctp_binding_usb *)ctx->inst->mctp_binding;

	LOG_DBG("size=%d", buf_size);
	LOG_HEXDUMP_DBG(ctx->out_buf, buf_size, "buf = ");

	for (int i = 0; i < buf_size; i++) {
		switch (usb->rx_state) {
		case STATE_WAIT_HDR_DMTF0: {
			if (ctx->out_buf[i] == MCTP_USB_DMTF_0) {
				usb->rx_state = STATE_WAIT_HDR_DMTF1;
			} else {
				LOG_ERR("Invalid DMTF0 %02X", ctx->out_buf[i]);
				return;
			}
			break;
		}
		case STATE_WAIT_HDR_DMTF1: {
			if (ctx->out_buf[i] == MCTP_USB_DMTF_1) {
				usb->rx_state = STATE_WAIT_HDR_RSVD0;
			} else {
				LOG_ERR("Invalid DMTF1 %02X", ctx->out_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				return;
			}
			break;
		}
		case STATE_WAIT_HDR_RSVD0: {
			if (ctx->out_buf[i] == 0) {
				usb->rx_state = STATE_WAIT_HDR_LEN;
			} else {
				LOG_ERR("Invalid RSVD0 %02X", ctx->out_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				return;
			}
			break;
		}
		case STATE_WAIT_HDR_LEN: {
			if (ctx->out_buf[i] > MCTP_USB_MAX_PACKET_LENGTH || ctx->out_buf[i] == 0) {
				LOG_ERR("Invalid LEN %02X", ctx->out_buf[i]);
				usb->rx_state = STATE_WAIT_HDR_DMTF0;
				return;
			}

			usb->rx_data_idx = 0;
			usb->rx_pkt = mctp_pktbuf_alloc(&usb->binding, ctx->out_buf[i]);
			if (usb->rx_pkt == NULL) {
				LOG_ERR("Could not allocate PKT buffer");
				mctp_usb_reset_rx_state(usb);
				return;
			}

			usb->rx_state = STATE_DATA;

			LOG_DBG("Expecting LEN=%d", (int)ctx->out_buf[i]);

			break;
		}
		case STATE_DATA: {
			usb->rx_pkt->data[usb->rx_data_idx++] = ctx->out_buf[i];

			if (usb->rx_data_idx == usb->rx_pkt->end) {
				LOG_DBG("Packet complete");
				mctp_bus_rx(&usb->binding, usb->rx_pkt);
				/* Explicitly set rx_pkt to NULL since it is not guaranteed */
				usb->rx_pkt = NULL;
				mctp_usb_reset_rx_state(usb);
			}

			break;
		}
		}
	}
}

static int mctp_usb_class_request(struct usbd_class_data *const c_data, struct net_buf *buf,
				  int err)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	LOG_DBG("request for EP 0x%x", bi->ep);

	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("request ep 0x%02x, len %u cancelled", bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed", bi->ep, buf->len);
		}

		goto exit;
	}

	if (bi->ep == mctp_usb_class_get_bulk_out(c_data)) {
		ctx->out_net_buf = buf;
		k_work_submit(&ctx->out_work);
	}

	if (bi->ep == mctp_usb_class_get_bulk_in(c_data)) {
		k_sem_give(&ctx->inst->mctp_binding->tx_lock);
		net_buf_unref(buf);
	}

exit:
	return 0;
}

static void *mctp_usb_class_get_desc(struct usbd_class_data *const c_data,
				     const enum usbd_speed speed)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return ctx->hs_desc;
	}

	return ctx->fs_desc;
}

static void mctp_usb_class_enable(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);

	if (!atomic_test_and_set_bit(&ctx->state, MCTP_USB_ENABLED)) {
		k_work_submit(&ctx->out_work);
	}

	LOG_DBG("Enabled %s", c_data->name);
}

static void mctp_usb_class_disable(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);

	atomic_clear_bit(&ctx->state, MCTP_USB_ENABLED);

	LOG_DBG("Disabled %s", c_data->name);
}

static int mctp_usb_class_init(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);
	size_t num_instances = 0;

	STRUCT_SECTION_COUNT(mctp_usb_class_inst, &num_instances);

	if (num_instances != MCTP_USB_NUM_INSTANCES) {
		LOG_ERR("The number of application instances (%d) does not match the number "
			"specified by CONFIG_MCTP_USB_CLASS_INSTANCES_COUNT (%d)",
			num_instances, MCTP_USB_NUM_INSTANCES);
		return -EINVAL;
	}

	STRUCT_SECTION_GET(mctp_usb_class_inst, ctx->inst_idx, &ctx->inst);

	ctx->class_data = c_data;
	ctx->state = 0;

	/* Share USB class data with the MCTP USB binding */
	ctx->inst->mctp_binding->usb_class_data = c_data;

	k_work_init(&ctx->out_work, mctp_usb_class_out_work);

	if (ctx->inst->sublcass == USBD_MCTP_SUBCLASS_MANAGEMENT_CONTROLLER ||
	    ctx->inst->sublcass == USBD_MCTP_SUBCLASS_MANAGED_DEVICE_ENDPOINT ||
	    ctx->inst->sublcass == USBD_MCTP_SUBCLASS_HOST_INTERFACE_ENDPOINT) {
		ctx->desc->if0.bInterfaceSubClass = ctx->inst->sublcass;
	} else {
		LOG_ERR("Invalid USB MCTP sublcass");
		return -EINVAL;
	}

	if (ctx->inst->mctp_protocol == USBD_MCTP_PROTOCOL_1_X ||
	    ctx->inst->mctp_protocol == USBD_MCTP_PROTOCOL_2_X) {
		ctx->desc->if0.bInterfaceProtocol = ctx->inst->mctp_protocol;
	} else {
		LOG_ERR("Invalid MCTP protocol");
		return -EINVAL;
	}

	LOG_DBG("MCTP device %s initialized", ctx->inst->mctp_binding->binding.name);

	return 0;
}

struct usbd_class_api mctp_usb_class_api = {
	.request = mctp_usb_class_request,
	.enable = mctp_usb_class_enable,
	.disable = mctp_usb_class_disable,
	.init = mctp_usb_class_init,
	.get_desc = mctp_usb_class_get_desc,
};

#define DEFINE_MCTP_USB_CLASS_DESCRIPTORS(n, _)						\
	static struct mctp_usb_class_desc mctp_usb_class_desc_##n = {			\
		.if0 = {								\
			.bLength = sizeof(struct usb_if_descriptor),			\
			.bDescriptorType = USB_DESC_INTERFACE,				\
			.bInterfaceNumber = 0,						\
			.bAlternateSetting = 0,						\
			.bNumEndpoints = 2,						\
			.bInterfaceClass = USB_BCC_MCTP,				\
			.bInterfaceSubClass = 0,					\
			.bInterfaceProtocol = 1,					\
			.iInterface = 0							\
		},									\
		.if0_fs_out_ep = {							\
			.bLength = sizeof(struct usb_ep_descriptor),			\
			.bDescriptorType = USB_DESC_ENDPOINT,				\
			.bEndpointAddress = 0x01,					\
			.bmAttributes = USB_EP_TYPE_BULK,				\
			.wMaxPacketSize = sys_cpu_to_le16(64),				\
			.bInterval = 1							\
		},									\
		.if0_fs_in_ep = {							\
			.bLength = sizeof(struct usb_ep_descriptor),			\
			.bDescriptorType = USB_DESC_ENDPOINT,				\
			.bEndpointAddress = 0x81,					\
			.bmAttributes = USB_EP_TYPE_BULK,				\
			.wMaxPacketSize = sys_cpu_to_le16(64),				\
			.bInterval = 1							\
		},									\
		.if0_hs_out_ep = {							\
			.bLength = sizeof(struct usb_ep_descriptor),			\
			.bDescriptorType = USB_DESC_ENDPOINT,				\
			.bEndpointAddress = 0x01,					\
			.bmAttributes = USB_EP_TYPE_BULK,				\
			.wMaxPacketSize = sys_cpu_to_le16(512),				\
			.bInterval = 1							\
		},									\
		.if0_hs_in_ep = {							\
			.bLength = sizeof(struct usb_ep_descriptor),			\
			.bDescriptorType = USB_DESC_ENDPOINT,				\
			.bEndpointAddress = 0x81,					\
			.bmAttributes = USB_EP_TYPE_BULK,				\
			.wMaxPacketSize = sys_cpu_to_le16(512),				\
			.bInterval = 1							\
		},									\
		.nil_desc = {								\
			.bLength = 0,							\
			.bDescriptorType = 0						\
		}									\
	};										\
	const static struct usb_desc_header *mctp_usb_class_fs_desc_##n[] = {		\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0,			\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0_fs_in_ep,	\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0_fs_out_ep,	\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.nil_desc		\
	};										\
											\
	const static struct usb_desc_header *mctp_usb_class_hs_desc_##n[] = {		\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0,			\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0_hs_in_ep,	\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.if0_hs_out_ep,	\
		(struct usb_desc_header *)&mctp_usb_class_desc_##n.nil_desc		\
	};

#define DEFINE_MCTP_USB_CLASS_DATA(n, _)						\
	static struct mctp_usb_class_ctx mctp_usb_class_ctx_##n = {			\
		.desc = &mctp_usb_class_desc_##n,					\
		.fs_desc = mctp_usb_class_fs_desc_##n,					\
		.hs_desc = mctp_usb_class_hs_desc_##n,					\
		.inst_idx = n,								\
		.out_net_buf = NULL							\
	};										\
											\
	USBD_DEFINE_CLASS(mctp_##n, &mctp_usb_class_api, &mctp_usb_class_ctx_##n, NULL);

LISTIFY(MCTP_USB_NUM_INSTANCES, DEFINE_MCTP_USB_CLASS_DESCRIPTORS, ())
LISTIFY(MCTP_USB_NUM_INSTANCES, DEFINE_MCTP_USB_CLASS_DATA, ())
