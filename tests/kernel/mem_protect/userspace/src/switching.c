/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/libc-hooks.h> /* for z_libc_partition */

#define NUM_THREADS     3
#define TIMES_SWITCHING 10
#define STACKSIZE       (256 + CONFIG_TEST_EXTRA_STACK_SIZE)

extern void clear_fault(void);

#ifdef CONFIG_USERSPACE_SWITCHING_TESTS
/*
 * Even numbered threads use domain_a.
 * Odd numbered threads use domain_b.
 */

struct k_mem_domain domain_a;
K_APPMEM_PARTITION_DEFINE(partition_a);
K_APP_BMEM(partition_a) volatile unsigned int part_a_loops[NUM_THREADS];

struct k_mem_domain domain_b;
K_APPMEM_PARTITION_DEFINE(partition_b);
K_APP_BMEM(partition_b) volatile unsigned int part_b_loops[NUM_THREADS];

static struct k_thread threads[NUM_THREADS];
static K_THREAD_STACK_ARRAY_DEFINE(threads_stacks, NUM_THREADS, STACKSIZE);

static K_SEM_DEFINE(sem_switching, 1, 1);

static void switch_thread_fn(void *arg1, void *arg2, void *arg3)
{
	volatile unsigned int *loop_ptr;
	const uintptr_t thread_id = (uintptr_t)arg1;

	if ((thread_id % 2) == 0) {
		loop_ptr = &part_a_loops[thread_id];
	} else {
		loop_ptr = &part_b_loops[thread_id];
	}

	for (int i = 0; i < TIMES_SWITCHING; i++) {
#ifdef CONFIG_DEBUG
		TC_PRINT("Thread %lu (%u)\n", thread_id, *loop_ptr);
#endif

		*loop_ptr += 1;
		compiler_barrier();

		/* Make sure this can still use kernel objects. */
		k_sem_take(&sem_switching, K_FOREVER);
		k_sem_give(&sem_switching);

		k_yield();
	}
}

#endif /* CONFIG_USERSPACE_SWITCHING_TESTS */

static void run_switching(int num_kernel_threads)
{
#ifdef CONFIG_USERSPACE_SWITCHING_TESTS
	unsigned int i;
	int remaining_kernel_threads = num_kernel_threads;

	/* Not expecting any errors. */
	clear_fault();

	for (i = 0; i < NUM_THREADS; i++) {
		uint32_t perms;
		bool is_kernel_thread = remaining_kernel_threads > 0;

		if (is_kernel_thread) {
			perms = K_INHERIT_PERMS;

			remaining_kernel_threads--;
		} else {
			perms = K_INHERIT_PERMS | K_USER;
		}

		/* Clear loop counters. */
		part_a_loops[i] = 0;
		part_b_loops[i] = 0;

		/* Must delay start of threads to apply memory domains to them. */
		k_thread_create(&threads[i], threads_stacks[i], STACKSIZE, switch_thread_fn,
				(void *)(uintptr_t)i, NULL, NULL, -1, perms, K_FOREVER);

#ifdef CONFIG_SCHED_CPU_MASK
		/*
		 * Make sure all created threads run on the same CPU
		 * so that memory domain switching is being tested.
		 */
		(void)k_thread_cpu_pin(&threads[i], 0);
#endif /* CONFIG_SCHED_CPU_MASK */

		k_thread_access_grant(&threads[i], &sem_switching);

		/*
		 * Kernel threads by default has access to all memory.
		 * So no need to put them into memory domains.
		 */
		if (!is_kernel_thread) {
			/* Remember EVEN -> A, ODD -> B. */
			if ((i % 2) == 0) {
				k_mem_domain_add_thread(&domain_a, &threads[i]);
			} else {
				k_mem_domain_add_thread(&domain_b, &threads[i]);
			}
		}
	}

	/* Start the thread loops. */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_start(&threads[i]);
	}

	/* Wait for all threads to finish. */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}

	/* Check to make sure all threads have looped enough times. */
	for (i = 0; i < NUM_THREADS; i++) {
		int loops;

		/*
		 * Each thread should never have access to the loop counters on
		 * the other partition. Accessing them should generate faults.
		 * Though we check just in case.
		 */
		if ((i % 2) == 0) {
			loops = part_a_loops[i];

			zassert_equal(part_b_loops[i], 0, "part_b_loops[%i] should be zero but not",
				      i);
		} else {
			loops = part_b_loops[i];

			zassert_equal(part_a_loops[i], 0, "part_a_loops[%i] should be zero but not",
				      i);
		}

		zassert_equal(loops, TIMES_SWITCHING,
			      "thread %u has not done enough loops (%u != %u)", i, loops,
			      TIMES_SWITCHING);
	}
#else  /* CONFIG_USERSPACE_SWITCHING_TESTS */
	ztest_test_skip();
#endif /* CONFIG_USERSPACE_SWITCHING_TESTS */
}

ZTEST(userspace_domain_switching, test_kernel_only_switching)
{
	/*
	 * Run with all kernel threads.
	 *
	 * This should work as kernel threads by default have access to
	 * all memory, without having to attach them to memory domains.
	 * This serves as a baseline check.
	 */
	run_switching(NUM_THREADS);
}

ZTEST(userspace_domain_switching, test_user_only_switching)
{
	/* Run with all user threads. */
	run_switching(0);
}

ZTEST(userspace_domain_switching, test_kernel_user_mix_switching)
{
	/* Run with one kernel thread while others are all user threads. */
	run_switching(1);
}

void *switching_setup(void)
{
#ifdef CONFIG_USERSPACE_SWITCHING_TESTS
	static bool already_inited;

	if (already_inited) {
		return NULL;
	}

	struct k_mem_partition *parts_a[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ztest_mem_partition, &partition_a
	};

	struct k_mem_partition *parts_b[] = {
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ztest_mem_partition, &partition_b
	};

	zassert_equal(k_mem_domain_init(&domain_a, ARRAY_SIZE(parts_a), parts_a), 0,
		      "failed to initialize memory domain A");

	zassert_equal(k_mem_domain_init(&domain_b, ARRAY_SIZE(parts_b), parts_b), 0,
		      "failed to initialize memory domain B");

	already_inited = true;
#endif /* CONFIG_USERSPACE_SWITCHING_TESTS */

	return NULL;
}

void switching_before(void *fixture)
{
#ifdef CONFIG_USERSPACE_SWITCHING_TESTS
	int i;

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_access_grant(k_current_get(), &threads[i]);
	}
#endif /* CONFIG_USERSPACE_SWITCHING_TESTS */
}

ZTEST_SUITE(userspace_domain_switching, NULL, switching_setup, switching_before, NULL, NULL);
