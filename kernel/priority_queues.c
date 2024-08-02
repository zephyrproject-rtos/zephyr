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
