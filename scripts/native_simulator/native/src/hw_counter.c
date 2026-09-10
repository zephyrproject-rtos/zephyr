/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "nsi_cpu0_interrupts.h"
#include "irq_ctrl.h"
#include "nsi_tasks.h"
#include "nsi_hws_models_if.h"

static uint64_t hw_counter_timer;

static bool counter_running;
static uint64_t counter_value;
static uint64_t counter_target;
static uint64_t counter_period;
static uint64_t counter_wrap;

/**
 * Initialize the counter with prescaler of HW
 */
static void hw_counter_init(void)
{
	hw_counter_timer = NSI_NEVER;
	counter_target = NSI_NEVER;
	counter_value = 0;
	counter_running = false;
	counter_period = NSI_NEVER;
	counter_wrap = NSI_NEVER;
}

NSI_TASK(hw_counter_init, HW_INIT, 10);

static void hw_counter_triggered(void)
{
	if (!counter_running) {
		hw_counter_timer = NSI_NEVER;
		return;
	}

	hw_counter_timer = nsi_hws_get_time() + counter_period;
	counter_value = (counter_value + 1) % counter_wrap;

	if (counter_value == counter_target) {
		hw_irq_ctrl_set_irq(COUNTER_EVENT_IRQ);
	}
}

NSI_HW_EVENT(hw_counter_timer, hw_counter_triggered, 20);

/**
 * Configures the counter period.
 * The counter will be incremented every 'period' microseconds.
 */
void hw_counter_set_period(uint64_t period)
{
	counter_period = period;
}

/*
 * Set the count value at which the counter will wrap
 * The counter will count up to  (counter_wrap-1), i.e.:
 * 0, 1, 2,.., (counter_wrap - 1), 0
 */
void hw_counter_set_wrap_value(uint64_t wrap_value)
{
	counter_wrap = wrap_value;
}

/**
 * Starts the counter. It must be previously configured with
 * hw_counter_set_period() and hw_counter_set_target().
 */
void hw_counter_start(void)
{
	if (counter_running) {
		return;
	}

	counter_running = true;

	hw_counter_timer = nsi_hws_get_time() + counter_period;
	nsi_hws_find_next_event();
}

/**
 * Stops the counter at current value.
 * On the next call to hw_counter_start, the counter will
 * start from the value at which it was stopped.
 */
void hw_counter_stop(void)
{
	counter_running = false;
	hw_counter_timer = NSI_NEVER;
	nsi_hws_find_next_event();
}

bool hw_counter_is_started(void)
{
	return counter_running;
}

/**
 * Returns the current counter value.
 */
uint64_t hw_counter_get_value(void)
{
	return counter_value;
}

/**
 * Resets the counter value.
 */
void hw_counter_reset(void)
{
	counter_value = 0;
}

/**
 * Configures the counter to generate an interrupt
 * when its count value reaches target.
 */
void hw_counter_set_target(uint64_t target)
{
	counter_target = target;
}
