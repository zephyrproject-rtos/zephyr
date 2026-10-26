/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/irq_offload.h>
#include <zephyr/arch/arch_inlines.h>

/**
 * @brief Architecture CPU identification tests
 * @defgroup arch_cpu_id_tests Architecture CPU identification
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify that arch_num_cpus() stays within the configured range.
 *
 * @details
 * The value may be computed at runtime on some SoCs, but it can never be zero
 * and can never exceed the number of CPUs the kernel allocated state for.
 *
 * Test steps:
 * - Read arch_num_cpus().
 *
 * Expected result:
 * - The value is at least 1 and at most CONFIG_MP_MAX_NUM_CPUS.
 *
 * @see arch_num_cpus()
 */
ZTEST(arch_cpu_id, test_arch_num_cpus_within_configured_range)
{
	unsigned int num_cpus = arch_num_cpus();

	zassert_true(num_cpus >= 1U, "no CPUs reported");
	zassert_true(num_cpus <= CONFIG_MP_MAX_NUM_CPUS, "%u CPUs exceed the configured %u",
		     num_cpus, CONFIG_MP_MAX_NUM_CPUS);
}

/**
 * @brief Verify that arch_proc_id() is stable while interrupts are locked.
 *
 * @details
 * With interrupts locked the calling thread cannot migrate, so two consecutive
 * reads must observe the same CPU. This holds on every architecture, including
 * uniprocessor ones where the function always reports the single CPU.
 *
 * Test steps:
 * - Lock interrupts.
 * - Read arch_proc_id() twice.
 * - Unlock interrupts.
 *
 * Expected result:
 * - Both reads return the same value.
 *
 * @see arch_proc_id()
 */
ZTEST(arch_cpu_id, test_arch_proc_id_stable_with_interrupts_locked)
{
	unsigned int key = irq_lock();
	uint32_t first = arch_proc_id();
	uint32_t second = arch_proc_id();

	irq_unlock(key);

	zassert_equal(first, second, "CPU ID changed from %u to %u", first, second);
}

static void read_proc_id_in_isr(const void *param)
{
	uint32_t *id = (uint32_t *)param;

	zassert_true(k_is_in_isr(), "offload did not run in ISR context");
	*id = arch_proc_id();
}

/**
 * @brief Verify that arch_proc_id() reports the same CPU from ISR context.
 *
 * @details
 * The suite holds every other CPU while this test runs, so the test thread
 * cannot migrate. An interrupt synchronously offloaded from the thread runs on
 * the same CPU and must therefore observe the same ID the thread read.
 *
 * Test steps:
 * - Read arch_proc_id() from the test thread.
 * - Run a handler through irq_offload() that reads arch_proc_id() again.
 *
 * Expected result:
 * - The handler ran in ISR context and read the same value as the thread.
 *
 * @see arch_proc_id()
 * @see irq_offload()
 */
ZTEST(arch_cpu_id, test_arch_proc_id_isr_matches_thread)
{
	uint32_t thread_id = arch_proc_id();
	uint32_t isr_id = UINT32_MAX;

	irq_offload(read_proc_id_in_isr, &isr_id);

	zassert_equal(isr_id, thread_id, "ISR read CPU %u, thread read CPU %u", isr_id, thread_id);
}

ZTEST_SUITE(arch_cpu_id, NULL, NULL, ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);

#if defined(CONFIG_SMP) && defined(CONFIG_SCHED_CPU_MASK) && (CONFIG_MP_MAX_NUM_CPUS > 1)

#define STACK_SIZE 1024
#define PRIORITY   7

struct cpu_sample {
	uint32_t proc_id;
	int cpu_index;
};

static void sample_cpu(void *p1, void *p2, void *p3)
{
	struct cpu_sample *sample = p1;
	unsigned int key;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	key = irq_lock();
	sample->proc_id = arch_proc_id();
	sample->cpu_index = arch_curr_cpu()->id;
	irq_unlock(key);
}

/* Run one thread pinned to each CPU and collect what it sees there. */
static void sample_all_cpus(struct cpu_sample *samples, unsigned int num_cpus)
{
	static K_THREAD_STACK_ARRAY_DEFINE(stacks, CONFIG_MP_MAX_NUM_CPUS, STACK_SIZE);
	static struct k_thread threads[CONFIG_MP_MAX_NUM_CPUS];

	for (unsigned int i = 0; i < num_cpus; i++) {
		samples[i].proc_id = UINT32_MAX;
		samples[i].cpu_index = -1;
		k_thread_create(&threads[i], stacks[i], STACK_SIZE, sample_cpu, &samples[i], NULL,
				NULL, PRIORITY, 0, K_FOREVER);
		zassert_ok(k_thread_cpu_pin(&threads[i], i), "cannot pin a thread to CPU %u", i);
		k_thread_start(&threads[i]);
	}

	for (unsigned int i = 0; i < num_cpus; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}
}

/**
 * @brief Verify that arch_curr_cpu() identifies the CPU a thread is pinned to.
 *
 * @details
 * The kernel indexes its per-CPU state by the sequential CPU number, so a
 * thread pinned to CPU n must find id n in the struct arch_curr_cpu() returns.
 *
 * Test steps:
 * - Create one thread per CPU, pin each to its CPU and start it.
 * - Each thread reads arch_curr_cpu()->id with interrupts locked.
 * - Join all threads.
 *
 * Expected result:
 * - The thread pinned to CPU n read id n.
 *
 * @see arch_curr_cpu()
 * @see k_thread_cpu_pin()
 */
ZTEST(arch_cpu_id_smp, test_arch_curr_cpu_matches_pinned_cpu)
{
	struct cpu_sample samples[CONFIG_MP_MAX_NUM_CPUS];
	unsigned int num_cpus = arch_num_cpus();

	sample_all_cpus(samples, num_cpus);

	for (unsigned int i = 0; i < num_cpus; i++) {
		zassert_equal(samples[i].cpu_index, (int)i, "thread pinned to CPU %u ran on CPU %d",
			      i, samples[i].cpu_index);
	}
}

/**
 * @brief Verify that arch_proc_id() is unique on every CPU.
 *
 * @details
 * The hardware ID is platform-defined and need not match the kernel's CPU
 * index, but two CPUs must never report the same value.
 *
 * Test steps:
 * - Create one thread per CPU, pin each to its CPU and start it.
 * - Each thread reads arch_proc_id() with interrupts locked.
 * - Join all threads and compare every pair of values.
 *
 * Expected result:
 * - Every CPU reported a different ID.
 *
 * @see arch_proc_id()
 * @see k_thread_cpu_pin()
 */
ZTEST(arch_cpu_id_smp, test_arch_proc_id_unique_per_cpu)
{
	struct cpu_sample samples[CONFIG_MP_MAX_NUM_CPUS];
	unsigned int num_cpus = arch_num_cpus();

	sample_all_cpus(samples, num_cpus);

	for (unsigned int i = 0; i < num_cpus; i++) {
		for (unsigned int j = i + 1; j < num_cpus; j++) {
			zassert_not_equal(samples[i].proc_id, samples[j].proc_id,
					  "CPUs %u and %u both report ID %u", i, j,
					  samples[i].proc_id);
		}
	}
}

ZTEST_SUITE(arch_cpu_id_smp, NULL, NULL, NULL, NULL, NULL);

#endif /* CONFIG_SMP && CONFIG_SCHED_CPU_MASK && CONFIG_MP_MAX_NUM_CPUS > 1 */

/**
 * @}
 */
