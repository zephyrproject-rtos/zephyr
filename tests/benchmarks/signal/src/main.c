/*
 * Copyright (c) 2026, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(bench_bmem_partition);
K_APPMEM_PARTITION_DEFINE(bench_dmem_partition);
#endif

#ifdef CONFIG_USERSPACE
#define BENCH_BMEM K_APP_BMEM(bench_bmem_partition)
#define BENCH_DMEM K_APP_DMEM(bench_dmem_partition)
#else
#define BENCH_BMEM
#define BENCH_DMEM
#endif

#define STACK_SIZE K_THREAD_STACK_LEN(CONFIG_TEST_STACK_SIZE)
#define NUM_CPUS CONFIG_MP_MAX_NUM_CPUS

static void thread_fun(void *arg1, void *arg2, void *arg3);

/* array of thread stacks */
#define K_THREAD_ARRAY_DEFINE_ELEM(idx, name_base, stack_size, entry, prio, options, delay) \
	K_THREAD_DEFINE(name_base##_##idx, stack_size, entry, NULL, NULL, NULL, prio, options, delay)

#define K_THREAD_ARRAY_ELEM_ID(idx, name_base) \
	&_k_thread_obj_##name_base##_##idx

#define K_THREAD_ARRAY_STACK_ELEM_ID(idx, name_base) \
	(k_thread_stack_t *)&_k_thread_stack_##name_base##_##idx

#define K_THREAD_ARRAY_DEFINE_STATIC(name, num, stack_size, entry, prio, options, delay) \
	LISTIFY(num, K_THREAD_ARRAY_DEFINE_ELEM, (;), \
		  name, stack_size, entry, prio, options, delay); \
	BENCH_DMEM static struct k_thread *name[] = { \
		LISTIFY(num, K_THREAD_ARRAY_ELEM_ID, (,), name) \
	}; \
	BENCH_BMEM static k_thread_stack_t *name##_stack[] = { \
		LISTIFY(num, K_THREAD_ARRAY_STACK_ELEM_ID, (,), name) \
	}

K_THREAD_ARRAY_DEFINE_STATIC(thread, NUM_CPUS, STACK_SIZE, thread_fun, K_PRIO_PREEMPT(10), K_USER, K_TICKS_FOREVER);

BENCH_BMEM static uint32_t sigcount[NUM_CPUS];
BENCH_BMEM static uint32_t sigretry[NUM_CPUS];
BENCH_BMEM static atomic_t threads_up;
BENCH_BMEM static atomic_t finish;
BENCH_BMEM static atomic_t alive[NUM_CPUS];

static void print_group_stats(void)
{
	uint32_t count = 0;
	uint32_t retry = 0;

	for (int i = 0; i < NUM_CPUS; ++i) {
		count += sigcount[i];
		retry += sigretry[i];
	}

	printk("%u, %u, %u, %u, %u, %u\n", CONFIG_TEST_DURATION_S, NUM_CPUS, retry, count,
	       retry / CONFIG_TEST_DURATION_S / NUM_CPUS, count / CONFIG_TEST_DURATION_S / NUM_CPUS);
}

static int get_index(void)
{
	k_tid_t const self = k_current_get();

	ARRAY_FOR_EACH(thread, i) {
		if (self == thread[i]) {
			return i;
		}
	}

	__ASSERT(false, "Thread %p not found in thread array", self);

	return -1;
}

static void handler(int sig, struct k_sig_info *info, void *context)
{
	ARG_UNUSED(sig);
	ARG_UNUSED(info);
	ARG_UNUSED(context);

	int i = get_index();

	++sigcount[i];
}

static void thread_fun(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int i = get_index();
	__maybe_unused int ret;
	struct k_sig_action action = {
		.handler = handler,
	};
	struct k_sig_set mask;

	k_sig_emptyset(&mask);
	k_sig_addset(&mask, K_SIG_USR1);

	ret = k_sig_action(K_SIG_USR1, &action, NULL);
	__ASSERT(ret == 0, "%s() failed: %d", "k_sig_action", ret);

	alive[i] = true;

	if (IS_ENABLED(CONFIG_USERSPACE)) {
		__ASSERT(k_is_user_context(), "Thread is not running in user mode");
	}

	if (!k_is_user_context()) {
		ret = k_sig_mask(K_SIG_UNBLOCK, &mask, NULL);
		__ASSERT(ret == 0, "%s() failed: %d", "k_sig_mask", ret);
	}

	atomic_inc(&threads_up);

	while (atomic_get(&threads_up) < NUM_CPUS) {
		k_msleep(1);
	}

	while (!atomic_get(&finish)) {
		for (ret = -ESRCH; (ret == -EAGAIN) || (ret == -ESRCH); ) {
			int j = 0;

			if (NUM_CPUS > 1) {
				do {
					j = sys_rand8_get() % NUM_CPUS;
				} while(!alive[j]);
			}

			ret = k_sig_queue(thread[j], K_SIG_USR1, (union k_sig_val){0});

			if (ret == -EAGAIN) {
				/* Retries indicate SIGNAL_QUEUE_SIZE is too low */
				++sigretry[j];
			}
		}
		__ASSERT(ret == 0, "%s() failed: %d", "k_sig_queue", ret);
	}

	alive[i] = false;
	atomic_dec(&threads_up);

	if (!k_is_user_context()) {
		action.handler = K_SIG_DFL;
		ret = k_sig_action(K_SIG_USR1, &action, NULL);
		__ASSERT(ret == 0, "%s() failed: %d", "k_sig_action", ret);
	}
}

static void run_benchmark(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	if (IS_ENABLED(CONFIG_USERSPACE)) {
		__ASSERT(!k_is_user_context(), "Main thread is not running in kernel mode");
	}

	int __maybe_unused ret;

	printk("USERSPACE: %c\n", IS_ENABLED(CONFIG_USERSPACE) ? 'y' : 'n');
	printk("ASSERT: %c\n", IS_ENABLED(CONFIG_ASSERT) ? 'y' : 'n');
	printk("BOARD: %s\n", CONFIG_BOARD);
	printk("NUM_CPUS: %u\n", NUM_CPUS);
	printk("TEST_DURATION_S: %u\n", CONFIG_TEST_DURATION_S);
	printk("SMP: %c\n", IS_ENABLED(CONFIG_SMP) ? 'y' : 'n');

#ifdef CONFIG_USERSPACE
	ret = k_mem_domain_add_partition(&k_mem_domain_default,
				   &bench_bmem_partition);
	__ASSERT(ret == 0, "%s() failed: %d", "k_mem_domain_add_partition", ret);

	ret = k_mem_domain_add_partition(&k_mem_domain_default,
				   &bench_dmem_partition);
	__ASSERT(ret == 0, "%s() failed: %d", "k_mem_domain_add_partition", ret);
#endif

	for (int i = 0; i < NUM_CPUS; ++i) {
		if (IS_ENABLED(CONFIG_USERSPACE)) {
			k_thread_access_grant(thread[i], thread_stack[i]);
		}

		k_thread_start(thread[i]);
	}

	while (atomic_get(&threads_up) < NUM_CPUS) {
		k_msleep(1);
	}

	k_timeout_t timeout = K_SECONDS(CONFIG_TEST_DURATION_S);
	k_timepoint_t end = sys_timepoint_calc(timeout);

	do {
		k_sleep(timeout);
		timeout = sys_timepoint_timeout(end);
	} while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));
	atomic_set(&finish, 1);

	for (int i = 0; i < NUM_CPUS; ++i) {
		ret = k_thread_join(thread[i], K_NO_WAIT);
		__ASSERT(ret == 0, "%s() failed: %d", "k_thread_join", ret);
	}

	print_group_stats();

	printk("PROJECT EXECUTION SUCCESSFUL\n");
}


K_THREAD_DEFINE(run_benchmark_thread, STACK_SIZE, run_benchmark, NULL, NULL, NULL,
		K_PRIO_PREEMPT(10), 0, 0);

int main(void)
{
	k_thread_join(run_benchmark_thread, K_FOREVER);
	return 0;
}
