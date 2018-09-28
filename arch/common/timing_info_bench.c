/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>

/* #include <kernel_structs.h> */

u64_t  __start_swap_time;
u64_t  __end_swap_time;
u64_t  __start_intr_time;
u64_t  __end_intr_time;
u64_t  __start_tick_time;
u64_t  __end_tick_time;
u64_t  __end_drop_to_usermode_time;

/* location of the time stamps*/
u32_t __read_swap_end_time_value;
u64_t __common_var_swap_end_time;

#if CONFIG_ARM
#include <arch/arm/cortex_m/cmsis.h>
#endif
#ifdef CONFIG_NRF_RTC_TIMER

/* To get current count of timer, first 1 need to be written into
 * Capture Register and Current Count will be copied into corresponding
 * current count register.
 */
#define TIMING_INFO_PRE_READ()        (NRF_TIMER2->TASKS_CAPTURE[0] = 1)
#define TIMING_INFO_OS_GET_TIME()     (NRF_TIMER2->CC[0])
#define TIMING_INFO_GET_TIMER_VALUE() (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)    (val)

#elif CONFIG_X86
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (_tsc_read())
#define TIMING_INFO_GET_TIMER_VALUE()  (TIMING_INFO_OS_GET_TIME())
#define SUBTRACT_CLOCK_CYCLES(val)     (val)

#elif CONFIG_ARM
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (SysTick->VAL)
#define SUBTRACT_CLOCK_CYCLES(val)     (SysTick->LOAD - (u32_t)val)

#elif CONFIG_ARC
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()     (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE() (_arc_v2_aux_reg_read(_ARC_V2_TMR0_COUNT))
#define SUBTRACT_CLOCK_CYCLES(val)    ((u32_t)val)

#elif CONFIG_XTENSA
#include <xtensa_timer.h>
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (k_cycle_get_32())
#define SUBTRACT_CLOCK_CYCLES(val)     ((u32_t)val)

#elif CONFIG_NIOS2
#include "altera_avalon_timer_regs.h"
#define TIMING_INFO_PRE_READ()         \
	(IOWR_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE, 10))

#define TIMING_INFO_OS_GET_TIME()      (SUBTRACT_CLOCK_CYCLES(\
	((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE))))

#define TIMING_INFO_GET_TIMER_VALUE()  (\
	((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPH(TIMER_0_BASE) << 16)\
	| ((u32_t)IORD_ALTERA_AVALON_TIMER_SNAPL(TIMER_0_BASE)))

#define SUBTRACT_CLOCK_CYCLES(val)     \
	((IORD_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE)	\
	  << 16 |						\
	  (IORD_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE)))	\
	 - ((u32_t)val))


#elif CONFIG_RISCV32
#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()      (k_cycle_get_32())
#define TIMING_INFO_GET_TIMER_VALUE()  (k_cycle_get_32())
#define SUBTRACT_CLOCK_CYCLES(val)     ((u32_t)val)

#else
/* Default case */
#error "Benchmarks have not been implemented for this architecture"
#endif	/* CONFIG_NRF_RTC_TIMER */


void read_timer_start_of_swap(void)
{
	if (__read_swap_end_time_value == 1) {
		TIMING_INFO_PRE_READ();
		__start_swap_time = (u32_t) TIMING_INFO_OS_GET_TIME();
	}
}

void read_timer_end_of_swap(void)
{
	if (__read_swap_end_time_value == 1) {
		TIMING_INFO_PRE_READ();
		__read_swap_end_time_value = 2;
		__common_var_swap_end_time = (u64_t)TIMING_INFO_OS_GET_TIME();
	}
}

/* ARM processors read current value of time through sysTick timer
 * and nrf soc read it though timer
 */
void read_timer_start_of_isr(void)
{
	TIMING_INFO_PRE_READ();
	__start_intr_time  = (u32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_isr(void)
{
	TIMING_INFO_PRE_READ();
	__end_intr_time  = (u32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_start_of_tick_handler(void)
{
	TIMING_INFO_PRE_READ();
	__start_tick_time  = (u32_t)TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_tick_handler(void)
{
	TIMING_INFO_PRE_READ();
	 __end_tick_time  = (u32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_userspace_enter(void)
{
	TIMING_INFO_PRE_READ();
	__end_drop_to_usermode_time  = (u32_t) TIMING_INFO_GET_TIMER_VALUE();
}
