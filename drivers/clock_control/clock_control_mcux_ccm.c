/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ccm
#include <errno.h>
#include <soc.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#ifdef CONFIG_SPI_MCUX_LPSPI
static const clock_name_t lpspi_clocks[] = {
	kCLOCK_Usb1PllPfd1Clk,
	kCLOCK_Usb1PllPfd0Clk,
	kCLOCK_SysPllClk,
	kCLOCK_SysPllPfd2Clk,
};
#endif
#ifdef CONFIG_UART_MCUX_IUART
static const clock_root_control_t uart_clk_root[] = {
	kCLOCK_RootUart1,
	kCLOCK_RootUart2,
	kCLOCK_RootUart3,
	kCLOCK_RootUart4,
};
#endif

static int mcux_ccm_on(const struct device *dev,
			      clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_off(const struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_get_subsys_rate(const struct device *dev,
				    clock_control_subsys_t sub_system,
				    uint32_t *rate)
{
#ifdef CONFIG_ARM64
	uint32_t clock_name = (uint32_t)(uint64_t) sub_system;
#else
	uint32_t clock_name = (uint32_t) sub_system;
#endif
	uint32_t mux __unused;

	switch (clock_name) {

#ifdef CONFIG_I2C_MCUX_LPI2C
	case IMX_CCM_LPI2C_CLK:
		if (CLOCK_GetMux(kCLOCK_Lpi2cMux) == 0) {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 8
				/ (CLOCK_GetDiv(kCLOCK_Lpi2cDiv) + 1);
		} else {
			*rate = CLOCK_GetOscFreq()
				/ (CLOCK_GetDiv(kCLOCK_Lpi2cDiv) + 1);
		}

		break;
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
	case IMX_CCM_LPSPI_CLK:
	{
		uint32_t lpspi_mux = CLOCK_GetMux(kCLOCK_LpspiMux);
		clock_name_t lpspi_clock = lpspi_clocks[lpspi_mux];

		*rate = CLOCK_GetFreq(lpspi_clock)
			/ (CLOCK_GetDiv(kCLOCK_LpspiDiv) + 1);
		break;
	}
#endif

#ifdef CONFIG_UART_MCUX_LPUART
	case IMX_CCM_LPUART_CLK:
		if (CLOCK_GetMux(kCLOCK_UartMux) == 0) {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		} else {
			*rate = CLOCK_GetOscFreq()
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		}

		break;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc1), okay) && CONFIG_IMX_USDHC
	case IMX_CCM_USDHC1_CLK:
		*rate = CLOCK_GetSysPfdFreq(kCLOCK_Pfd0) /
				(CLOCK_GetDiv(kCLOCK_Usdhc1Div) + 1U);
		break;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(usdhc2), okay) && CONFIG_IMX_USDHC
	case IMX_CCM_USDHC2_CLK:
		*rate = CLOCK_GetSysPfdFreq(kCLOCK_Pfd0) /
				(CLOCK_GetDiv(kCLOCK_Usdhc2Div) + 1U);
		break;
#endif

#ifdef CONFIG_DMA_MCUX_EDMA
	case IMX_CCM_EDMA_CLK:
		*rate = CLOCK_GetIpgFreq();
		break;
#endif

#ifdef CONFIG_PWM_MCUX
	case IMX_CCM_PWM_CLK:
		*rate = CLOCK_GetIpgFreq();
		break;
#endif

#ifdef CONFIG_UART_MCUX_IUART
	case IMX_CCM_UART1_CLK:
	case IMX_CCM_UART2_CLK:
	case IMX_CCM_UART3_CLK:
	case IMX_CCM_UART4_CLK:
	{
		uint32_t instance = clock_name & IMX_CCM_INSTANCE_MASK;
		clock_root_control_t clk_root = uart_clk_root[instance];
		uint32_t uart_mux = CLOCK_GetRootMux(clk_root);

		if (uart_mux == 0) {
			*rate = MHZ(24);
		} else if (uart_mux == 1) {
			*rate = CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) /
				(CLOCK_GetRootPreDivider(clk_root)) /
				(CLOCK_GetRootPostDivider(clk_root)) /
				10;
		}

	} break;
#endif

#ifdef CONFIG_CAN_MCUX_FLEXCAN
	case IMX_CCM_CAN_CLK:
	{
		uint32_t can_mux = CLOCK_GetMux(kCLOCK_CanMux);

		if (can_mux == 0) {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 8
				/ (CLOCK_GetDiv(kCLOCK_CanDiv) + 1);
		} else if  (can_mux == 1) {
			*rate = CLOCK_GetOscFreq()
				/ (CLOCK_GetDiv(kCLOCK_CanDiv) + 1);
		} else {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6
				/ (CLOCK_GetDiv(kCLOCK_CanDiv) + 1);
		}
	} break;
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT
	case IMX_CCM_GPT_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_PerClk);
		break;
#endif

#ifdef CONFIG_COUNTER_MCUX_QTMR
	case IMX_CCM_QTMR_CLK:
		*rate = CLOCK_GetIpgFreq();
		break;
#endif

#ifdef CONFIG_I2S_MCUX_SAI
	case IMX_CCM_SAI1_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_AudioPllClk)
				/ (CLOCK_GetDiv(kCLOCK_Sai1PreDiv) + 1)
				/ (CLOCK_GetDiv(kCLOCK_Sai1Div) + 1);
		break;
	case IMX_CCM_SAI2_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_AudioPllClk)
				/ (CLOCK_GetDiv(kCLOCK_Sai2PreDiv) + 1)
				/ (CLOCK_GetDiv(kCLOCK_Sai2Div) + 1);
		break;
	case IMX_CCM_SAI3_CLK:
		*rate = CLOCK_GetFreq(kCLOCK_AudioPllClk)
				/ (CLOCK_GetDiv(kCLOCK_Sai3PreDiv) + 1)
				/ (CLOCK_GetDiv(kCLOCK_Sai3Div) + 1);
		break;
#endif
	}

	return 0;
}

static int mcux_ccm_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_ccm_driver_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0,
		    &mcux_ccm_init,
		    NULL,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		    &mcux_ccm_driver_api);
