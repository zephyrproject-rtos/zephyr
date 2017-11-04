/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>

/* #include <kernel_structs.h> */

u64_t __noinit __start_swap_time;
u64_t __noinit __end_swap_time;
u64_t __noinit __start_intr_time;
u64_t __noinit __end_intr_time;
u64_t __noinit __start_tick_time;
u64_t __noinit __end_tick_time;

/* location of the time stamps*/
u32_t __read_swap_end_time_value;
u64_t __common_var_swap_end_time;

/* NRF RTC TIMER runs ar very slow rate (32KHz), So in order to measure
 * Kernel stats dedicated timer is used to measure kernel stats
 */
#if defined(CONFIG_NRF_RTC_TIMER)
#include <arch/arm/cortex_m/cmsis.h>
/* To get current count of timer, first 1 need to be written into
 * Capture Register and Current Count will be copied into corresponding
 * current count register.
 */
#define TIMING_INFO_PRE_READ() (NRF_TIMER2->TASKS_CAPTURE[0] = 1)
#define TIMING_INFO_OS_GET_TIME()  (NRF_TIMER2->CC[0])
#define TIMING_INFO_GET_TIMER_VALUE()	TIMING_INFO_OS_GET_TIME()

#else  /* All other architectures */

#define TIMING_INFO_PRE_READ()
#define TIMING_INFO_OS_GET_TIME()  k_cycle_get_32()

#ifdef CONFIG_ARM
#include <arch/arm/cortex_m/cmsis.h>
#define TIMING_INFO_GET_TIMER_VALUE()   SysTick->VAL
#endif	/* CONFIG_ARM */

#endif /* CONFIG_NRF_RTC_TIMER */

#ifdef CONFIG_ARM
void read_timer_start_of_swap(void)
{
	TIMING_INFO_PRE_READ();
	__start_swap_time = (u32_t) TIMING_INFO_GET_TIMER_VALUE();
}

void read_timer_end_of_swap(void)
{
	if (__read_swap_end_time_value == 1) {
		TIMING_INFO_PRE_READ();
		__read_swap_end_time_value = 2;
		__common_var_swap_end_time = TIMING_INFO_OS_GET_TIME();
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

#endif /* CONFIG_ARM */
