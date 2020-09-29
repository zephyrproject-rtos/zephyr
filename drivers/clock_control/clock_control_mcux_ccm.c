/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ccm
#include <errno.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/imx_ccm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

#ifdef CONFIG_SPI_MCUX_LPSPI
static const clock_name_t lpspi_clocks[] = {
	kCLOCK_Usb1PllPfd1Clk,
	kCLOCK_Usb1PllPfd0Clk,
	kCLOCK_SysPllClk,
	kCLOCK_SysPllPfd2Clk,
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
	uint32_t clock_name = (uint32_t) sub_system;

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

#ifdef CONFIG_DISK_ACCESS_USDHC1
	case IMX_CCM_USDHC1_CLK:
		*rate = CLOCK_GetSysPfdFreq(kCLOCK_Pfd0) /
				(CLOCK_GetDiv(kCLOCK_Usdhc1Div) + 1U);
		break;
#endif

#ifdef CONFIG_DISK_ACCESS_USDHC2
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

#ifdef CONFIG_UART_MCUX_IUART
	case IMX_CCM_UART_CLK:
		*rate = CLOCK_GetPllFreq(kCLOCK_SystemPll1Ctrl) /
				(CLOCK_GetRootPreDivider(kCLOCK_RootUart4)) /
				(CLOCK_GetRootPostDivider(kCLOCK_RootUart4)) /
				10;
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

DEVICE_AND_API_INIT(mcux_ccm, DT_INST_LABEL(0),
		    &mcux_ccm_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ccm_driver_api);
