/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
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

/* Calculated constants for timer operation. */
#define CYCLE_PER_TICK ((sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_TICKS      ((k_ticks_t)(RA_ULPT_RELOAD_MAX / CYCLE_PER_TICK) - 1)

/* Static variables for maintaining timer state. */
static uint32_t cycle_announced;
static struct k_spinlock lock;

static void ra_ulpt_timer_isr(void)
{
	uint32_t cycles;
	uint32_t dcycles;
	uint32_t dticks;
	IRQn_Type irq = R_FSP_CurrentIrqGet();

	/* Clear pending IRQ to prevent re-triggering. */
	R_BSP_IrqStatusClear(irq);

	if (RA_ULPT_INST0_REG->ULPTCR_b.TUNDF) {
		k_spinlock_key_t key = k_spin_lock(&lock);

		if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
			/* Calculate elapsed cycles and ticks. */
			cycles = ~RA_ULPT_INST1_REG->ULPTCNT;
			dcycles = cycles - cycle_announced;
			dticks = dcycles / CYCLE_PER_TICK;
			cycle_announced += dticks * CYCLE_PER_TICK;
		} else {
			/* In tickful mode, announce one tick at a time. */
			dticks = 1;
		}
		/* Clear the underflow flag. */
		RA_ULPT_INST0_REG->ULPTCR_b.TUNDF = 0;
		k_spin_unlock(&lock, key);

		/* Announce the elapsed ticks to the kernel. */
		sys_clock_announce(dticks);
	}
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	/* Timeout configuration is unsupported in tickful mode. */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* No timeout change for K_TICKS_FOREVER or INT32_MAX. */
	if (ticks == K_TICKS_FOREVER || ticks == INT32_MAX) {
		return;
	}

	/* Clamp the ticks value to a valid range. */
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	/* Calculate the timer delay in cycles. */
	uint32_t cycles = ~RA_ULPT_INST1_REG->ULPTCNT;
	uint32_t unannounced = cycles - cycle_announced;
	uint32_t delay = ticks * CYCLE_PER_TICK;

	/* Adjust delay to align with tick boundaries. */
	delay += unannounced;
	delay = DIV_ROUND_UP(delay, CYCLE_PER_TICK) * CYCLE_PER_TICK;
	delay -= unannounced;
	delay = MAX(delay, RA_ULPT_RELOAD_MIN + RA_ULPT_RELOAD_DELAY);
	delay -= RA_ULPT_RELOAD_DELAY;

	/* Update the timer counter. */
	RA_ULPT_INST0_REG->ULPTCNT = delay - 1U;
}

uint32_t sys_clock_elapsed(void)
{
	/* Elapsed time calculation is unsupported in tickful mode. */
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	/* Calculate and return the number of elapsed cycles. */
	uint32_t cycles = ~RA_ULPT_INST1_REG->ULPTCNT - cycle_announced;

	return (cycles / CYCLE_PER_TICK);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ~RA_ULPT_INST1_REG->ULPTCNT;
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
	RA_ULPT_INST0_REG->ULPTCNT = CYCLE_PER_TICK - 1U;
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
	cycle_announced = 0U;

	return 0;
}

/* Initialize the system timer driver during pre-kernel stage 2. */
SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
