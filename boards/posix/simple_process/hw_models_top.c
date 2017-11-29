/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Barebones HW model sufficient to run some of the sample apps
 */

#include <stdint.h>
#include "hw_models_top.h"
#include "timer_model.h"
#include "irq_ctrl.h"
#include "main.h"


static hwtime_t device_time; /*The actual time as known by the device*/
static hwtime_t end_of_time = NEVER; /*When will this device stop*/

/* List of HW model timers: */
extern hwtime_t HWTimer_timer; /*When does the timer_model want to be called*/
extern hwtime_t irq_ctrl_timer;

typedef enum { HWTimer = 0, IRQCnt, Number_of_timers, None } timer_types_t;
static hwtime_t *Timer_list[Number_of_timers] = {
	&HWTimer_timer,
	&irq_ctrl_timer
};
static timer_types_t next_timer_index = None;
static hwtime_t next_timer_time;


static void hwm_sleep_until_next_timer(void)
{
	if (next_timer_time >= device_time) {
		device_time = next_timer_time;
	} else {
		ps_print_warning(
"next_timer_time corrupted (%"PRItime"<= %"PRItime", Timertype=%i)\n",
			next_timer_time,
			device_time,
			next_timer_index);
	}

	if (device_time > end_of_time) {
		ps_print_trace(
			"\n\n\n\n\n\nAutostopped after %.3Lfs\n",
			((long double)end_of_time)/1.0e6);

		main_clean_up(0);
	}
}


/**
 * Find in between all timers which is the next one
 * and update  next_timer_to_trigger* accordingly
 */
void hwm_find_next_timer(void)
{
	next_timer_index = 0;
	next_timer_time  = *Timer_list[0];

	for (unsigned int i = 1; i < Number_of_timers ; i++) {
		if (next_timer_time > *Timer_list[i]) {
			next_timer_index = i;
			next_timer_time = *Timer_list[i];
		}
	}
}

/**
 * Entry point for the HW models
 * The HW models execute in an infinite loop until terminated
 */
void hwm_main_loop(void)
{
	while (1) {
		hwm_sleep_until_next_timer();

		switch (next_timer_index) {
		case HWTimer:
			hwtimer_timer_reached();
			break;
		case IRQCnt:
			hw_irq_ctr_timer_triggered();
			break;
		default:
			ps_print_error_and_exit(
					"next_timer_index corrupted\n");
			break;
		}

		hwm_find_next_timer();
	}
}

/**
 * Set the simulated time when the process will stop
 */
void hwm_set_end_of_time(hwtime_t new_end_of_time)
{
	end_of_time = new_end_of_time;
}

hwtime_t hwm_get_time(void)
{
	return device_time;
}


/**
 * Function to initialize the HW models
 */
void hwm_init(void)
{
	hwtimer_init();
	hw_irq_ctrl_init();

	hwm_find_next_timer();
}

/**
 * Function to free any resources allocated by the HW models
 */
void hwm_cleanup(void)
{
	hwtimer_cleanup();
	hw_irq_ctrl_cleanup();
}


