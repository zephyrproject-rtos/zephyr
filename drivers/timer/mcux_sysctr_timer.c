/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP System Counter (SYS_CTR) based system timer driver.
 *
 * SYS_CTR provides a 56-bit free-running counter with two compare frames,
 * each capable of generating a maskable interrupt. This driver uses compare
 * frame 0 for the system timer.
 *
 * Shared-resource ownership:
 *   SYS_CTR is a shared time base for multiple processors. This driver
 *   assumes the Zephyr core exclusively owns SYS_CTR initialization:
 *   sys_clock_driver_init() calls SYSCTR_Init(), which resets the shared
 *   counter value to 0 and clears BOTH compare frames (including frame 1).
 *   Do not enable this driver on a core if another core or an earlier boot
 *   stage already relies on the same SYS_CTR instance (its counter value or
 *   compare frame 1) — that state would be wiped at init.
 *
 * Tickless operation:
 *   - ISR disables compare (clears ISTAT) and masks the interrupt.
 *   - sys_clock_set_timeout() programs a new compare value and re-enables.
 *   - Under sloppy idle with no near deadline, the compare interrupt
 *     stays disabled until sys_clock_idle_exit() or the next set_timeout
 *     call.
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>

#include <fsl_sysctr.h>

#define SYSCTR_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,sysctr node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), nxp_sysctr),
	     "zephyr,system-timer must point to an nxp,sysctr compatible node");
BUILD_ASSERT((CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC % CONFIG_SYS_CLOCK_TICKS_PER_SEC) == 0,
	     "SYS_CTR: CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC must be an integer "
	     "multiple of CONFIG_SYS_CLOCK_TICKS_PER_SEC to avoid timer drift");

/* Hardware register bases from devicetree */
#define CTRL_BASE ((SYS_CTR_CONTROL_Type *)DT_REG_ADDR_BY_NAME(SYSCTR_NODE, control))
#define READ_BASE ((SYS_CTR_READ_Type *)DT_REG_ADDR_BY_NAME(SYSCTR_NODE, read))
#define CMP_BASE  ((SYS_CTR_COMPARE_Type *)DT_REG_ADDR_BY_NAME(SYSCTR_NODE, compare))

#define SYSCTR_IRQN     DT_IRQN(SYSCTR_NODE)
#define SYSCTR_IRQ_PRIO DT_IRQ(SYSCTR_NODE, priority)

/* This driver uses compare frame 0 at runtime and does not touch frame 1. */
#define TIMER_CMP_FRAME    kSYSCTR_CompareFrame_0
#define TIMER_CMP_INT_MASK kSYSCTR_Compare0InterruptEnable

/* 56-bit counter maximum value */
#define COUNTER_SPAN ((1ULL << 56) - 1)

#define CYC_PER_TICK ((uint64_t)sys_clock_hw_cycles_per_sec() \
		      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Limit the maximum programmed compare distance to half the 56-bit counter
 * span. This keeps the ">=" comparison unambiguous when the counter
 * eventually wraps.
 */
#define MAX_CYCLES (COUNTER_SPAN / 2)

#define MIN_DELAY_CYCLES  CONFIG_MCUX_SYSCTR_TIMER_MIN_DELAY

/*
 * The minimum compare setup distance must stay below one tick. Otherwise a
 * normal single-tick timeout would be pushed out to "now + MIN_DELAY_CYCLES"
 * in sys_clock_set_timeout(), losing ticks and introducing systematic drift.
 */
BUILD_ASSERT(MIN_DELAY_CYCLES <
	     (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC),
	     "CONFIG_MCUX_SYSCTR_TIMER_MIN_DELAY must be smaller than one tick "
	     "(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)");

static struct k_spinlock lock;
static uint64_t last_cycle;  /* Counter value at last sys_clock_announce() */

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = SYSCTR_IRQN;
#endif

static inline uint64_t counter_read(void)
{
	uint32_t hi1, hi2, lo;

	/*
	 * The 56-bit count value is read from two 32-bit registers, so a
	 * combined read is not atomic: if the low word rolls over between
	 * the two halves the result can be off by ~2^32. Read the high word,
	 * the low word, then the high word again and retry while the high
	 * word changes, discarding any sample taken across a rollover.
	 */
	do {
		hi1 = READ_BASE->CNTCV1;
		lo = READ_BASE->CNTCV0;
		hi2 = READ_BASE->CNTCV1;
	} while (hi1 != hi2);

	return ((uint64_t)hi2 << 32) | lo;
}

static void sysctr_set_compare(uint64_t val)
{
	/*
	 * Disable compare first to clear ISTAT and prevent a race between
	 * the two 32-bit writes that make up the 64-bit compare value.
	 * Then program the new value and re-enable.
	 */
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
	SYSCTR_SetCompareValue(CTRL_BASE, CMP_BASE, TIMER_CMP_FRAME, val);
	SYSCTR_EnableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, true);
}

static void sysctr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t curr_cycle = counter_read();
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (uint32_t)(delta_cycles / CYC_PER_TICK);

	last_cycle += (uint64_t)delta_ticks * CYC_PER_TICK;

	/* Disable compare to clear ISTAT and prevent re-trigger */
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Non-tickless: schedule the next tick immediately */
		uint64_t next = last_cycle + CYC_PER_TICK;

		sysctr_set_compare(next);
	} else {
		/* Tickless: mask interrupt until next sys_clock_set_timeout() */
		SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
	}

	k_spin_unlock(&lock, key);

	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	uint64_t next_cycle;
	uint64_t now;
	uint64_t min;

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) && ticks == SYS_CLOCK_MAX_WAIT) {
		/*
		 * No near deadline to schedule: under sloppy idle, disable the
		 * compare interrupt entirely. sys_clock_idle_exit() will
		 * re-enable it when the CPU wakes from an external source.
		 */
		SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
		SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	now = counter_read();

	/*
	 * Compute elapsed ticks from the most recent announce locally
	 * instead of relying on a cached value from sys_clock_elapsed(),
	 * which may be stale (or never have been called) by the time we
	 * arrive here.
	 */
	uint64_t elapsed_ticks = (now - last_cycle) / CYC_PER_TICK;

	next_cycle = last_cycle + (elapsed_ticks + (uint64_t)ticks) * CYC_PER_TICK;
	if ((next_cycle - last_cycle) > MAX_CYCLES) {
		next_cycle = last_cycle + MAX_CYCLES;
	}

	min = now + MIN_DELAY_CYCLES;

	if ((int64_t)(next_cycle - min) < 0) {
		next_cycle = min;
	}

	sysctr_set_compare(next_cycle);

	k_spin_unlock(&lock, key);
}

void sys_clock_idle_exit(void)
{
	/*
	 * The compare interrupt may still be masked after PM idle. Re-enable it
	 * here, otherwise a pending compare fires late and the periodic timer
	 * jitters (one tick long, next one short). No tick is lost, and we can't
	 * trigger spuriously: ISTAT needs the counter to reach an enabled compare.
	 */
	SYSCTR_EnableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t curr_cycle = counter_read();
	uint64_t delta_cycles = curr_cycle - last_cycle;
	uint32_t delta_ticks = (uint32_t)(delta_cycles / CYC_PER_TICK);

	k_spin_unlock(&lock, key);

	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)counter_read();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return counter_read();
}

void sys_clock_disable(void)
{
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
	SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
	irq_disable(SYSCTR_IRQN);
}

static int sys_clock_driver_init(void)
{
	sysctr_config_t cfg;

	SYSCTR_GetDefaultConfig(&cfg);
	SYSCTR_Init(CTRL_BASE, CMP_BASE, &cfg);
	/*
	 * The system timer always runs on the base frequency: compare values
	 * are only valid on the base clock, so the optional clock-source
	 * devicetree property (alternate/low-power clock) is intentionally not
	 * honored here.
	 */
	SYSCTR_SetCounterClockSource(CTRL_BASE, kSYSCTR_BaseFrequency);

	IRQ_CONNECT(SYSCTR_IRQN, SYSCTR_IRQ_PRIO, sysctr_timer_isr, NULL, 0);
	irq_enable(SYSCTR_IRQN);

	SYSCTR_StartCounter(CTRL_BASE);

	/* Align to tick boundary */
	last_cycle = (counter_read() / CYC_PER_TICK) * CYC_PER_TICK;

	/* Arm the first compare */
	sysctr_set_compare(last_cycle + CYC_PER_TICK);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
