/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <init.h>
#include <soc.h>
#include <drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <arch/cpu.h>
#include <cortex_m/exc.h>

#define PLLFLLSEL_MCGFLLCLK	(0)
#define PLLFLLSEL_MCGPLLCLK	(1)
#define PLLFLLSEL_IRC48MHZ	(3)

#define ER32KSEL_OSC32KCLK	(0)
#define ER32KSEL_RTC		(2)
#define ER32KSEL_LPO1KHZ	(3)

#define TIMESRC_OSCERCLK        (2)

static const osc_config_t oscConfig = {
	.freq = CONFIG_OSC_XTAL0_FREQ,
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
		.enableMode = kOSC_ErClkEnable | kOSC_ErClkEnableInStop,
#if (defined(FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER) && \
	FSL_FEATURE_OSC_HAS_EXT_REF_CLOCK_DIVIDER)
		.erclkDiv = 0U,
#endif
	},
};

static const mcg_pll_config_t pll0Config = {
	.enableMode = 0U,
	.prdiv = CONFIG_MCG_PRDIV0,
	.vdiv = CONFIG_MCG_VDIV0,
};

static const sim_clock_config_t simConfig = {
	.pllFllSel = PLLFLLSEL_MCGPLLCLK, /* PLLFLLSEL select PLL. */
	.er32kSrc = ER32KSEL_RTC,         /* ERCLK32K selection, use RTC. */
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CONFIG_KW2XD_CORE_CLOCK_DIVIDER - 1) |
		   SIM_CLKDIV1_OUTDIV2(CONFIG_KW2XD_BUS_CLOCK_DIVIDER - 1) |
		   SIM_CLKDIV1_OUTDIV4(CONFIG_KW2XD_FLASH_CLOCK_DIVIDER - 1),
};

/**
 *
 * @brief Initialize radio transceiver clock output
 *
 * The clock output of the transceiver can be used as an input clock
 * for the PLL of the SoC. The clock output (CLK_OUT) is internally connected
 * to the input pin EXTAL0 of the SoC. This routine will initialize the clock
 * output of the transceiver at 4 MHz. The default frequency of the CLK_OUT
 * depends on the state of GPIO5 during transceiver reset. The frequency
 * will be 4 MHz if the GPIO5 pin is low, otherwise it will be 32.78689 kHz.
 *
 * @return N/A
 *
 */
static void set_modem_clock(void)
{
	/* Ungate PORTB and PORTC clock */
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK;
	/* Set PORTB.19 as output - modem RESET pin */
	GPIOB->PDDR |= 0x00080000u;
	/* Set PORTC.0 as output - modem GPIO5 pin */
	GPIOC->PDDR |= 0x00000001u;
	/* PORTB.19 as GPIO */
	PORTB->PCR[19] = (PORTB->PCR[19] & ~PORT_PCR_MUX_MASK) |
			 PORT_PCR_MUX(0x01u);
	/* PORTC.0 as GPIO */
	PORTC->PCR[0] = (PORTC->PCR[0] & ~PORT_PCR_MUX_MASK) |
			PORT_PCR_MUX(0x01u);
	/* Clear modem GPIO5 pin */
	GPIOC->PCOR = 0x00000001u;
	/* Clear modem RESET pin */
	GPIOB->PCOR = 0x00080000u;
	/* Set modem RESET pin */
	GPIOB->PSOR = 0x00080000u;
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
 * PLL Engaged External (PEE) mode and generate the maximum 48 MHz system
 * clock.
 *
 * @return N/A
 *
 */
static ALWAYS_INLINE void clock_init(void)
{
	CLOCK_SetSimSafeDivs();

	set_modem_clock();

	CLOCK_InitOsc0(&oscConfig);
	CLOCK_SetXtal0Freq(CONFIG_OSC_XTAL0_FREQ);

	CLOCK_BootToPeeMode(kMCG_OscselOsc, kMCG_PllClkSelPll0, &pll0Config);

	CLOCK_SetInternalRefClkConfig(kMCG_IrclkEnable, kMCG_IrcSlow,
				      CONFIG_MCG_FCRDIV);

	CLOCK_SetSimConfig(&simConfig);
#if CONFIG_USB_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0,
				DT_ARM_CORTEX_M4_0_CLOCK_FREQUENCY);
#endif
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
static int kw2xd_init(struct device *arg)
{
	ARG_UNUSED(arg);

	unsigned int oldLevel; /* old interrupt lock level */

	/* disable interrupts */
	oldLevel = irq_lock();

	/* release I/O power hold to allow normal run state */
	PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

	/* Initialize PLL/system clock to 48 MHz */
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

SYS_INIT(kw2xd_init, PRE_KERNEL_1, 0);
