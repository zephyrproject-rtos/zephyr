/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/wait_q.h>
#include <ksched.h>

#include "app_threads.h"
#include "user.h"

#define MAIN_PRIO 8
#define THREADS_PRIO 9

enum {
	MEAS_START,
	MEAS_END,
	NUM_STAMP_STATES
};

uint32_t stamps[NUM_STAMP_STATES];

static inline int stamp(int state)
{
	uint32_t t;

	/* In theory the TSC has much lower overhead and higher
	 * precision.  In practice it's VERY jittery in recent qemu
	 * versions and frankly too noisy to trust.
	 */
#ifdef CONFIG_X86
	__asm__ volatile("rdtsc" : "=a"(t) : : "edx");
#else
	t = k_cycle_get_32();
#endif

	stamps[state] = t;
	return t;
}

static int yielder_status;

void yielder_entry(void *_thread, void *_tid, void *_nb_threads)
{
	struct k_app_thread *thread = (struct k_app_thread *) _thread;
	int ret;

	struct k_mem_partition *parts[] = {
		thread->partition,
	};

	ret = k_mem_domain_init(&thread->domain, ARRAY_SIZE(parts), parts);
	if (ret != 0) {
		printk("k_mem_domain_init failed %d\n", ret);
		yielder_status = 1;
		return;
	}

	k_mem_domain_add_thread(&thread->domain, k_current_get());

	k_thread_user_mode_enter(context_switch_yield, _nb_threads, NULL, NULL);
}


static k_tid_t threads[MAX_NB_THREADS];

static int exec_test(uint8_t nb_threads)
{
	if (nb_threads > MAX_NB_THREADS) {
		printk("Too many threads\n");
		return 1;
	}

	yielder_status = 0;

	for (size_t tid = 0; tid < nb_threads; tid++) {
		app_threads[tid].partition = app_partitions[tid];
		app_threads[tid].stack = &app_thread_stacks[tid];

		void *_tid = (void *)(uintptr_t)tid;

		threads[tid] = k_thread_create(&app_threads[tid].thread,
					app_thread_stacks[tid],
					APP_STACKSIZE, yielder_entry,
					&app_threads[tid], _tid, (void *)(uintptr_t)nb_threads,
					THREADS_PRIO, 0, K_FOREVER);
	}

	/* make sure the main thread has a higher priority
	 * this way, user threads all start together
	 * (lower number --> higher prio)
	 */
	k_thread_priority_set(k_current_get(), MAIN_PRIO);

	stamp(MEAS_START);
	for (size_t tid = 0; tid < nb_threads; tid++) {
		k_thread_start(threads[tid]);
	}
	for (size_t tid = 0; tid < nb_threads; tid++) {
		k_thread_join(threads[tid], K_FOREVER);
	}
	stamp(MEAS_END);

	uint32_t full_time = stamps[MEAS_END] - stamps[MEAS_START];
	uint64_t time_ms = k_cyc_to_ns_near64(full_time)/NB_YIELDS;

	printk("Swapping %2u threads: %8" PRIu32 " cyc & %6" PRIu32 " rounds -> %6"
				PRIu64 " ns per ctx\n", nb_threads, full_time,
				NB_YIELDS, time_ms);

	return yielder_status;
}


void main(void)
{
	int ret;

	printk("Userspace scheduling benchmark started on board %s\n", CONFIG_BOARD);

	size_t nb_threads_list[] = {2, 8, 16, 32, 0};

	printk("============================\n");
	printk("user/user^n swapping (yield)\n");

	for (size_t i = 0; nb_threads_list[i] > 0; i++) {
		ret = exec_test(nb_threads_list[i]);
		if (ret != 0) {
			printk("FAIL\n");
			return;
		}
	}

	printk("SUCCESS\n");
}
