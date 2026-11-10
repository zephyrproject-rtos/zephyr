/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A separate test binary so these are the first virtq_create() calls the
 * process makes: the lazy wake workqueue is still unstarted when they race.
 */

#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#define THREAD_STACK_SIZE 1024

K_THREAD_STACK_DEFINE(startup_a_stack, THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(startup_b_stack, THREAD_STACK_SIZE);
static struct k_thread startup_a;
static struct k_thread startup_b;

static struct virtq startup_vq_a;
static struct virtq startup_vq_b;
static int startup_ret_a;
static int startup_ret_b;
static struct k_sem startup_ready;
static struct k_sem startup_start;
static struct k_sem startup_done;

static void create_queue_thread(void *p1, void *p2, void *p3)
{
	int *ret = p1;
	struct virtq *vq = p2;

	ARG_UNUSED(p3);

	k_sem_give(&startup_ready);
	zassert_ok(k_sem_take(&startup_start, K_FOREVER));
	*ret = virtq_create(vq, 4);
	k_sem_give(&startup_done);
}

ZTEST(queue_startup, test_concurrent_queue_creation)
{
	k_sem_init(&startup_ready, 0, 2);
	k_sem_init(&startup_start, 0, 2);
	k_sem_init(&startup_done, 0, 2);

	k_thread_create(&startup_a, startup_a_stack, K_THREAD_STACK_SIZEOF(startup_a_stack),
			create_queue_thread, &startup_ret_a, &startup_vq_a, NULL, K_PRIO_PREEMPT(2),
			0, K_NO_WAIT);
	k_thread_create(&startup_b, startup_b_stack, K_THREAD_STACK_SIZEOF(startup_b_stack),
			create_queue_thread, &startup_ret_b, &startup_vq_b, NULL, K_PRIO_PREEMPT(2),
			0, K_NO_WAIT);

	/* Both creators must be ready before either can initialize the shared worker. */
	zassert_ok(k_sem_take(&startup_ready, K_MSEC(2000)), "creator did not become ready");
	zassert_ok(k_sem_take(&startup_ready, K_MSEC(2000)), "creator did not become ready");
	k_sem_give(&startup_start);
	k_sem_give(&startup_start);

	zassert_ok(k_sem_take(&startup_done, K_MSEC(2000)), "creator did not finish");
	zassert_ok(k_sem_take(&startup_done, K_MSEC(2000)), "creator did not finish");
	zassert_ok(k_thread_join(&startup_a, K_MSEC(2000)));
	zassert_ok(k_thread_join(&startup_b, K_MSEC(2000)));

	zassert_ok(startup_ret_a, "first concurrent create failed");
	zassert_ok(startup_ret_b, "second concurrent create failed");
	zassert_equal(startup_vq_a.num, 4);
	zassert_equal(startup_vq_b.num, 4);

	virtq_free(&startup_vq_a);
	virtq_free(&startup_vq_b);
}

ZTEST_SUITE(queue_startup, NULL, NULL, NULL, NULL, NULL);
