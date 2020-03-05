/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <spinlock.h>
#include <arch/xtensa/xtensa_rtos.h>

/**
 * @file
 * @brief CAVS DSP Wall Clock Timer driver
 *
 * The CAVS DSP on Intel SoC has a timer with one counter and two compare
 * registers that is external to the CPUs. This timer is accessible from
 * all available CPU cores and provides a synchronized timer under SMP.
 */

#define TIMER		0
#define TIMER_IRQ	DSP_WCT_IRQ(TIMER)
#define CYC_PER_TICK	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC	\
			/ CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define MAX_CYC		0xFFFFFFFFUL
#define MAX_TICKS	((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 100000)

BUILD_ASSERT(MIN_DELAY < CYC_PER_TICK);

static struct k_spinlock lock;
static u64_t last_count;

static volatile struct soc_dsp_shim_regs *shim_regs =
	(volatile struct soc_dsp_shim_regs *)SOC_DSP_SHIM_REG_BASE;

static void set_compare(u64_t time)
{
#if (TIMER == 0)
	/* Set compare register */
	shim_regs->dspwct0c = time;
#elif (TIMER == 1)
	/* Set compare register */
	shim_regs->dspwct1c = time;
#else
#error "TIMER has to be 0 or 1!"
#endif

	/* Arm the timer */
	shim_regs->dspwctcs |= DSP_WCT_CS_TA(TIMER);
}

static u64_t count(void)
{
	return shim_regs->walclk;
}

static u32_t count32(void)
{
	return shim_regs->walclk32_lo;
}

static void compare_isr(void *arg)
{
	ARG_UNUSED(arg);
	u64_t curr;
	u32_t dticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	curr = count();

#ifdef CONFIG_SMP
	/* If it has been too long since last_count,
	 * this interrupt is likely the same interrupt
	 * event but being processed by another CPU.
	 * Since it has already been processed and
	 * ticks announced, skip it.
	 */
	if ((count32() - (u32_t)last_count) < MIN_DELAY) {
		k_spin_unlock(&lock, key);
		return;
	}
#endif

	dticks = (u32_t)((curr - last_count) / CYC_PER_TICK);

	/* Clear the triggered bit */
	shim_regs->dspwctcs |= DSP_WCT_CS_TT(TIMER);

	last_count += dticks * CYC_PER_TICK;

#ifndef CONFIG_TICKLESS_KERNEL
	u64_t next = last_count + CYC_PER_TICK;

	if ((s64_t)(next - curr) < MIN_DELAY) {
		next += CYC_PER_TICK;
	}
	set_compare(next);
#endif

	k_spin_unlock(&lock, key);

	z_clock_announce(dticks);
}

int z_clock_driver_init(struct device *device)
{
	u64_t curr = count();

	IRQ_CONNECT(TIMER_IRQ, 0, compare_isr, 0, 0);
	set_compare(curr + CYC_PER_TICK);
	last_count = curr;
	irq_enable(TIMER_IRQ);
	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#ifdef CONFIG_TICKLESS_KERNEL
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = MAX(MIN(ticks - 1, (s32_t)MAX_TICKS), 0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	u64_t curr = count();
	u64_t next;
	u32_t adj, cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary */
	adj = (u32_t)(curr - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;
	next = last_count + cyc;

	if (((u32_t)next - (u32_t)curr) < MIN_DELAY) {
		next += CYC_PER_TICK;
	}

	set_compare(next);
	k_spin_unlock(&lock, key);
#endif
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}
	k_spinlock_key_t key = k_spin_lock(&lock);
	u32_t ret = (count32() - (u32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

u32_t z_timer_cycle_get_32(void)
{
	return count32();
}

#if defined(CONFIG_SMP) && CONFIG_MP_NUM_CPUS > 1
void smp_timer_init(void)
{
	/* This enables the Timer 0 (or 1) interrupt for CPU n.
	 *
	 * FIXME: Done in this way because we don't have an API
	 * to enable interrupts per CPU.
	 */
	sys_set_bit(DT_CAVS_ICTL_BASE_ADDR
			+ CAVS_ICTL_INT_CPU_OFFSET(arch_curr_cpu()->id)
			+ 0x04,
		    22 + TIMER);
	irq_enable(XTENSA_IRQ_NUMBER(TIMER_IRQ));
}
#endif
