/*
 * Copyright (c) 2018,2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/dlist.h>

bool z_priq_rb_lessthan(struct rbnode *a, struct rbnode *b)
{
	struct k_thread *thread_a, *thread_b;
	int32_t cmp;

	thread_a = CONTAINER_OF(a, struct k_thread, base.qnode_rb);
	thread_b = CONTAINER_OF(b, struct k_thread, base.qnode_rb);

	cmp = z_sched_prio_cmp(thread_a, thread_b);

	if (cmp > 0) {
		return true;
	} else if (cmp < 0) {
		return false;
	} else {
		return thread_a->base.order_key < thread_b->base.order_key
			? 1 : 0;
	}
}

/* Renumber at wraparound.  This is tiny code, and in practice
 * will almost never be hit on real systems.  BUT on very
 * long-running systems where a priq never completely empties
 * AND that contains very large numbers of threads, it can be
 * a latency glitch to loop over all the threads like this.
 *
 * Kept out of line so that the iterator's stack allocation does not
 * land in every caller that inlines z_priq_rb_add().
 */
void z_priq_rb_renumber(struct _priq_rb *pq)
{
	struct k_thread *t;

	RB_FOR_EACH_CONTAINER(&pq->tree, t, base.qnode_rb) {
		t->base.order_key = pq->next_order_key;
		++pq->next_order_key;
	}
}
