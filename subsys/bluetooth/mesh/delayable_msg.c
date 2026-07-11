/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/sys/slist.h>

#include <zephyr/net_buf.h>
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

/* Serializes access to busy_ctx, free_ctx, free_chunks, and the fired_time
 * field of any ctx currently on busy_ctx. Must be released around any call
 * that can block (bt_mesh_access_send(), bt_rand(), user callbacks).
 */
static struct k_spinlock lock;

/* Caller must hold `lock`. */
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
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (msg) {
		put_ctx_to_busy_list(msg);
	}

	pending_msg = peek_pending_msg();

	if (!pending_msg) {
		k_spin_unlock(&lock, key);
		return;
	}

	curr_time = k_uptime_get_32();
	if (curr_time < pending_msg->fired_time) {
		delay = K_MSEC(pending_msg->fired_time - curr_time);
	}

	/* k_work_reschedule() is safe to call under our spinlock: it uses its
	 * own internal spinlock and does not block. Holding the lock here
	 * makes the "select head + arm timer" step atomic w.r.t. concurrent
	 * insertions.
	 */
	k_work_reschedule(&access_delayable_msg.random_delay, delay);
	k_spin_unlock(&lock, key);
}

static int allocate_delayable_msg_chunks(struct delayable_msg_ctx *msg, int number)
{
	sys_snode_t *node;
	k_spinlock_key_t key = k_spin_lock(&lock);

	for (int i = 0; i < number; i++) {
		node = sys_slist_get(&access_delayable_msg.free_chunks);
		if (!node) {
			k_spin_unlock(&lock, key);
			LOG_WRN("Unable allocate %u chunks, allocated %u", number, i);
			return i;
		}
		sys_slist_append(&msg->chunks, node);
	}

	k_spin_unlock(&lock, key);
	return number;
}

static void release_delayable_msg_chunks(struct delayable_msg_ctx *msg)
{
	sys_snode_t *node;
	k_spinlock_key_t key = k_spin_lock(&lock);

	while ((node = sys_slist_get(&msg->chunks))) {
		sys_slist_append(&access_delayable_msg.free_chunks, node);
	}
	k_spin_unlock(&lock, key);
}

static struct delayable_msg_ctx *allocate_delayable_msg_ctx(void)
{
	struct delayable_msg_ctx *msg;
	sys_snode_t *node;
	k_spinlock_key_t key;

	key = k_spin_lock(&lock);
	node = sys_slist_get(&access_delayable_msg.free_ctx);
	k_spin_unlock(&lock, key);

	if (!node) {
		LOG_WRN("Purge pending delayable message.");
		/* push_msg_from_delayable_msgs() may block; call it without our
		 * lock held. It manages its own locking internally.
		 */
		if (!push_msg_from_delayable_msgs()) {
			return NULL;
		}

		key = k_spin_lock(&lock);
		node = sys_slist_get(&access_delayable_msg.free_ctx);
		k_spin_unlock(&lock, key);

		if (!node) {
			return NULL;
		}
	}

	msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	/* msg is now privately owned by the caller until re-inserted into a
	 * list, so its chunks head can be initialised without the lock.
	 */
	sys_slist_init(&msg->chunks);

	return msg;
}

static void release_delayable_msg_ctx(struct delayable_msg_ctx *ctx)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	/* ctx may or may not be on busy_ctx (e.g. the manage() ENOMEM error
	 * path releases a ctx that was never inserted). find_and_remove() is
	 * a no-op in that case; the unconditional append below returns the
	 * ctx to the free list either way. Fixes a pre-existing leak where
	 * the ctx would otherwise be lost from both lists.
	 */
	(void)sys_slist_find_and_remove(&access_delayable_msg.busy_ctx, &ctx->node);
	sys_slist_append(&access_delayable_msg.free_ctx, &ctx->node);
	k_spin_unlock(&lock, key);
}

static bool push_msg_from_delayable_msgs(void)
{
	sys_snode_t *node;
	struct delayable_msg_chunk *chunk;
	struct delayable_msg_ctx *msg;
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
	uint16_t len;
	int err;
	k_spinlock_key_t key;

	/* Atomically detach the head msg from busy_ctx. Two concurrent
	 * pushers therefore cannot try to send the same msg (previously,
	 * peek_pending_msg() left the msg on the list while the send ran,
	 * allowing double-send and double-release).
	 */
	key = k_spin_lock(&lock);
	node = sys_slist_get(&access_delayable_msg.busy_ctx);
	k_spin_unlock(&lock, key);

	if (!node) {
		return false;
	}

	msg = CONTAINER_OF(node, struct delayable_msg_ctx, node);
	len = msg->len;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);

	/* msg is now privately owned; its chunks list is not visible to any
	 * other thread and can be walked without the lock.
	 */
	SYS_SLIST_FOR_EACH_NODE(&msg->chunks, node) {
		uint16_t tmp = MIN((uint16_t)CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE, len);

		chunk = CONTAINER_OF(node, struct delayable_msg_chunk, node);
		memcpy(net_buf_simple_add(&buf, tmp), chunk->data, tmp);
		len -= tmp;
	}

	msg->ctx.rnd_delay = false;
	/* Blocking call: must NOT hold the spinlock. */
	err = bt_mesh_access_send(&msg->ctx, &buf, msg->src_addr, msg->cb, msg->cb_data);
	msg->ctx.rnd_delay = true;

	if (err == -EBUSY || err == -ENOBUFS) {
		/* Transient failure: re-insert the msg with a backoff bump on
		 * fired_time (matching the original behaviour that lived in
		 * delayable_msg_handler()), and re-arm the work timer for the
		 * new head. The msg is inserted at its sorted position, which
		 * may or may not be the head after the bump.
		 */
		key = k_spin_lock(&lock);
		msg->fired_time += 10;
		put_ctx_to_busy_list(msg);
		k_spin_unlock(&lock, key);

		reschedule_delayable_msg(NULL);
		return false;
	}

	/* Capture callback pointers before returning the ctx to the free
	 * list; once released, another caller may re-allocate the ctx and
	 * overwrite ctx->cb / ctx->cb_data.
	 */
	cb = msg->cb;
	cb_data = msg->cb_data;

	release_delayable_msg_chunks(msg);
	release_delayable_msg_ctx(msg);

	if (err && cb && cb->start) {
		/* User callback: must NOT hold the spinlock. */
		cb->start(0, err, cb_data);
	}

	return true;
}

static void delayable_msg_handler(struct k_work *w)
{
	/* push_msg_from_delayable_msgs() now handles both the empty-list
	 * case (returns false, no state change) and the transient-failure
	 * retry (returns false after re-inserting with a fired_time bump
	 * and re-arming the timer) internally. On success (return true) we
	 * re-arm the timer for the next pending msg (if any).
	 *
	 * The previous NULL dereference in this function (using the result
	 * of sys_slist_get() without checking) is eliminated by construction:
	 * we no longer touch busy_ctx directly here.
	 */
	if (push_msg_from_delayable_msgs()) {
		reschedule_delayable_msg(NULL);
	}
}

int bt_mesh_delayable_msg_manage(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf,
				 uint16_t src_addr, const struct bt_mesh_send_cb *cb, void *cb_data)
{
	sys_snode_t *node;
	struct delayable_msg_ctx *msg;
	uint16_t random_delay;
	int total_number = DIV_ROUND_UP(buf->len, CONFIG_BT_MESH_ACCESS_DELAYABLE_MSG_CHUNK_SIZE);
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
	const struct bt_mesh_send_cb *cb;
	void *cb_data;
	k_spinlock_key_t key;

	k_work_cancel_delayable(&access_delayable_msg.random_delay);

	for (;;) {
		key = k_spin_lock(&lock);
		node = sys_slist_get(&access_delayable_msg.busy_ctx);
		if (!node) {
			k_spin_unlock(&lock, key);
			break;
		}
		ctx = CONTAINER_OF(node, struct delayable_msg_ctx, node);

		/* Return chunks to the free list. */
		while ((node = sys_slist_get(&ctx->chunks))) {
			sys_slist_append(&access_delayable_msg.free_chunks, node);
		}

		/* Return ctx to the free list. */
		sys_slist_append(&access_delayable_msg.free_ctx, &ctx->node);

		/* Capture callback pointers so we can invoke them outside the
		 * spinlock (the callback is user-provided and may block).
		 */
		cb = ctx->cb;
		cb_data = ctx->cb_data;
		k_spin_unlock(&lock, key);

		if (cb && cb->start) {
			cb->start(0, -ENODEV, cb_data);
		}
	}
}
