/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <kernel_internal.h>

uint64_t  arch_timing_swap_start;
uint64_t  arch_timing_swap_end;
uint64_t  arch_timing_irq_start;
uint64_t  arch_timing_irq_end;
uint64_t  arch_timing_tick_start;
uint64_t  arch_timing_tick_end;
uint64_t  arch_timing_enter_user_mode_end;

/* location of the time stamps*/
uint32_t arch_timing_value_swap_end;
uint64_t arch_timing_value_swap_common;
uint64_t arch_timing_value_swap_temp;

#if defined(CONFIG_NRF_RTC_TIMER)
#include <nrfx.h>

/* To get current count of timer, first 1 need to be written into
 * Capture Register and Current Count will be copied into corresponding
 * current count register.
 */
#define TIMING_INFO_PRE_READ()        (NRF_TIMER2->TASKS_CAPTURE[0] = 1)
#define TIMING_INFO_OS_GET_TIME()     (NRF_TIMER2->CC[0])
#define TIMING_INFO_GET_TIMER_VALUE() (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)    (val)

#elif defined(CONFIG_SOC_SERIES_MEC1501X)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (B32TMR1_REGS->CNT)
#define TIMING_INFO_GET_TIMER_VALUE() (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)    (val)

#elif defined(CONFIG_X86)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (z_tsc_read())
#define TIMING_INFO_GET_TIMER_VALUE()  (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)     (val)

#elif defined(CONFIG_CPU_CORTEX_M)
#include <arch/arm/aarch32/cortex_m/cmsis.h>
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (SysTick->VAL)
#define SUBTRACT_CLOCK_CYCLES(val)     (SysTick->LOAD - (uint32_t)val)

#elif defined(CONFIG_ARC)
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE() (z_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT))
#define SUBTRACT_CLOCK_CYCLES(val)    ((uint32_t)val)

#elif defined(CONFIG_NIOS2)
#include "altera_avalon_timer_regs.h"
#define TIMING_INFO_PRE_READ()         \
	(IOWR_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE, 10))

#define TIMING_INFO_OS_GET_TIME()      (SUBTRACT_CLOCK_CYCLES(\
	((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE))))

#define TIMING_INFO_GET_TIMER_VALUE()  (\
	((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((uint32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE)))

#define SUBTRACT_CLOCK_CYCLES(val)     \
	((IORD_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE)	\
	  << 16 |						\
	  (IORD_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE)))	\
	 - ((uint32_t)val))

#else
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (k_cycle_get_32())
#define SUBTRACT_CLOCK_CYCLES(val)     ((uint32_t)val)
#endif	/* CONFIG_NRF_RTC_TIMER */


void read_timer_start_of_swap(void)
{
	if (arch_timing_value_swap_end == 1U) {
		TIMING_INFO_PRE_READ();
		arch_timing_swap_start = (uint32_t) TIMING_INFO_OS_GET_TIME();
	}
}

void read_timer_end_of_swap(void)
{
	if (arch_timing_value_swap_end == 1U) {
		TIMING_INFO_PRE_READ();
		arch_timing_value_swap_end = 2U;
		arch_timing_value_swap_common =
			(uint64_t)TIMING_INFO_OS_GET_TIME();
	}
}

/* ARM processors read current value of time through sysTick timer
 * and nrf soc read it though timer
 */
void read_timer_start_of_isr(void)
{
	TIMING_INFO_PRE_READ();
	arch_timing_irq_start  = (uint32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_isr(void)
{
	TIMING_INFO_PRE_READ();
	arch_timing_irq_end  = (uint32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_start_of_tick_handler(void)
{
	TIMING_INFO_PRE_READ();
	arch_timing_tick_start  = (uint32_t)TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_tick_handler(void)
{
	TIMING_INFO_PRE_READ();
	 arch_timing_tick_end  = (uint32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_userspace_enter(void)
{
	TIMING_INFO_PRE_READ();
	arch_timing_enter_user_mode_end = (uint32_t)TIMING_INFO_GET_TIMER_VALUE();
}
