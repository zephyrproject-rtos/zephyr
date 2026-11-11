/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include "buf_waiter.h"
#include "hci_core.h"

/* Called with waiters->lock held, so that a waiter that bt_buf_waiter_cancel()
 * has returned for is not resumed any more.
 */
static void waiter_resume(struct bt_buf_waiter *waiter)
{
	__maybe_unused int err;

	err = bt_work_submit(&waiter->work);
	__ASSERT(err >= 0, "Waiter %p not resumed (err %d)", waiter, err);
}

/* Called with waiters->lock held. The waiter that has waited longest may find
 * a buffer in the pool that no blocked allocator has taken.
 */
static void first_waiter_resume(struct bt_buf_waiters *waiters)
{
	sys_snode_t *node = sys_slist_peek_head(&waiters->list);

	if (node != NULL) {
		waiter_resume(CONTAINER_OF(node, struct bt_buf_waiter, node));
	}
}

/* Stop waiting. Returns the buffer that was kept for the waiter, if any. */
static struct net_buf *waiter_leave(struct bt_buf_waiters *waiters, struct bt_buf_waiter *waiter)
{
	k_spinlock_key_t key;
	struct net_buf *buf;

	key = k_spin_lock(&waiters->lock);

	if (waiter->is_waiting) {
		const bool is_first = sys_slist_peek_head(&waiters->list) == &waiter->node;

		(void)sys_slist_find_and_remove(&waiters->list, &waiter->node);
		waiter->is_waiting = false;

		if (is_first) {
			first_waiter_resume(waiters);
		}
	}

	buf = waiter->buf;
	waiter->buf = NULL;

	k_spin_unlock(&waiters->lock, key);

	return buf;
}

void bt_buf_waiter_init(struct bt_buf_waiter *waiter, k_work_handler_t handler)
{
	waiter->buf = NULL;
	waiter->is_waiting = false;
	k_work_init(&waiter->work, handler);
}

struct net_buf *bt_buf_waiter_alloc(struct bt_buf_waiters *waiters, struct bt_buf_waiter *waiter,
				    struct net_buf_pool *pool)
{
	k_spinlock_key_t key;
	struct net_buf *buf;
	bool is_first;

	key = k_spin_lock(&waiters->lock);

	buf = waiter->buf;
	waiter->buf = NULL;

	/* The waiter joins the list before the pool is tried, so that it is
	 * resumed for a buffer that is freed in between.
	 */
	if (buf == NULL && !waiter->is_waiting) {
		sys_slist_append(&waiters->list, &waiter->node);
		waiter->is_waiting = true;
	}

	is_first = sys_slist_peek_head(&waiters->list) == &waiter->node;

	k_spin_unlock(&waiters->lock, key);

	/* The pool is left to the waiter that has waited longest */
	if (buf != NULL || !is_first) {
		return buf;
	}

	buf = net_buf_alloc(pool, K_NO_WAIT);
	if (buf != NULL) {
		struct net_buf *kept = waiter_leave(waiters, waiter);

		/* Kept for the waiter while it got this one from the pool */
		net_buf_drop(&kept);
	}

	return buf;
}

void bt_buf_waiters_destroy(struct bt_buf_waiters *waiters, struct net_buf *buf)
{
	struct bt_buf_waiter *waiter = NULL;
	k_spinlock_key_t key;

	key = k_spin_lock(&waiters->lock);

	/* Alternate between the first waiter and the pool, where a blocked
	 * allocator gets the buffer first, so that neither starves the other
	 */
	if (!waiters->is_last_kept) {
		sys_snode_t *node = sys_slist_get(&waiters->list);

		if (node != NULL) {
			waiter = CONTAINER_OF(node, struct bt_buf_waiter, node);
			waiter->is_waiting = false;
			waiter->buf = net_buf_reuse(&buf);
			waiter_resume(waiter);
		}
	}

	waiters->is_last_kept = waiter != NULL;

	k_spin_unlock(&waiters->lock, key);

	if (waiter == NULL) {
		net_buf_destroy(buf);
	}

	/* For the buffer that went to the pool, if no blocked thread has it
	 * by now, and for one that is in the pool since the waiter that was
	 * first got this one.
	 */
	key = k_spin_lock(&waiters->lock);
	first_waiter_resume(waiters);
	k_spin_unlock(&waiters->lock, key);
}

void bt_buf_waiter_cancel(struct bt_buf_waiters *waiters, struct bt_buf_waiter *waiter)
{
	struct net_buf *kept = waiter_leave(waiters, waiter);

	net_buf_drop(&kept);
}
