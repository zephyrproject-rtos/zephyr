/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "hw_models_top.h"
#include "board_soc.h"
#include "board_irq.h"
#include "irq_ctrl.h"

uint64_t hw_counter_timer;

static bool counter_running;
static uint64_t counter_value;
static uint64_t counter_target;
static uint64_t counter_period;

/**
 * Initialize the counter with prescaler of HW
 */
void hw_counter_init(void)
{
	hw_counter_timer = NEVER;
	counter_target = NEVER;
	counter_value = 0;
	counter_running = false;
	counter_period = NEVER;
}

void hw_counter_triggered(void)
{
	if (!counter_running) {
		hw_counter_timer = NEVER;
		return;
	}

	hw_counter_timer = hwm_get_time() + counter_period;
	counter_value = counter_value + 1;

	if (counter_value == counter_target) {
		hw_irq_ctrl_set_irq(COUNTER_EVENT_IRQ);
	}
}

/**
 * Configures the counter period.
 * The counter will be incremented every 'period' microseconds.
 */
void hw_counter_set_period(uint64_t period)
{
	counter_period = period;
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

	hw_counter_timer = hwm_get_time() + counter_period;
	hwm_find_next_timer();
}

/**
 * Stops the counter at current value.
 * On the next call to hw_counter_start, the counter will
 * start from the value at which it was stopped.
 */
void hw_counter_stop(void)
{
	counter_running = false;
	hw_counter_timer = NEVER;
	hwm_find_next_timer();
}

/**
 * Returns the current counter value.
 */
uint64_t hw_counter_get_value(void)
{
	return counter_value;
}

/**
 * Configures the counter to generate an interrupt
 * when its count value reaches target.
 */
void hw_counter_set_target(uint64_t target)
{
	counter_target = target;
}
