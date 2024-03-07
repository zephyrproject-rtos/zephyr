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

	extern void hifi_set(uint8_t *aed_buffer);
	extern void hifi_get(uint8_t *aed_buffer);

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

ZTEST(hifi, test_register_sanity)
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
}

ZTEST_SUITE(hifi, NULL, NULL, NULL, NULL, NULL);
