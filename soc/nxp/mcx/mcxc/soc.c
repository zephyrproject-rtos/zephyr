/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <zephyr/arch/cpu.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define IRC48M_CLK_FREQ (48000000UL)

#define MCG_NODE DT_NODELABEL(mcg)
#define OSC_NODE DT_NODELABEL(osc)

#define SIM_MODULE_CLK_SEL_DISABLED     0U /*!< Module clock select: Disabled */
#define SIM_MODULE_CLK_SEL_IRC48M_CLK   1U /*!< Module clock select: IRC48M clock */
#define SIM_MODULE_CLK_SEL_OSCERCLK_CLK 2U /*!< Module clock select: OSCERCLK clock */
#define SIM_MODULE_CLK_SEL_MCGIRCLK_CLK 3U /*!< Module clock select: MCGIRCLK clock */

#define CLOCK_NODEID(clk) DT_CHILD(DT_INST(0, nxp_kinetis_sim), clk)

#define CLOCK_DIVIDER(clk) DT_PROP_OR(CLOCK_NODEID(clk), clock_div, 1) - 1

#define LPUART_CLOCK_SEL(label) \
	(DT_PHA(DT_NODELABEL(label), clocks, name) == kCLOCK_McgIrc48MClk \
		 ? SIM_MODULE_CLK_SEL_IRC48M_CLK                                                   \
	 : DT_PHA(DT_NODELABEL(label), clocks, name) == kCLOCK_Osc0ErClk \
		 ? SIM_MODULE_CLK_SEL_OSCERCLK_CLK                                                 \
	 : DT_PHA(DT_NODELABEL(label), clocks, name) == kCLOCK_McgInternalRefClk \
		 ? SIM_MODULE_CLK_SEL_MCGIRCLK_CLK                                                 \
		 : SIM_MODULE_CLK_SEL_DISABLED)

#define TPM_CLOCK_SEL(node_id)                                                                     \
	(DT_PHA(node_id, clocks, name) == kCLOCK_McgIrc48MClk ? SIM_MODULE_CLK_SEL_IRC48M_CLK      \
	 : DT_PHA(node_id, clocks, name) == kCLOCK_Osc0ErClk  ? SIM_MODULE_CLK_SEL_OSCERCLK_CLK    \
	 : DT_PHA(node_id, clocks, name) == kCLOCK_McgInternalRefClk                               \
		 ? SIM_MODULE_CLK_SEL_MCGIRCLK_CLK                                                 \
		 : SIM_MODULE_CLK_SEL_DISABLED)

/*******************************************************************************
 * Variables
 ******************************************************************************/

const mcglite_config_t mcgliteConfig_BOARD_BootClockRUN = {
	.outSrc = kMCGLITE_ClkSrcHirc, /* MCGOUTCLK source is HIRC */
	.irclkEnableMode = kMCGLITE_IrclkEnable, /* MCGIRCLK enabled */
	.ircs = kMCGLITE_Lirc8M, /* Slow internal reference (LIRC) 8 MHz clock */
	/* Low-frequency Reference Clock Divider */
	.fcrdiv = DT_PROP_OR(MCG_NODE, fcrdiv, 0),
	/* Second Low-frequency Reference Clock Divider */
	.lircDiv2 = DT_PROP_OR(MCG_NODE, lircdiv2, 0),
	.hircEnableInNotHircMode = true, /* HIRC source is enabled */
};

const sim_clock_config_t simConfig_BOARD_BootClockRUN = {
	.er32kSrc = DT_PROP(DT_INST(0, nxp_kinetis_sim), er32k_select),
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CLOCK_DIVIDER(core_clk)) |
		   SIM_CLKDIV1_OUTDIV4(CLOCK_DIVIDER(flash_clk)),
};

const osc_config_t oscConfig_BOARD_BootClockRUN = {
	.freq = DT_PROP(OSC_NODE, clock_frequency),
	.capLoad = 0,
#if DT_ENUM_HAS_VALUE(OSC_NODE, mode, external)
	.workMode = kOSC_ModeExt,
#elif DT_ENUM_HAS_VALUE(OSC_NODE, mode, low_power)
	.workMode = kOSC_ModeOscLowPower,
#elif DT_ENUM_HAS_VALUE(OSC_NODE, mode, high_gain)
	.workMode = kOSC_ModeOscHighGain,
#else
	#error "An oscillator mode must be defined"
#endif
	.oscerConfig = {
		.enableMode = kOSC_ErClkEnable,
	}
};

static void clock_init(void)
{
	/* Set the system clock dividers in SIM to safe value. */
	CLOCK_SetSimSafeDivs();
	/* Initializes OSC0 according to board configuration. */
	CLOCK_InitOsc0(&oscConfig_BOARD_BootClockRUN);
	CLOCK_SetXtal0Freq(oscConfig_BOARD_BootClockRUN.freq);
	/* Set MCG to HIRC mode. */
	CLOCK_SetMcgliteConfig(&mcgliteConfig_BOARD_BootClockRUN);
	/* Set the clock configuration in SIM module. */
	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
	/* Set SystemCoreClock variable. */
	SystemCoreClock = DT_PROP(DT_NODELABEL(cpu0), clock_frequency);
	/* Set LPUART0 clock source. */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	CLOCK_SetLpuart0Clock(LPUART_CLOCK_SEL(lpuart0));
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_tpm)
	/* All TPM instances share common clock source for counter clock.
	 * Select the clock source using an arbitrary enabled TPM node.
	 * All TPM nodes should use the same clock source in device tree.
	 */
	CLOCK_SetTpmClock(TPM_CLOCK_SEL(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_kinetis_tpm)));
#endif
#if CONFIG_USB_KINETIS || CONFIG_UDC_KINETIS
	CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, IRC48M_CLK_FREQ);
#endif
}

void soc_early_init_hook(void)
{
#ifdef CONFIG_TEMP_KINETIS
	/* enable bandgap buffer */
	PMC->REGSC |= PMC_REGSC_BGBE_MASK;
#endif /* CONFIG_TEMP_KINETIS */

	clock_init();
}

#ifdef CONFIG_SOC_RESET_HOOK

void soc_reset_hook(void)
{
	SystemInit();
}

#endif /* CONFIG_SOC_RESET_HOOK */
