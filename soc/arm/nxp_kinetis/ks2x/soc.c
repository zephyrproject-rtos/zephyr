/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2018 Prevas A/S
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 * Copyright (c) 2020 Rick Talbott <rtalbott@fastmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for fsl_frdm_ks22f platform
 *
 * This module provides routines to initialize and support board-level
 * hardware for the fsl_frdm_ks22f platform.
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <arch/cpu.h>
#include <arch/arm/aarch32/cortex_m/cmsis.h>

#define MCG_IRCLK_DISABLE    0U  /*!< MCGIRCLK disabled */
#define PLLFLLSEL_MCGFLLCLK	(0)
#define PLLFLLSEL_MCGPLLCLK	(1)
#define PLLFLLSEL_IRC48MHZ	(3)

#define ER32KSEL_OSC32KCLK	(0)
#define ER32KSEL_RTC		(2)
#define ER32KSEL_LPO1KHZ	(3)

#define TIMESRC_OSCERCLK        (2)

#define SIM_OSC32KSEL_OSC32KCLK_CLK      0U
#define SIM_PLLFLLSEL_IRC48MCLK_CLK      3U
#define SIM_LPI2C_CLK_SEL_PLLFLLSEL_CLK  1U
#define SIM_USB_CLK_48000000HZ           48000000U

static const sim_clock_config_t simConfig = {
	.pllFllSel = SIM_PLLFLLSEL_IRC48MCLK_CLK,
	.er32kSrc = SIM_OSC32KSEL_OSC32KCLK_CLK,

	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CONFIG_KS22_CORE_CLOCK_DIVIDER - 1) |
			SIM_CLKDIV1_OUTDIV2(CONFIG_KS22_BUS_CLOCK_DIVIDER - 1) |
			SIM_CLKDIV1_OUTDIV4(CONFIG_KS22_FLASH_CLOCK_DIVIDER - 1),
};

/**
 *
 * @brief This function is used to delay for FLL stable.
 *
 * @return N/A
 *
 */
static void CLOCK_CONFIG_FllStableDelay(void)
{
    uint32_t i = 30000U;
    while (i--)
		__NOP();
}

/**
 *
 * @brief Initialize the system clock
 *
 * This routine will configure the multipurpose clock generator (MCG) to
 * set up the system clock.
 * The MCG has nine possible modes, including Stop mode.  This routine assumes
 * that the current MCG mode is FLL Engaged Internal (FEI), as from reset.
 * It transitions through the FLL Bypassed External (FBE) and
 * PLL Bypassed External (PBE) modes to get to the desired
 * PLL Engaged External (PEE) mode and generate the maximum 120 MHz system
 * clock.
 *
 * @return N/A
 *
 */
static ALWAYS_INLINE void clock_init(void)
{
	CLOCK_SetSimSafeDivs();

	//CLOCK_InitOsc0(&oscConfig);
	//CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);


	CLOCK_SetInternalRefClkConfig(MCG_IRCLK_DISABLE, kMCG_IrcSlow, 1);

	/* Configure FLL external reference divider (FRDIV). */
	//CLOCK_SetFllExtRefDiv(0);

	/* Set MCG to FEI mode. */
#if FSL_CLOCK_DRIVER_VERSION >= MAKE_VERSION(2, 2, 0)
    CLOCK_BootToFeiMode(kMCG_Dmx32Fine, kMCG_DrsMidHigh,
		CLOCK_CONFIG_FllStableDelay);
#else
	CLOCK_BootToFeiMode(kMCG_DrsMidHigh, CLOCK_CONFIG_FllStableDelay);
#endif

	/* Selects 48 MHz IRC Oscillator */
	CLOCK_SetExternalRefClkConfig(kMCG_OscselIrc);
    /* Set the clock configuration in SIM module. */
	CLOCK_SetSimConfig(&simConfig);

#if CONFIG_USB_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0,
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
#endif

	/* Enable USB FS clock. */
	//CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, SIM_USB_CLK_48000000HZ);
	//CLOCK_SetLpi2c0Clock(1);
	/* Set LPI2C0 clock source. */
	CLOCK_SetLpi2c0Clock(SIM_LPI2C_CLK_SEL_PLLFLLSEL_CLK);
	//CLOCK_SetPllFllSelClock(3U); /* IRC48 MHz clock. */

	CLOCK_SetLpuart0Clock(SIM_LPI2C_CLK_SEL_PLLFLLSEL_CLK);
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

static int fsl_frdm_ks22f_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */
#if !defined(CONFIG_ARM_MPU)
#if defined(SYSMPU)
	u32_t temp_reg;
#endif
#endif /* !CONFIG_ARM_MPU */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* release I/O power hold to allow normal run state */
	PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

#if !defined(CONFIG_ARM_MPU)
	/*
	 * Disable memory protection and clear slave port errors.
	 * Note that the KS22F does not implement the optional ARMv7-M memory
	 * protection unit (MPU), specified by the architecture (PMSAv7), in the
	 * Cortex-M4 core.  Instead, the processor includes its own MPU module.
	 */
#if defined(SYSMPU)
	temp_reg = SYSMPU->CESR;
	temp_reg &= ~SYSMPU_CESR_VLD_MASK;
	temp_reg |= SYSMPU_CESR_SPERR_MASK;
	SYSMPU->CESR = temp_reg;
#endif
#endif /* !CONFIG_ARM_MPU */

	/* Initialize clock */
	clock_init();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(fsl_frdm_ks22f_init, PRE_KERNEL_1, 0);
