/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/drivers/interrupt_controller/dw_ace.h>

#include <cavs-idc.h>
#include <adsp_shim.h>
#include <adsp_interrupt.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT intel_adsp_timer

/**
 * @file
 * @brief Intel Audio DSP Wall Clock Timer driver
 *
 * The Audio DSP on Intel SoC has a timer with one counter and two compare
 * registers that is external to the CPUs. This timer is accessible from
 * all available CPU cores and provides a synchronized timer under SMP.
 */

#define COMPARATOR_IDX  0 /* 0 or 1 */

#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
#define TIMER_IRQ ACE_IRQ_TO_ZEPHYR(ACE_INTL_TTS)
#else
#define TIMER_IRQ DSP_WCT_IRQ(COMPARATOR_IDX)
#endif

#define MIN_DELAY	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 100000)

BUILD_ASSERT(COMPARATOR_IDX >= 0 && COMPARATOR_IDX <= 1);

#define DSP_WCT_CS_TT(x)                     BIT(4 + x)

/* Not using current syscon driver due to overhead due to MMU support */
#define SYSCON_REG_ADDR	DT_REG_ADDR(DT_INST_PHANDLE(0, syscon))

#define DSPWCTCS_ADDR (SYSCON_REG_ADDR + ADSP_DSPWCTCS_OFFSET)
#define DSPWCT0C_LO_ADDR (SYSCON_REG_ADDR + ADSP_DSPWCT0C_OFFSET)
#define DSPWCT0C_HI_ADDR (SYSCON_REG_ADDR + ADSP_DSPWCT0C_OFFSET + 4)
#define DSPWC_LO_ADDR (SYSCON_REG_ADDR + ADSP_DSPWC_OFFSET)
#define DSPWC_HI_ADDR (SYSCON_REG_ADDR + ADSP_DSPWC_OFFSET + 4)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ; /* See tests/kernel/context */
#endif

static void set_compare(uint64_t time)
{
	/* Disarm the comparator to prevent spurious triggers */
	sys_write32(sys_read32(DSPWCTCS_ADDR) & (~DSP_WCT_CS_TA(COMPARATOR_IDX)),
			SYSCON_REG_ADDR + ADSP_DSPWCTCS_OFFSET);

	sys_write32((uint32_t)time, DSPWCT0C_LO_ADDR);
	sys_write32((uint32_t)(time >> 32), DSPWCT0C_HI_ADDR);

	/* Arm the timer */
	sys_write32(sys_read32(DSPWCTCS_ADDR) | (DSP_WCT_CS_TA(COMPARATOR_IDX)),
			DSPWCTCS_ADDR);
}

static uint64_t count(void)
{
	/* The count register is 64 bits, but we're a 32 bit CPU that
	 * can only read four bytes at a time, so a bit of care is
	 * needed to prevent racing against a wraparound of the low
	 * word.  Wrap the low read between two reads of the high word
	 * and make sure it didn't change.
	 */
	uint32_t hi0, hi1, lo;

	do {
		hi0 = sys_read32(DSPWC_HI_ADDR);
		lo = sys_read32(DSPWC_LO_ADDR);
		hi1 = sys_read32(DSPWC_HI_ADDR);
	} while (hi0 != hi1);
	return (((uint64_t)hi0) << 32) | lo;
}

static uint32_t count32(void)
{
	uint32_t counter_lo;

	counter_lo = sys_read32(DSPWC_LO_ADDR);
	return counter_lo;
}

/*
 * Free-running 64-bit wall clock plus an absolute comparator: a COMPARE
 * backend. The core owns the tick accounting; the driver reads the counter and
 * arms the comparator. Arming disarms and rearms the comparator, so
 * timer_driver_set_compare() keeps a target that latency left too close at least
 * MIN_DELAY ahead, so the sequence cannot race the counter past it.
 */
#define TIMER_CORE_BACKEND_COMPARE
#define TIMER_CORE_64BIT_CYCLES
#define TIMER_CORE_HAVE_CYCLE_GET_32
/* The wall clock is a genuine 64-bit counter on a 32-bit CPU: use its full
 * range (the native register width would clamp deltas to 32 bits).
 */
#define TIMER_CORE_CYCLES_WIDTH 64

static inline uint64_t timer_driver_cycle_get(void)
{
	return count();
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	uint64_t curr = count();

	if ((int64_t)(cycles - curr) < MIN_DELAY) {
		cycles = curr + MIN_DELAY;
	}
	set_compare(cycles);
}

#include "system_timer_generic.h"

uint32_t sys_clock_cycle_get_32(void)
{
	return count32();
}

static void compare_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	/* Clear the triggered bit */
	sys_write32(sys_read32(DSPWCTCS_ADDR) | DSP_WCT_CS_TT(COMPARATOR_IDX),
			DSPWCTCS_ADDR);

	timer_core_announce_from(key);
}

/* Interrupt setup is partially-cpu-local state, so needs to be
 * repeated for each core when it starts.  Note that this conforms to
 * the Zephyr convention of sending timer interrupts to all cpus (for
 * the benefit of timeslicing).
 */
static void irq_init(void)
{
	int cpu = arch_curr_cpu()->id;

	/* These platforms have an extra layer of interrupt masking
	 * (for per-core control) above the interrupt controller.
	 * Drivers need to do that part.
	 */
#ifdef CONFIG_SOC_SERIES_INTEL_ADSP_ACE
	ACE_DINT[cpu].ie[ACE_INTL_TTS] |= BIT(COMPARATOR_IDX + 1);
	sys_write32(sys_read32(DSPWCTCS_ADDR) | ADSP_SHIM_DSPWCTCS_TTIE(COMPARATOR_IDX),
			DSPWCTCS_ADDR);
#else
	CAVS_INTCTRL[cpu].l2.clear = CAVS_L2_DWCT0;
#endif
	irq_enable(TIMER_IRQ);
}

void smp_timer_init(void)
{
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, 0, compare_isr, 0, 0);

	/* Seed the announce baseline from the wall clock and arm the first tick. */
	timer_core_init();
	irq_init();
	return 0;
}

/* Runs on core 0 only */
void intel_adsp_clock_soft_off_exit(void)
{
	(void)sys_clock_driver_init();
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
