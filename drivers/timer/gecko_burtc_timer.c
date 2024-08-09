/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * Based on:
 *  sam0_rtc_timer.c Copyright (c) 2018 omSquare s.r.o.
 *  intel_adsp_timer.c Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_burtc

/**
 * @file
 * @brief SiLabs Gecko BURTC-based sys_clock driver
 *
 */

#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/logging/log.h>

#include "em_device.h"
#include "em_cmu.h"
#include "em_burtc.h"


LOG_MODULE_REGISTER(gecko_burtc_timer);


/* Maximum time interval between timer interrupts (in hw_cycles) */
#define MAX_TIMEOUT_CYC (UINT32_MAX >> 1)

/*
 * Mininum time interval between now and IRQ firing that can be scheduled.
 * The main cause for this is LFSYNC register update, which requires several
 * LF clk cycles for synchronization.
 * Seee e.g. "4.2.4.4.4 LFSYNC Registers" in "EFR32xG22 Reference Manual"
 */
#define MIN_DELAY_CYC (6u)

#define TIMER_IRQ (DT_INST_IRQN(0))

#if defined(CONFIG_TEST)
/* See tests/kernel/context */
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ;
#endif

/* With CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME, that's where we
 * should write hw_cycles timer clock frequency upon init
 */
extern int z_clock_hw_cycles_per_sec;

/* Number of hw_cycles clocks per 1 kernel tick */
static uint32_t g_cyc_per_tick;

/* MAX_TIMEOUT_CYC expressed as ticks */
static uint32_t g_max_timeout_ticks;

/* Value of BURTC counter when the previous kernel tick was announced */
static atomic_t g_last_count;

/* Spinlock to sync between Compare ISR and update of Compare register */
static struct k_spinlock g_lock;

/* Set to true when timer is initialized */
static bool g_init;

static void burtc_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Clear the interrupt */
	BURTC_IntClear(BURTC_IF_COMP);

	uint32_t curr = BURTC_CounterGet();

	/* NOTE: this is the only place where g_last_count is modified,
	 * so we don't need to do make the whole read-and-modify atomic, just
	 * writing it behind the memory barrier is enough
	 */
	uint32_t prev = atomic_get(&g_last_count);

	/* How many ticks have we not announced since the last announcement */
	uint32_t unannounced = (curr - prev) / g_cyc_per_tick;

	atomic_set(&g_last_count, prev + unannounced * g_cyc_per_tick);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Counter value on which announcement should be made */
		uint32_t next = prev + g_cyc_per_tick;

		/* `next` can be too close in the future since we're trying to
		 * announce the very next tick - in that case we skip one and
		 * announce the one after it instead
		 */
		if ((next - curr) < MIN_DELAY_CYC) {
			next += g_cyc_per_tick;
		}

		BURTC_CompareSet(0, next);
	}

	sys_clock_announce(unannounced);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/*
	 * calculate 'ticks' value that specifies which tick to announce,
	 * beginning from the closest upcoming one:
	 * 0 - announce upcoming tick itself
	 * 1 - skip upcoming one, but announce the one after it, etc.
	 */
	ticks = (ticks == K_TICKS_FOREVER) ? g_max_timeout_ticks : ticks;
	ticks = CLAMP(ticks - 1, 0, g_max_timeout_ticks);

	k_spinlock_key_t key = k_spin_lock(&g_lock);

	uint32_t curr = BURTC_CounterGet();
	uint32_t prev = atomic_get(&g_last_count);

	/* How many ticks have we not announced since the last announcement */
	uint32_t unannounced = (curr - prev) / g_cyc_per_tick;

	/* Which tick to announce (counting from the last announced one) */
	uint32_t to_announce = unannounced + ticks + 1;

	/* Force maximum interval between announcements. If we sit without
	 * announcements for too long, counter will roll over and we'll lose
	 * track of unannounced ticks.
	 */
	to_announce = MIN(to_announce, g_max_timeout_ticks);

	/* Counter value on which announcement should be made */
	uint32_t next = prev + to_announce * g_cyc_per_tick;

	/* `next` can be too close in the future if we're trying to announce
	 * the very next tick - in that case we skip one and announce the one
	 * after it instead
	 */
	if ((next - curr) < MIN_DELAY_CYC) {
		next += g_cyc_per_tick;
	}

	BURTC_CompareSet(0, next);
	k_spin_unlock(&g_lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || !g_init) {
		return 0;
	} else {
		return (BURTC_CounterGet() - g_last_count) / g_cyc_per_tick;
	}
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* API note: this function is unrelated to kernel ticks, it returns
	 * a value of some 32-bit hw_cycles counter which counts with
	 * z_clock_hw_cycles_per_sec frequency
	 */
	if (!g_init) {
		return 0;
	} else {
		return BURTC_CounterGet();
	}
}

int init_sys_clock_driver(void)
{
	uint32_t hw_clock_freq;
	BURTC_Init_TypeDef init = BURTC_INIT_DEFAULT;

	/* Enable clock for BURTC CSRs on APB */
	CMU_ClockEnable(cmuClock_BURTC, true);

	/* Configure BURTC LF clocksource according to Kconfig */
#if defined(CONFIG_CMU_BURTCCLK_LFXO)
	CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_LFXO);
#elif defined(CONFIG_CMU_BURTCCLK_LFRCO)
	CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_LFRCO);
#elif defined(CONFIG_CMU_BURTCCLK_ULFRCO)
	CMU_ClockSelectSet(cmuClock_BURTC, cmuSelect_ULFRCO);
#else
#error "Unsupported BURTC clock specified"
#endif

	/* Calculate timing constants and init BURTC */
	hw_clock_freq = CMU_ClockFreqGet(cmuClock_BURTC);
	z_clock_hw_cycles_per_sec = hw_clock_freq;

	BUILD_ASSERT(CONFIG_SYS_CLOCK_TICKS_PER_SEC > 0,
			"Invalid CONFIG_SYS_CLOCK_TICKS_PER_SEC value");
	g_cyc_per_tick = hw_clock_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	__ASSERT(g_cyc_per_tick >= MIN_DELAY_CYC,
		"%u cycle-long tick is too short to be scheduled "
		"(min is %u). Config: SYS_CLOCK_TICKS_PER_SEC is "
		"%d and timer frequency is %u",
		g_cyc_per_tick, MIN_DELAY_CYC, CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		hw_clock_freq);

	g_max_timeout_ticks = MAX_TIMEOUT_CYC / g_cyc_per_tick;

	init.clkDiv = 1;
	init.start = false;
	BURTC_Init(&init);
	g_init = true;

	/* Enable compare match interrupt */
	BURTC_IntClear(BURTC_IF_COMP);
	BURTC_IntEnable(BURTC_IF_COMP);
	NVIC_ClearPendingIRQ(TIMER_IRQ);
	IRQ_CONNECT(TIMER_IRQ, DT_INST_IRQ(0, priority), burtc_isr, 0, 0);
	irq_enable(TIMER_IRQ);

	/* Start the timer and announce 1 kernel tick */
	atomic_set(&g_last_count, 0);
	BURTC_CompareSet(0, g_cyc_per_tick);

	BURTC_SyncWait();
	BURTC->CNT = 0;
	BURTC_Start();

	return 0;
}
