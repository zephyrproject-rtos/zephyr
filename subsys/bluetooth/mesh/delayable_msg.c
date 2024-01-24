/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net/buf.h>
#include <zephyr/bluetooth/mesh.h>

#include "msg.h"
#include "access.h"
#include "net.h"

#define LOG_LEVEL CONFIG_BT_MESH_ACCESS_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_mesh_delayable_msg);

static void delayable_msg_handler(struct k_work *w);
static bool push_msg_from_delayable_msgs(void);

static struct delayable_msg_chunk {
	sys_snode_t node;
	uint8_t data[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE];
} delayable_msg_chunks[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT];

static struct delayable_msg_ctx {
	sys_snode_t node;
	sys_slist_t chunks;
	struct bt_mesh_msg_ctx ctx;
	uint16_t src_addr;
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
	uint32_t fired_time;
	uint16_t len;
} delayable_msgs_ctx[CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT];

static struct {
	sys_slist_t busy_ctx;
	sys_slist_t free_ctx;
	sys_slist_t free_chunks;
	struct k_work_delayable random_delay;
} access_delayable_msg = {.random_delay = Z_WORK_DELAYABLE_INITIALIZER(delayable_msg_handler)};

static void put_ctx_to_busy_list(struct delayable_msg_ctx *ctx)
{
	struct delayable_msg_ctx *curr_ctx;
	sys_slist_t *list = &access_delayable_msg.busy_ctx;
	sys_snode_t *head = sys_slist_peek_head(list);
	sys_snode_t *curr = head;
	sys_snode_t *prev = curr;

	if (!head) {
		sys_slist_append(list, &ctx->node);
		return;
	}

	do {
		curr_ctx = CONTAINER_OF(curr, struct delayable_msg_ctx, node);
		if (ctx->fired_time < curr_ctx->fired_time) {
			if (curr == head) {
				sys_slist_prepend(list, &ctx->node);
			} else {
				sys_slist_insert(list, prev, &ctx->node);
			}
			return;
		}
		prev = curr;
	} while ((curr = sys_slist_peek_next(curr)));

	sys_slist_append(list, &ctx->node);
}

static struct delayable_msg_ctx *peek_pending_msg(void)
{
	struct delayable_msg_ctx *pending_msg = NULL;
	sys_snode_t *node = sys_slist_peek_head(&access_delayable_msg.busy_ctx);

	if (node) {
		pending_msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	}

	return pending_msg;
}

static void reschedule_delayable_msg(struct delayable_msg_ctx *msg)
{
	uint32_t curr_time;
	k_timeout_t delay = K_NO_WAIT;
	struct delayable_msg_ctx *pending_msg;

	if (msg) {
		put_ctx_to_busy_list(msg);
	}

	pending_msg = peek_pending_msg();

	if (!pending_msg) {
		return;
	}

	curr_time = k_uptime_get_32();
	if (curr_time < pending_msg->fired_time) {
		delay = K_MSEC(pending_msg->fired_time - curr_time);
	}

	k_work_reschedule(&access_delayable_msg.random_delay, delay);
}

static int allocate_delayable_msg_chunks(struct delayable_msg_ctx *msg, int number)
{
	sys_snode_t *node;

	for (int i = 0; i < number; i++) {
		node = sys_slist_get(&access_delayable_msg.free_chunks);
		if (!node) {
			LOG_WRN("Unable allocate %u chunks, allocated %u", number, i);
			return i;
		}
		sys_slist_append(&msg->chunks, node);
	}

	return number;
}

static void release_delayable_msg_chunks(struct delayable_msg_ctx *msg)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&msg->chunks))) {
		sys_slist_append(&access_delayable_msg.free_chunks, node);
	}
}

static struct delayable_msg_ctx *allocate_delayable_msg_ctx(void)
{
	struct delayable_msg_ctx *msg;
	sys_snode_t *node;

	if (sys_slist_is_empty(&access_delayable_msg.free_ctx)) {
		LOG_WRN("Purge pending delayable message.");
		if (!push_msg_from_delayable_msgs()) {
			return NULL;
		}
	}

	node = sys_slist_get(&access_delayable_msg.free_ctx);
	msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	sys_slist_init(&msg->chunks);

	return msg;
}

static void release_delayable_msg_ctx(struct delayable_msg_ctx *ctx)
{
	if (sys_slist_find_and_remove(&access_delayable_msg.busy_ctx, &ctx->node)) {
		sys_slist_append(&access_delayable_msg.free_ctx, &ctx->node);
	}
}

static bool push_msg_from_delayable_msgs(void)
{
	sys_snode_t *node;
	struct delayable_msg_chunk *chunk;
	struct delayable_msg_ctx *msg = peek_pending_msg();
	uint16_t len;
	int err;

	if (!msg) {
		return false;
	}

	len = msg->len;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);

	SYS_SLIST_FOR_EACH_NODE(&msg->chunks, node) {
		uint16_t tmp = MIN(CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE, len);

		chunk = CONTAINER_OF(node, struct delayable_msg_chunk, node);
		memcpy(net_buf_simple_add(&buf, tmp), chunk->data, tmp);
		len -= tmp;
	}

	msg->ctx.rnd_delay = false;
	err = bt_mesh_access_send(&msg->ctx, &buf, msg->src_addr, msg->cb, msg->cb_data);
	msg->ctx.rnd_delay = true;

	if (err == -EBUSY || err == -ENOBUFS) {
		return false;
	}

	release_delayable_msg_chunks(msg);
	release_delayable_msg_ctx(msg);

	if (err && msg->cb && msg->cb->start) {
		msg->cb->start(0, err, msg->cb_data);
	}

	return true;
}

static void delayable_msg_handler(struct k_work *w)
{
	if (!push_msg_from_delayable_msgs()) {
		sys_snode_t *node = sys_slist_get(&access_delayable_msg.busy_ctx);
		struct delayable_msg_ctx *pending_msg =
			CONTAINER_OF(node, struct delayable_msg_ctx, node);

		pending_msg->fired_time += 10;
		reschedule_delayable_msg(pending_msg);
	} else {
		reschedule_delayable_msg(NULL);
	}
}

int bt_mesh_delayable_msg_manage(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf,
				 uint16_t src_addr, const struct bt_mesh_send_cb *cb, void *cb_data)
{
	sys_snode_t *node;
	struct delayable_msg_ctx *msg;
	uint16_t random_delay;
	int total_number = DIV_ROUND_UP(buf->size, CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE);
	int allocated_number = 0;
	uint16_t len = buf->len;

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_SUSPENDED)) {
		LOG_WRN("Refusing to allocate message context while suspended");
		return -ENODEV;
	}

	if (total_number > CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT) {
		return -EINVAL;
	}

	msg = allocate_delayable_msg_ctx();
	if (!msg) {
		LOG_WRN("No available free delayable message context.");
		return -ENOMEM;
	}

	do {
		allocated_number +=
			allocate_delayable_msg_chunks(msg, total_number - allocated_number);

		if (total_number > allocated_number) {
			LOG_DBG("Unable allocate %u chunks, allocated %u", total_number,
				allocated_number);
			if (!push_msg_from_delayable_msgs()) {
				LOG_WRN("No available chunk memory.");
				release_delayable_msg_chunks(msg);
				release_delayable_msg_ctx(msg);
				return -ENOMEM;
			}
		}
	} while (total_number > allocated_number);

	SYS_SLIST_FOR_EACH_NODE(&msg->chunks, node) {
		uint16_t tmp = MIN(CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE, buf->len);

		struct delayable_msg_chunk *chunk =
			CONTAINER_OF(node, struct delayable_msg_chunk, node);

		memcpy(chunk->data, net_buf_simple_pull_mem(buf, tmp), tmp);
	}

	bt_rand(&random_delay, sizeof(uint16_t));
	random_delay = 20 + random_delay % (BT_MESH_ADDR_IS_UNICAST(ctx->recv_dst) ? 30 : 480);
	msg->fired_time = k_uptime_get_32() + random_delay;
	msg->ctx = *ctx;
	msg->src_addr = src_addr;
	msg->cb = cb;
	msg->cb_data = cb_data;
	msg->len = len;

	reschedule_delayable_msg(msg);

	return 0;
}

void bt_mesh_delayable_msg_init(void)
{
	sys_slist_init(&access_delayable_msg.busy_ctx);
	sys_slist_init(&access_delayable_msg.free_ctx);
	sys_slist_init(&access_delayable_msg.free_chunks);

	for (int i = 0; i < CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_COUNT; i++) {
		sys_slist_append(&access_delayable_msg.free_ctx, &delayable_msgs_ctx[i].node);
	}

	for (int i = 0; i < CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_COUNT; i++) {
		sys_slist_append(&access_delayable_msg.free_chunks, &delayable_msg_chunks[i].node);
	}
}

void bt_mesh_delayable_msg_stop(void)
{
	sys_snode_t *node;
	struct delayable_msg_ctx *ctx;

	k_work_cancel_delayable(&access_delayable_msg.random_delay);

	while ((node = sys_slist_peek_head(&access_delayable_msg.busy_ctx))) {
		ctx = CONTAINER_OF(node, struct delayable_msg_ctx, node);
		release_delayable_msg_chunks(ctx);
		release_delayable_msg_ctx(ctx);

		if (ctx->cb && ctx->cb->start) {
			ctx->cb->start(0, -ENODEV, ctx->cb_data);
		}
	}
}
