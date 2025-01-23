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
#if defined(CONFIG_SOC_FAMILY_LPC) || defined(CONFIG_SOC_SERIES_RW6XX) ||                          \
	defined(CONFIG_SOC_SERIES_MCXN)
		CLOCK_EnableClock(kCLOCK_Mrt);
#elif defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
		CLOCK_EnableClock(kCLOCK_Mrt0);
#endif
	}
#if defined(CONFIG_SOC_SERIES_RW6XX)
	if ((uint32_t)sub_system == MCUX_FREEMRT_CLK) {
		CLOCK_EnableClock(kCLOCK_FreeMrt);
	}
#endif
#endif /* defined(CONFIG_COUNTER_NXP_MRT) */
#if defined(CONFIG_MIPI_DBI_NXP_LCDIC)
	if ((uint32_t)sub_system == MCUX_LCDIC_CLK) {
		CLOCK_EnableClock(kCLOCK_Lcdic);
	}
#endif

#if defined(CONFIG_PINCTRL_NXP_PORT)
	switch ((uint32_t)sub_system) {
#if defined(CONFIG_SOC_SERIES_MCXA)
	case MCUX_PORT0_CLK:
		CLOCK_EnableClock(kCLOCK_GatePORT0);
		break;
	case MCUX_PORT1_CLK:
		CLOCK_EnableClock(kCLOCK_GatePORT1);
		break;
	case MCUX_PORT2_CLK:
		CLOCK_EnableClock(kCLOCK_GatePORT2);
		break;
	case MCUX_PORT3_CLK:
		CLOCK_EnableClock(kCLOCK_GatePORT3);
		break;
#if (defined(FSL_FEATURE_SOC_PORT_COUNT) && (FSL_FEATURE_SOC_PORT_COUNT > 4))
	case MCUX_PORT4_CLK:
		CLOCK_EnableClock(kCLOCK_GatePORT4);
		break;
#endif /* defined(FSL_FEATURE_SOC_PORT_COUNT) */
#else
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
#endif /* defined(CONFIG_SOC_SERIES_MCXA) */
	default:
		break;
	}
#endif /* defined(CONFIG_PINCTRL_NXP_PORT) */

#ifdef CONFIG_ETH_NXP_ENET_QOS
	if ((uint32_t)sub_system == MCUX_ENET_QOS_CLK) {
		CLOCK_EnableClock(kCLOCK_Enet);
	}
#endif

#if defined(CONFIG_CAN_MCUX_FLEXCAN)
	switch ((uint32_t)sub_system) {
#if defined(CONFIG_SOC_SERIES_MCXA)
	case MCUX_FLEXCAN0_CLK:
		CLOCK_EnableClock(kCLOCK_GateFLEXCAN0);
		break;
#else
	case MCUX_FLEXCAN0_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan0);
		break;
	case MCUX_FLEXCAN1_CLK:
		CLOCK_EnableClock(kCLOCK_Flexcan1);
		break;
#endif /* defined(CONFIG_SOC_SERIES_MCXA) */
	default:
		break;
	}
#endif /* defined(CONFIG_CAN_MCUX_MCAN) */

#ifdef CONFIG_ETH_NXP_ENET
	if ((uint32_t)sub_system == MCUX_ENET_CLK) {
#ifdef CONFIG_SOC_SERIES_RW6XX
		CLOCK_EnableClock(kCLOCK_TddrMciEnetClk);
		CLOCK_EnableClock(kCLOCK_EnetIpg);
		CLOCK_EnableClock(kCLOCK_EnetIpgS);
#endif
	}
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rtc), okay)
#if CONFIG_SOC_SERIES_IMXRT5XX
	CLOCK_EnableOsc32K(true);
#elif CONFIG_SOC_SERIES_IMXRT6XX
	/* No configuration */
#else /* !CONFIG_SOC_SERIES_IMXRT5XX | !CONFIG_SOC_SERIES_IMXRT6XX */
/* 0x0 Clock Select Value Set IRTC to use FRO 16K Clk */
#if DT_PROP(DT_NODELABEL(rtc), clock_select) == 0x0
	CLOCK_SetupClk16KClocking(kCLOCK_Clk16KToVbat | kCLOCK_Clk16KToMain);
/* 0x1 Clock Select Value Set IRTC to use Osc 32K Clk */
#elif DT_PROP(DT_NODELABEL(rtc), clock_select) == 0x1
	CLOCK_SetupOsc32KClocking(kCLOCK_Osc32kToVbat | kCLOCK_Osc32kToMain);
#endif /* DT_PROP(DT_NODELABEL(rtc), clock_select) */
	CLOCK_EnableClock(kCLOCK_Rtc0);
#endif /* CONFIG_SOC_SERIES_IMXRT5XX */
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(rtc), okay) */

	return 0;
}

static int mcux_lpc_syscon_clock_control_off(const struct device *dev,
					     clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_lpc_syscon_clock_control_get_subsys_rate(const struct device *dev,
							 clock_control_subsys_t sub_system,
							 uint32_t *rate)
{
	uint32_t clock_name = (uint32_t)sub_system;

	switch (clock_name) {

#if defined(CONFIG_I2C_MCUX_FLEXCOMM) || defined(CONFIG_SPI_MCUX_FLEXCOMM) ||                      \
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

	case MCUX_FLEXCOMM10_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(10);
		break;

	case MCUX_FLEXCOMM11_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(11);
		break;

	case MCUX_FLEXCOMM12_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(12);
		break;

	case MCUX_FLEXCOMM13_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(13);
		break;

	case MCUX_FLEXCOMM17_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(17);
		break;

	case MCUX_FLEXCOMM18_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(18);
		break;

	case MCUX_FLEXCOMM19_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(19);
		break;

	case MCUX_FLEXCOMM20_CLK:
		*rate = CLOCK_GetLPFlexCommClkFreq(20);
		break;
#endif

		/* On RT7xx, flexcomm14 and 16 only can be LPSPI, flexcomm15 only can be I2C. */
#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
	case MCUX_LPSPI14_CLK:
		*rate = CLOCK_GetLPSpiClkFreq(14);
		break;
	case MCUX_LPI2C15_CLK:
		*rate = CLOCK_GetLPI2cClkFreq(15);
		break;
	case MCUX_LPSPI16_CLK:
		*rate = CLOCK_GetLPSpiClkFreq(16);
		break;
#endif

#if (defined(FSL_FEATURE_SOC_USDHC_COUNT) && FSL_FEATURE_SOC_USDHC_COUNT)

#if defined(CONFIG_SOC_SERIES_MCXN)
	case MCUX_USDHC1_CLK:
		*rate = CLOCK_GetUsdhcClkFreq();
		break;
#elif defined(CONFIG_SOC_SERIES_IMXRT7XX)
	case MCUX_USDHC1_CLK:
		*rate = CLOCK_GetUsdhcClkFreq(0);
		break;
	case MCUX_USDHC2_CLK:
		*rate = CLOCK_GetUsdhcClkFreq(1);
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

#if (defined(FSL_FEATURE_SOC_SDIF_COUNT) && (FSL_FEATURE_SOC_SDIF_COUNT)) && CONFIG_MCUX_SDIF
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
	case MCUX_CTIMER5_CLK:
		*rate = CLOCK_GetCTimerClkFreq(5);
		break;
	case MCUX_CTIMER6_CLK:
		*rate = CLOCK_GetCTimerClkFreq(6);
		break;
	case MCUX_CTIMER7_CLK:
		*rate = CLOCK_GetCTimerClkFreq(7);
		break;
#endif

#if defined(CONFIG_COUNTER_NXP_MRT)
	case MCUX_MRT_CLK:
#if defined(CONFIG_SOC_SERIES_RW6XX)
	case MCUX_FREEMRT_CLK:
#endif /* RW */
#endif /* MRT */
#if defined(CONFIG_PWM_MCUX_SCTIMER)
	case MCUX_SCTIMER_CLK:
#endif
#ifdef CONFIG_SOC_SERIES_RW6XX
		/* RW6XX uses core clock for SCTimer, not bus clock */
		*rate = CLOCK_GetCoreSysClkFreq();
		break;
#else
	case MCUX_BUS_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_BusClk);
		break;
#endif

#if defined(CONFIG_I3C_MCUX)
	case MCUX_I3C_CLK:
#if CONFIG_SOC_SERIES_MCXN
		*rate = CLOCK_GetI3cClkFreq(0);
#elif CONFIG_SOC_SERIES_MCXA
		*rate = CLOCK_GetI3CFClkFreq();
#else
		*rate = CLOCK_GetI3cClkFreq();
#endif
		break;
#if (FSL_FEATURE_SOC_I3C_COUNT == 2)
	case MCUX_I3C2_CLK:
#if CONFIG_SOC_SERIES_MCXN
		*rate = CLOCK_GetI3cClkFreq(1);
#else
		*rate = CLOCK_GetI3cClkFreq();
#endif
		break;
#endif

#endif /* CONFIG_I3C_MCUX */

#if defined(CONFIG_MIPI_DSI_MCUX_2L)
	case MCUX_MIPI_DSI_DPHY_CLK:
		*rate = CLOCK_GetMipiDphyClkFreq();
		break;
	case MCUX_MIPI_DSI_ESC_CLK:
		*rate = CLOCK_GetMipiDphyEscTxClkFreq();
		break;
	case MCUX_LCDIF_PIXEL_CLK:
#if defined(CONFIG_SOC_SERIES_IMXRT7XX) && defined(CONFIG_SOC_FAMILY_NXP_IMXRT)
		*rate = CLOCK_GetLcdifClkFreq();
#else
		*rate = CLOCK_GetDcPixelClkFreq();
#endif
		break;
#endif
#if defined(CONFIG_AUDIO_DMIC_MCUX)
	case MCUX_DMIC_CLK:
		*rate = CLOCK_GetDmicClkFreq();
		break;
#endif
#if defined(CONFIG_MEMC_MCUX_FLEXSPI)
	case MCUX_FLEXSPI_CLK:
#if (FSL_FEATURE_SOC_FLEXSPI_COUNT == 1)
		*rate = CLOCK_GetFlexspiClkFreq();
#else
		*rate = CLOCK_GetFlexspiClkFreq(0);
#endif
		break;
#if (FSL_FEATURE_SOC_FLEXSPI_COUNT == 2)
	case MCUX_FLEXSPI2_CLK:
		*rate = CLOCK_GetFlexspiClkFreq(1);
		break;
#endif
#endif /* CONFIG_MEMC_MCUX_FLEXSPI */

#if defined(CONFIG_I2S_MCUX_SAI)
	case MCUX_SAI0_CLK:
#if (FSL_FEATURE_SOC_I2S_COUNT == 1)
		*rate = CLOCK_GetSaiClkFreq();
#else
		*rate = CLOCK_GetSaiClkFreq(0);
#endif
		break;
#if (FSL_FEATURE_SOC_I2S_COUNT == 2)
	case MCUX_SAI1_CLK:
		*rate = CLOCK_GetSaiClkFreq(1);
		break;
#endif
#endif /* CONFIG_I2S_MCUX_SAI */

#ifdef CONFIG_ETH_NXP_ENET_QOS
	case MCUX_ENET_QOS_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_BusClk);
		break;
#endif

#ifdef CONFIG_ETH_NXP_ENET
	case MCUX_ENET_CLK:
#ifdef CONFIG_SOC_SERIES_RW6XX
		*rate = CLOCK_GetTddrMciEnetClkFreq();
#endif
		break;
#endif

#if defined(CONFIG_MIPI_DBI_NXP_LCDIC)
	case MCUX_LCDIC_CLK:
		*rate = CLOCK_GetLcdClkFreq();
		break;
#endif

#if defined(CONFIG_ADC_MCUX_LPADC)
	case MCUX_LPADC1_CLK:
#if (FSL_FEATURE_SOC_LPADC_COUNT == 1)
		*rate = CLOCK_GetAdcClkFreq();
#else
		*rate = CLOCK_GetAdcClkFreq(0);
#endif
		break;
#if (FSL_FEATURE_SOC_LPADC_COUNT == 2)
	case MCUX_LPADC2_CLK:
		*rate = CLOCK_GetAdcClkFreq(1);
		break;
#endif
#endif /* CONFIG_ADC_MCUX_LPADC */

#if defined(CONFIG_CAN_MCUX_FLEXCAN)
#if defined(CONFIG_SOC_SERIES_MCXA)
	case MCUX_FLEXCAN0_CLK:
		*rate = CLOCK_GetFlexcanClkFreq();
		break;
#else
	case MCUX_FLEXCAN0_CLK:
		*rate = CLOCK_GetFlexcanClkFreq(0);
		break;
	case MCUX_FLEXCAN1_CLK:
		*rate = CLOCK_GetFlexcanClkFreq(1);
		break;
#endif /* defined(CONFIG_SOC_SERIES_MCXA) */
#endif /* defined(CONFIG_CAN_MCUX_FLEXCAN) */

#if defined(CONFIG_MCUX_FLEXIO)
	case MCUX_FLEXIO0_CLK:
		*rate = CLOCK_GetFlexioClkFreq();
		break;
#endif /* defined(CONFIG_MCUX_FLEXIO) */

#if defined(CONFIG_I2S_MCUX_FLEXCOMM)
	case MCUX_AUDIO_MCLK:
		*rate = CLOCK_GetMclkClkFreq();
		break;
#endif /* defined(CONFIG_I2S_MCUX_FLEXCOMM) */

#if (defined(CONFIG_UART_MCUX_LPUART) && CONFIG_SOC_SERIES_MCXA)
	case MCUX_LPUART0_CLK:
		*rate = CLOCK_GetLpuartClkFreq(0);
		break;
	case MCUX_LPUART1_CLK:
		*rate = CLOCK_GetLpuartClkFreq(1);
		break;
	case MCUX_LPUART2_CLK:
		*rate = CLOCK_GetLpuartClkFreq(2);
		break;
	case MCUX_LPUART3_CLK:
		*rate = CLOCK_GetLpuartClkFreq(3);
		break;
	case MCUX_LPUART4_CLK:
		*rate = CLOCK_GetLpuartClkFreq(4);
		break;
#endif /* defined(CONFIG_UART_MCUX_LPUART) */

#if (defined(CONFIG_I2C_MCUX_LPI2C) && CONFIG_SOC_SERIES_MCXA)
	case MCUX_LPI2C0_CLK:
		*rate = CLOCK_GetLpi2cClkFreq(0);
		break;
	case MCUX_LPI2C1_CLK:
		*rate = CLOCK_GetLpi2cClkFreq(1);
		break;
	case MCUX_LPI2C2_CLK:
		*rate = CLOCK_GetLpi2cClkFreq(2);
		break;
	case MCUX_LPI2C3_CLK:
		*rate = CLOCK_GetLpi2cClkFreq(3);
		break;
#endif /* defined(CONFIG_I2C_MCUX_LPI2C) */

#if defined(CONFIG_DT_HAS_NXP_XSPI_ENABLED)
	case MCUX_XSPI0_CLK:
		*rate = CLOCK_GetXspiClkFreq(0);
		break;
	case MCUX_XSPI1_CLK:
		*rate = CLOCK_GetXspiClkFreq(1);
		break;
	case MCUX_XSPI2_CLK:
		*rate = CLOCK_GetXspiClkFreq(2);
		break;
#endif /* defined(CONFIG_DT_HAS_NXP_XSPI_ENABLED) */

#if (defined(CONFIG_SPI_MCUX_LPSPI) && CONFIG_SOC_SERIES_MCXA)
	case MCUX_LPSPI0_CLK:
		*rate = CLOCK_GetLpspiClkFreq(0);
		break;
	case MCUX_LPSPI1_CLK:
		*rate = CLOCK_GetLpspiClkFreq(1);
		break;
#endif /* defined(CONFIG_SPI_MCUX_LPSPI) */
	}

	return 0;
}

#if defined(CONFIG_MEMC)
/*
 * Weak implemenetation of flexspi_clock_set_freq- SOC implementations are
 * expected to override this
 */
__weak int flexspi_clock_set_freq(uint32_t clock_name, uint32_t freq)
{
	ARG_UNUSED(clock_name);
	ARG_UNUSED(freq);
	return -ENOTSUP;
}
#endif

/*
 * Since this function is used to reclock the FlexSPI when running in
 * XIP, it must be located in RAM when MEMC driver is enabled.
 */
#ifdef CONFIG_MEMC
#define SYSCON_SET_FUNC_ATTR __ramfunc
#else
#define SYSCON_SET_FUNC_ATTR
#endif

static int SYSCON_SET_FUNC_ATTR mcux_lpc_syscon_clock_control_set_subsys_rate(
	const struct device *dev, clock_control_subsys_t subsys, clock_control_subsys_rate_t rate)
{
	uint32_t clock_name = (uintptr_t)subsys;
	uint32_t clock_rate = (uintptr_t)rate;

	switch (clock_name) {
	case MCUX_FLEXSPI_CLK:
#if defined(CONFIG_MEMC)
		/* The SOC is using the FlexSPI for XIP. Therefore,
		 * the FlexSPI itself must be managed within the function,
		 * which is SOC specific.
		 */
		return flexspi_clock_set_freq(clock_name, clock_rate);
#endif
#if defined(CONFIG_MIPI_DBI_NXP_LCDIC)
	case MCUX_LCDIC_CLK:
		/* Set LCDIC clock div */
		uint32_t root_rate = (CLOCK_GetLcdClkFreq() *
				      ((CLKCTL0->LCDFCLKDIV & CLKCTL0_LCDFCLKDIV_DIV_MASK) + 1));
		CLOCK_SetClkDiv(kCLOCK_DivLcdClk, (root_rate / clock_rate));
		return 0;
#endif
	default:
		/* Silence unused variable warning */
		ARG_UNUSED(clock_rate);
		return -ENOTSUP;
	}
}

static DEVICE_API(clock_control, mcux_lpc_syscon_api) = {
	.on = mcux_lpc_syscon_clock_control_on,
	.off = mcux_lpc_syscon_clock_control_off,
	.get_rate = mcux_lpc_syscon_clock_control_get_subsys_rate,
	.set_rate = mcux_lpc_syscon_clock_control_set_subsys_rate,
};

#define LPC_CLOCK_INIT(n)                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, PRE_KERNEL_1,                             \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &mcux_lpc_syscon_api);

DT_INST_FOREACH_STATUS_OKAY(LPC_CLOCK_INIT)
