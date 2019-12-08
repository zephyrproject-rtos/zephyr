/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for nxp_lpc55s69 platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the nxp_lpc55s69 platform.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <aarch32/cortex_m/exc.h>
#include <fsl_power.h>
#include <fsl_clock.h>
#include <fsl_common.h>
#include <fsl_device_registers.h>
#include <fsl_pint.h>

/**
 *
 * @brief Initialize the system clock
 *
 * @return N/A
 *
 */

static ALWAYS_INLINE void clock_init(void)
{
#ifdef CONFIG_SOC_LPC55S69_CPU0
    /*!< Set up the clock sources */
    /*!< Configure FRO192M */
	/*!< Ensure FRO is on  */
	POWER_DisablePD(kPDRUNCFG_PD_FRO192M);
	/*!< Set up FRO to the 12 MHz, just for sure */
	CLOCK_SetupFROClocking(12000000U);
	/*!< Switch to FRO 12MHz first to ensure we can change the clock */
	CLOCK_AttachClk(kFRO12M_to_MAIN_CLK);

	/* Enable FRO HF(96MHz) output */
	CLOCK_SetupFROClocking(96000000U);

	/*!< Set FLASH wait states for core */
	CLOCK_SetFLASHAccessCyclesForFreq(96000000U);

    /*!< Set up dividers */
	CLOCK_SetClkDiv(kCLOCK_DivAhbClk, 1U, false);

    /*!< Set up clock selectors - Attach clocks to the peripheries */
	CLOCK_AttachClk(kFRO_HF_to_MAIN_CLK);

	/* Enables the clock for the I/O controller.: Enable Clock. */
    CLOCK_EnableClock(kCLOCK_Iocon);

#ifdef CONFIG_I2C_4
	/* attach 12 MHz clock to FLEXCOMM4 */
	CLOCK_AttachClk(kFRO12M_to_FLEXCOMM4);

	/* reset FLEXCOMM for I2C */
	RESET_PeripheralReset(kFC4_RST_SHIFT_RSTn);
#endif /* CONFIG_I2C_4 */

#ifdef CONFIG_SPI_8
	/* Attach 12 MHz clock to FLEXCOMM8 */
	CLOCK_AttachClk(kFRO12M_to_HSLSPI);

	/* reset FLEXCOMM for SPI */
	RESET_PeripheralReset(kHSLSPI_RST_SHIFT_RSTn);
#endif /* CONFIG_SPI_8 */

#endif /* CONFIG_SOC_LPC55S69_CPU0 */
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

static int nxp_lpc55s69_init(struct device *arg)
{
	ARG_UNUSED(arg);

	/* old interrupt lock level */
	unsigned int oldLevel;

	/* disable interrupts */
	oldLevel = irq_lock();

	z_arm_clear_faults();

	/* Initialize FRO/system clock to 96 MHz */
	clock_init();

#ifdef CONFIG_GPIO_MCUX_LPC
	/* Turn on PINT device*/
	PINT_Init(PINT);
#endif

	/*
	 * install default handler that simply resets the CPU if configured in
	 * the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);

	return 0;
}

SYS_INIT(nxp_lpc55s69_init, PRE_KERNEL_1, 0);
