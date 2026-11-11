/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/slist.h>

/* Buffers that the Host owes while their pool is empty, for code that must not
 * block in the allocator: freed buffers alternate between the waiter that has
 * waited longest and the threads blocked in net_buf_alloc().
 */

/* The waiters of one pool. Define it with BT_BUF_WAITERS_DEFINE(). */
struct bt_buf_waiters {
	struct k_spinlock lock;
	/* Waiters without a buffer, in the order in which they started to wait */
	sys_slist_t list;
	/* The buffer freed last was kept for a waiter */
	bool is_last_kept;
};

#define BT_BUF_WAITERS_DEFINE(_name)                                                               \
	struct bt_buf_waiters _name = {                                                            \
		.list = SYS_SLIST_STATIC_INIT(&_name.list),                                        \
	}

/* One party that needs buffers of a pool. Set it up with bt_buf_waiter_init();
 * its members belong to the functions below.
 */
struct bt_buf_waiter {
	sys_snode_t node;
	/* Kept for the waiter by bt_buf_waiters_destroy() */
	struct net_buf *buf;
	struct k_work work;
	/* In the list of its pool's waiters */
	bool is_waiting;
};

/* The handler runs on bt_workq each time the waiter may get a buffer, for it
 * to call bt_buf_waiter_alloc() again.
 */
void bt_buf_waiter_init(struct bt_buf_waiter *waiter, k_work_handler_t handler);

/* Returns a buffer as net_buf_alloc() does, or NULL: the waiter then waits,
 * and its handler runs on bt_workq at its turn. Called from threads; the caller
 * serializes the calls for one waiter, bt_buf_waiter_cancel() included.
 */
struct net_buf *bt_buf_waiter_alloc(struct bt_buf_waiters *waiters, struct bt_buf_waiter *waiter,
				    struct net_buf_pool *pool);

/* Frees a buffer of the pool: called in place of net_buf_destroy() by the
 * destroy callback of the pool, or by what that defers to, in any context
 * that a buffer may be unreferenced in.
 */
void bt_buf_waiters_destroy(struct bt_buf_waiters *waiters, struct net_buf *buf);

/* Ends the wait and frees a buffer kept for the waiter. Its handler can still
 * run once: a waiter in memory about to be released needs k_work_cancel_sync().
 */
void bt_buf_waiter_cancel(struct bt_buf_waiters *waiters, struct bt_buf_waiter *waiter);
