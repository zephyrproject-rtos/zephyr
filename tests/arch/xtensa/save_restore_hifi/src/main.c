/*
 * Copyright (c) 2024, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>

#define STACK_SIZE  1024
#define NUM_THREADS (CONFIG_MP_MAX_NUM_CPUS * 2)

K_THREAD_STACK_ARRAY_DEFINE(thread_stack, NUM_THREADS, STACK_SIZE);

struct k_thread thread[NUM_THREADS];

extern void hifi_set(uint8_t *aed_buffer);
extern void hifi_get(uint8_t *aed_buffer);

static void thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t i;
	uint32_t j;
	uint32_t index = (uint32_t)(uintptr_t)p1;
	uint8_t  init_regs[8 * 16] __aligned(16) = {0};
	uint8_t  value_regs[8 * 16] __aligned(16) = {0};

	if (index < (NUM_THREADS - 1)) {
		k_thread_start(&thread[index + 1]);
	}

	/* Initialize the AE regs with known values */

	for (i = 0; i < sizeof(init_regs); i++) {
		init_regs[i] = (index & 0xff);
	}

	hifi_set(init_regs);

	for (i = 0; i < 10; i++) {
		k_yield();    /* Switch to a new thread */

		/*
		 * Verify that the HiFi AE regs have not been corrupted
		 * by another thread.
		 */

		hifi_get(value_regs);

		for (j = 0; j < sizeof(value_regs); j++) {
			zassert_equal(value_regs[j], init_regs[j],
				      "Expected %u, got %u\n",
				      init_regs[j], value_regs[j]);
		}
	}
}

ZTEST(hifi, test_register_coherence)
{
	int       priority;
	uint32_t  i;

	priority = k_thread_priority_get(k_current_get());

	/* Create twice as many threads as there are CPUs */

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_create(&thread[i], thread_stack[i], STACK_SIZE,
				thread_entry, (void *)(uintptr_t)i, NULL, NULL,
				priority - 1, 0, K_FOREVER);
	}

	k_thread_start(&thread[0]);

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_join(&thread[i], K_FOREVER);
	}
}

#define PREEMPT_ITERATIONS 100

K_THREAD_STACK_DEFINE(preempt_low_stack, STACK_SIZE);
K_THREAD_STACK_DEFINE(preempt_high_stack, STACK_SIZE);

static struct k_thread preempt_low_thread;
static struct k_thread preempt_high_thread;
static atomic_t preempt_done;

static void preempt_low_entry(void *p1, void *p2, void *p3)
{
	uint8_t init_regs[8 * 16] __aligned(16);
	uint8_t value_regs[8 * 16] __aligned(16);

	memset(init_regs, 0x5a, sizeof(init_regs));
	hifi_set(init_regs);

	/* Keep the HiFi registers live until the high priority thread is
	 * done preempting this thread from its wake-up interrupt.
	 */
	while (atomic_get(&preempt_done) == 0) {
		hifi_get(value_regs);
		zassert_mem_equal(value_regs, init_regs, sizeof(init_regs),
				  "HiFi registers corrupted by preemption");
	}
}

static void preempt_high_entry(void *p1, void *p2, void *p3)
{
	uint8_t init_regs[8 * 16] __aligned(16);
	uint8_t value_regs[8 * 16] __aligned(16);

	memset(init_regs, 0xa5, sizeof(init_regs));

	for (uint32_t i = 0; i < PREEMPT_ITERATIONS; i++) {
		/* The timer interrupt that ends the sleep preempts the low
		 * priority thread, which is still using the HiFi registers.
		 */
		k_sleep(K_MSEC(1));

		hifi_set(init_regs);
		hifi_get(value_regs);
		zassert_mem_equal(value_regs, init_regs, sizeof(init_regs),
				  "HiFi registers not loaded after preemption");
	}

	atomic_set(&preempt_done, 1);
}

ZTEST(hifi, test_register_coherence_preemption)
{
	atomic_set(&preempt_done, 0);

	k_thread_create(&preempt_low_thread, preempt_low_stack, STACK_SIZE, preempt_low_entry, NULL,
			NULL, NULL, K_PRIO_PREEMPT(2), 0, K_FOREVER);
	k_thread_create(&preempt_high_thread, preempt_high_stack, STACK_SIZE, preempt_high_entry,
			NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_FOREVER);

#ifdef CONFIG_SCHED_CPU_MASK
	/* Both threads must share a CPU for one to preempt the other */
	zassert_ok(k_thread_cpu_pin(&preempt_low_thread, 0));
	zassert_ok(k_thread_cpu_pin(&preempt_high_thread, 0));
#endif

	k_thread_start(&preempt_low_thread);
	k_thread_start(&preempt_high_thread);

	k_thread_join(&preempt_high_thread, K_FOREVER);
	k_thread_join(&preempt_low_thread, K_FOREVER);
}

ZTEST_SUITE(hifi, NULL, NULL, NULL, NULL, NULL);
