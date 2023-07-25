/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include "timer_model.h"
#include <zephyr/arch/posix/posix_soc_if.h>
#include <posix_board_if.h>
#include <posix_soc.h>

/**
 * Replacement to the kernel k_busy_wait()
 * Will block this thread (and therefore the whole Zephyr) during usec_to_wait
 *
 * Note that interrupts may be received in the meanwhile and that therefore this
 * thread may lose context.
 * Therefore the wait time may be considerably longer.
 *
 * All this function ensures is that it will return after usec_to_wait or later.
 *
 * This special arch_busy_wait() is necessary due to how the POSIX arch/SOC INF
 * models a CPU. Conceptually it could be thought as if the MCU was running
 * at an infinitely high clock, and therefore no simulated time passes while
 * executing instructions(*1).
 * Therefore to be able to busy wait this function does the equivalent of
 * programming a dedicated timer which will raise a non-maskable interrupt,
 * and halting the CPU.
 *
 * (*1) In reality simulated time is simply not advanced just due to the "MCU"
 * running. Meaning, the SW running on the MCU is assumed to take 0 time.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	uint64_t time_end = hwm_get_time() + usec_to_wait;

	while (hwm_get_time() < time_end) {
		/*
		 * There may be wakes due to other interrupts including
		 * other threads calling arch_busy_wait
		 */
		hwtimer_wake_in_time(time_end);
		posix_halt_cpu();
	}
}

/**
 * Will block this thread (and therefore the whole Zephyr) during usec_to_waste
 *
 * Very similar to arch_busy_wait(), but if an interrupt or context switch
 * occurs this function will continue waiting after, ensuring that
 * usec_to_waste are spent in this context, irrespectively of how much more
 * time would be spent on interrupt handling or possible switched-in tasks.
 *
 * Can be used to emulate code execution time.
 */
void posix_cpu_hold(uint32_t usec_to_waste)
{
	uint64_t time_start;
	int64_t to_wait = usec_to_waste;

	while (to_wait > 0) {
		/*
		 * There may be wakes due to other interrupts or nested calls to
		 * cpu_hold in interrupt handlers
		 */
		time_start = hwm_get_time();
		hwtimer_wake_in_time(time_start + to_wait);
		posix_change_cpu_state_and_wait(true);
		to_wait -= hwm_get_time() - time_start;

		posix_irq_handler();
	}
}
