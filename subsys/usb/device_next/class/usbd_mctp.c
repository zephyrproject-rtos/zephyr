/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usbd_mctp.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_mctp, CONFIG_USBD_MCTP_LOG_LEVEL);

#define MCTP_ENABLED       0
#define MCTP_NUM_INSTANCES CONFIG_USBD_MCTP_INSTANCES_COUNT

UDC_BUF_POOL_DEFINE(mctp_ep_pool, MCTP_NUM_INSTANCES * 2, USBD_MAX_BULK_MPS,
		    sizeof(struct udc_buf_info), NULL);

struct usbd_mctp_desc {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_fs_out_ep;
	struct usb_ep_descriptor if0_fs_in_ep;
	struct usb_ep_descriptor if0_hs_out_ep;
	struct usb_ep_descriptor if0_hs_in_ep;
	struct usb_desc_header nil_desc;
};

struct usbd_mctp_ctx {
	struct usbd_class_data *class_node;
	struct usbd_mctp_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;
	uint8_t inst_idx;
	struct usbd_mctp_inst *inst;
	struct k_sem in_sem;
	struct k_work out_work;
	atomic_t state;
};

static struct {
	const struct usbd_mctp_inst *inst;
	struct usbd_class_data *c_data;
} usbd_mctp_inst_to_c_data[MCTP_NUM_INSTANCES];

static struct net_buf *mctp_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&mctp_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static uint8_t mctp_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(ctx->class_node)) == USBD_SPEED_HS) {
		return ctx->desc->if0_hs_in_ep.bEndpointAddress;
	}
#endif

	return ctx->desc->if0_fs_in_ep.bEndpointAddress;
}

static uint8_t mctp_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(ctx->class_node)) == USBD_SPEED_HS) {
		return ctx->desc->if0_hs_out_ep.bEndpointAddress;
	}
#endif

	return ctx->desc->if0_fs_out_ep.bEndpointAddress;
}

static void mctp_out_work(struct k_work *work)
{
	struct usbd_mctp_ctx *ctx = CONTAINER_OF(work, struct usbd_mctp_ctx, out_work);
	struct net_buf *buf;

	if (!atomic_test_bit(&ctx->state, MCTP_ENABLED)) {
		return;
	}

	buf = mctp_buf_alloc(mctp_get_bulk_out(ctx->class_node));
	if (buf == NULL) {
		if (ctx->inst->error_cb != NULL) {
			ctx->inst->error_cb(-ENOBUFS, mctp_get_bulk_in(ctx->class_node),
					    ctx->inst->priv);
		}
		LOG_ERR("Failed to allocate OUT buffer");
		return;
	}

	if (usbd_ep_enqueue(ctx->class_node, buf)) {
		net_buf_unref(buf);
		if (ctx->inst->error_cb != NULL) {
			ctx->inst->error_cb(-EIO, mctp_get_bulk_in(ctx->class_node),
					    ctx->inst->priv);
		}
		LOG_ERR("Failed to enqueue OUT buffer");
	}
}

static int usbd_mctp_request(struct usbd_class_data *const c_data, struct net_buf *buf, int err)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = udc_get_buf_info(buf);

	LOG_DBG("request for EP 0x%x", bi->ep);

	if (err) {
		if (err == -ECONNABORTED) {
			LOG_WRN("request ep 0x%02x, len %u cancelled", bi->ep, buf->len);
		} else {
			LOG_ERR("request ep 0x%02x, len %u failed", bi->ep, buf->len);
		}

		if (ctx->inst->error_cb != NULL) {
			ctx->inst->error_cb(err, bi->ep, ctx->inst->priv);
		}

		goto ep_request_exit;
	}

	if (bi->ep == mctp_get_bulk_out(c_data)) {
		if (ctx->inst->data_recv_cb != NULL) {
			ctx->inst->data_recv_cb(buf->data, buf->len, ctx->inst->priv);
		}

		k_work_submit(&ctx->out_work);
	}

	if (bi->ep == mctp_get_bulk_in(c_data)) {
		k_sem_give(&ctx->in_sem);
	}

ep_request_exit:
	net_buf_unref(buf);
	return 0;
}

static void *usbd_mctp_get_desc(struct usbd_class_data *const c_data, const enum usbd_speed speed)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return ctx->hs_desc;
	}

	return ctx->fs_desc;
}

static void usbd_mctp_enable(struct usbd_class_data *const c_data)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

	if (!atomic_test_and_set_bit(&ctx->state, MCTP_ENABLED)) {
		k_work_submit(&ctx->out_work);
	}

	LOG_DBG("Enabled %s", c_data->name);
}

static void usbd_mctp_disable(struct usbd_class_data *const c_data)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

	atomic_clear_bit(&ctx->state, MCTP_ENABLED);

	LOG_DBG("Disabled %s", c_data->name);
}

static int usbd_mctp_init(struct usbd_class_data *const c_data)
{
	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);
	size_t num_instances = 0;

	STRUCT_SECTION_COUNT(usbd_mctp_inst, &num_instances);

	if (num_instances != MCTP_NUM_INSTANCES) {
		LOG_ERR("The number of application instances (%d) does not match the number "
			"specified by CONFIG_USBD_MCTP_INSTANCES_COUNT (%d)",
			num_instances, MCTP_NUM_INSTANCES);
		return -EINVAL;
	}

	ctx->class_node = c_data;
	ctx->state = 0;

	k_sem_init(&ctx->in_sem, 1, 1);
	k_work_init(&ctx->out_work, mctp_out_work);

	STRUCT_SECTION_GET(usbd_mctp_inst, ctx->inst_idx, &ctx->inst);
	usbd_mctp_inst_to_c_data[ctx->inst_idx].inst = ctx->inst;
	usbd_mctp_inst_to_c_data[ctx->inst_idx].c_data = c_data;

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

	LOG_DBG("MCTP device %s initialized", ctx->inst->name);

	return 0;
}

struct usbd_class_api usbd_mctp_api = {
	.request = usbd_mctp_request,
	.enable = usbd_mctp_enable,
	.disable = usbd_mctp_disable,
	.init = usbd_mctp_init,
	.get_desc = usbd_mctp_get_desc,
};

int usbd_mctp_send(const struct usbd_mctp_inst *const inst, const void *const data,
		   const uint16_t size)
{
	struct usbd_class_data *c_data = NULL;
	struct net_buf *buf = NULL;
	int err;

	/* Get the class data associated with the MCTP instance */
	for (int i = 0; i < MCTP_NUM_INSTANCES; i++) {
		if (usbd_mctp_inst_to_c_data[i].inst == inst) {
			c_data = usbd_mctp_inst_to_c_data[i].c_data;
			break;
		}
	}

	if (c_data == NULL) {
		LOG_ERR("MCTP instance not found");
		return -ENODEV;
	}

	struct usbd_mctp_ctx *ctx = usbd_class_get_private(c_data);

	if (!atomic_test_bit(&ctx->state, MCTP_ENABLED)) {
		return -EPERM;
	}

	k_sem_take(&ctx->in_sem, K_FOREVER);

	buf = mctp_buf_alloc(mctp_get_bulk_in(c_data));
	if (buf == NULL) {
		k_sem_give(&ctx->in_sem);
		LOG_ERR("Failed to allocate IN buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, data, size);

	err = usbd_ep_enqueue(c_data, buf);
	if (err) {
		k_sem_give(&ctx->in_sem);
		LOG_ERR("Failed to enqueue IN buffer");
		net_buf_unref(buf);
		return err;
	}

	return 0;
}

#define DEFINE_MCTP_DESCRIPTORS(n, _)							\
	static struct usbd_mctp_desc usbd_mctp_desc_##n = {				\
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
											\
	const static struct usb_desc_header *usbd_mctp_fs_desc_##n[] = {		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0,			\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_fs_in_ep,		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_fs_out_ep,		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.nil_desc			\
	};										\
											\
	const static struct usb_desc_header *usbd_mctp_hs_desc_##n[] = {		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0,			\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_hs_in_ep,		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.if0_hs_out_ep,		\
		(struct usb_desc_header *)&usbd_mctp_desc_##n.nil_desc			\
	};

#define DEFINE_MCTP_CLASS_DATA(n, _)							\
	static struct usbd_mctp_ctx usbd_mctp_ctx_##n = {				\
		.desc = &usbd_mctp_desc_##n,						\
		.fs_desc = usbd_mctp_fs_desc_##n,					\
		.hs_desc = usbd_mctp_hs_desc_##n,					\
		.inst_idx = n 								\
	};										\
											\
	USBD_DEFINE_CLASS(mctp_##n, &usbd_mctp_api, &usbd_mctp_ctx_##n, NULL);

LISTIFY(MCTP_NUM_INSTANCES, DEFINE_MCTP_DESCRIPTORS, ())
LISTIFY(MCTP_NUM_INSTANCES, DEFINE_MCTP_CLASS_DATA, ())
