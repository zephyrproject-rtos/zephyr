/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Reduced set of HW models sufficient to run some of the sample apps
 * and regression tests
 */

#include <stdint.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>
#include "hw_models_top.h"
#include "timer_model.h"
#include "irq_ctrl.h"
#include "posix_board_if.h"
#include "hw_counter.h"
#include <zephyr/arch/posix/posix_soc_if.h>
#include "posix_arch_internal.h"
#include <zephyr/sys/util.h>


static uint64_t simu_time; /* The actual time as known by the HW models */
static uint64_t end_of_time = NEVER; /* When will this device stop */

/* List of HW model timers: */
extern uint64_t hw_timer_timer; /* When should this timer_model be called */
extern uint64_t irq_ctrl_timer;
extern uint64_t hw_counter_timer;

static enum {
	HWTIMER = 0,
	IRQCNT,
	HW_COUNTER,
	NUMBER_OF_TIMERS,
	NONE
} next_timer_index = NONE;

static uint64_t *Timer_list[NUMBER_OF_TIMERS] = {
	&hw_timer_timer,
	&irq_ctrl_timer,
	&hw_counter_timer,
};

static uint64_t next_timer_time;

/* Have we received a SIGTERM or SIGINT */
static volatile sig_atomic_t signaled_end;

/**
 * Handler for SIGTERM and SIGINT
 */
void hwm_signal_end_handler(int sig)
{
	signaled_end = 1;
}

/**
 * Set the handler for SIGTERM and SIGINT which will cause the
 * program to exit gracefully when they are received the 1st time
 *
 * Note that our handler only sets a variable indicating the signal was
 * received, and in each iteration of the hw main loop this variable is
 * evaluated.
 * If for some reason (the program is stuck) we never evaluate it, the program
 * would never exit.
 * Therefore we set SA_RESETHAND: This way, the 2nd time the signal is received
 * the default handler would be called to terminate the program no matter what.
 *
 * Note that SA_RESETHAND requires either _POSIX_C_SOURCE>=200809L or
 * _XOPEN_SOURCE>=500
 */
void hwm_set_sig_handler(void)
{
	struct sigaction act;

	act.sa_handler = hwm_signal_end_handler;
	PC_SAFE_CALL(sigemptyset(&act.sa_mask));

	act.sa_flags = SA_RESETHAND;

	PC_SAFE_CALL(sigaction(SIGTERM, &act, NULL));
	PC_SAFE_CALL(sigaction(SIGINT, &act, NULL));
}


static void hwm_sleep_until_next_timer(void)
{
	if (next_timer_time >= simu_time) { /* LCOV_EXCL_BR_LINE */
		simu_time = next_timer_time;
	} else {
		/* LCOV_EXCL_START */
		posix_print_warning("next_timer_time corrupted (%"PRIu64"<= %"
				PRIu64", timer idx=%i)\n",
				(uint64_t)next_timer_time,
				(uint64_t)simu_time,
				next_timer_index);
		/* LCOV_EXCL_STOP */
	}

	if (signaled_end || (simu_time > end_of_time)) {
		posix_print_trace("\nStopped at %.3Lfs\n",
				((long double)simu_time)/1.0e6L);
		posix_exit(0);
	}
}


/**
 * Find in between all timers which is the next one
 * and update  next_timer_* accordingly
 */
void hwm_find_next_timer(void)
{
	next_timer_index = 0;
	next_timer_time  = *Timer_list[0];

	for (unsigned int i = 1; i < NUMBER_OF_TIMERS ; i++) {
		if (next_timer_time > *Timer_list[i]) {
			next_timer_index = i;
			next_timer_time = *Timer_list[i];
		}
	}
}

/**
 * Execute the next scheduled HW event/timer
 */
void hwm_one_event(void)
{
	hwm_sleep_until_next_timer();

	switch (next_timer_index) { /* LCOV_EXCL_BR_LINE */
	case HWTIMER:
		hwtimer_timer_reached();
		break;
	case IRQCNT:
		hw_irq_ctrl_timer_triggered();
		break;
	case HW_COUNTER:
		hw_counter_triggered();
		break;
	default:
		/* LCOV_EXCL_START */
		posix_print_error_and_exit(
					   "next_timer_index corrupted\n");
		break;
		/* LCOV_EXCL_STOP */
	}

	hwm_find_next_timer();
}

/**
 * Set the simulated time when the process will stop
 */
void hwm_set_end_of_time(uint64_t new_end_of_time)
{
	end_of_time = new_end_of_time;
}

/**
 * Return the current time as known by the device
 */
uint64_t hwm_get_time(void)
{
	return simu_time;
}

uint64_t posix_get_hw_cycle(void)
{
	return hwm_get_time();
}

/**
 * Function to initialize the HW models
 */
void hwm_init(void)
{
	hwm_set_sig_handler();
	hwtimer_init();
	hw_counter_init();
	hw_irq_ctrl_init();

	hwm_find_next_timer();
}

/**
 * Function to free any resources allocated by the HW models
 * Note that this function needs to be designed so it is possible
 * to call it more than once during cleanup
 */
void hwm_cleanup(void)
{
	hwtimer_cleanup();
	hw_irq_ctrl_cleanup();
}
