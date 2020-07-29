/*
 * Copyright (c) 2017-2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_internal.h>
#include <timing/timing.h>

timing_t arch_timing_swap_start;
timing_t arch_timing_swap_end;
timing_t arch_timing_irq_start;
timing_t arch_timing_irq_end;
timing_t arch_timing_tick_start;
timing_t arch_timing_tick_end;
timing_t arch_timing_enter_user_mode_end;

/* If == 1U, record timing during swap */
uint32_t arch_timing_value_swap_end;

void read_timer_start_of_swap(void)
{
	if (arch_timing_value_swap_end == 1U) {
		arch_timing_swap_start = timing_counter_get();
	}
}

void read_timer_end_of_swap(void)
{
	if (arch_timing_value_swap_end == 1U) {
		arch_timing_value_swap_end = 2U;
		arch_timing_swap_end = timing_counter_get();
	}
}

/* ARM processors read current value of time through sysTick timer
 * and nrf soc read it though timer
 */
void read_timer_start_of_isr(void)
{
	arch_timing_irq_start = timing_counter_get();
}

void read_timer_end_of_isr(void)
{
	arch_timing_irq_end = timing_counter_get();
}

void read_timer_start_of_tick_handler(void)
{
	arch_timing_tick_start = timing_counter_get();
}

void read_timer_end_of_tick_handler(void)
{
	arch_timing_tick_end = timing_counter_get();
}

void read_timer_end_of_userspace_enter(void)
{
	arch_timing_enter_user_mode_end = timing_counter_get();
}
