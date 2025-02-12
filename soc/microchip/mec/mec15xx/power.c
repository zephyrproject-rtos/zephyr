/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/pm/pm.h>
#include <soc.h>
#include "device_power.h"

/*
 * Deep Sleep
 * Pros:
 * Lower power dissipation, 48MHz PLL is off
 * Cons:
 * Longer wake latency. CPU start running on ring oscillator
 * between 16 to 25 MHz. Minimum 3ms until PLL reaches lock
 * frequency of 48MHz.
 *
 * Implementation Notes:
 * We touch the Cortex-M's primary mask and base priority registers
 * because we do not want to enter an ISR immediately upon wake.
 * We must restore any hardware state that was modified upon sleep
 * entry before allowing interrupts to be serviced. Zephyr arch level
 * does not provide API's to manipulate both primary mask and base priority.
 *
 * DEBUG NOTES:
 * If a JTAG/SWD debug probe is connected driving TRST# high and
 * possibly polling the DUT then MEC1501 will not shut off its 48MHz
 * PLL. Firmware should not disable JTAG/SWD in the EC subsystem
 * while a probe is using the interface. This can leave the JTAG/SWD
 * TAP controller in a state of requesting clocks preventing the PLL
 * from being shut off.
 */
static void z_power_soc_deep_sleep(void)
{
	/* Mask all exceptions and interrupts except NMI and HardFault */
	__set_PRIMASK(1);

	soc_deep_sleep_periph_save();

	soc_deep_sleep_enable();

	soc_deep_sleep_wait_clk_idle();
	soc_deep_sleep_non_wake_en();

	/*
	 * Unmask all interrupts in BASEPRI. PRIMASK is used above to
	 * prevent entering an ISR after unmasking in BASEPRI.
	 */
	__set_BASEPRI(0);
	barrier_dsync_fence_full();
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();

	soc_deep_sleep_disable();

	soc_deep_sleep_non_wake_dis();

	/* Wait for PLL to lock */
	while ((PCR_REGS->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK) == 0) {
	}

	soc_deep_sleep_periph_restore();

	/*
	 * pm_state_exit_post_ops() is not being called
	 * after exiting deep sleep, so need to unmask exceptions
	 * and interrupts here.
	 */
	__set_PRIMASK(0);
}

/*
 * Light Sleep
 * Pros:
 * Fast wake response:
 * Cons:
 * Higher power dissipation, 48MHz PLL remains on.
 */
static void z_power_soc_sleep(void)
{
	__set_PRIMASK(1);

	soc_lite_sleep_enable();

	__set_BASEPRI(0); /* Make sure wake interrupts are not masked! */
	barrier_dsync_fence_full();
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();
}

/*
 * Called from pm_system_suspend(int32_t ticks) in subsys/power.c
 * For deep sleep pm_system_suspend has executed all the driver
 * power management call backs.
 */
void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
		z_power_soc_sleep();
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		z_power_soc_deep_sleep();
		break;
	default:
		break;
	}
}

/*
 * Zephyr PM code expects us to enabled interrupts at post op exit. Zephyr used
 * arch_irq_lock() which sets BASEPRI to a non-zero value masking all interrupts
 * preventing wake. MCHP z_power_soc_(deep)_sleep sets PRIMASK=1 and BASEPRI=0
 * allowing wake from any enabled interrupt and prevents the CPU from entering
 * an ISR on wake except for faults. We re-enable interrupts by setting PRIMASK
 * to 0.
 */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
		__set_PRIMASK(0);
		barrier_isync_fence_full();
		break;

	default:
		irq_unlock(0);
		break;
	}
}
