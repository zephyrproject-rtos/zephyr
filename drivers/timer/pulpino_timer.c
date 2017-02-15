/*
 * Copyright (c) 2016 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <system_timer.h>
#include <board.h>

/* Timer Ctrl Bitfields */
#define TIMER_CTRL_EN              (1 << 0)            /* Timer Enable Bit */
#define TIMER_CTRL_PRE(x)          (((x) & 0x07) << 3) /* Prescaler Value */

typedef struct {
	uint32_t val;
	uint32_t ctrl;
	uint32_t cmp;
} pulpino_timer_t;

static volatile pulpino_timer_t *timer = (pulpino_timer_t *)PULP_TIMER_A_BASE;

static uint32_t accumulated_cycle_count;

static void pulpino_timer_irq_handler(void *unused)
{
	ARG_UNUSED(unused);

	/* Reset counter */
	timer->val = 0;

	accumulated_cycle_count += sys_clock_hw_cycles_per_tick;

	_sys_clock_tick_announce();
}

#ifdef CONFIG_TICKLESS_IDLE
#error "Tickless idle not yet implemented for pulpino timer"
#endif

int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);
	IRQ_CONNECT(PULP_TIMER_A_CMP_IRQ, 0,
		    pulpino_timer_irq_handler, NULL, 0);
	irq_enable(PULP_TIMER_A_CMP_IRQ);

	/*
	 * Initialize timer.
	 * Reset counter and set timer to generate interrupt
	 * every sys_clock_hw_cycles_per_tick
	 */
	timer->val = 0;
	timer->cmp = sys_clock_hw_cycles_per_tick;
	timer->ctrl = TIMER_CTRL_EN;

	return 0;
}

/**
 *
 * @brief Read the platform's timer hardware
 *
 * This routine returns the current time in terms of timer hardware clock
 * cycles.
 *
 * @return up counter of elapsed clock cycles
 */
uint32_t _timer_cycle_get_32(void)
{
	return accumulated_cycle_count + timer->val;
}
