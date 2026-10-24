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
#include <zephyr/sys/clock.h>
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

/*
 * A free-running 56-bit counter and an absolute compare that fires once the
 * counter reaches or has passed the value, so a COMPARE_ORDERED backend. The
 * declared width bounds the arm range to half the counter span, which keeps
 * the comparison unambiguous when the counter wraps.
 */
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#define TIMER_CORE_COUNTER_WIDTH 56

static inline uint64_t timer_driver_cycle_get(void)
{
	return counter_read();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	sysctr_set_compare(cycles);
}

#include "system_timer_generic.h"

static void sysctr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Disable compare to clear ISTAT and prevent re-trigger. The core arms
	 * the next deadline, which re-enables it.
	 */
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
	SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);

	timer_core_announce();
}

void sys_clock_no_timeout(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* Nothing pending: mask the compare interrupt. The counter keeps
	 * running, so the cycle getters go on working, and the next
	 * sys_clock_set_timeout() re-enables both the interrupt and the
	 * compare.
	 */
	SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
	SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
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

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
