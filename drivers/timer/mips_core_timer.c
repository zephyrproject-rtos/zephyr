/*
 * Copyright (c) 2017 Imagination Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief MIPS core timer driver
 *
 * This module implements the kernel's MIPS core timer driver.
 * It provides the standard kernel "system clock driver" interfaces.
 *
 * The driver utilizes systick to provide kernel ticks.
 */

#include <kernel.h>
#include <toolchain.h>
#include <linker/sections.h>
#include <misc/__assert.h>
#include <mips/hal.h>
#include <mips/m32c0.h>
#include <system_timer.h>
#include <kernel_structs.h>

/* A number just larger than the number of timer ticks between the getcount
 * call in the timer ISR and the point where the timer ISR can be triggered
 * again.
 */

#define MIPS_TIMER_FUDGE 256

/* running total of timer count */
static u32_t accumulated_cycle_count;

/**
 *
 * @brief System clock tick handler
 *
 * This routine handles the system clock tick interrupt. A TICK_EVENT event
 * is pushed onto the kernel stack.
 *
 * The symbol for this routine is either _timer_int_handler.
 *
 * @return N/A
 */
void _timer_int_handler(void *unused)
{
	ARG_UNUSED(unused);

	mips_biccr(CR_HINT5); /* HINT5 is IRQ 7 (SINT0 SINT1 HINT0...) */

	u32_t next = mips32_getcompare() + (sys_clock_hw_cycles_per_tick / 2);
	u32_t eor = mips32_getcount() + MIPS_TIMER_FUDGE;

	if ((next - eor) > (((u32_t)-1) / 2)) {
		next = eor;
	}

	mips32_setcompare(next);

	accumulated_cycle_count += sys_clock_hw_cycles_per_tick;

	_sys_clock_tick_announce();
}

/**
 *
 * @brief Initialize and enable the system clock
 *
 * This routine is used to program the systick to deliver interrupts at the
 * rate specified via the 'sys_clock_us_per_tick' global variable.
 *
 * @return 0
 */
int _sys_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* init core timer interrupt */
	IRQ_CONNECT(MIPS_MACHINE_TIMER_IRQ, 0, _timer_int_handler, 0, 0);

	mips_bissr(SR_HINT5);

	mips32_setcompare(mips32_getcount() +
			(sys_clock_hw_cycles_per_tick / 2));

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
u32_t _timer_cycle_get_32(void)
{
	return mips32_getcount() << 1;
}
