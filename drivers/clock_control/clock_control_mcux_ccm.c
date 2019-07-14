/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <dt-bindings/clock/imx_ccm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static const clock_name_t lpspi_clocks[] = {
	kCLOCK_Usb1PllPfd1Clk,
	kCLOCK_Usb1PllPfd0Clk,
	kCLOCK_SysPllClk,
	kCLOCK_SysPllPfd2Clk,
};

static int mcux_ccm_on(struct device *dev,
			      clock_control_subsys_t sub_system)
{
	return 0;
}

static int mcux_ccm_off(struct device *dev,
			       clock_control_subsys_t sub_system)
{
	return 0;
}

#ifdef CONFIG_NXP_MCUX_CSI
enum mcux_csi_clk_sel {
	CSI_CLK_SEL_24M = 0,
	CSI_CLK_SEL_PLL2_PFD2 = 1,
	CSI_CLK_SEL_120M = 2,
	CSI_CLK_SEL_PLL3_PFD1 = 3,
};
#endif

static int mcux_ccm_get_subsys_rate(struct device *dev,
				    clock_control_subsys_t sub_system,
				    u32_t *rate)
{
	u32_t clock_name = (u32_t) sub_system;

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

	case IMX_CCM_LPSPI_CLK:
	{
		u32_t lpspi_mux = CLOCK_GetMux(kCLOCK_LpspiMux);
		clock_name_t lpspi_clock = lpspi_clocks[lpspi_mux];

		*rate = CLOCK_GetFreq(lpspi_clock)
			/ (CLOCK_GetDiv(kCLOCK_LpspiDiv) + 1);
		break;
	}

	case IMX_CCM_LPUART_CLK:
		if (CLOCK_GetMux(kCLOCK_UartMux) == 0) {
			*rate = CLOCK_GetPllFreq(kCLOCK_PllUsb1) / 6
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		} else {
			*rate = CLOCK_GetOscFreq()
				/ (CLOCK_GetDiv(kCLOCK_UartDiv) + 1);
		}

		break;

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

#ifdef CONFIG_NXP_MCUX_CSI
	case IMX_CCM_CSI_CLK:
	{
		enum mcux_csi_clk_sel clk_sel =
			(enum mcux_csi_clk_sel)
			CLOCK_GetMux(kCLOCK_CsiMux);

		if (clk_sel == CSI_CLK_SEL_24M) {
			*rate = (24 * 1024 * 1024) /
				(CLOCK_GetDiv(kCLOCK_CsiDiv) + 1U);
		} else if (clk_sel == CSI_CLK_SEL_PLL2_PFD2) {
			*rate = CLOCK_GetSysPfdFreq(kCLOCK_Pfd2) /
				(CLOCK_GetDiv(kCLOCK_CsiDiv) + 1U);
		} else if (clk_sel == CSI_CLK_SEL_120M) {
			*rate = (120 * 1024 * 1024) /
				(CLOCK_GetDiv(kCLOCK_CsiDiv) + 1U);
		} else if (clk_sel == CSI_CLK_SEL_PLL3_PFD1) {
			*rate = CLOCK_GetSysPfdFreq(kCLOCK_Pfd1) /
				(CLOCK_GetDiv(kCLOCK_CsiDiv) + 1U);
		} else {
			return -EINVAL;
		}
	}
	break;
#endif
	}

	return 0;
}

static int mcux_ccm_init(struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api mcux_ccm_driver_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_subsys_rate,
};

DEVICE_AND_API_INIT(mcux_ccm, DT_MCUX_CCM_NAME,
		    &mcux_ccm_init,
		    NULL, NULL,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ccm_driver_api);
