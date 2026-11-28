/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

/* A CCOMPARE timer interrupt is armed to fire a given number of cycles
 * later while the thread runs a deep call chain. Sweeping that delay
 * lands it on every instruction of the chain, including the register
 * window overflow and underflow handlers, which run with PS.EXCM set.
 * The interrupt above XCHAL_EXCM_LEVEL must preempt them without
 * corrupting the chain; the one at XCHAL_EXCM_LEVEL must never land
 * there.
 */

#define IRQ_LEVEL(irq) UTIL_CAT(UTIL_CAT(XCHAL_INT, irq), _LEVEL)

#define HIGH_IRQ   XCHAL_TIMER2_INTERRUPT
#define HIGH_LEVEL IRQ_LEVEL(HIGH_IRQ)
#define LOW_IRQ    XCHAL_TIMER1_INTERRUPT
#define LOW_LEVEL  IRQ_LEVEL(LOW_IRQ)

BUILD_ASSERT(HIGH_LEVEL > XCHAL_EXCM_LEVEL, "timer 2 must be above EXCM level");
BUILD_ASSERT(LOW_LEVEL <= XCHAL_EXCM_LEVEL, "timer 1 must not be above EXCM level");

#define SWEEP_CYCLES 3000U
#define CHAIN_DEPTH  12U

static uint32_t (*volatile chain_fn)(uint32_t n);
static volatile uint32_t hits;
static volatile uint32_t excm_hits;
static volatile int timer;

/* Recurse deep enough to overflow and underflow the register windows. The
 * call goes through chain_fn so the compiler cannot flatten it, and the
 * result checks that no frame was corrupted.
 */
static __noinline uint32_t chain(uint32_t n)
{
	volatile uint32_t pad[4] = {n, n * 3U, n ^ 0x55U, n + 7U};

	if (n == 0U) {
		return 1U;
	}

	return chain_fn(n - 1U) + pad[0] + pad[1] + pad[2] + pad[3];
}

static inline uint32_t read_ccount(void)
{
	uint32_t v;

	__asm__ volatile("rsr.ccount %0" : "=r"(v));
	return v;
}

static void set_ccompare(uint32_t val)
{
	if (timer == 1) {
		__asm__ volatile("wsr.ccompare1 %0; esync" : : "r"(val));
	} else {
		__asm__ volatile("wsr.ccompare2 %0; esync" : : "r"(val));
	}
}

static void high_isr(const void *arg)
{
	uint32_t eps;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.eps" STRINGIFY(HIGH_LEVEL) " %0" : "=r"(eps));
	if ((eps & PS_EXCM_MASK) != 0U) {
		excm_hits++;
	}
	set_ccompare(0U);
	(void)chain_fn(6U);
	hits++;
}

static void low_isr(const void *arg)
{
	uint32_t eps;

	ARG_UNUSED(arg);

	__asm__ volatile("rsr.eps" STRINGIFY(LOW_LEVEL) " %0" : "=r"(eps));
	if ((eps & PS_EXCM_MASK) != 0U) {
		excm_hits++;
	}
	set_ccompare(0U);
	(void)chain_fn(6U);
	hits++;
}

/* Fire the timer interrupt at every cycle offset of a deep call chain. */
static uint32_t sweep(void)
{
	uint32_t want = chain(CHAIN_DEPTH);
	uint32_t bad = 0U;

	hits = 0U;
	excm_hits = 0U;

	for (uint32_t d = 1U; d < SWEEP_CYCLES; d++) {
		uint32_t before = hits;
		uint32_t start;

		set_ccompare(read_ccount() + d);
		if (chain_fn(CHAIN_DEPTH) != want) {
			bad++;
		}
		start = read_ccount();
		while (hits == before) {
			if ((read_ccount() - start) > 100000U) {
				set_ccompare(read_ccount() + 50U);
				start = read_ccount();
			}
		}
	}

	return bad;
}

/* An ISR above EXCM level preempts window exception handlers without corrupting them. */
ZTEST(xtensa_high_priority_irq, test_above_excm)
{
	uint32_t bad;

	timer = 2;
	irq_enable(HIGH_IRQ);
	bad = sweep();
	irq_disable(HIGH_IRQ);

	zassert_equal(bad, 0U, "%u corrupted results", bad);
	zassert_equal(hits, SWEEP_CYCLES - 1U, "%u interrupts", hits);
	zassert_true(excm_hits > 0U, "no interrupt landed with PS.EXCM set");
	TC_PRINT("%u of %u interrupts landed with PS.EXCM set\n", excm_hits, hits);
}

/* An ISR at EXCM level never preempts PS.EXCM code. */
ZTEST(xtensa_high_priority_irq, test_at_excm)
{
	uint32_t bad;

	timer = 1;
	irq_enable(LOW_IRQ);
	bad = sweep();
	irq_disable(LOW_IRQ);

	zassert_equal(bad, 0U, "%u corrupted results", bad);
	zassert_equal(hits, SWEEP_CYCLES - 1U, "%u interrupts", hits);
	zassert_equal(excm_hits, 0U, "%u interrupts landed with PS.EXCM set", excm_hits);
}

static void *setup(void)
{
	chain_fn = chain;
	IRQ_CONNECT(HIGH_IRQ, 0, high_isr, NULL, 0);
	IRQ_CONNECT(LOW_IRQ, 0, low_isr, NULL, 0);
	return NULL;
}

ZTEST_SUITE(xtensa_high_priority_irq, NULL, setup, NULL, NULL, NULL);
