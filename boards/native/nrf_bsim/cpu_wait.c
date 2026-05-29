/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "fake_timer.h"
#include <zephyr/arch/posix/posix_soc_if.h>
#include <posix_board_if.h>
#include <posix_soc.h>
#include "nsi_hw_scheduler.h"

/**
 * Replacement to the kernel k_busy_wait()
 * Will block this thread (and therefore the whole Zephyr) during usec_to_wait
 *
 * Note that interrupts may be received in the meanwhile and that therefore this
 * thread may lose context.
 * Therefore the wait time may be considerably longer.
 *
 * All this function ensures is that it will return after usec_to_wait or later.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	bs_time_t time_end = nsi_hws_get_time() + usec_to_wait;

	while (nsi_hws_get_time() < time_end) {
		/*
		 * There may be wakes due to other interrupts or nested calls to
		 * k_busy_wait in interrupt handlers
		 */
		nhw_fake_timer_wake_in_time(CONFIG_NATIVE_SIMULATOR_MCU_N, time_end);
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
	bs_time_t time_start;
	int64_t to_wait = usec_to_waste;

	while (to_wait > 0) {
		/*
		 * There may be wakes due to other interrupts or nested calls to
		 * cpu_hold in interrupt handlers
		 */
		time_start = nsi_hws_get_time();
		nhw_fake_timer_wake_in_time(CONFIG_NATIVE_SIMULATOR_MCU_N, time_start + to_wait);
		posix_change_cpu_state_and_wait(true);
		to_wait -= nsi_hws_get_time() - time_start;

		posix_irq_handler();
	}
}
