/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_msg, CONFIG_USBD_LOG_LEVEL);

static void msg_work_handler(struct k_work *work);
static K_WORK_DEFINE(msg_work, msg_work_handler);
K_FIFO_DEFINE(msg_queue);

struct usbd_msg_pkt {
	sys_snode_t node;
	struct usbd_contex *ctx;
	struct usbd_msg msg;
};

K_MEM_SLAB_DEFINE_STATIC(usbd_msg_slab, sizeof(struct usbd_msg_pkt),
			 CONFIG_USBD_MSG_SLAB_COUNT, sizeof(void *));

static inline void usbd_msg_pub(struct usbd_contex *const ctx,
				const struct usbd_msg msg)
{
	struct usbd_msg_pkt *m_pkt;

	if (k_mem_slab_alloc(&usbd_msg_slab, (void **)&m_pkt, K_NO_WAIT)) {
		LOG_DBG("Failed to allocate message memory");
		return;
	}

	m_pkt->ctx = ctx;
	m_pkt->msg = msg;

	k_fifo_put(&msg_queue, m_pkt);
	if (k_work_submit(&msg_work) < 0) {
		__ASSERT(false, "Failed to submit work");
	}
}

static void msg_work_handler(struct k_work *work)
{
	struct usbd_msg_pkt *m_pkt;

	m_pkt = k_fifo_get(&msg_queue, K_NO_WAIT);
	if (m_pkt != NULL) {
		m_pkt->ctx->msg_cb(&m_pkt->msg);
		k_mem_slab_free(&usbd_msg_slab, (void *)m_pkt);
	}

	if (!k_fifo_is_empty(&msg_queue)) {
		(void)k_work_submit(work);
	}
}

int usbd_msg_register_cb(struct usbd_contex *const uds_ctx,
			 const usbd_msg_cb_t cb)
{
	int ret = 0;

	usbd_device_lock(uds_ctx);

	if (uds_ctx->msg_cb != NULL) {
		ret = -EALREADY;
		goto register_cb_exit;
	}

	uds_ctx->msg_cb = cb;

register_cb_exit:
	usbd_device_unlock(uds_ctx);

	return ret;
}

void usbd_msg_pub_simple(struct usbd_contex *const ctx,
			 const enum usbd_msg_type type, const int status)
{
	const struct usbd_msg msg = {
		.type = type,
		.status = status,
	};

	if (ctx->msg_cb != NULL) {
		usbd_msg_pub(ctx, msg);
	}
}
