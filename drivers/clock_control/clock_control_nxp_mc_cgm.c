/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mc_cgm

#include <errno.h>
#include <zephyr/drivers/clock_control/nxp_clock_controller_sources.h>
#include <zephyr/dt-bindings/clock/nxp_mc_cgm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
const fxosc_config_t fxosc_config = {.freqHz = NXP_FXOSC_FREQ,
				     .workMode = NXP_FXOSC_WORKMODE,
				     .startupDelay = NXP_FXOSC_DELAY,
				     .overdriveProtect = NXP_FXOSC_OVERDRIVE};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
const pll_config_t pll_config = {.workMode = NXP_PLL_WORKMODE,
				 .preDiv = NXP_PLL_PREDIV, /* PLL input clock predivider: 2 */
				 .postDiv = NXP_PLL_POSTDIV,
				 .multiplier = NXP_PLL_MULTIPLIER,
				 .fracLoopDiv = NXP_PLL_FRACLOOPDIV,
				 .stepSize = NXP_PLL_STEPSIZE,
				 .stepNum = NXP_PLL_STEPNUM,
				 .accuracy = NXP_PLL_ACCURACY,
				 .outDiv = NXP_PLL_OUTDIV_POINTER};
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
const clock_pcfs_config_t pcfs_config = {.maxAllowableIDDchange = NXP_PLL_MAXIDOCHANGE,
					 .stepDuration = NXP_PLL_STEPDURATION,
					 .clkSrcFreq = NXP_PLL_CLKSRCFREQ};
#endif

static int mc_cgm_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
#if defined(CONFIG_CAN_MCUX_FLEXCAN)
	switch ((uint32_t)sub_system) {
	case MCUX_FLEXCAN0_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan0);
		break;
	case MCUX_FLEXCAN1_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan1);
		break;
	case MCUX_FLEXCAN2_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan2);
		break;
	case MCUX_FLEXCAN3_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan3);
		break;
	case MCUX_FLEXCAN4_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan4);
		break;
	case MCUX_FLEXCAN5_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan5);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

#if defined(CONFIG_UART_MCUX_LPUART)
	switch ((uint32_t)sub_system) {
	case MCUX_LPUART0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart0);
		break;
	case MCUX_LPUART1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart1);
		break;
	case MCUX_LPUART2_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart2);
		break;
	case MCUX_LPUART3_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart3);
		break;
	case MCUX_LPUART4_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart4);
		break;
	case MCUX_LPUART5_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart5);
		break;
	case MCUX_LPUART6_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart6);
		break;
	case MCUX_LPUART7_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart7);
		break;
	case MCUX_LPUART8_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart8);
		break;
	case MCUX_LPUART9_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart9);
		break;
	case MCUX_LPUART10_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart10);
		break;
	case MCUX_LPUART11_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart11);
		break;
	case MCUX_LPUART12_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart12);
		break;
	case MCUX_LPUART13_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart13);
		break;
	case MCUX_LPUART14_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart14);
		break;
	case MCUX_LPUART15_CLK:
		CLOCK_EnableClock(kCLOCK_Lpuart15);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_UART_MCUX_LPUART) */

#if defined(CONFIG_SPI_NXP_LPSPI)
	switch ((uint32_t)sub_system) {
	case MCUX_LPSPI0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi0);
		break;
	case MCUX_LPSPI1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi1);
		break;
	case MCUX_LPSPI2_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi2);
		break;
	case MCUX_LPSPI3_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi3);
		break;
	case MCUX_LPSPI4_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi4);
		break;
	case MCUX_LPSPI5_CLK:
		CLOCK_EnableClock(kCLOCK_Lpspi5);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_SPI_NXP_LPSPI) */

#if defined(CONFIG_I2C_MCUX_LPI2C)
	switch ((uint32_t)sub_system) {
	case MCUX_LPI2C0_CLK:
		CLOCK_EnableClock(kCLOCK_Lpi2c0);
		break;
	case MCUX_LPI2C1_CLK:
		CLOCK_EnableClock(kCLOCK_Lpi2c1);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_I2C_MCUX_LPI2C) */

#if defined(CONFIG_COUNTER_MCUX_STM)
	switch ((uint32_t)sub_system) {
	case MCUX_STM0_CLK:
		CLOCK_EnableClock(kCLOCK_Stm0);
		break;
	case MCUX_STM1_CLK:
		CLOCK_EnableClock(kCLOCK_Stm1);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */

	return 0;
}

static int mc_cgm_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	return 0;
}

static int mc_cgm_get_subsys_rate(const struct device *dev, clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	uint32_t clock_name = (uint32_t)sub_system;

	switch (clock_name) {
#if defined(CONFIG_UART_MCUX_LPUART)
	case MCUX_LPUART0_CLK:
	case MCUX_LPUART8_CLK:
		*rate = CLOCK_GetAipsPlatClkFreq();
		break;
	case MCUX_LPUART1_CLK:
	case MCUX_LPUART2_CLK:
	case MCUX_LPUART3_CLK:
	case MCUX_LPUART4_CLK:
	case MCUX_LPUART5_CLK:
	case MCUX_LPUART6_CLK:
	case MCUX_LPUART7_CLK:
	case MCUX_LPUART9_CLK:
	case MCUX_LPUART10_CLK:
	case MCUX_LPUART11_CLK:
	case MCUX_LPUART12_CLK:
	case MCUX_LPUART13_CLK:
	case MCUX_LPUART14_CLK:
	case MCUX_LPUART15_CLK:
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_UART_MCUX_LPUART) */

#if defined(CONFIG_SPI_NXP_LPSPI)
	case MCUX_LPSPI0_CLK:
		*rate = CLOCK_GetAipsPlatClkFreq();
		break;
	case MCUX_LPSPI1_CLK:
	case MCUX_LPSPI2_CLK:
	case MCUX_LPSPI3_CLK:
	case MCUX_LPSPI4_CLK:
	case MCUX_LPSPI5_CLK:
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_SPI_NXP_LPSPI) */

#if defined(CONFIG_I2C_MCUX_LPI2C)
	case MCUX_LPI2C0_CLK:
	case MCUX_LPI2C1_CLK:
		*rate = CLOCK_GetAipsSlowClkFreq();
		break;
#endif /* defined(CONFIG_I2C_MCUX_LPI2C) */

#if defined(CONFIG_CAN_MCUX_FLEXCAN)
	case MCUX_FLEXCAN0_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(0);
		break;
	case MCUX_FLEXCAN1_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(1);
		break;
	case MCUX_FLEXCAN2_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(2);
		break;
	case MCUX_FLEXCAN3_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(3);
		break;
	case MCUX_FLEXCAN4_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(4);
		break;
	case MCUX_FLEXCAN5_CLK:
		*rate = CLOCK_GetFlexcanPeClkFreq(5);
		break;
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

#if defined(CONFIG_COUNTER_MCUX_STM)
	case MCUX_STM0_CLK:
		*rate = CLOCK_GetStmClkFreq(0);
		break;
	case MCUX_STM1_CLK:
		*rate = CLOCK_GetStmClkFreq(1);
		break;
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */
	}
	return 0;
}

static int mc_cgm_init(const struct device *dev)
{
#if defined(FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR) && (FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR)
	/* Enables PMC last mile regulator before enable PLL.  */
	if ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
		/* External bipolar junction transistor is connected between external voltage and
		 * V15 input pin.
		 */
		PMC->CONFIG |= PMC_CONFIG_LMBCTLEN_MASK;
	}
	while ((PMC->LVSC & PMC_LVSC_LVD15S_MASK) != 0U) {
	}
	PMC->CONFIG |= PMC_CONFIG_LMEN_MASK;
	while ((PMC->CONFIG & PMC_CONFIG_LMSTAT_MASK) == 0u) {
	}
#endif /* FSL_FEATURE_PMC_HAS_LAST_MILE_REGULATOR */

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(firc), nxp_firc, okay)
	/* Switch the FIRC_DIV_SEL to the desired diveder. */
	CLOCK_SetFircDiv(NXP_FIRC_DIV);
	/* Disable FIRC in standby mode. */
	CLOCK_DisableFircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sirc), nxp_sirc, okay)
	/* Disable SIRC in standby mode. */
	CLOCK_DisableSircInStandbyMode();
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(fxosc), nxp_fxosc, okay)
	/* Enable FXOSC. */
	CLOCK_InitFxosc(&fxosc_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(pll), nxp_plldig, okay)
	/* Enable PLL. */
	CLOCK_InitPll(&pll_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(mc_cgm), nxp_mc_cgm, okay)
	CLOCK_SelectSafeClock(kFIRC_CLK_to_MUX0);
	/* Configure MUX_0_CSC dividers */
	CLOCK_SetClkMux0DivTriggerType(KCLOCK_CommonTriggerUpdate);
	CLOCK_SetClkDiv(kCLOCK_DivCoreClk, NXP_PLL_MUX_0_DC_0_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsPlatClk, NXP_PLL_MUX_0_DC_1_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivAipsSlowClk, NXP_PLL_MUX_0_DC_2_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivHseClk, NXP_PLL_MUX_0_DC_3_DIV);
	CLOCK_SetClkDiv(kCLOCK_DivDcmClk, NXP_PLL_MUX_0_DC_4_DIV);
#ifdef MC_CGM_MUX_0_DC_5_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivLbistClk, NXP_PLL_MUX_0_DC_5_DIV);
#endif
#ifdef MC_CGM_MUX_0_DC_6_DIV_MASK
	CLOCK_SetClkDiv(kCLOCK_DivQspiClk, NXP_PLL_MUX_0_DC_6_DIV);
#endif
	CLOCK_CommonTriggerClkMux0DivUpdate();
	CLOCK_ProgressiveClockFrequencySwitch(kPLL_PHI0_CLK_to_MUX0, &pcfs_config);
#if defined(CONFIG_COUNTER_MCUX_STM)
	CLOCK_SetClkDiv(kCLOCK_DivStm0Clk, NXP_PLL_MUX_1_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM0);
#if defined(FSL_FEATURE_SOC_STM_COUNT) && (FSL_FEATURE_SOC_STM_COUNT == 2U)
	CLOCK_SetClkDiv(kCLOCK_DivStm1Clk, NXP_PLL_MUX_2_DC_0_DIV);
	CLOCK_AttachClk(kAIPS_PLAT_CLK_to_STM1);
#endif /* FSL_FEATURE_SOC_STM_COUNT == 2U */
#endif /* defined(CONFIG_COUNTER_MCUX_STM) */
#endif

	/* Set SystemCoreClock variable. */
	SystemCoreClockUpdate();

	return 0;
}

static DEVICE_API(clock_control, mcux_mcxe31x_clock_api) = {
	.on = mc_cgm_clock_control_on,
	.off = mc_cgm_clock_control_off,
	.get_rate = mc_cgm_get_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, mc_cgm_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mcux_mcxe31x_clock_api);
