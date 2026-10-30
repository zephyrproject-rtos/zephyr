/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* A level-1 software interrupt arms a CCOMPARE timer of a higher level
 * to fire a given number of cycles later. Sweeping that delay lands the
 * nested interrupt on every instruction of the level-1 interrupt exit
 * path, while the interrupted thread holds live caller frames in the
 * register file.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>
#include <xtensa/config/core-isa.h>

#define LOW_IRQ_MASK (XCHAL_INTTYPE_MASK_SOFTWARE & XCHAL_INTLEVEL1_MASK)
#define LOW_IRQ      __builtin_ctz(LOW_IRQ_MASK)

#define HIGH_LEVEL_MASK                                                                            \
	(UTIL_CAT(XCHAL_INTLEVEL, UTIL_CAT(XCHAL_EXCM_LEVEL, _ANDBELOW_MASK)) &                    \
	 ~XCHAL_INTLEVEL1_MASK)

#ifdef CONFIG_XTENSA_TIMER
#define SYS_TIMER_ID CONFIG_XTENSA_TIMER_ID
#else
#define SYS_TIMER_ID -1
#endif

#if XCHAL_NUM_TIMERS > 1 && SYS_TIMER_ID != 1 &&                                                   \
	(HIGH_LEVEL_MASK & (1 << XCHAL_TIMER1_INTERRUPT)) != 0
#define HIGH_TIMER 1
#elif XCHAL_NUM_TIMERS > 2 && SYS_TIMER_ID != 2 &&                                                 \
	(HIGH_LEVEL_MASK & (1 << XCHAL_TIMER2_INTERRUPT)) != 0
#define HIGH_TIMER 2
#elif XCHAL_NUM_TIMERS > 0 && SYS_TIMER_ID != 0 &&                                                 \
	(HIGH_LEVEL_MASK & (1 << XCHAL_TIMER0_INTERRUPT)) != 0
#define HIGH_TIMER 0
#else
#error "No CCOMPARE timer between level 2 and XCHAL_EXCM_LEVEL is available"
#endif

#define HIGH_IRQ UTIL_CAT(UTIL_CAT(XCHAL_TIMER, HIGH_TIMER), _INTERRUPT)

BUILD_ASSERT(XCHAL_HAVE_CCOUNT, "CCOUNT is required");
BUILD_ASSERT(LOW_IRQ_MASK != 0, "A level-1 software interrupt is required");

#define CHAIN_DEPTH  4
#define SPILL_DEPTH  16
#define SWEEP_MARGIN 16
#define PHASES       4
#define WAIT_LIMIT   1000000U

static volatile uint32_t sweep_delay;
static volatile uint32_t sweep_phase;
static volatile uint32_t isr_ccount;
static volatile uint32_t high_hits;
static volatile uint32_t late;
static volatile uint32_t misses;
static volatile uint32_t corrupt;
static volatile uint32_t seed;
static uint32_t expected[CHAIN_DEPTH + 1];

static inline uint32_t read_ccount(void)
{
	uint32_t val;

	__asm__ volatile("rsr.ccount %0" : "=r"(val));
	return val;
}

static inline void ccompare_write(uint32_t val)
{
	__asm__ volatile("wsr.ccompare" STRINGIFY(HIGH_TIMER) " %0; esync" : : "r"(val));
}

static uint32_t spill_frames(uint32_t n);
static uint32_t (*volatile spill_fn)(uint32_t n) = spill_frames;

static __noinline uint32_t spill_frames(uint32_t n)
{
	volatile uint32_t pad = n;

	if (n == 0U) {
		return pad;
	}

	return spill_fn(n - 1U) + pad;
}

static void low_isr(const void *arg)
{
	uint32_t now;
	uint32_t hits;
	uint32_t target;

	ARG_UNUSED(arg);

	now = read_ccount();
	if (sweep_delay != 0U) {
		hits = high_hits;
		target = now + sweep_delay;
		ccompare_write(target);
		if ((int32_t)(read_ccount() - target) >= 0 && high_hits == hits) {
			ccompare_write(read_ccount() - 1U);
			late++;
		}
	}

	if ((sweep_phase & BIT(0)) != 0U) {
		__asm__ volatile("nop");
	}
	if ((sweep_phase & BIT(1)) != 0U) {
		__asm__ volatile("nop; nop");
	}

	isr_ccount = now;
	__asm__ volatile("wsr.intclear %0; rsync" : : "r"(BIT(LOW_IRQ)));
}

static void high_isr(const void *arg)
{
	ARG_UNUSED(arg);

	ccompare_write(read_ccount() - 1U);
	(void)spill_fn(SPILL_DEPTH);
	high_hits++;
}

static void connect_irqs(void)
{
	IRQ_CONNECT(LOW_IRQ, 0, low_isr, NULL, 0);
	IRQ_CONNECT(HIGH_IRQ, 0, high_isr, NULL, 0);
	irq_enable(LOW_IRQ);
	irq_enable(HIGH_IRQ);
}

static __noinline uint32_t trigger(void)
{
	uint32_t start = high_hits;
	uint32_t late_start = late;
	uint32_t guard = WAIT_LIMIT;
	uint32_t resumed;

	z_xt_set_intset(BIT(LOW_IRQ));
	resumed = read_ccount();

	if (sweep_delay == 0U) {
		return resumed - isr_ccount;
	}

	while (high_hits == start && late == late_start) {
		guard--;
		if (guard == 0U) {
			ccompare_write(read_ccount() - 1U);
			misses++;
			break;
		}
	}

	return 0U;
}

static uint32_t chain(uint32_t depth);
static uint32_t (*volatile chain_fn)(uint32_t depth) = chain;

static __noinline uint32_t chain(uint32_t depth)
{
	uint32_t token = seed ^ depth;
	uint32_t ret;

	expected[depth] = token;

	if (depth == 0U) {
		ret = trigger();
	} else {
		ret = chain_fn(depth - 1U);
	}

	if (token != expected[depth]) {
		corrupt++;
	}

	return ret;
}

ZTEST(xtensa_nested_irq_exit, test_nested_irq_on_exit_path)
{
	uint32_t exit_len;
	uint32_t runs = 0U;

	connect_irqs();

	sweep_delay = 0U;
	sweep_phase = 0U;
	exit_len = chain_fn(CHAIN_DEPTH);
	zassert_true(exit_len > 0U, "level-1 interrupt did not run");

	TC_PRINT("low irq %d, high irq %d (CCOMPARE%d), exit path %u cycles\n", LOW_IRQ, HIGH_IRQ,
		 HIGH_TIMER, exit_len);

	for (uint32_t phase = 0U; phase < PHASES; phase++) {
		sweep_phase = phase;
		for (uint32_t delay = 1U; delay <= exit_len + SWEEP_MARGIN; delay++) {
			sweep_delay = delay;
			seed = (phase << 16) | delay;
			(void)chain_fn(CHAIN_DEPTH);
			runs++;
		}
	}

	TC_PRINT("%u runs, %u high interrupts, %u armed late, %u misses\n", runs, high_hits, late,
		 misses);

	zassert_equal(corrupt, 0U, "caller frames corrupted in %u runs", corrupt);
	zassert_equal(misses, 0U, "timer missed in %u runs", misses);
	zassert_true(high_hits > late, "timer armed too late in %u runs", late);
}

ZTEST_SUITE(xtensa_nested_irq_exit, NULL, NULL, NULL, NULL, NULL);
