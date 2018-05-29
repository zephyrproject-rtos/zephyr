/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _sched_priq__h_
#define _sched_priq__h_

#include <misc/util.h>
#include <misc/dlist.h>
#include <misc/rb.h>

/* Two abstractions are defined here for "thread priority queues".
 *
 * One is a "dumb" list implementation appropriate for systems with
 * small numbers of threads and sensitive to code size.  It is stored
 * in sorted order, taking an O(N) cost every time a thread is added
 * to the list.  This corresponds to the way the original _wait_q_t
 * abstraction worked and is very fast as long as the number of
 * threads is small.
 *
 * The other is a balanced tree "fast" implementation with rather
 * larger code size (due to the data structure itself, the code here
 * is just stubs) and higher constant-factor performance overhead, but
 * much better O(logN) scaling in the presence of large number of
 * threads.
 *
 * Each can be used for either the wait_q or system ready queue,
 * configurable at build time.
 */

struct k_thread;

struct k_thread *_priq_dumb_best(sys_dlist_t *pq);
void _priq_dumb_remove(sys_dlist_t *pq, struct k_thread *thread);
void _priq_dumb_add(sys_dlist_t *pq, struct k_thread *thread);

struct _priq_rb {
	struct rbtree tree;
	int next_order_key;
};

void _priq_rb_add(struct _priq_rb *pq, struct k_thread *thread);
void _priq_rb_remove(struct _priq_rb *pq, struct k_thread *thread);
struct k_thread *_priq_rb_best(struct _priq_rb *pq);

#endif /* _sched_priq__h_ */
