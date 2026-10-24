/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

#include <zephyr/ztest.h>

#define STACK_SIZE         2048
#define NUM_THREADS        4
#define YIELD_ITERATIONS   10
#define PREEMPT_ITERATIONS 100

/* PIE registers whose bits are all writable, laid out as expected by pie.S */
struct pie_regs {
	uint8_t q[8][16];
	uint32_t qacc_h[5];
	uint32_t qacc_l[5];
	uint32_t accx;
	uint32_t ua_state[4];
} __aligned(16);

BUILD_ASSERT(offsetof(struct pie_regs, qacc_h) == 128);
BUILD_ASSERT(offsetof(struct pie_regs, qacc_l) == 148);
BUILD_ASSERT(offsetof(struct pie_regs, accx) == 168);
BUILD_ASSERT(offsetof(struct pie_regs, ua_state) == 172);

extern void pie_set(const struct pie_regs *regs);
extern void pie_get(struct pie_regs *regs);

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, NUM_THREADS, STACK_SIZE);
static struct k_thread thread[NUM_THREADS];
static atomic_t preempt_done;

static void pie_check(const struct pie_regs *expected)
{
	struct pie_regs actual;

	pie_get(&actual);

	zassert_mem_equal(actual.q, expected->q, sizeof(actual.q), "Q registers corrupted");
	zassert_mem_equal(actual.qacc_h, expected->qacc_h, sizeof(actual.qacc_h),
			  "QACC_H corrupted");
	zassert_mem_equal(actual.qacc_l, expected->qacc_l, sizeof(actual.qacc_l),
			  "QACC_L corrupted");
	zassert_equal(actual.accx, expected->accx, "ACCX corrupted");
	zassert_mem_equal(actual.ua_state, expected->ua_state, sizeof(actual.ua_state),
			  "UA_STATE corrupted");
}

static void yield_entry(void *p1, void *p2, void *p3)
{
	uint32_t index = POINTER_TO_UINT(p1);
	struct pie_regs regs;

	if (index < (NUM_THREADS - 1U)) {
		k_thread_start(&thread[index + 1U]);
	}

	memset(&regs, (int)(index + 1U), sizeof(regs));
	pie_set(&regs);

	for (uint32_t i = 0; i < YIELD_ITERATIONS; i++) {
		k_yield();
		pie_check(&regs);
	}
}

ZTEST(pie, test_register_coherence)
{
	int priority = k_thread_priority_get(k_current_get());

	for (uint32_t i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&thread[i], thread_stack[i], STACK_SIZE, yield_entry,
				UINT_TO_POINTER(i), NULL, NULL, priority - 1, 0, K_FOREVER);
	}

	k_thread_start(&thread[0]);

	for (uint32_t i = 0; i < NUM_THREADS; i++) {
		k_thread_join(&thread[i], K_FOREVER);
	}
}

static void preempt_low_entry(void *p1, void *p2, void *p3)
{
	struct pie_regs regs;

	memset(&regs, 0x5a, sizeof(regs));
	pie_set(&regs);

	/* Keep the PIE registers live until the high priority thread is
	 * done preempting this thread from its wake-up interrupt.
	 */
	while (atomic_get(&preempt_done) == 0) {
		pie_check(&regs);
	}
}

static void preempt_high_entry(void *p1, void *p2, void *p3)
{
	struct pie_regs regs;

	memset(&regs, 0xa5, sizeof(regs));

	for (uint32_t i = 0; i < PREEMPT_ITERATIONS; i++) {
		/* The timer interrupt that ends the sleep preempts the low
		 * priority thread, which is still using the PIE registers.
		 */
		k_sleep(K_MSEC(1));

		pie_set(&regs);
		pie_check(&regs);
	}

	atomic_set(&preempt_done, 1);
}

ZTEST(pie, test_register_coherence_preemption)
{
	atomic_set(&preempt_done, 0);

	k_thread_create(&thread[0], thread_stack[0], STACK_SIZE, preempt_low_entry, NULL, NULL,
			NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	k_thread_create(&thread[1], thread_stack[1], STACK_SIZE, preempt_high_entry, NULL, NULL,
			NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	k_thread_join(&thread[1], K_FOREVER);
	k_thread_join(&thread[0], K_FOREVER);
}

ZTEST_SUITE(pie, NULL, NULL, NULL, NULL, NULL);
