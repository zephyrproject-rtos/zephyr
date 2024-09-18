/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright (c) 2017, Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/uart.h>
#include <fsl_common.h>
#include <fsl_clock.h>

#include <cmsis_core.h>

#define PLLFLLSEL_MCGFLLCLK	(0)
#define PLLFLLSEL_MCGPLLCLK	(1)
#define PLLFLLSEL_IRC48MHZ	(3)

#define ER32KSEL_OSC32KCLK	(0)
#define ER32KSEL_RTC		(2)
#define ER32KSEL_LPO1KHZ	(3)

#define TIMESRC_OSCERCLK        (2)

#define CLOCK_NODEID(clk) \
	DT_CHILD(DT_INST(0, nxp_kinetis_sim), clk)

#define CLOCK_DIVIDER(clk) \
	DT_PROP_OR(CLOCK_NODEID(clk), clock_div, 1) - 1

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
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CLOCK_DIVIDER(core_clk)) |
		   SIM_CLKDIV1_OUTDIV2(CLOCK_DIVIDER(bus_clk)) |
		   SIM_CLKDIV1_OUTDIV4(CLOCK_DIVIDER(flash_clk)),
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
#if CONFIG_USB_KINETIS || CONFIG_UDC_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcPll0,
				DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency));
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
static int kw2xd_init(void)
{
	/* release I/O power hold to allow normal run state */
	PMC->REGSC |= PMC_REGSC_ACKISO_MASK;

	/* Initialize PLL/system clock to 48 MHz */
	clock_init();

	return 0;
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */

SYS_INIT(kw2xd_init, PRE_KERNEL_1, 0);
