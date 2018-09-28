/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <linker/sections.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <arch/cpu.h>

#define LPSCI0SRC_MCGFLLCLK	(1)

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*
 * KL25Z Flash configuration fields
 * These 16 bytes, which must be loaded to address 0x400, include default
 * protection and security settings.
 * They are loaded at reset to various Flash Memory module (FTFE) registers.
 *
 * The structure is:
 * -Backdoor Comparison Key for unsecuring the MCU - 8 bytes
 * -Program flash protection bytes, 4 bytes, written to FPROT0-3
 * -Flash security byte, 1 byte, written to FSEC
 * -Flash nonvolatile option byte, 1 byte, written to FOPT
 * -Reserved, 1 byte, (Data flash protection byte for FlexNVM)
 * -Reserved, 1 byte, (EEPROM protection byte for FlexNVM)
 *
 */
u8_t __kinetis_flash_config_section __kinetis_flash_config[] = {
	/* Backdoor Comparison Key (unused) */
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	/* Program flash protection; 1 bit/region - 0=protected, 1=unprotected
	 */
	0xFF, 0xFF, 0xFF, 0xFF,
	/*
	 * Flash security: Backdoor key disabled, Mass erase enabled,
	 *                 Factory access enabled, MCU is unsecure
	 */
	0xFE,
	/* Flash nonvolatile option: NMI enabled, EzPort enabled, Normal boot */
	0xFF,
	/* Reserved for FlexNVM feature (unsupported by this MCU) */
	0xFF, 0xFF};

static ALWAYS_INLINE void clkInit(void)
{
   /*
    * Core clock: 48MHz
    * Bus clock: 24MHz
    */
	const mcg_pll_config_t pll0Config = {
	.enableMode = 0U, .prdiv = CONFIG_MCG_PRDIV0, .vdiv = CONFIG_MCG_VDIV0,
	};
	const sim_clock_config_t simConfig = {
		.pllFllSel = 1U,        /* PLLFLLSEL select PLL. */
		.er32kSrc = 3U,         /* ERCLK32K selection, use LPO. */
		.clkdiv1 = 0x10010000U, /* SIM_CLKDIV1. */
	};

	const osc_config_t oscConfig = {.freq = CONFIG_OSC_XTAL0_FREQ,
				.capLoad = 0,
#if defined(CONFIG_OSC_EXTERNAL)
				.workMode = kOSC_ModeExt,
#elif defined(CONFIG_OSC_LOW_POWER)
				.workMode = kOSC_ModeOscLowPower,
#elif defined(CONFIG_OSC_HIGH_GAIN)
				.workMode = kOSC_ModeOscHighGain,
#else
#error "An oscillator mode must be defined"
#endif
				.oscerConfig = {
				.enableMode = kOSC_ErClkEnable,
#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) &&	\
	FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
				.erclkDiv = 0U,
#endif
				} };

	CLOCK_SetSimSafeDivs();
	CLOCK_InitOsc0(&oscConfig);

	/* Passing the XTAL0 frequency to clock driver. */
	CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);

	CLOCK_BootToPeeMode(kMCG_OscselOsc, kMCG_PllClkSelPll0, &pll0Config);

	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcSlow, 0);
	CLOCK_SetSimConfig(&simConfig);

#ifdef CONFIG_UART_MCUX_LPSCI_0
	CLOCK_SetLpsci0Clock(LPSCI0SRC_MCGFLLCLK);
#endif
#if CONFIG_USB_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0,
				CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);
#endif
}

static int kl2x_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* Disable the watchdog */
	SIM->COPC = 0;

	/* Initialize system clock to 48 MHz */
	clkInit();

	/*
	 * install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	/* restore interrupt state */
	irq_unlock(oldLevel);
	return 0;
}

SYS_INIT(kl2x_init, PRE_KERNEL_1, 0);
