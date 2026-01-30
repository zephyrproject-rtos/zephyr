/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "zephyr/drivers/interrupt_controller/intc_aurix_ir.h"
#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>

#if CONFIG_SOC_SERIES_TC3X
#include <IfxStm_reg.h>
#elif CONFIG_SOC_SERIES_TC4X
#include <IfxCpu_reg.h>
#endif
#include <IfxCbs_reg.h>
#include <soc.h>

/* TC3x/TC4x adaption defines */
#if CONFIG_SOC_SERIES_TC3X
#define STM_IRQ_IDX 0
#elif CONFIG_SOC_SERIES_TC4X
#define ICR           STM_ICR
#define Ifx_STM_CMCON Ifx_CPU_STM_CMCON
#define Ifx_STM       Ifx_CPU_STM
#define STM_IRQ_IDX   COND_CODE_1(CONFIG_TRICORE_VIRTUALIZATION, (CONFIG_TRICORE_VM_ID), (1))
#endif
#define STM_IRQ DT_IRQN_BY_IDX(DT_NODELABEL(stm), STM_IRQ_IDX)

BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_SERIES_TC3X) || IS_ENABLED(CONFIG_SOC_SERIES_TC4X),
	     "aurix_stm requires CONFIG_SOC_SERIES_TC3X or CONFIG_SOC_SERIES_TC4X");

/* STM registers */
static Ifx_STM *const STM_TIMER = (Ifx_STM *)DT_REG_ADDR(DT_NODELABEL(stm));
/* Test value */
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = STM_IRQ;
#endif

/* Timer values */
#define TICKS_PER_SEC   CONFIG_SYS_CLOCK_TICKS_PER_SEC
#define CYCLES_PER_SEC  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define CYCLES_PER_TICK (CYCLES_PER_SEC / TICKS_PER_SEC)
/* 64-bit dcycles: a missed-IRQ window > 2^32 cycles must not silently
 * truncate. CYCLE_DIFF_MAX stays at 32 bits to keep the CMP0 set-ahead
 * cap (CYCLES_MAX) inside the STM compare register.
 */
#define cycle_diff_t    uint64_t
#define CYCLE_DIFF_MAX  ((cycle_diff_t)UINT32_MAX)

/*
 * We have two constraints on the maximum number of cycles we can wait for.
 *
 * 1) sys_clock_announce() accepts at most INT32_MAX ticks.
 *
 * 2) The number of cycles between two reports must fit in a cycle_diff_t
 *    variable before converting it to ticks.
 *
 * Then:
 *
 * 3) Pick the smallest between (1) and (2).
 *
 * 4) Take into account some room for the unavoidable IRQ servicing latency.
 *    Let's use 3/4 of the max range.
 *
 * Finally let's add the LSB value to the result so to clear out a bunch of
 * consecutive set bits coming from the original max values to produce a
 * nicer literal for assembly generation.
 */
#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYCLES_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX   (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

static uint64_t last_count;
static uint64_t last_ticks;
static uint32_t last_elapsed;

static uint32_t get_time32(void)
{
#if CONFIG_SOC_SERIES_TC3X
	return STM_TIMER->TIM0.U;
#elif CONFIG_SOC_SERIES_TC4X
	return (uint32_t)STM_TIMER->ABS.U;
#else
	return 0;
#endif
}

static uint64_t get_time64(void)
{
#if CONFIG_SOC_SERIES_TC3X
	return (uint64_t)STM_TIMER->TIM0SV.U | ((uint64_t)STM_TIMER->CAPSV.U << 32);
#elif CONFIG_SOC_SERIES_TC4X
	return STM_TIMER->ABS.U;
#else
	return 0;
#endif
}

static void set_compare(uint32_t cmp)
{
	STM_TIMER->CMP[0].U = cmp;
}

static bool stm_cmp0ir_pending(void)
{
#if CONFIG_SOC_SERIES_TC3X
	return STM_TIMER->ICR.B.CMP0IR != 0;
#elif CONFIG_SOC_SERIES_TC4X
	return STM_TIMER->ISR.B.CMP0IR != 0;
#else
	return false;
#endif
}

static void set_compare_irq(bool enabled)
{
	STM_TIMER->ICR.B.CMP0EN = enabled;
}

static void sys_clock_isr(void)
{
	k_spinlock_key_t key = sys_clock_lock();

	STM_TIMER->ISCR.B.CMP0IRR = 1;

	uint64_t now = get_time64();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYCLES_PER_TICK;

	last_count += (cycle_diff_t)dticks * CYCLES_PER_TICK;
	last_ticks += dticks;
	last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint64_t next = last_count + CYCLES_PER_TICK;

		set_compare((uint32_t)next);

		/* Check if previous compare value was missed and announce
		 * the missed tick.
		 */
		if (next - now < 100 && get_time64() > next && !stm_cmp0ir_pending()) {
			last_count += CYCLES_PER_TICK;
			last_ticks += 1;
			dticks += 1;
			next += CYCLES_PER_TICK;
			set_compare(next);
		}
	}

	sys_clock_announce_locked(dticks, key);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}
	uint64_t cyc;

	if (ticks == K_TICKS_FOREVER) {
		cyc = last_count + CYCLES_MAX;
	} else {
		cyc = (last_ticks + last_elapsed + ticks) * CYCLES_PER_TICK;
		if ((cyc - last_count) > CYCLES_MAX) {
			cyc = last_count + CYCLES_MAX;
		}
	}

	set_compare(cyc);

	/* Check if the compare value is already missed and trigger the interrupt if so. */
	if (ticks < 2 && get_time64() > cyc && !stm_cmp0ir_pending()) {
		intc_aurix_ir_irq_raise(STM_IRQ);
	}
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	uint64_t now = get_time64();
	uint64_t dcycles = now - last_count;
	uint32_t dticks = (cycle_diff_t)dcycles / CYCLES_PER_TICK;

	last_elapsed = dticks;
	return dticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* Return the current counter value */
	return get_time32();
}

uint64_t sys_clock_cycle_get_64(void)
{
	/* Return the current counter value */
	return get_time64();
}

void sys_clock_disable(void)
{
	set_compare_irq(false);
}

static int sys_clock_driver_init(void)
{
#if CONFIG_SOC_SERIES_TC4X
	Ifx_CPU *const CPU_MODULE = (Ifx_CPU *)DT_REG_ADDR(DT_NODELABEL(cpu));
	atomic_ptr_t stmwra = ATOMIC_PTR_INIT((void *)&CPU_MODULE->ACCENSTM.WRA);

	/* Check if the write access is enabled for the current core */
	if (!atomic_test_bit(stmwra, aurix_coreid_to_tagid(arch_proc_id()))) {
		aurix_apu_enable_write_select(&CPU_MODULE->PROTSTMSE, &CPU_MODULE->ACCENSTMCFG.WRA,
					      STM_IRQ_IDX, aurix_coreid_to_tagid(arch_proc_id()));
	}
#endif

	IRQ_CONNECT(STM_IRQ, 1, sys_clock_isr, NULL, 0);

	/* Initialise internal states */
	last_ticks = get_time64() / CYCLES_PER_TICK;
	last_count = last_ticks * CYCLES_PER_TICK;

#if DT_PROP(DT_NODELABEL(stm), freeze)
	/* Set debug freeze if selected and debugger connected */
	if (CBS_OSTATE.B.OEN) {
		STM_TIMER->OCS.U = 0x12000000;
	}
#endif
	Ifx_STM_CMCON cmcon = {.B.MSIZE0 = 31, .B.MSTART0 = 0};

	/* Set compare window */
	STM_TIMER->CMCON.B = cmcon.B;
	/* Set irq line */
	STM_TIMER->ICR.B.CMP0OS = 0;
	/* Clear irq */
	STM_TIMER->ISCR.B.CMP0IRR = 1;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		set_compare((uint32_t)(last_count + CYCLES_PER_TICK));
	} else {
		set_compare((uint32_t)(last_count + CYCLES_MAX));
	}
	set_compare_irq(true);

	irq_enable(STM_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
