/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <intel_adsp_ipc.h>
#include "tests.h"

#define RUN_ON_STACKSZ 2048
#define HAR_STACKSZ    1024
#define HAR_PRIORITY   7

/* Utility for spin-polled loops.  Avoids spamming shared resources
 * like SRAM or MMIO registers
 */
static ALWAYS_INLINE void delay_relax(void)
{
	for (volatile int j = 0; j < 1000; j++) {
	}
}

static void run_on_cpu_threadfn(void *a, void *b, void *c)
{
	void (*fn)(void *) = a;
	void *arg = b;
	volatile bool *done_flag = c;

	fn(arg);
	*done_flag = true;
}

static struct k_thread thread_har;
static K_THREAD_STACK_DEFINE(tstack_har, HAR_STACKSZ);

static struct k_thread run_on_threads[CONFIG_MP_NUM_CPUS];
static K_THREAD_STACK_ARRAY_DEFINE(run_on_stacks, CONFIG_MP_NUM_CPUS, RUN_ON_STACKSZ);
static volatile bool run_on_flags[CONFIG_MP_NUM_CPUS];

static uint32_t clk_ratios[CONFIG_MP_NUM_CPUS];

static void run_on_cpu(int cpu, void (*fn)(void *), void *arg, bool wait)
{
	__ASSERT_NO_MSG(cpu < CONFIG_MP_NUM_CPUS);

	/* Highest priority isn't actually guaranteed to preempt
	 * whatever's running, but we assume the test hasn't laid
	 * traps for itself.
	 */
	k_thread_create(&run_on_threads[cpu], run_on_stacks[cpu], RUN_ON_STACKSZ,
			run_on_cpu_threadfn, fn, arg, (void *)&run_on_flags[cpu],
			K_HIGHEST_THREAD_PRIO, 0, K_FOREVER);
	k_thread_cpu_mask_clear(&run_on_threads[cpu]);
	k_thread_cpu_mask_enable(&run_on_threads[cpu], cpu);
	run_on_flags[cpu] = false;
	k_thread_start(&run_on_threads[cpu]);

	if (wait) {
		while (!run_on_flags[cpu]) {
			delay_relax();
			k_yield();
		}
		k_thread_abort(&run_on_threads[cpu]);
	}
}

static inline uint32_t ccount(void)
{
	uint32_t ret;

	__asm__ volatile("rsr %0, CCOUNT" : "=r"(ret));
	return ret;
}

static void core_smoke(void *arg)
{
	int cpu = (int) arg;
	volatile int tag;
	static int static_tag;

	zassert_equal(cpu, arch_curr_cpu()->id, "wrong cpu");

	/* Un/cached regions should be configured and distinct */
	zassert_equal(&tag, arch_xtensa_cached_ptr((void *)&tag),
		      "stack memory not cached");
	zassert_not_equal(&tag, arch_xtensa_uncached_ptr((void *)&tag),
			  "stack memory not cached");
	zassert_not_equal(&static_tag, arch_xtensa_cached_ptr((void *)&static_tag),
		      "stack memory not cached");
	zassert_equal(&static_tag, arch_xtensa_uncached_ptr((void *)&static_tag),
			  "stack memory not cached");

	/* Un/cached regions should be working */
	printk(" Cache behavior check\n");
	volatile int *ctag = (volatile int *)arch_xtensa_cached_ptr((void *)&tag);
	volatile int *utag = (volatile int *)arch_xtensa_uncached_ptr((void *)&tag);

	tag = 99;
	zassert_true(*ctag == 99, "variable is cached");
	*utag = 42;
	zassert_true(*ctag == 99, "uncached assignment unexpectedly affected cache");
	zassert_true(*utag == 42, "uncached memory affected unexpectedly");
	z_xtensa_cache_flush((void *)ctag, sizeof(*ctag));
	zassert_true(*utag == 99, "cache flush didn't work");

	/* Calibrate clocks */
	uint32_t cyc1, cyc0 = k_cycle_get_32();
	uint32_t cc1, cc0 = ccount();

	do {
		cyc1 = k_cycle_get_32();
		cc1 = ccount();
	} while ((cc1 - cc0) < 1000 || (cyc1 - cyc0) < 1000);

	clk_ratios[cpu] = ((cc1 - cc0) * 1000) / (cyc1 - cyc0);
	printk(" CCOUNT/WALCLK ratio %d.%3.3d\n",
	       clk_ratios[cpu] / 1000, clk_ratios[cpu] % 1000);

	for (int i = 0; i < cpu; i++) {
		int32_t diff = MAX(1, abs(clk_ratios[i] - clk_ratios[cpu]));

		zassert_true((clk_ratios[cpu] / diff) > 100,
			     "clocks off by more than 1%");
	}

	/* Check tight loop performance to validate instruction cache */
	uint32_t count0 = 1000, count, dt, insns;

	count = count0;
	cyc0 = ccount();
	__asm__ volatile("1: addi %0, %0, -1; bnez %0, 1b" : "+r"(count));
	cyc1 = ccount();
	dt = cyc1 - cyc0;
	insns = count0 * 2;
	zassert_true((dt / insns) < 3,
		     "instruction rate too slow, icache disabled?");
	printk(" CPI = %d.%2.2d\n", dt / insns, ((1000 * dt) / insns) % 1000);
}

ZTEST(intel_adsp_boot, test_4th_cpu_behavior)
{
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		printk("Per-CPU smoke test %d...\n", i);
		run_on_cpu(i, core_smoke, (void *)i, true);
	}
}

static void alive_fn(void *arg)
{
	*(bool *)arg = true;
}

static void halt_and_restart(int cpu)
{
	printk("halt/restart core %d...\n", cpu);
	static bool alive_flag;
	uint32_t all_cpus = BIT(CONFIG_MP_NUM_CPUS) - 1;
	int ret;

	/* On older hardware we need to get the host to turn the core
	 * off.  Construct an ADSPCS with only this core disabled
	 */
	if (!IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V25)) {
		intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_ADSPCS,
				     (all_cpus & ~BIT(cpu)) << 16);
	}

	ret = soc_adsp_halt_cpu(cpu);
	zassert_ok(ret, "Couldn't halt CPU");

	alive_flag = false;
	run_on_cpu(cpu, alive_fn, &alive_flag, false);
	k_msleep(100);
	zassert_false(alive_flag, "cpu didn't halt");

	if (!IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V25)) {
		/* Likewise need to ask the host to turn it back on,
		 * and give it some time to spin up before we hit it.
		 * We don't have a return message wired to be notified
		 * of completion.
		 */
		intel_adsp_ipc_send_message(INTEL_ADSP_IPC_HOST_DEV, IPCCMD_ADSPCS,
				     all_cpus << 16);
		k_msleep(50);
	}

	z_smp_start_cpu(cpu);

	/* Startup can be slow */
	k_msleep(50);

	AWAIT(alive_flag == true);

	k_thread_abort(&run_on_threads[cpu]);
}

void halt_and_restart_thread(void *p1, void *p2, void *p3)
{
	for (int i = 1; i < CONFIG_MP_NUM_CPUS; i++) {
		halt_and_restart(i);
	}
}

ZTEST(intel_adsp_boot, test_2nd_cpu_halt)
{
	int ret;

	if (IS_ENABLED(CONFIG_SOC_INTEL_CAVS_V15)) {
		ztest_test_skip();
	}

	/* Obviously this only works on CPU0. So, we create a thread pinned
	 * to CPU0 to effectively run the test.
	 */
	k_thread_create(&thread_har, tstack_har, HAR_STACKSZ,
			halt_and_restart_thread, NULL, NULL, NULL,
			HAR_PRIORITY, 0, K_FOREVER);
	ret = k_thread_cpu_pin(&thread_har, 0);
	zassert_ok(ret, "Couldn't pin thread to CPU 0, test can't be run");
	k_thread_start(&thread_har);

	k_thread_join(&thread_har, K_FOREVER);
}
