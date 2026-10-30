/*
 * Copyright 2024, 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_clock.h>
#include <zephyr/dt-bindings/clock/kinetis_sim.h>
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

/* MCGOUTCLK as described by the MCG-Lite node */
#if DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, hirc)
#define MCG_LITE_OUT_SRC  kMCGLITE_ClkSrcHirc
#define MCG_LITE_OUT_FREQ IRC48M_CLK_FREQ
#elif DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, lirc)
#define MCG_LITE_OUT_SRC  kMCGLITE_ClkSrcLirc
#define MCG_LITE_OUT_FREQ (DT_PROP(MCG_NODE, lirc_frequency) >> DT_PROP(MCG_NODE, fcrdiv))
#elif DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, ext)
#if !DT_NODE_HAS_PROP(MCG_NODE, clocks)
#error "MCG-Lite EXT mode needs clocks = <&osc> on the mcg node"
#elif !DT_SAME_NODE(DT_CLOCKS_CTLR(MCG_NODE), OSC_NODE)
#error "MCG-Lite EXT mode only supports the osc node as clock input"
#endif
#define MCG_LITE_OUT_SRC  kMCGLITE_ClkSrcExt
#define MCG_LITE_OUT_FREQ DT_PROP(OSC_NODE, clock_frequency)
#endif

/* HIRC feeds MCGPCLK only while it is enabled or drives MCGOUTCLK */
#define MCG_LITE_HIRC_ON                                                                           \
	(DT_PROP(MCG_NODE, hirc_enable) || DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, hirc))
/* Same for LIRC and MCGIRCLK */
#define MCG_LITE_LIRC_ON                                                                           \
	(DT_PROP(MCG_NODE, lirc_enable) || DT_ENUM_HAS_VALUE(MCG_NODE, clock_source, lirc))

BUILD_ASSERT(MCG_LITE_OUT_FREQ > 0, "MCGOUTCLK frequency is 0, check the osc clock-frequency");
BUILD_ASSERT(!DT_PROP(MCG_NODE, lirc_stop_enable) || DT_PROP(MCG_NODE, lirc_enable),
	     "lirc-stop-enable has no effect without lirc-enable");
BUILD_ASSERT(CLOCK_DIVIDER(core_clk) <= 15, "core_clk clock-div must be 1..16");
BUILD_ASSERT(CLOCK_DIVIDER(flash_clk) <= 7, "flash_clk clock-div must be 1..8");

#define CPU_FREQ      DT_PROP(DT_NODELABEL(cpu0), clock_frequency)
#define CORE_CLK_DIV  DT_PROP(CLOCK_NODEID(core_clk), clock_div)
#define FLASH_CLK_DIV DT_PROP(CLOCK_NODEID(flash_clk), clock_div)
/* MCGOUTCLK actually seen by cpu0 after the core_clk divider */
#define CORE_CLK_FREQ ((uint64_t)CPU_FREQ * CORE_CLK_DIV)

BUILD_ASSERT(CORE_CLK_FREQ == MCG_LITE_OUT_FREQ,
	     "cpu0 clock-frequency does not match MCGOUTCLK / core_clk clock-div");
BUILD_ASSERT((uint64_t)CPU_FREQ <= 24000000ULL * FLASH_CLK_DIV, "Bus/flash clock above 24 MHz");

/* OSC_CR[SCxP] takes one bit per capacitor, not a value in pF */
#define OSC_CAP_LOAD(pf)                                                                           \
	((((pf) & 2) ? kOSC_Cap2P : 0U) | (((pf) & 4) ? kOSC_Cap4P : 0U) |                         \
	 (((pf) & 8) ? kOSC_Cap8P : 0U) | (((pf) & 16) ? kOSC_Cap16P : 0U))

/* Decode the clock name selector from the encoded `name` cell */
#define DT_CLOCK_NAME(node_id) \
	KINETIS_SIM_CLOCK_DECODE_NAME(DT_PHA(node_id, clocks, name))

#define LPUART_CLOCK_SEL(label) \
	(DT_CLOCK_NAME(DT_NODELABEL(label)) == kCLOCK_McgIrc48MClk \
		 ? SIM_MODULE_CLK_SEL_IRC48M_CLK                                                   \
	 : DT_CLOCK_NAME(DT_NODELABEL(label)) == kCLOCK_Osc0ErClk \
		 ? SIM_MODULE_CLK_SEL_OSCERCLK_CLK                                                 \
	 : DT_CLOCK_NAME(DT_NODELABEL(label)) == kCLOCK_McgInternalRefClk \
		 ? SIM_MODULE_CLK_SEL_MCGIRCLK_CLK                                                 \
		 : SIM_MODULE_CLK_SEL_DISABLED)

#define TPM_CLOCK_SEL(node_id)                                                                     \
	(DT_CLOCK_NAME(node_id) == kCLOCK_McgIrc48MClk ? SIM_MODULE_CLK_SEL_IRC48M_CLK      \
	 : DT_CLOCK_NAME(node_id) == kCLOCK_Osc0ErClk  ? SIM_MODULE_CLK_SEL_OSCERCLK_CLK    \
	 : DT_CLOCK_NAME(node_id) == kCLOCK_McgInternalRefClk                               \
		 ? SIM_MODULE_CLK_SEL_MCGIRCLK_CLK                                                 \
		 : SIM_MODULE_CLK_SEL_DISABLED)

/* A SOPT2 module clock selection is only useful if its source is running */
#define MODULE_CLK_SEL_RUNNING(sel)                                                                \
	((((sel) != SIM_MODULE_CLK_SEL_IRC48M_CLK) || MCG_LITE_HIRC_ON) &&                         \
	 (((sel) != SIM_MODULE_CLK_SEL_MCGIRCLK_CLK) || MCG_LITE_LIRC_ON))

/*******************************************************************************
 * Variables
 ******************************************************************************/

const mcglite_config_t mcgliteConfig_BOARD_BootClockRUN = {
	.outSrc = MCG_LITE_OUT_SRC,
	.irclkEnableMode = (DT_PROP(MCG_NODE, lirc_enable) ? kMCGLITE_IrclkEnable : 0U) |
			   (DT_PROP(MCG_NODE, lirc_stop_enable) ? kMCGLITE_IrclkEnableInStop : 0U),
	.ircs = DT_PROP(MCG_NODE, lirc_frequency) == 8000000 ? kMCGLITE_Lirc8M : kMCGLITE_Lirc2M,
	.fcrdiv = DT_PROP(MCG_NODE, fcrdiv),
	.lircDiv2 = DT_PROP(MCG_NODE, lircdiv2),
	.hircEnableInNotHircMode = DT_PROP(MCG_NODE, hirc_enable),
};

const sim_clock_config_t simConfig_BOARD_BootClockRUN = {
	.er32kSrc = DT_PROP(DT_INST(0, nxp_kinetis_sim), er32k_select),
	.clkdiv1 = SIM_CLKDIV1_OUTDIV1(CLOCK_DIVIDER(core_clk)) |
		   SIM_CLKDIV1_OUTDIV4(CLOCK_DIVIDER(flash_clk)),
};

const osc_config_t oscConfig_BOARD_BootClockRUN = {
	.freq = DT_PROP(OSC_NODE, clock_frequency),
	.capLoad = OSC_CAP_LOAD(DT_PROP_OR(OSC_NODE, load_capacitance_picofarads, 0)),
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
	}};

__weak void clock_init(void)
{
	/* Set the system clock dividers in SIM to safe value. */
	CLOCK_SetSimSafeDivs();
	/* Initializes OSC0 according to board configuration. */
	CLOCK_InitOsc0(&oscConfig_BOARD_BootClockRUN);
	CLOCK_SetXtal0Freq(oscConfig_BOARD_BootClockRUN.freq);
	/* Switch MCG-Lite to the clock source given in devicetree. */
	CLOCK_SetMcgliteConfig(&mcgliteConfig_BOARD_BootClockRUN);
	/* Set the clock configuration in SIM module. */
	CLOCK_SetSimConfig(&simConfig_BOARD_BootClockRUN);
	/* Set SystemCoreClock variable. */
	SystemCoreClock = DT_PROP(DT_NODELABEL(cpu0), clock_frequency);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart0))
	/* Set LPUART0 clock source. */
	BUILD_ASSERT(MODULE_CLK_SEL_RUNNING(LPUART_CLOCK_SEL(lpuart0)),
		     "lpuart0 clock source is not running, check the mcg node");
	CLOCK_SetLpuart0Clock(LPUART_CLOCK_SEL(lpuart0));
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(lpuart1))
	/* Set LPUART1 clock source. */
	BUILD_ASSERT(MODULE_CLK_SEL_RUNNING(LPUART_CLOCK_SEL(lpuart1)),
		     "lpuart1 clock source is not running, check the mcg node");
	CLOCK_SetLpuart1Clock(LPUART_CLOCK_SEL(lpuart1));
#endif
#if DT_HAS_COMPAT_STATUS_OKAY(nxp_kinetis_tpm)
	/* All TPM instances share common clock source for counter clock.
	 * Select the clock source using an arbitrary enabled TPM node.
	 * All TPM nodes should use the same clock source in device tree.
	 */
	BUILD_ASSERT(MODULE_CLK_SEL_RUNNING(
			     TPM_CLOCK_SEL(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_kinetis_tpm))),
		     "TPM clock source is not running, check the mcg node");
	CLOCK_SetTpmClock(TPM_CLOCK_SEL(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_kinetis_tpm)));
#endif
#if CONFIG_USB_KINETIS || CONFIG_UDC_KINETIS
	BUILD_ASSERT(MCG_LITE_HIRC_ON, "USB needs HIRC, set hirc-enable on the mcg node");
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
