/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

/* Deterministic regression for a waiting chain partially allocating descriptors. */

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#include "fake_device.h"

#define CHAIN_LEN  2
#define TIMEOUT_MS 2000

/* The submitter must outrank the ztest thread for the semaphore ordering. */
BUILD_ASSERT(K_PRIO_COOP(1) < CONFIG_ZTEST_THREAD_PRIORITY,
	     "submitter must be able to preempt the test thread");

static uint8_t submit_bufs[CHAIN_LEN][32] __aligned(4);

K_THREAD_STACK_DEFINE(waiting_stack, 1024);
static struct k_thread waiting_thread;

struct submitter {
	struct virtq *vq;
	atomic_t ret;
	struct k_sem entered;
};

static void submitter_entry(void *p1, void *p2, void *p3)
{
	struct submitter *s = p1;
	struct virtq_buf bufs[CHAIN_LEN];

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (uint16_t i = 0; i < CHAIN_LEN; i++) {
		bufs[i].addr = submit_bufs[i];
		bufs[i].len = sizeof(submit_bufs[i]);
	}

	k_sem_give(&s->entered);
	atomic_set(&s->ret, virtq_add_buffer_chain(s->vq, bufs, CHAIN_LEN, 0, NULL, NULL,
						   K_MSEC(TIMEOUT_MS)));
}

ZTEST(queue_submission_atomicity, test_waiting_chain_leaves_descriptors_available)
{
	struct virtq vq;
	struct submitter s = {.vq = &vq};
	struct virtq_buf small = {.addr = submit_bufs[0], .len = sizeof(submit_bufs[0])};
	uint16_t held;
	int waiting_ret;
	int small_ret;
	int64_t start;

	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		/* The ordering argument is single-core only; SMP runs the stress case. */
		ztest_test_skip();
	}

	zassert_ok(virtq_create(&vq, 2));
	fake_device_attach_queue(fake_device_a(), &vq);
	atomic_set(&s.ret, -1);

	/* One submitted chain leaves a single descriptor available. */
	zassert_ok(virtq_add_buffer_chain(&vq, &small, 1, 0, NULL, NULL, K_NO_WAIT));
	held = fake_device_last_submitted_head(&vq);

	k_sem_init(&s.entered, 0, 1);
	start = k_uptime_get();
	k_thread_create(&waiting_thread, waiting_stack, K_THREAD_STACK_SIZEOF(waiting_stack),
			submitter_entry, &s, NULL, NULL, K_PRIO_COOP(1), 0, K_NO_WAIT);

	/* A is cooperative: nothing else runs until it blocks. */
	zassert_ok(k_sem_take(&s.entered, K_MSEC(500)), "submitter did not start");

	/* Its deadline must not have passed, otherwise "not returned" means nothing. */
	zassert_true(k_uptime_get() - start < TIMEOUT_MS,
		     "submitter deadline already elapsed after %d ms",
		     (int)(k_uptime_get() - start));
	zassert_equal(atomic_get(&s.ret), -1, "submitter returned before inspecting the queue");

	/* T must not be blocked by A's pending allocation. */
	small_ret = virtq_add_buffer_chain(&vq, &small, 1, 0, NULL, NULL, K_NO_WAIT);

	/* Bounded cleanup before reporting a failure. */
	if (small_ret == 0) {
		fake_device_complete_request(fake_device_a(), fake_device_last_submitted_head(&vq),
					     0);
	}
	fake_device_complete_request(fake_device_a(), held, 0);
	zassert_ok(k_thread_join(&waiting_thread, K_MSEC(2000)), "submitter did not return");

	waiting_ret = (int)atomic_get(&s.ret);
	if (waiting_ret == 0) {
		fake_device_complete_request(fake_device_a(), fake_device_last_submitted_head(&vq),
					     0);
	}

	fake_device_attach_queue(fake_device_a(), NULL);
	virtq_free(&vq);

	zassert_true(waiting_ret == 0 || waiting_ret == -EAGAIN, "waiting chain returned %d",
		     waiting_ret);
	zassert_equal(small_ret, 0,
		      "one-descriptor submission failed with %d while a larger chain waited",
		      small_ret);
}

static void chain_submission_cleanup(void *fixture)
{
	ARG_UNUSED(fixture);

	k_thread_abort(&waiting_thread);
}

ZTEST_SUITE(queue_submission_atomicity, NULL, NULL, NULL, chain_submission_cleanup, NULL);
