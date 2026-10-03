/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/profiling/irq_stats.h>
#include <zephyr/interrupt_util.h>

#if defined(CONFIG_CPU_CORTEX_M)
/* Any unused NVIC line works; keep clear of device interrupts */
#define TEST_IRQ      (CONFIG_NUM_IRQS - 2)
#define TEST_IRQ_PRIO 1
#define HAVE_TEST_IRQ 1

#define test_trigger() trigger_irq(TEST_IRQ)
#define test_isr_ack()
#elif defined(CONFIG_RISCV) && !defined(CONFIG_SMP)
/*
 * Machine software interrupt, software ISR table index 3. mip.MSIP is
 * read-only; it is set and cleared through the CLINT msip register.
 * Level-triggered, so the ISR must clear it.
 *
 * Only usable on uniprocessor builds: on SMP this vector belongs to
 * the scheduler IPI (RISCV_IRQ_MSOFT), and hijacking it would break
 * the scheduler.
 */
#define TEST_IRQ      3
#define TEST_IRQ_PRIO 0
#define HAVE_TEST_IRQ 1
#define MSIP_ADDR     DT_REG_ADDR(DT_INST(0, sifive_clint0))

#define test_trigger() sys_write32(1, MSIP_ADDR)
#define test_isr_ack() sys_write32(0, MSIP_ADDR)
#else
/* No vector this test may safely claim on this platform */
#define TEST_IRQ      0
#define TEST_IRQ_PRIO 0
#define HAVE_TEST_IRQ 0

#define test_trigger()
#define test_isr_ack()
#endif

#define SKIP_WITHOUT_TEST_IRQ()                                                                    \
	do {                                                                                       \
		if (!HAVE_TEST_IRQ) {                                                              \
			ztest_test_skip();                                                         \
		}                                                                                  \
	} while (0)

#define TRIGGER_COUNT 5

static volatile uint32_t isr_runs;

static void test_isr(const void *arg)
{
	ARG_UNUSED(arg);
	test_isr_ack();
	isr_runs++;
	/* Give the duration measurement something to measure */
	k_busy_wait(100);
}

ZTEST(irq_stats, test_counts_and_duration)
{
	struct irq_stats_entry entry;

	SKIP_WITHOUT_TEST_IRQ();

	irq_stats_reset();
	isr_runs = 0;

	arch_irq_connect_dynamic(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, 0);
	irq_enable(TEST_IRQ);

	for (int i = 0; i < TRIGGER_COUNT; i++) {
		test_trigger();
		k_busy_wait(1000);
	}
	irq_disable(TEST_IRQ);

	zassert_equal(isr_runs, TRIGGER_COUNT, "ISR ran %u times", isr_runs);

	zassert_ok(irq_stats_get(TEST_IRQ, &entry));
	zassert_equal(entry.count, TRIGGER_COUNT, "counted %u of %u invocations", entry.count,
		      TRIGGER_COUNT);
	zassert_true(entry.max_cycles > 0U, "no duration recorded");
	zassert_true(entry.total_cycles >= entry.max_cycles,
		     "total (%llu) below max (%u)",
		     (unsigned long long)entry.total_cycles, entry.max_cycles);
	/* 5 x >=100us busy-wait must be way over 5x the max of one run */
	zassert_true(entry.total_cycles > (uint64_t)entry.max_cycles,
		     "total should accumulate across invocations");
}

ZTEST(irq_stats, test_per_cpu_matches_aggregate)
{
	struct irq_stats_entry aggregate, per_cpu;
	uint32_t summed = 0;

	SKIP_WITHOUT_TEST_IRQ();

	irq_stats_reset();

	arch_irq_connect_dynamic(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, 0);
	irq_enable(TEST_IRQ);
	for (int i = 0; i < TRIGGER_COUNT; i++) {
		test_trigger();
		k_busy_wait(1000);
	}
	irq_disable(TEST_IRQ);

	zassert_ok(irq_stats_get(TEST_IRQ, &aggregate));
	for (unsigned int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		zassert_ok(irq_stats_get_cpu(TEST_IRQ, cpu, &per_cpu));
		summed += per_cpu.count;
	}

	zassert_equal(summed, aggregate.count,
		      "per-CPU counts (%u) do not sum to the aggregate (%u)", summed,
		      aggregate.count);
	zassert_equal(aggregate.count, TRIGGER_COUNT);

	zassert_equal(irq_stats_get_cpu(TEST_IRQ, CONFIG_MP_MAX_NUM_CPUS, &per_cpu), -EINVAL);
}

ZTEST(irq_stats, test_reset_single_irq)
{
	struct irq_stats_entry entry;

	SKIP_WITHOUT_TEST_IRQ();

	irq_stats_reset();

	arch_irq_connect_dynamic(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, 0);
	irq_enable(TEST_IRQ);
	test_trigger();
	k_busy_wait(1000);
	irq_disable(TEST_IRQ);

	zassert_ok(irq_stats_get(TEST_IRQ, &entry));
	zassert_true(entry.count > 0U, "nothing recorded to reset");

	zassert_ok(irq_stats_reset_irq(TEST_IRQ));
	zassert_ok(irq_stats_get(TEST_IRQ, &entry));
	zassert_equal(entry.count, 0U);
	zassert_equal(entry.total_cycles, 0ULL);
	zassert_equal(entry.max_cycles, 0U);

	zassert_equal(irq_stats_reset_irq(CONFIG_NUM_IRQS), -EINVAL);
}

ZTEST(irq_stats, test_reset)
{
	struct irq_stats_entry entry;

	SKIP_WITHOUT_TEST_IRQ();

	arch_irq_connect_dynamic(TEST_IRQ, TEST_IRQ_PRIO, test_isr, NULL, 0);
	irq_enable(TEST_IRQ);
	test_trigger();
	k_busy_wait(1000);
	irq_disable(TEST_IRQ);

	irq_stats_reset();
	zassert_ok(irq_stats_get(TEST_IRQ, &entry));
	zassert_equal(entry.count, 0U);
	zassert_equal(entry.total_cycles, 0ULL);
	zassert_equal(entry.max_cycles, 0U);
}

ZTEST(irq_stats, test_get_bad_args)
{
	struct irq_stats_entry entry;

	zassert_equal(irq_stats_get(CONFIG_NUM_IRQS, &entry), -EINVAL);
	zassert_equal(irq_stats_get(0, NULL), -EINVAL);
}

ZTEST(irq_stats, test_per_cpu_invariant)
{
	/*
	 * Runs on every platform, including SMP where the trigger-based
	 * tests are skipped: whatever interrupts the system takes on its
	 * own must still satisfy "the aggregate is the sum of the
	 * per-CPU counters". Counters only grow, so sampling the
	 * aggregate before the per-CPU values makes the comparison
	 * race-free without stopping the world.
	 */
	bool saw_activity = false;

	k_msleep(20);

	for (unsigned int irq = 0; irq < CONFIG_NUM_IRQS; irq++) {
		struct irq_stats_entry aggregate, per_cpu;
		uint32_t summed = 0;

		zassert_ok(irq_stats_get(irq, &aggregate));
		for (unsigned int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
			zassert_ok(irq_stats_get_cpu(irq, cpu, &per_cpu));
			summed += per_cpu.count;
			zassert_true(per_cpu.count <= aggregate.count || IS_ENABLED(CONFIG_SMP),
				     "IRQ %u: CPU %u count %u exceeds aggregate %u", irq, cpu,
				     per_cpu.count, aggregate.count);
		}

		zassert_true(summed >= aggregate.count,
			     "IRQ %u: per-CPU counts (%u) below the aggregate (%u)", irq, summed,
			     aggregate.count);

		if (aggregate.count > 0U) {
			saw_activity = true;
		}
	}

	if (IS_ENABLED(CONFIG_RISCV)) {
		/* The RISC-V timer is dispatched through the ISR table */
		zassert_true(saw_activity, "no interrupt activity recorded at all");
	}
}

ZTEST_SUITE(irq_stats, NULL, NULL, NULL, NULL, NULL);
