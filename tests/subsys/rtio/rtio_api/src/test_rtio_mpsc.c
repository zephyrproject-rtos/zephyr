/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_loops.h>
#include <zephyr/timing/timing.h>
#include <zephyr/rtio/rtio_spsc.h>
#include <zephyr/rtio/rtio_mpsc.h>

#include "rtio_api.h"

static struct rtio_mpsc push_pop_q;
static struct rtio_mpsc_node push_pop_nodes[2];

/*
 * @brief Push and pop one element
 *
 * @see rtio_mpsc_push(), rtio_mpsc_pop()
 *
 * @ingroup rtio_tests
 */
ZTEST(rtio_mpsc, test_push_pop)
{

	mpsc_ptr_t node, head;
	struct rtio_mpsc_node *stub, *next, *tail;

	rtio_mpsc_init(&push_pop_q);

	head = mpsc_ptr_get(push_pop_q.head);
	tail = push_pop_q.tail;
	stub = &push_pop_q.stub;
	next = stub->next;

	zassert_equal(head, stub, "Head should point at stub");
	zassert_equal(tail, stub, "Tail should point at stub");
	zassert_is_null(next, "Next should be null");

	node = rtio_mpsc_pop(&push_pop_q);
	zassert_is_null(node, "Pop on empty queue should return null");

	rtio_mpsc_push(&push_pop_q, &push_pop_nodes[0]);

	head = mpsc_ptr_get(push_pop_q.head);

	zassert_equal(head, &push_pop_nodes[0], "Queue head should point at push_pop_node");
	next = mpsc_ptr_get(push_pop_nodes[0].next);
	zassert_is_null(next, NULL, "push_pop_node next should point at null");
	next = mpsc_ptr_get(push_pop_q.stub.next);
	zassert_equal(next, &push_pop_nodes[0], "Queue stub should point at push_pop_node");
	tail = push_pop_q.tail;
	stub = &push_pop_q.stub;
	zassert_equal(tail, stub, "Tail should point at stub");

	node = rtio_mpsc_pop(&push_pop_q);
	stub = &push_pop_q.stub;

	zassert_not_equal(node, stub, "Pop should not return stub");
	zassert_not_null(node, "Pop should not return null");
	zassert_equal(node, &push_pop_nodes[0],
		      "Pop should return push_pop_node %p, instead was %p",
		      &push_pop_nodes[0], node);

	node = rtio_mpsc_pop(&push_pop_q);
	zassert_is_null(node, "Pop on empty queue should return null");
}

#define MPSC_FREEQ_SZ 8
#define MPSC_ITERATIONS 100000
#define MPSC_STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define MPSC_THREADS_NUM 4

static struct thread_info mpsc_tinfo[MPSC_THREADS_NUM];
static struct k_thread mpsc_thread[MPSC_THREADS_NUM];
static K_THREAD_STACK_ARRAY_DEFINE(mpsc_stack, MPSC_THREADS_NUM, MPSC_STACK_SIZE);

struct mpsc_node {
	uint32_t id;
	struct rtio_mpsc_node n;
};


struct rtio_spsc_node_sq {
	struct rtio_spsc _spsc;
	struct mpsc_node *const buffer;
};

#define SPSC_DEFINE(n, sz) RTIO_SPSC_DEFINE(_spsc_##n, struct mpsc_node, sz)
#define SPSC_NAME(n, _) (struct rtio_spsc_node_sq *)&_spsc_##n

LISTIFY(MPSC_THREADS_NUM, SPSC_DEFINE, (;), MPSC_FREEQ_SZ)

struct rtio_spsc_node_sq *node_q[MPSC_THREADS_NUM] = {
	LISTIFY(MPSC_THREADS_NUM, SPSC_NAME, (,))
};

static struct rtio_mpsc mpsc_q;

static void mpsc_consumer(void *p1, void *p2, void *p3)
{
	struct rtio_mpsc_node *n;
	struct mpsc_node *nn;

	for (int i = 0; i < (MPSC_ITERATIONS)*(MPSC_THREADS_NUM - 1); i++) {
		do {
			n = rtio_mpsc_pop(&mpsc_q);
			if (n == NULL) {
				k_yield();
			}
		} while (n == NULL);

		zassert_not_equal(n, &mpsc_q.stub, "mpsc should not produce stub");

		nn = CONTAINER_OF(n, struct mpsc_node, n);

		rtio_spsc_acquire(node_q[nn->id]);
		rtio_spsc_produce(node_q[nn->id]);
	}
}

static void mpsc_producer(void *p1, void *p2, void *p3)
{
	struct mpsc_node *n;
	uint32_t id = (uint32_t)(uintptr_t)p1;

	for (int i = 0; i < MPSC_ITERATIONS; i++) {
		do {
			n = rtio_spsc_consume(node_q[id]);
			if (n == NULL) {
				k_yield();
			}
		} while (n == NULL);

		rtio_spsc_release(node_q[id]);
		n->id = id;
		rtio_mpsc_push(&mpsc_q, &n->n);
	}
}

/**
 * @brief Test that the producer and consumer are indeed thread safe
 *
 * This can and should be validated on SMP machines where incoherent
 * memory could cause issues.
 */
ZTEST(rtio_mpsc, test_mpsc_threaded)
{
	rtio_mpsc_init(&mpsc_q);

	TC_PRINT("setting up mpsc producer free queues\n");
	/* Setup node free queues */
	for (int i = 0; i < MPSC_THREADS_NUM; i++) {
		for (int j = 0; j < MPSC_FREEQ_SZ; j++) {
			rtio_spsc_acquire(node_q[i]);
		}
		rtio_spsc_produce_all(node_q[i]);
	}

	TC_PRINT("starting consumer\n");
	mpsc_tinfo[0].tid =
		k_thread_create(&mpsc_thread[0], mpsc_stack[0], MPSC_STACK_SIZE,
				(k_thread_entry_t)mpsc_consumer,
				NULL, NULL, NULL,
				K_PRIO_PREEMPT(5),
				K_INHERIT_PERMS, K_NO_WAIT);

	for (int i = 1; i < MPSC_THREADS_NUM; i++) {
		TC_PRINT("starting producer %i\n", i);
		mpsc_tinfo[i].tid =
			k_thread_create(&mpsc_thread[i], mpsc_stack[i], MPSC_STACK_SIZE,
					(k_thread_entry_t)mpsc_producer,
					(void *)(uintptr_t)i, NULL, NULL,
					K_PRIO_PREEMPT(5),
					K_INHERIT_PERMS, K_NO_WAIT);
	}

	for (int i = 0; i < MPSC_THREADS_NUM; i++) {
		TC_PRINT("joining mpsc thread %d\n", i);
		k_thread_join(mpsc_tinfo[i].tid, K_FOREVER);
	}
}

#define THROUGHPUT_ITERS 100000

ZTEST(rtio_mpsc, test_mpsc_throughput)
{
	struct rtio_mpsc_node node;
	timing_t start_time, end_time;

	rtio_mpsc_init(&mpsc_q);
	timing_init();
	timing_start();

	start_time = timing_counter_get();

	for (int i = 0; i < THROUGHPUT_ITERS; i++) {
		rtio_mpsc_push(&mpsc_q, &node);

		rtio_mpsc_pop(&mpsc_q);
	}

	end_time = timing_counter_get();

	uint64_t cycles = timing_cycles_get(&start_time, &end_time);
	uint64_t ns = timing_cycles_to_ns(cycles);

	TC_PRINT("%llu ns for %d iterations, %llu ns per op\n", ns,
		 THROUGHPUT_ITERS, ns/THROUGHPUT_ITERS);
}

ZTEST_SUITE(rtio_mpsc, NULL, NULL, NULL, NULL, NULL);
