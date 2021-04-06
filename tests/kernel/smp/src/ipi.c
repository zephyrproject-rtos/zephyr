/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <ztest.h>
#include <kernel.h>
#include <kswap.h>


BUILD_ASSERT(CONFIG_MP_NUM_CPUS > 1);

#define STACK_SIZE 1024

K_THREAD_STACK_DEFINE(tstack, STACK_SIZE);
static struct k_thread tdata;

static atomic_t start_flag;

static volatile int cpu_running[CONFIG_MP_NUM_CPUS];
static volatile int cpu_run_again[CONFIG_MP_NUM_CPUS];
static struct k_sem start_sem;


static FUNC_NORETURN void entry_reinit(void *arg)
{
	atomic_t *cpu_start_flag = arg;
	struct k_thread dummy_thread;

	/* Wait for the signal to begin scheduling */
	while (!atomic_get(cpu_start_flag)) {
	}

	z_dummy_thread_init(&dummy_thread);
	smp_timer_init();

	z_swap_unlocked();

	CODE_UNREACHABLE;
}

static void t_setup_precond(void *p1, void *p2, void *p3)
{
	k_tid_t main_thread = (k_tid_t)p2;

	/* make main thread do not use cpu 1 */
	k_thread_cpu_mask_disable(main_thread, 1);

	printk("complete precond setup\n");

	k_sem_give(&start_sem);
}

static void t_run_after_reinit(void *p1, void *p2, void *p3)
{
	int curr_cpuid = arch_curr_cpu()->id;

	cpu_run_again[curr_cpuid] = 1;
}
/**
 * @brief Test to verify CPU can restart by arch-SMP API
 *
 * @ingroup kernel_smp_tests
 *
 * @details Validate the cpu can be restart by API zepyhr provided:
 * - Setup the precondition to make main test thread not run in cpu 1
 * - Call arch_start_cpu() to restart the cpu 1
 * - Check if the target function was been executed.
 * - Check if a thread can be run after reinit.
 *
 * @see arch_start_cpu()
 */
void test_smp_restart(void)
{
	int main_thread_cpuid = arch_curr_cpu()->id;
	int target_cpuid;
	k_tid_t curr = k_current_get();

	k_sem_init(&start_sem, 0, 1);

	/*
	 * Spawn a thread to setup precondition, the main goal is
	 * to make sure no thread using cpu 1
	 */
	k_tid_t tid = k_thread_create(&tdata, tstack, STACK_SIZE,
			(k_thread_entry_t)t_setup_precond,
			(void *)&tid, (void *)&curr, NULL,
			K_PRIO_PREEMPT(0),
			K_INHERIT_PERMS, K_FOREVER);

	/* make setup precond thread not use cpu 1 */
	k_thread_start(tid);

	k_sem_take(&start_sem, K_FOREVER);
	k_sem_give(&start_sem);
	printk("wait...\n");

	/* wait until no one use cpu 1 */
	while (main_thread_cpuid == 1) {
		main_thread_cpuid = arch_curr_cpu()->id;
		k_usleep(1);
	}

	target_cpuid = 1;
	printk("precondition ready and start: main(%d), target(%d)\n",
			main_thread_cpuid, target_cpuid);

	/* start to reinit target cpu */
	(void)atomic_clear(&start_flag);

	arch_start_cpu(target_cpuid, z_interrupt_stacks[target_cpuid],
				CONFIG_ISR_STACK_SIZE,
				entry_reinit, &start_flag);

	(void)atomic_set(&start_flag, 1);

	/* wait 50 ms for the cpu 1 restart */
	k_busy_wait(50*1000);

	zassert_equal(cpu_running[target_cpuid], 1,
			"cpu not re-init and run");

	/* start a thread to test target cpu run */
	tid = k_thread_create(&tdata, tstack, STACK_SIZE,
				(k_thread_entry_t)t_run_after_reinit,
				(void *)&tid, NULL, NULL,
				K_PRIO_PREEMPT(0),
				K_INHERIT_PERMS, K_FOREVER);

	/* make the thread only run in cpu 1*/
	k_thread_cpu_mask_disable(tid, 0);

	k_thread_start(tid);

	k_thread_join(tid, K_FOREVER);

	zassert_equal(cpu_run_again[target_cpuid], 1,
			"thread not run after reinit");
}

