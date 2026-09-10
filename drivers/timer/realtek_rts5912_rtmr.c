/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Lin Yu-Cheng <lin_yu_cheng@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_rtmr

#include <stdint.h>

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>

#include <reg/reg_rtmr.h>
#include <reg/reg_system.h>
#include <soc_clock.h>

#define RTS5912_SCCON_REG_BASE ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Realtek RTOS timer is not supported multiple instances");

#define RTMR_REG ((RTOSTMR_Type *)DT_INST_REG_ADDR(0))

#define SLWTMR_REG ((RTOSTMR_Type *)(DT_REG_ADDR(DT_NODELABEL(slwtmr0))))

#define SSCON_REG ((SYSTEM_Type *)(DT_REG_ADDR(DT_NODELABEL(sccon))))

#define RTMR_COUNTER_MAX   0x0ffffffful
#define RTMR_COUNTER_MSK   0x0ffffffful
#define RTMR_TIMER_STOPPED 0xf0000000ul

/* Adjust cycle count programmed into timer for HW restart latency */
#define RTMR_ADJUST_LIMIT  8
#define RTMR_ADJUST_CYCLES 7

/* Whole reloads consumed so far, the low bits of the synthesized count */
static uint32_t accumulated_cycles;
static uint32_t previous_cnt; /* Record the counter set into RTMR */

#if defined(CONFIG_PM)
static uint64_t cyc_sleep_compensated;
static uint32_t cyc_enter_heavy_sleep;
#endif

static void rtmr_restart(uint32_t counter)
{
	RTMR_REG->CTRL = 0ul;
	RTMR_REG->LDCNT = counter;
	RTMR_REG->CTRL = RTOSTMR_CTRL_INTEN_Msk | RTOSTMR_CTRL_EN_Msk;
}

static uint32_t rtmr_get_counter(void)
{
	uint32_t counter = RTMR_REG->CNT;

	if ((counter == 0) && (RTMR_REG->CTRL & RTOSTMR_CTRL_EN_Msk) &&
	    !(RTMR_REG->INTSTS & RTOSTMR_INTSTS_STS_Msk)) {
		counter = previous_cnt;
	}

	return counter;
}

/*
 * A down-counter reloaded from LDCNT, which interrupts at zero and does not
 * free-run: a RELOAD backend. The counter the core reads is synthesized, the
 * reloads consumed so far plus what the current one has counted down, and it is
 * 28 bits wide like the hardware. That read touches state the ISR and the
 * reload path also write, hence TIMER_CORE_COUNTER_NONATOMIC.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_COUNTER_WIDTH 28
#define TIMER_CORE_COUNTER_NONATOMIC

/* One reload cannot express more than the preload register holds. */
#define TIMER_CORE_ALARM_MAX_CYCLES RTMR_COUNTER_MAX

static inline uint32_t timer_driver_cycle_get(void)
{
	return (accumulated_cycles + (previous_cnt - rtmr_get_counter())) & RTMR_COUNTER_MSK;
}

static void timer_driver_set_reload(uint32_t cycles)
{
	uint32_t cur_cnt = rtmr_get_counter();

	RTMR_REG->CTRL = 0U;

	/* Fold what the reload in flight has already counted down into the
	 * synthesized count before replacing it.
	 */
	accumulated_cycles = (accumulated_cycles + previous_cnt - cur_cnt) & RTMR_COUNTER_MSK;
	previous_cnt = cycles;

	/* adjust for up to one 32KHz cycle startup time */
	if (cycles > RTMR_ADJUST_LIMIT) {
		cycles -= RTMR_ADJUST_CYCLES;
	}

	rtmr_restart(cycles);
}

#include "system_timer_generic.h"

static void rtmr_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	RTMR_REG->INTSTS = RTOSTMR_INTSTS_STS_Msk;

	/* The reload is consumed: account it and restart. A tickless kernel gets
	 * the next deadline from the core right after the announce, so this
	 * reload only has to keep the counter running; a ticked one leaves the
	 * period to the driver.
	 */
	accumulated_cycles = (accumulated_cycles + previous_cnt) & RTMR_COUNTER_MSK;
	previous_cnt = IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? RTMR_COUNTER_MAX : CYCLES_PER_TICK;
	rtmr_restart(previous_cnt);

	timer_core_announce_from(key);
}

void sys_clock_idle_enter(uint32_t ticks)
{
	if (ticks != SYS_CLOCK_IDLE_FOREVER) {
		sys_clock_set_timeout(ticks, false);
		return;
	}

	/* Nothing to wake up for and the uptime accounting may drift: stop the
	 * timer. Only this path may, and only because the CPU is on its way to
	 * sleep, so nothing is left to read the synthesized count that stops
	 * with it. sys_clock_idle_exit() starts it again.
	 */
	RTMR_REG->CTRL = 0U;
	previous_cnt = RTMR_TIMER_STOPPED;
}

void sys_clock_idle_exit(void)
{
	if (previous_cnt == RTMR_TIMER_STOPPED) {
		previous_cnt = CYCLES_PER_TICK;
		rtmr_restart(previous_cnt);
	}
}

void sys_clock_disable(void)
{
	/* Disable RTMR. */
	RTMR_REG->CTRL = 0ul;
}

#if defined(CONFIG_PM)
void rts5912_clock_capture_low_freq_timer(void)
{
	cyc_enter_heavy_sleep = sys_clock_cycle_get_32();
}

void rts5912_clock_compensate_system_timer(void)
{
	uint32_t cyc_exit_heavy_sleep = sys_clock_cycle_get_32();
	uint32_t cyc_elapsed = cyc_exit_heavy_sleep - cyc_enter_heavy_sleep;

	cyc_sleep_compensated += cyc_elapsed;
}

uint64_t rts5912_clock_get_sleep_ticks(void)
{
	return cyc_sleep_compensated / CYCLES_PER_TICK;
}
#endif /* CONFIG_PM */

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT

void arch_busy_wait(uint32_t n_usec)
{
	if (n_usec == 0) {
		return;
	}

	uint32_t start = SLWTMR_REG->CNT;

	for (;;) {
		uint32_t curr = SLWTMR_REG->CNT;

		if ((start - curr) >= n_usec) {
			break;
		}
	}
}
#endif

static int sys_clock_driver_init(void)
{
	/* Enable RTMR clock power */
	RTMR_REG->INTSTS = RTOSTMR_INTSTS_STS_Msk;
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));

	SYSTEM_Type *sys_reg = RTS5912_SCCON_REG_BASE;

	sys_reg->PERICLKPWR1 |= SYSTEM_PERICLKPWR1_RTMRCLKPWR_Msk;

	/* Enable RTMR interrupt. */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtmr_isr, 0, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Trigger RTMR and wait it start to counting */
	previous_cnt = RTMR_COUNTER_MAX;

	rtmr_restart(previous_cnt);
	while (RTMR_REG->CNT == 0) {
	};

	timer_core_init();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* The core leaves the period of a reload backend to the driver,
		 * so program the periodic tick here. The ISR reloads it.
		 */
		timer_driver_set_reload(CYCLES_PER_TICK);
	}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
	/* Enable SLWTMR0 clock power */
	SSCON_REG->PERICLKPWR1 |= BIT(SYSTEM_PERICLKPWR1_SLWTMR0CLKPWR_Pos);

	/* Enable SLWTMR0 */
	SLWTMR_REG->LDCNT = UINT32_MAX;
	SLWTMR_REG->CTRL = RTOSTMR_CTRL_MDSEL_Msk | RTOSTMR_CTRL_EN_Msk;
#endif

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
