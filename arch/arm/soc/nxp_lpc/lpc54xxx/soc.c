/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc54114 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc54114 platform.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <uart.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>

/**
 *
 * @brief Initialize the system clock
 *
 * @return N/A
 *
 */

static ALWAYS_INLINE void clkInit(void)
{
	/* Set up the clock sources */

	/* Ensure FRO is on */
	POWER_DisablePD(kPDRUNCFG_PD_FRO_EN);

	/*
	 * Switch to FRO 12MHz first to ensure we can change voltage without
	 * accidentally being below the voltage for current speed.
	 */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* Set up high frequency FRO output to selected frequency */
	CLOCK_SetupFROClocking(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	/* Set up dividers */
	/* Set AHBCLKDIV divider to value 1 */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

	/* Set up clock selectors - Attach clocks to the peripheries */
	/* Switch MAIN_CLK to FRO_HF */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);

	/* Attach 12 MHz clock to FLEXCOMM0 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);
}

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the interrupt controller device drivers.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int nxp_lpc54114_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	_ClearFaults();

	/* Initialize FRO/system clock to 48 MHz */
	clkInit();

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_lpc54114_init, PRE_KERNEL_1, 0);
