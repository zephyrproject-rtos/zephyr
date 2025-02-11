/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/usb/usbd.h>

#include "usbd_device.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(usbd_msg, CONFIG_USBD_LOG_LEVEL);

static void msg_work_handler(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(msg_work, msg_work_handler);
static struct k_spinlock ml_lock;
static sys_slist_t msg_list;

struct usbd_msg_pkt {
	sys_snode_t node;
	struct usbd_context *ctx;
	struct usbd_msg msg;
};

K_MEM_SLAB_DEFINE_STATIC(usbd_msg_slab, sizeof(struct usbd_msg_pkt),
			 CONFIG_USBD_MSG_SLAB_COUNT, sizeof(void *));

static inline void usbd_msg_pub(struct usbd_context *const ctx,
				const struct usbd_msg msg)
{
	struct usbd_msg_pkt *m_pkt;
	k_spinlock_key_t key;

	if (k_mem_slab_alloc(&usbd_msg_slab, (void **)&m_pkt, K_NO_WAIT)) {
		LOG_DBG("Failed to allocate message memory");
		return;
	}

	m_pkt->ctx = ctx;
	m_pkt->msg = msg;

	key = k_spin_lock(&ml_lock);
	sys_slist_append(&msg_list, &m_pkt->node);
	k_spin_unlock(&ml_lock, key);

	if (k_work_schedule(&msg_work, K_NO_WAIT) < 0) {
		__ASSERT(false, "Failed to schedule work");
	}
}

static void msg_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbd_msg_pkt *m_pkt;
	k_spinlock_key_t key;
	sys_snode_t *node;

	key = k_spin_lock(&ml_lock);
	node = sys_slist_peek_head(&msg_list);
	k_spin_unlock(&ml_lock, key);

	__ASSERT(node != NULL, "slist appears to be empty");
	m_pkt = SYS_SLIST_CONTAINER(node, m_pkt, node);

	if (!usbd_is_initialized(m_pkt->ctx)) {
		LOG_DBG("USB device support is not yet initialized");
		(void)k_work_reschedule(dwork, K_MSEC(CONFIG_USBD_MSG_WORK_DELAY));
		return;
	}

	key = k_spin_lock(&ml_lock);
	node = sys_slist_get(&msg_list);
	k_spin_unlock(&ml_lock, key);

	if (node != NULL) {
		m_pkt = SYS_SLIST_CONTAINER(node, m_pkt, node);
		m_pkt->ctx->msg_cb(m_pkt->ctx, &m_pkt->msg);
		k_mem_slab_free(&usbd_msg_slab, (void *)m_pkt);
	}

	if (!sys_slist_is_empty(&msg_list)) {
		(void)k_work_schedule(dwork, K_NO_WAIT);
	}
}

int usbd_msg_register_cb(struct usbd_context *const uds_ctx,
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

void usbd_msg_pub_simple(struct usbd_context *const ctx,
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

void usbd_msg_pub_device(struct usbd_context *const ctx,
			 const enum usbd_msg_type type, const struct device *const dev)
{
	const struct usbd_msg msg = {
		.type = type,
		.dev = dev,
	};

	if (ctx->msg_cb != NULL) {
		usbd_msg_pub(ctx, msg);
	}
}
