/*
 * Copyright 2020-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_syscon
#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <soc.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static int mcux_lpc_syscon_clock_control_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
#if defined(CONFIG_CAN_MCUX_MCAN)
	if ((uint32_t)sub_system == MCUX_MCAN_CLK) {
		CLOCK_EnableClock(kCLOCK_Mcan);
	}
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */
#if defined(CONFIG_COUNTER_NXP_MRT)
	if ((uint32_t)sub_system == MCUX_MRT_CLK) {
#if defined(CONFIG_SOC_FAMILY_LPC)
		CLOCK_EnableClock(kCLOCK_Mrt);
#elif defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
		CLOCK_EnableClock(kCLOCK_Mrt0);
#endif
	}
#endif /* defined(CONFIG_COUNTER_NXP_MRT) */

#if defined(CONFIG_PINCTRL_NXP_KINETIS)
	switch ((uint32_t)sub_system) {
	case MCUX_PORT0_CLK:
		CLOCK_EnableClock(kCLOCK_Port0);
		break;
	case MCUX_PORT1_CLK:
		CLOCK_EnableClock(kCLOCK_Port1);
		break;
	case MCUX_PORT2_CLK:
		CLOCK_EnableClock(kCLOCK_Port2);
		break;
	case MCUX_PORT3_CLK:
		CLOCK_EnableClock(kCLOCK_Port3);
		break;
	case MCUX_PORT4_CLK:
		CLOCK_EnableClock(kCLOCK_Port4);
		break;
	default:
		break;
	}
#endif /* defined(CONFIG_PINCTRL_NXP_KINETIS) */

	return 0;
}

static int mcux_lpc_syscon_clock_control_off(const struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_lpc_syscon_clock_control_get_subsys_rate(
					const struct device *dev,
				    clock_control_subsys_t sub_system,
				    uint32_t *rate)
{
	uint32_t clock_name = (uint32_t) sub_system;

	switch (clock_name) {

#if defined(CONFIG_I2C_MCUX_FLEXCOMM) || \
		defined(CONFIG_SPI_MCUX_FLEXCOMM) || \
		defined(CONFIG_UART_MCUX_FLEXCOMM)
	case MCUX_FLEXCOMM0_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(0);
		break;
	case MCUX_FLEXCOMM1_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(1);
		break;
	case MCUX_FLEXCOMM2_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(2);
		break;
	case MCUX_FLEXCOMM3_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(3);
		break;
	case MCUX_FLEXCOMM4_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(4);
		break;
	case MCUX_FLEXCOMM5_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(5);
		break;
	case MCUX_FLEXCOMM6_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(6);
		break;
	case MCUX_FLEXCOMM7_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(7);
		break;
	case MCUX_FLEXCOMM8_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(8);
		break;
	case MCUX_FLEXCOMM9_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(9);
		break;
	case MCUX_FLEXCOMM10_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(10);
		break;
	case MCUX_FLEXCOMM11_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(11);
		break;
	case MCUX_FLEXCOMM12_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(12);
		break;
	case MCUX_FLEXCOMM13_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(13);
		break;
	case MCUX_PMIC_I2C_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(15);
		break;
	case MCUX_HS_SPI_CLK:
#if defined(SYSCON_HSLSPICLKSEL_SEL_MASK)
		*rate = CLOCK_GetHsLspiClkFreq();
#else
		*rate = CLOCK_GetFlexCommClkFreq(14);
#endif
		break;
	case MCUX_HS_SPI1_CLK:
		*rate = CLOCK_GetFlexCommClkFreq(16);
		break;
#elif defined(CONFIG_NXP_LP_FLEXCOMM)
	case MCUX_FLEXCOMM0_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(0);
		break;
	case MCUX_FLEXCOMM1_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(1);
		break;
	case MCUX_FLEXCOMM2_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(2);
		break;
	case MCUX_FLEXCOMM3_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(3);
		break;
	case MCUX_FLEXCOMM4_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(4);
		break;
	case MCUX_FLEXCOMM5_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(5);
		break;
	case MCUX_FLEXCOMM6_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(6);
		break;
	case MCUX_FLEXCOMM7_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(7);
		break;
	case MCUX_FLEXCOMM8_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(8);
		break;
	case MCUX_FLEXCOMM9_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(9);
		break;

#endif

#if (defined(FSL_FEATURE_SOC_USDHC_COUNT) && FSL_FEATURE_SOC_USDHC_COUNT)

#if CONFIG_SOC_FAMILY_NXP_MCX
	case MCUX_USDHC1_CLK:
		*rate = CLOCK_GetUsdhcClkFreq();
		break;
#else
	case MCUX_USDHC1_CLK:
		*rate = CLOCK_GetSdioClkFreq(0);
		break;
	case MCUX_USDHC2_CLK:
		*rate = CLOCK_GetSdioClkFreq(1);
		break;
#endif

#endif

#if (defined(FSL_FEATURE_SOC_SDIF_COUNT) && \
	(FSL_FEATURE_SOC_SDIF_COUNT)) && \
	CONFIG_MCUX_SDIF
	case MCUX_SDIF_CLK:
		*rate = CLOCK_GetSdioClkFreq();
		break;
#endif

#if defined(CONFIG_CAN_MCUX_MCAN)
	case MCUX_MCAN_CLK:
		*rate = CLOCK_GetMCanClkFreq();
		break;
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

#if defined(CONFIG_COUNTER_MCUX_CTIMER) || defined(CONFIG_PWM_MCUX_CTIMER)
	case MCUX_CTIMER0_CLK:
		*rate = CLOCK_GetCTimerClkFreq(0);
		break;
	case MCUX_CTIMER1_CLK:
		*rate = CLOCK_GetCTimerClkFreq(1);
		break;
	case MCUX_CTIMER2_CLK:
		*rate = CLOCK_GetCTimerClkFreq(2);
		break;
	case MCUX_CTIMER3_CLK:
		*rate = CLOCK_GetCTimerClkFreq(3);
		break;
	case MCUX_CTIMER4_CLK:
		*rate = CLOCK_GetCTimerClkFreq(4);
		break;
#endif

#if defined(CONFIG_COUNTER_NXP_MRT)
	case MCUX_MRT_CLK:
#endif
#if defined(CONFIG_PWM_MCUX_SCTIMER)
	case MCUX_SCTIMER_CLK:
#endif

#ifndef CONFIG_SOC_SERIES_RW6XX
	case MCUX_BUS_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_BusClk);
		break;
#endif

#if defined(CONFIG_I3C_MCUX)
	case MCUX_I3C_CLK:
		*rate = CLOCK_GetI3cClkFreq();
		break;
#endif

#if defined(CONFIG_MIPI_DSI_MCUX_2L)
	case MCUX_MIPI_DSI_DPHY_CLK:
		*rate = CLOCK_GetMipiDphyClkFreq();
		break;
	case MCUX_MIPI_DSI_ESC_CLK:
		*rate = CLOCK_GetMipiDphyEscTxClkFreq();
		break;
	case MCUX_LCDIF_PIXEL_CLK:
		*rate = CLOCK_GetDcPixelClkFreq();
		break;
#endif
#if defined(CONFIG_AUDIO_DMIC_MCUX)
	case MCUX_DMIC_CLK:
		*rate = CLOCK_GetDmicClkFreq();
		break;
#endif
	}

	return 0;
}

static const struct clock_control_driver_api mcux_lpc_syscon_api = {
	.on = mcux_lpc_syscon_clock_control_on,
	.off = mcux_lpc_syscon_clock_control_off,
	.get_rate = mcux_lpc_syscon_clock_control_get_subsys_rate,
};

#define LPC_CLOCK_INIT(n) \
	\
DEVICE_DT_INST_DEFINE(n, \
		    NULL, \
		    NULL, \
		    NULL, NULL, \
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, \
		    &mcux_lpc_syscon_api);

DT_INST_FOREACH_STATUS_OKAY(LPC_CLOCK_INIT)
