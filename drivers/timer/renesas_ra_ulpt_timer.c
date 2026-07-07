/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <soc.h>

#define DT_DRV_COMPAT renesas_ra_ulpt_timer

/* Ensure there are exactly two ULPT timer instances
 * enabled in the device tree.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 2,
	     "Requires two instances of the ULPT timer to be enabled.");

/* ULPT instance 0: Used to announce ticks to the kernel. */
#define RA_ULPT_INST0_NODE    DT_INST_PARENT(0)
#define RA_ULPT_INST0_REG     ((R_ULPT0_Type *)DT_REG_ADDR(RA_ULPT_INST0_NODE))
#define RA_ULPT_INST0_IRQN    DT_IRQ_BY_NAME(RA_ULPT_INST0_NODE, ulpti, irq)
#define RA_ULPT_INST0_IRQP    0U
#define RA_ULPT_INST0_CHANNEL DT_PROP(RA_ULPT_INST0_NODE, channel)

/* ULPT instance 1: Used for synchronization with hardware cycle clock. */
#define RA_ULPT_INST1_NODE    DT_INST_PARENT(1)
#define RA_ULPT_INST1_REG     ((R_ULPT0_Type *)DT_REG_ADDR(RA_ULPT_INST1_NODE))
#define RA_ULPT_INST1_CHANNEL DT_PROP(RA_ULPT_INST1_NODE, channel)

/* Constants for timer configuration and behavior. */
#define RA_ULPT_RELOAD_DELAY 4U
#define RA_ULPT_RELOAD_MIN   4U
#define RA_ULPT_RELOAD_MAX   UINT32_MAX

#define RA_ULPT_PRV_ULPTCR_STATUS_FLAGS 0xE0U
#define RA_ULPT_PRV_ULPTCR_START_TIMER  0xE1U

/* Macro to get ELC event for ULPT interrupt based on the channel. */
#define ELC_EVENT_ULPT_INT(channel) CONCAT(ELC_EVENT_ULPT, channel, _INT)

/*
 * INST1 free-runs and provides the cycle counter (its down-counter read back
 * inverted counts up); INST0 is armed with a relative delay and its underflow
 * announces ticks. That makes this a RELOAD backend whose cycle source is a
 * genuine free-running counter, so the generic core owns the tick accounting
 * and the tick-aligned deadline.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_MIN_DELAY (RA_ULPT_RELOAD_MIN + RA_ULPT_RELOAD_DELAY)

static inline uint64_t timer_driver_cycle_get(void)
{
	return ~RA_ULPT_INST1_REG->ULPTCNT;
}

static void timer_driver_set_reload(uint32_t cycles)
{
	/* INST0 counts down 'cycles' then underflows. Account for the fixed
	 * reload delay of the hardware and the off-by-one of the count register.
	 */
	RA_ULPT_INST0_REG->ULPTCNT = (cycles - RA_ULPT_RELOAD_DELAY) - 1U;
}

#include "system_timer_generic.h"

static void ra_ulpt_timer_isr(void)
{
	IRQn_Type irq = R_FSP_CurrentIrqGet();

	/* Clear pending IRQ to prevent re-triggering. */
	R_BSP_IrqStatusClear(irq);

	if (RA_ULPT_INST0_REG->ULPTCR_b.TUNDF) {
		/* Clear the underflow flag and let the core announce. */
		RA_ULPT_INST0_REG->ULPTCR_b.TUNDF = 0;
		timer_core_announce();
	}
}

static int sys_clock_driver_init(void)
{
	/* Power on ULPT modules. */
	R_BSP_MODULE_START(FSP_IP_ULPT, RA_ULPT_INST0_CHANNEL);
	R_BSP_MODULE_START(FSP_IP_ULPT, RA_ULPT_INST1_CHANNEL);

	/* Stop timers and reset control registers. */
	RA_ULPT_INST0_REG->ULPTCR = 0U;
	RA_ULPT_INST1_REG->ULPTCR = 0U;

	/* Wait for timers to stop. */
	FSP_HARDWARE_REGISTER_WAIT(0U, RA_ULPT_INST0_REG->ULPTCR_b.TCSTF);
	FSP_HARDWARE_REGISTER_WAIT(0U, RA_ULPT_INST1_REG->ULPTCR_b.TCSTF);

	/* Clear configuration registers before setup. */
	RA_ULPT_INST0_REG->ULPTMR2 = 0U;
	RA_ULPT_INST1_REG->ULPTMR2 = 0U;

	/* Configure timer instance 0. */
	RA_ULPT_INST0_REG->ULPTMR1 = 0U;
	RA_ULPT_INST0_REG->ULPTMR2 = 0U;
	RA_ULPT_INST0_REG->ULPTMR3 = 0U;
	RA_ULPT_INST0_REG->ULPTIOC = 0U;
	RA_ULPT_INST0_REG->ULPTISR = 0U;
	RA_ULPT_INST0_REG->ULPTCMSR = 0U;

	/* Configure timer instance 1. */
	RA_ULPT_INST1_REG->ULPTMR1 = 0U;
	RA_ULPT_INST1_REG->ULPTMR2 = 0U;
	RA_ULPT_INST1_REG->ULPTMR3 = 0U;
	RA_ULPT_INST1_REG->ULPTIOC = 0U;
	RA_ULPT_INST1_REG->ULPTISR = 0U;
	RA_ULPT_INST1_REG->ULPTCMSR = 0U;

	/* Initialize timer counters. */
	RA_ULPT_INST0_REG->ULPTCNT = TIMER_CORE_CYC_PER_TICK - 1U;
	RA_ULPT_INST1_REG->ULPTCNT = RA_ULPT_RELOAD_MAX;

	/* Set up interrupts for timer instance 0. */
	R_ICU->IELSR[RA_ULPT_INST0_IRQN] = ELC_EVENT_ULPT_INT(RA_ULPT_INST0_CHANNEL);
	BSP_ASSIGN_EVENT_TO_CURRENT_CORE(ELC_EVENT_ULPT_INT(RA_ULPT_INST0_CHANNEL));
	IRQ_CONNECT(RA_ULPT_INST0_IRQN, RA_ULPT_INST0_IRQP, ra_ulpt_timer_isr, NULL, 0);
	irq_enable(RA_ULPT_INST0_IRQN);

	/* Start both timers. */
	RA_ULPT_INST0_REG->ULPTCR = RA_ULPT_PRV_ULPTCR_START_TIMER;
	RA_ULPT_INST1_REG->ULPTCR = RA_ULPT_PRV_ULPTCR_START_TIMER;

	/* Wait for timers to start completely. */
	FSP_HARDWARE_REGISTER_WAIT(RA_ULPT_INST0_REG->ULPTCR_b.TSTART,
				   RA_ULPT_INST0_REG->ULPTCR_b.TCSTF);
	FSP_HARDWARE_REGISTER_WAIT(RA_ULPT_INST1_REG->ULPTCR_b.TSTART,
				   RA_ULPT_INST1_REG->ULPTCR_b.TCSTF);

	/* Seed the core baseline from the free-running counter and arm the first
	 * tick.
	 */
	timer_core_init();

	return 0;
}

/* Initialize the system timer driver during pre-kernel stage 2. */
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
