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

/* Workqueue budget: OUT buffers to parse per k_work run */
#define MCTP_USB_RX_WORK_BUDGET 4
#define MCTP_USB_ENABLED 0
#define MCTP_USB_BUFS_PER_INSTANCE 4
#define MCTP_USB_NUM_INSTANCES CONFIG_MCTP_USB_CLASS_INSTANCES_COUNT

UDC_BUF_POOL_DEFINE(mctp_usb_ep_pool,
		    MCTP_USB_NUM_INSTANCES * MCTP_USB_BUFS_PER_INSTANCE,
		    USBD_MAX_BULK_MPS,
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
	struct k_fifo rx_fifo;
	struct k_work out_work;
	atomic_t state;
	atomic_t in_pending;
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
	size_t tx_len = len + MCTP_USB_HEADER_SIZE;
	struct net_buf *buf = NULL;
	struct net_buf *zlp = NULL;
	int err;
	uint16_t mps;
	bool need_zlp;

	if (!atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
		return -EPERM;
	}

	if (tx_len > MCTP_USB_MAX_PACKET_LENGTH) {
		return -E2BIG;
	}

	err = k_sem_take(&usb->tx_lock, K_MSEC(CONFIG_MCTP_USB_TX_TIMEOUT));
	if (err != 0) {
		LOG_ERR("Semaphore could not be obtained");
		return err;
	}

	/* Determine MaxPacketSize for this speed */
#if USBD_SUPPORTS_HIGH_SPEED
	if (usbd_bus_speed(usbd_class_get_ctx(ctx->class_data)) == USBD_SPEED_HS) {
		mps = sys_le16_to_cpu(ctx->desc->if0_hs_in_ep.wMaxPacketSize);
	} else {
#endif
		mps = sys_le16_to_cpu(ctx->desc->if0_fs_in_ep.wMaxPacketSize);
#if USBD_SUPPORTS_HIGH_SPEED
	}
#endif

	need_zlp = (mps != 0U) && ((tx_len % mps) == 0U);

	/* If we need ZLP, allocate it FIRST to avoid half-inflight states */
	if (need_zlp) {
		zlp = mctp_usb_class_buf_alloc(mctp_usb_class_get_bulk_in(c_data));
		if (!zlp) {
			k_sem_give(&usb->tx_lock);
			LOG_ERR("Failed to allocate ZLP buffer");
			return -ENOMEM;
		}
	}

	/* Completion may happen very fast: set pending BEFORE enqueue */
	atomic_set(&ctx->in_pending, need_zlp ? 2 : 1);

	usb->tx_buf[0] = MCTP_USB_DMTF_0;
	usb->tx_buf[1] = MCTP_USB_DMTF_1;
	usb->tx_buf[2] = 0;
	usb->tx_buf[3] = tx_len;

	memcpy((void *)&usb->tx_buf[MCTP_USB_HEADER_SIZE], pkt->data, len);

	LOG_HEXDUMP_DBG(usb->tx_buf, len + MCTP_USB_HEADER_SIZE, "buf = ");

	if (usb->usb_class_data == NULL) {
		LOG_ERR("MCTP instance not found");
		return -ENODEV;
	}

	buf = mctp_usb_class_buf_alloc(mctp_usb_class_get_bulk_in(c_data));
	if (buf == NULL) {
		atomic_set(&ctx->in_pending, 0);
		if (zlp) {
			net_buf_unref(zlp);
		}
		k_sem_give(&usb->tx_lock);
		LOG_ERR("Failed to allocate IN buffer");
		return -ENOMEM;
	}

	net_buf_add_mem(buf, usb->tx_buf, len + MCTP_USB_HEADER_SIZE);

	err = usbd_ep_enqueue(c_data, buf);
	if (err != 0) {
		atomic_set(&ctx->in_pending, 0);
		if (zlp) {
			net_buf_unref(zlp);
		}
		k_sem_give(&usb->tx_lock);
		LOG_ERR("Failed to enqueue IN buffer");
		net_buf_unref(buf);
		return err;
	}

	if (need_zlp) {
		LOG_DBG("TX len %zu is multiple of MPS %u, sending ZLP", tx_len, mps);
		err = usbd_ep_enqueue(c_data, zlp);
		if (err != 0) {
		/* Data is already in-flight; reduce pending so IN completion releases once */
			net_buf_unref(zlp);
			atomic_set(&ctx->in_pending, 1);
			k_sem_give(&usb->tx_lock);
			LOG_ERR("Failed to enqueue ZLP: %d", err);
			return err;
		}
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
	struct mctp_binding_usb *usb = ctx->inst->mctp_binding;
	uint32_t budget = MCTP_USB_RX_WORK_BUDGET;

	if (!atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
		return;
	}

	while (budget-- != 0U) {
		struct net_buf *rx = k_fifo_get(&ctx->rx_fifo, K_NO_WAIT);
		size_t i;

		if (rx == NULL) {
			break;
		}

		/* Parse directly from net_buf */
		for (i = 0U; i < rx->len; i++) {
			uint8_t byte = rx->data[i];

			switch (usb->rx_state) {
			case STATE_WAIT_HDR_DMTF0:
				if (byte == MCTP_USB_DMTF_0) {
					usb->rx_state = STATE_WAIT_HDR_DMTF1;
				}
				break;

			case STATE_WAIT_HDR_DMTF1:
				if (byte == MCTP_USB_DMTF_1) {
					usb->rx_state = STATE_WAIT_HDR_RSVD0;
				} else if (byte == MCTP_USB_DMTF_0) {
					/* treat as potential new start */
					usb->rx_state = STATE_WAIT_HDR_DMTF1;
				} else {
					usb->rx_state = STATE_WAIT_HDR_DMTF0;
				}
				break;

			case STATE_WAIT_HDR_RSVD0:
				/* As per the spec reserved byte(s) need to be ignored */
				if (byte != 0U) {
					LOG_DBG("Non-zero RSVD0 %02X ignored", byte);
				}
				usb->rx_state = STATE_WAIT_HDR_LEN;
				break;

			case STATE_WAIT_HDR_LEN:
				/*
				 * LEN is total framed length (hdr + payload)
				 * Payload length = LEN - MCTP_USB_HEADER_SIZE
				 */
				if ((byte < MCTP_USB_HEADER_SIZE) ||
				    (byte > MCTP_USB_MAX_PACKET_LENGTH)) {
					LOG_ERR("Invalid LEN %02X", byte);
					mctp_usb_reset_rx_state(usb);
					break;
				}

				usb->rx_data_idx = 0U;
				usb->rx_pkt = mctp_pktbuf_alloc(&usb->binding,
								byte - MCTP_USB_HEADER_SIZE);
				if (usb->rx_pkt == NULL) {
					LOG_ERR("Failed to alloc pktbuf");
					mctp_usb_reset_rx_state(usb);
					break;
				}

				usb->rx_state = STATE_DATA;
				break;

			case STATE_DATA:
				/*
				 * If rx_pkt becomes NULL due to a reset, don't write.
				 * This also guards against any future logic changes.
				 */
				if (usb->rx_pkt == NULL) {
					mctp_usb_reset_rx_state(usb);
					break;
				}

				usb->rx_pkt->data[usb->rx_data_idx++] = byte;

				if (usb->rx_data_idx == usb->rx_pkt->end) {
					mctp_bus_rx(&usb->binding, usb->rx_pkt);
					usb->rx_pkt = NULL;
					mctp_usb_reset_rx_state(usb);
				}
				break;
			}
		}

		net_buf_unref(rx);
	}

	/* If there is still work, reschedule */
	if (!k_fifo_is_empty(&ctx->rx_fifo)) {
		(void)k_work_submit(&ctx->out_work);
	}
}

static int mctp_usb_class_request(struct usbd_class_data *const c_data,
				  struct net_buf *buf, int err)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);
	struct udc_buf_info *bi = udc_get_buf_info(buf);
	const uint8_t ep_in = mctp_usb_class_get_bulk_in(c_data);
	const uint8_t ep_out = mctp_usb_class_get_bulk_out(c_data);

	LOG_DBG("request for EP 0x%x", bi->ep);

	if (err != 0) {
		/* OUT error: free and try to re-arm if still enabled */
		if (bi->ep == ep_out) {
			net_buf_unref(buf);

			if (atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
				struct net_buf *nb = mctp_usb_class_buf_alloc(ep_out);

				if (nb != NULL) {
					int e = usbd_ep_enqueue(c_data, nb);

					if (e != 0) {
						net_buf_unref(nb);
					}
				}
			}
			return 0;
		}

		/* IN error path: consume + account pending */
		if (bi->ep == ep_in) {
			net_buf_unref(buf);
			if (atomic_dec(&ctx->in_pending) == 1) {
				k_sem_give(&ctx->inst->mctp_binding->tx_lock);
			}
			return 0;
		}

		net_buf_unref(buf);
		return 0;
	}

	if (bi->ep == ep_out) {
		/* Re-arm OUT first to avoid host-side stalls */
		if (atomic_test_bit(&ctx->state, MCTP_USB_ENABLED)) {
			struct net_buf *nb = mctp_usb_class_buf_alloc(ep_out);

			if (nb != NULL) {
				int e = usbd_ep_enqueue(c_data, nb);

				if (e != 0) {
					net_buf_unref(nb);
				}
			} else {
				LOG_ERR("OUT: failed to alloc next OUT buffer");
			}
		}

		/* Drop ZLP/empty OUT completions: do not schedule worker */
		if (buf->len == 0U) {
			net_buf_unref(buf);
			return 0;
		}

		/* Queue buffer for parsing and kick worker */
		k_fifo_put(&ctx->rx_fifo, buf);
		(void)k_work_submit(&ctx->out_work);
		return 0;
	}

	if (bi->ep == ep_in) {
		net_buf_unref(buf);
		if (atomic_dec(&ctx->in_pending) == 1) {
			k_sem_give(&ctx->inst->mctp_binding->tx_lock);
		}
		return 0;
	}

	net_buf_unref(buf);
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
	struct net_buf *nb;
	int e;

	if (!atomic_test_and_set_bit(&ctx->state, MCTP_USB_ENABLED)) {
		/* Arm FIRST OUT transfer so the host's first write can complete */
		nb = mctp_usb_class_buf_alloc(mctp_usb_class_get_bulk_out(c_data));
		if (!nb) {
			LOG_ERR("Failed to allocate initial OUT buffer");
			return;
		}

		e = usbd_ep_enqueue(c_data, nb);
		if (e) {
			LOG_ERR("Failed to enqueue initial OUT buffer: %d", e);
			net_buf_unref(nb);
			return;
		}

		LOG_INF("MCTP USB enabled: initial OUT armed");
	}

	LOG_DBG("Enabled %s", c_data->name);
}

static void mctp_usb_class_disable(struct usbd_class_data *const c_data)
{
	struct mctp_usb_class_ctx *ctx = usbd_class_get_private(c_data);
	struct mctp_binding_usb *usb = ctx->inst->mctp_binding;

	atomic_clear_bit(&ctx->state, MCTP_USB_ENABLED);

	/* Stop worker first so it doesn't race while we drain FIFO */
	(void)k_work_cancel(&ctx->out_work);

	/* Drain and free any queued OUT buffers */
	while (1) {
		struct net_buf *rx = k_fifo_get(&ctx->rx_fifo, K_NO_WAIT);

		if (!rx) {
			break;
		}
		net_buf_unref(rx);
	}

	/* Reset RX parser state */
	mctp_usb_reset_rx_state(usb);

	/* Clear pending IN accounting */
	atomic_set(&ctx->in_pending, 0);

	/* Unblock TX if host disconnects mid-IN */
	if (k_sem_count_get(&usb->tx_lock) == 0) {
		k_sem_give(&usb->tx_lock);
	}

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

	k_fifo_init(&ctx->rx_fifo);
	k_work_init(&ctx->out_work, mctp_usb_class_out_work);
	atomic_set(&ctx->in_pending, 0);

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
		.rx_fifo = Z_FIFO_INITIALIZER(mctp_usb_class_ctx_##n.rx_fifo),		\
		.state = ATOMIC_INIT(0),						\
		.in_pending = ATOMIC_INIT(0),						\
	};										\
											\
	USBD_DEFINE_CLASS(mctp_##n, &mctp_usb_class_api, &mctp_usb_class_ctx_##n, NULL);

LISTIFY(MCTP_USB_NUM_INSTANCES, DEFINE_MCTP_USB_CLASS_DESCRIPTORS, ())
LISTIFY(MCTP_USB_NUM_INSTANCES, DEFINE_MCTP_USB_CLASS_DATA, ())
