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
 * Tickless operation:
 *   - ISR disables compare (clears ISTAT) and masks the interrupt.
 *   - sys_clock_set_timeout() programs a new compare value and re-enables.
 *   - For K_TICKS_FOREVER + idle, the compare interrupt stays disabled
 *     until sys_clock_idle_exit() or the next set_timeout call.
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

/* This driver uses compare frame 0. Frame 1 remains free for other uses. */
#define TIMER_CMP_FRAME    kSYSCTR_CompareFrame_0
#define TIMER_CMP_INT_MASK kSYSCTR_Compare0InterruptEnable

/* 56-bit counter maximum value */
#define COUNTER_SPAN ((1ULL << 56) - 1)

#define CYC_PER_TICK ((uint64_t)sys_clock_hw_cycles_per_sec() \
		      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/*
 * Limit the maximum programmed compare distance. Two constraints apply:
 *
 *   1) Half of the 56-bit counter span. This prevents ambiguity with the
 *      ">=" comparison when the counter eventually wraps.
 *
 *   2) INT32_MAX * CYC_PER_TICK. The tick delta computed in the ISR
 *      (delta_cycles / CYC_PER_TICK) must fit in uint32_t and, more
 *      importantly, in the int32_t parameter accepted by
 *      sys_clock_announce(). With a 56-bit counter this bound is tighter
 *      than (1) at typical tick rates (e.g. 24 MHz / 10 kHz ticks leaves
 *      ~5e12 cycles, far below 2^55).
 *
 * Use the smaller of the two so both invariants hold.
 */
#define MAX_CYCLES MIN((uint64_t)INT32_MAX * CYC_PER_TICK, COUNTER_SPAN / 2)
#define MAX_TICKS  ((int32_t)(MAX_CYCLES / CYC_PER_TICK))

#define MIN_DELAY_CYCLES  CONFIG_MCUX_SYSCTR_TIMER_MIN_DELAY

static struct k_spinlock lock;
static uint64_t last_cycle;  /* Counter value at last sys_clock_announce() */
static uint64_t last_tick;   /* Tick count at last sys_clock_announce() */
static uint32_t last_elapsed;

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = SYSCTR_IRQN;
#endif

static inline uint64_t counter_read(void)
{
	return SYSCTR_GetCounterlValue(READ_BASE);
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
	last_tick += delta_ticks;
	last_elapsed = 0;

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

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	uint64_t next_cycle;
	uint64_t now;
	uint64_t min;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (idle && (ticks == K_TICKS_FOREVER)) {
		/*
		 * No kernel timeout pending and going to idle — disable the
		 * compare interrupt entirely.  sys_clock_idle_exit() will
		 * re-enable if the CPU wakes from an external source.
		 */
		SYSCTR_EnableCompare(CMP_BASE, TIMER_CMP_FRAME, false);
		SYSCTR_DisableInterrupts(CMP_BASE, TIMER_CMP_INT_MASK);
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	now = counter_read();

	if (ticks == K_TICKS_FOREVER) {
		next_cycle = last_cycle + MAX_CYCLES;
	} else {
		/* Per sys_clock API: treat any other negative value as "fire ASAP". */
		if (ticks < 0) {
			ticks = 0;
		}
		next_cycle = ((uint64_t)(last_tick + last_elapsed + ticks)) * CYC_PER_TICK;
		if ((next_cycle - last_cycle) > MAX_CYCLES) {
			next_cycle = last_cycle + MAX_CYCLES;
		}
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
	 * Re-enable the compare interrupt in case it was disabled for idle
	 * (K_TICKS_FOREVER).  Compare frame stays disabled — the next
	 * sys_clock_set_timeout() call will program a valid compare value
	 * and enable the frame, so no spurious interrupt can occur here.
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

	last_elapsed = delta_ticks;

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
	SYSCTR_SetCounterClockSource(CTRL_BASE, kSYSCTR_BaseFrequency);

	IRQ_CONNECT(SYSCTR_IRQN, SYSCTR_IRQ_PRIO, sysctr_timer_isr, NULL, 0);
	irq_enable(SYSCTR_IRQN);

	SYSCTR_StartCounter(CTRL_BASE);

	/* Align to tick boundary */
	last_tick = counter_read() / CYC_PER_TICK;
	last_cycle = last_tick * CYC_PER_TICK;

	/* Arm the first compare */
	sysctr_set_compare(last_cycle + CYC_PER_TICK);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
