/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#include <cmsis_core.h>

FUNC_NORETURN void stm32_enter_poweroff(void)
{
	/* Disable interrupts */
	__disable_irq();

	/* Enable SoC-specific low-power state */
	SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

	/* Make sure IRQs don't set the Event Register */
	SCB->SCR &= ~SCB_SCR_SEVONPEND_Msk;

	/*
	 * Trigger entry in low-power mode.
	 *
	 * Note that "sys_poweroff()" should only be called when
	 * the system is in "a safe state", but we still make an
	 * effort here to ensure entry low-power state suceeds
	 * even if the system is still somewhat active...
	 *
	 * According to the ARM Architecture Reference Manual,
	 * "[WFI is] the only architectually-defined mechanism
	 * that completely suspends execution", but WFE is also
	 * described as being able to suspend execution and make
	 * the CPU enter in low-power state! In practice, both
	 * instructions seem to trigger entry in low-power state.
	 *
	 * WFE can "guarantee" entry in low-power state despite
	 * pending or incoming interrupts, but spurious Events
	 * (as in "WFE wake-up event") turn it into a no-op; on
	 * the other hand, WFI does ignore Events but not already
	 * pending interrupts. Since spurious interrupts have a
	 * higher chance of occurring, we try with WFE first then
	 * fall back to WFI.
	 *
	 * The sequence is restarted forever; since this function
	 * must never return anyways, it doesn't really matter.
	 */

	while (1) {
		/* Clear the Event Register */
		__SEV();
		__WFE();

		/*
		 * Attempt entry in low-power state using WFE.
		 *
		 * This ought to always succeed because SEVONPEND=0,
		 * interrupts are masked and we just cleared the Event
		 * Register, but who knows...
		 *
		 * It can take up to two cycles for the processor
		 * to actually turn off - NOPs are recommended here.
		 */
		__WFE();
		__NOP();
		__NOP();

		/* WFE failed - try WFI instead */
		__WFI();
		__NOP();
		__NOP();

		/* Neither worked... try again */
	}

	CODE_UNREACHABLE;
}
