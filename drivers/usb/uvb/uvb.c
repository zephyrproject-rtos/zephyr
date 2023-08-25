/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/iterable_sections.h>

#include "uvb.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uvb, CONFIG_UVB_LOG_LEVEL);

static struct k_fifo uvb_queue;
static void uvb_work_handler(struct k_work *work);
static K_WORK_DEFINE(uvb_work, uvb_work_handler);

enum uvb_msg_type {
	UVB_MSG_ADVERT,
	UVB_MSG_TO_HOST,
	UVB_MSG_SUBSCRIBE,
	UVB_MSG_UNSUBSCRIBE,
};

struct uvb_msg {
	sys_snode_t node;
	enum uvb_msg_type type;
	const struct uvb_node *source;
	union {
		struct uvb_node *sink;
		struct {
			enum uvb_event_type type;
			const void *data;
		} event;
	};
};

K_MEM_SLAB_DEFINE_STATIC(uvb_msg_slab, sizeof(struct uvb_msg),
			 CONFIG_UVB_MAX_MESSAGES, sizeof(void *));

K_MEM_SLAB_DEFINE_STATIC(uvb_pkt_slab, sizeof(struct uvb_packet),
			 CONFIG_UVB_MAX_MESSAGES, sizeof(void *));

struct uvb_packet *uvb_alloc_pkt(const enum uvb_request request,
				 const uint8_t addr, const uint8_t ep,
				 uint8_t *const data,
				 const size_t length)
{
	static uint32_t seq;
	struct uvb_packet *pkt;

	if (k_mem_slab_alloc(&uvb_pkt_slab, (void **)&pkt, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate packet memory");
		return NULL;
	}

	seq++;
	pkt->seq = seq;
	pkt->request = request;
	pkt->reply = UVB_REPLY_TIMEOUT;
	pkt->addr = addr;
	pkt->ep = ep;
	pkt->data = data;
	pkt->length = length;

	return pkt;
}

void uvb_free_pkt(struct uvb_packet *const pkt)
{
	k_mem_slab_free(&uvb_pkt_slab, (void *)pkt);
}

static ALWAYS_INLINE int submit_new_work(struct uvb_msg *const msg)
{
	k_fifo_put(&uvb_queue, msg);
	return k_work_submit(&uvb_work);
}

static struct uvb_msg *uvb_alloc_msg(const struct uvb_node *const node)
{
	struct uvb_msg *msg;

	if (k_mem_slab_alloc(&uvb_msg_slab, (void **)&msg, K_NO_WAIT)) {
		LOG_ERR("Failed to allocate msg memory");
		return NULL;
	}

	memset(msg, 0, sizeof(struct uvb_msg));
	msg->source = node;

	return msg;
}

int uvb_advert(const struct uvb_node *const host_node,
	       const enum uvb_event_type type,
	       const struct uvb_packet *const pkt)
{
	struct uvb_msg *msg;
	int err;

	msg = uvb_alloc_msg(host_node);
	if (msg == NULL) {
		return -ENOMEM;
	}

	msg->type = UVB_MSG_ADVERT;
	msg->event.type = type;
	msg->event.data = (void *)pkt;
	err = submit_new_work(msg);

	return err < 0 ? err : 0;
}

int uvb_to_host(const struct uvb_node *const dev_node,
		const enum uvb_event_type type,
		const struct uvb_packet *const pkt)
{
	struct uvb_msg *msg;
	int err;

	msg = uvb_alloc_msg(dev_node);
	if (msg == NULL) {
		return -ENOMEM;
	}

	msg->type = UVB_MSG_TO_HOST;
	msg->event.type = type;
	msg->event.data = (void *)pkt;
	err = submit_new_work(msg);

	return err < 0 ? err : 0;
}

static int subscribe_msg(const struct uvb_node *const host_node,
			 struct uvb_node *const dev_node,
			 const enum uvb_msg_type type)
{
	struct uvb_msg *msg;
	int err;

	msg = uvb_alloc_msg(host_node);
	if (msg == NULL) {
		return -ENOMEM;
	}

	msg->type = type;
	msg->sink = dev_node;
	err = submit_new_work(msg);

	return err < 0 ? err : 0;
}

static int unsubscribe_msg(const struct uvb_node *const host_node,
			   struct uvb_node *const dev_node)
{
	struct uvb_msg *msg;
	int err;

	msg = uvb_alloc_msg(host_node);
	if (msg == NULL) {
		return -ENOMEM;
	}

	msg->type = UVB_MSG_UNSUBSCRIBE;
	msg->sink = dev_node;
	err = submit_new_work(msg);

	return err < 0 ? err : 0;
}

static struct uvb_node *find_host_node(const char *name)
{
	if (name == NULL || name[0] == '\0') {
		return NULL;
	}

	STRUCT_SECTION_FOREACH(uvb_node, host) {
		if (strcmp(name, host->name) == 0) {
			return host;
		}
	}

	return NULL;
}

int uvb_subscribe(const char *name, struct uvb_node *const dev_node)
{
	const struct uvb_node *host_node;

	host_node = find_host_node(name);
	if (host_node == NULL) {
		return -ENOENT;
	}

	return subscribe_msg(host_node, dev_node, UVB_MSG_SUBSCRIBE);
}

int uvb_unsubscribe(const char *name, struct uvb_node *const dev_node)
{
	const struct uvb_node *host_node;

	host_node = find_host_node(name);
	if (host_node == NULL) {
		return -ENOENT;
	}

	return unsubscribe_msg(host_node, dev_node);
}

static ALWAYS_INLINE void handle_msg_subscribe(struct uvb_msg *const msg)
{
	struct uvb_node *host_node;
	struct uvb_node *dev_node;

	host_node = (struct uvb_node *)msg->source;
	dev_node = msg->sink;
	if (atomic_get(&dev_node->subscribed)) {
		LOG_ERR("%p already subscribed", dev_node);
		return;
	}

	LOG_DBG("%p -> %p", dev_node, host_node);
	sys_dnode_init(&dev_node->node);
	if (msg->type == UVB_MSG_SUBSCRIBE) {
		sys_dlist_prepend(&host_node->list, &dev_node->node);
	}

	atomic_inc(&dev_node->subscribed);
}

static ALWAYS_INLINE void handle_msg_unsubscribe(struct uvb_msg *const msg)
{
	struct uvb_node *dev_node;
	atomic_t tmp;

	dev_node = msg->sink;
	tmp = atomic_clear(&dev_node->subscribed);
	if (tmp) {
		LOG_DBG("unsubscribe %p", dev_node);
		sys_dlist_remove(&dev_node->node);
	} else {
		LOG_ERR("%p is not subscribed", dev_node);
	}
}

static ALWAYS_INLINE void handle_msg_event(struct uvb_msg *const msg)
{
	struct uvb_node *host_node;
	struct uvb_node *dev_node;

	host_node = (struct uvb_node *)msg->source;
	SYS_DLIST_FOR_EACH_CONTAINER(&host_node->list, dev_node, node) {
		LOG_DBG("%p from %p to %p", msg, host_node, dev_node);
		if (dev_node->notify) {
			dev_node->notify(dev_node->priv,
					 msg->event.type,
					 msg->event.data);
		}
	}
}

static ALWAYS_INLINE void handle_msg_to_host(struct uvb_msg *const msg)
{
	struct uvb_node *host_node;
	struct uvb_node *source;

	source = (struct uvb_node *)msg->source;
	if (source->head) {
		LOG_ERR("Host may not reply");
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&source->node, host_node, node) {
		LOG_DBG("%p from %p to %p", msg, source, host_node);
		if (host_node->head && host_node->notify) {
			host_node->notify(host_node->priv,
					  msg->event.type,
					  msg->event.data);
		}
	}
}

static void uvb_work_handler(struct k_work *work)
{
	struct uvb_msg *msg;

	msg = k_fifo_get(&uvb_queue, K_NO_WAIT);
	if (msg == NULL) {
		return;
	}

	LOG_DBG("Message %p %s", msg->source, msg->source->name);
	switch (msg->type) {
	case UVB_MSG_SUBSCRIBE:
		handle_msg_subscribe(msg);
		break;
	case UVB_MSG_UNSUBSCRIBE:
		handle_msg_unsubscribe(msg);
		break;
	case UVB_MSG_ADVERT:
		handle_msg_event(msg);
		break;
	case UVB_MSG_TO_HOST:
		handle_msg_to_host(msg);
		break;
	default:
		break;
	}

	k_mem_slab_free(&uvb_msg_slab, (void *)msg);
	if (!k_fifo_is_empty(&uvb_queue)) {
		(void)k_work_submit(work);
	}
}

static int uvb_init(void)
{
	STRUCT_SECTION_FOREACH(uvb_node, host) {
		LOG_DBG("Host %p - %s", host, host->name);
		sys_dlist_init(&host->list);
	}

	k_fifo_init(&uvb_queue);

	return 0;
}

SYS_INIT(uvb_init, PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
