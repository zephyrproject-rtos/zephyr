/*
 * Copyright (c) 2019 Microchip Technology Inc.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <pm/pm.h>
#include <soc.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>
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

/* NOTE: Zephyr masks interrupts using BASEPRI before calling PM subsystem */
static void z_power_soc_deep_sleep(void)
{
#ifdef DEBUG_SLEEP
	GPIO_REGS->CTRL[MCHP_GPIO_0162_ID] = 0x00240U; /* Drive Low */
#endif

	soc_deep_sleep_periph_save();
	soc_deep_sleep_wake_en();
	soc_deep_sleep_non_wake_en();

#ifdef DEBUG_SLEEP
	GPIO_REGS->CTRL[MCHP_GPIO_0161_ID] = 0x00240U; /* Drive Low */
#endif

	/*
	 * Enable deep sleep mode in CM4 and MEC172x.
	 * Enable CM4 deep sleep and sleep signals assertion on WFI.
	 * Set MCHP Heavy sleep (PLL OFF when all CLK_REQ clear) and SLEEP_ALL
	 * to assert SLP_EN to all peripherals on WFI.
	 * Set PRIMASK = 1 so on wake the CPU will not vector to any ISR.
	 * Set BASEPRI = 0 to allow any priority to wake.
	 */
	SCB->SCR |= BIT(2);
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_HEAVY;
	PCR_REGS->OSC_ID = PCR_REGS->SYS_SLP_CTRL;
#ifdef DEBUG_DEEP_SLEEP_CLK_REQ
	soc_debug_sleep_clk_req();
#endif
	__set_PRIMASK(1);
	__set_BASEPRI(0);
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();

	/*
	 * Clear SLEEP_ALL manually since we are not vectoring to an ISR until
	 * PM post ops. This de-asserts peripheral SLP_EN signals.
	 */
	PCR_REGS->SYS_SLP_CTRL = 0U;
	SCB->SCR &= ~BIT(2);

#ifdef DEBUG_SLEEP
	GPIO_REGS->CTRL[MCHP_GPIO_0161_ID] = 0x10240U; /* Drive High */
#endif

	/* Wait for PLL to lock with timeout */
	HTMR_0_REGS->PRLD = 0U; /* make sure its stopped */
	HTMR_0_REGS->CTRL = 0U; /* 30.5 us per tick */
	GIRQ23_REGS->EN_CLR = MCHP_HTMR0_GIRQ_VAL;
	GIRQ23_REGS->SRC = MCHP_HTMR0_GIRQ_VAL;
	HTMR_0_REGS->PRLD = 216U; /* ~6.6 ms 2x the expected lock time */

	while ((PCR_REGS->OSC_ID & MCHP_PCR_OSC_ID_PLL_LOCK) == 0) {
		if (GIRQ23_REGS->SRC & MCHP_HTMR0_GIRQ_VAL) {
			break;
		}
	}

	HTMR_0_REGS->PRLD = 0U; /* stop */
	GIRQ23_REGS->SRC = MCHP_HTMR0_GIRQ_VAL;

	soc_deep_sleep_non_wake_dis();
	soc_deep_sleep_wake_dis();
	soc_deep_sleep_periph_restore();

#ifdef DEBUG_DEEP
	GPIO_REGS->CTRL[MCHP_GPIO_0162_ID] = 0x10240U; /* Drive High */
#endif
}

/*
 * Light Sleep
 * Pros:
 * Fast wake response:
 * Cons:
 * Higher power dissipation, 48MHz PLL remains on.
 *
 * When the kernel calls this it has masked interrupt by setting NVIC BASEPRI
 * equal to a value equal to the highest Zephyr ISR priority. Only NVIC
 * exceptions will be served.
 */
static void z_power_soc_sleep(void)
{
	SCB->SCR &= ~BIT(2);
	PCR_REGS->SYS_SLP_CTRL = MCHP_PCR_SYS_SLP_LIGHT;
	PCR_REGS->OSC_ID = PCR_REGS->SYS_SLP_CTRL;
	__set_PRIMASK(1);
	__set_BASEPRI(0);
	__WFI();	/* triggers sleep hardware */
	__NOP();
	__NOP();
	PCR_REGS->SYS_SLP_CTRL = 0U;
}

/*
 * Called from pm_system_suspend(int32_t ticks) in subsys/power.c
 * For deep sleep pm_system_suspend has executed all the driver
 * power management call backs.
 */
void pm_power_state_set(struct pm_state_info info)
{
	switch (info.state) {
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
 * Zephyr PM code expects us to enabled interrupt at post op exit. Zephyr used
 * arch_irq_lock() which sets BASEPRI to a non-zero value masking all interrupts
 * preventing wake. MCHP z_power_soc_(deep)_sleep sets PRIMASK=1 and BASEPRI=0
 * allowing wake from any enabled interrupt and prevent CPU from entering any
 * ISR on wake except for faults. We re-enable interrupt by setting PRIMASK to 0.
 * Side-effect is we set BASEPRI=0. Is this the same value as Zephyr uses during
 * NVIC initialization?
 */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	switch (info.state) {
	case PM_STATE_SUSPEND_TO_IDLE:
	case PM_STATE_SUSPEND_TO_RAM:
		__set_PRIMASK(0);
		__ISB();
		break;
	default:
		irq_unlock(0);
		break;
	}
}
