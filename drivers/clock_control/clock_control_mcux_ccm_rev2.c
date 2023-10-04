/*
 * Copyright (c) 2021, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_ccm_rev2
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/imx_ccm_rev2.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

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
	uint32_t clock_root, peripheral, instance;

	peripheral = (clock_name & IMX_CCM_PERIPHERAL_MASK);
	instance = (clock_name & IMX_CCM_INSTANCE_MASK);
	switch (peripheral) {
#ifdef CONFIG_I2C_MCUX_LPI2C
	case IMX_CCM_LPI2C1_CLK:
		clock_root = kCLOCK_Root_Lpi2c1 + instance;
		break;
#endif

#ifdef CONFIG_SPI_MCUX_LPSPI
	case IMX_CCM_LPSPI1_CLK:
		clock_root = kCLOCK_Root_Lpspi1 + instance;
		break;
#endif

#ifdef CONFIG_UART_MCUX_LPUART
	case IMX_CCM_LPUART1_CLK:
		clock_root = kCLOCK_Root_Lpuart1 + instance;
		break;
#endif

#if CONFIG_IMX_USDHC
	case IMX_CCM_USDHC1_CLK:
	case IMX_CCM_USDHC2_CLK:
		clock_root = kCLOCK_Root_Usdhc1 + instance;
		break;
#endif

#ifdef CONFIG_DMA_MCUX_EDMA
	case IMX_CCM_EDMA_CLK:
		clock_root = kCLOCK_Root_Bus;
		break;
	case IMX_CCM_EDMA_LPSR_CLK:
		clock_root = kCLOCK_Root_Bus_Lpsr;
		break;
#endif

#ifdef CONFIG_PWM_MCUX
	case IMX_CCM_PWM_CLK:
		clock_root = kCLOCK_Root_Bus;
		break;
#endif

#ifdef CONFIG_CAN_MCUX_FLEXCAN
	case IMX_CCM_CAN1_CLK:
		clock_root = kCLOCK_Root_Can1 + instance;
		break;
#endif

#ifdef CONFIG_COUNTER_MCUX_GPT
	case IMX_CCM_GPT_CLK:
		clock_root = kCLOCK_Root_Gpt1 + instance;
		break;
#endif

#ifdef CONFIG_I2S_MCUX_SAI
	case IMX_CCM_SAI1_CLK:
		clock_root =  kCLOCK_Root_Sai1;
		break;
	case IMX_CCM_SAI2_CLK:
		clock_root =  kCLOCK_Root_Sai2;
		break;
	case IMX_CCM_SAI3_CLK:
		clock_root =  kCLOCK_Root_Sai3;
		break;
	case IMX_CCM_SAI4_CLK:
		clock_root =  kCLOCK_Root_Sai4;
		break;
#endif
	default:
		return -EINVAL;
	}

	*rate = CLOCK_GetRootClockFreq(clock_root);
	return 0;
}

static const struct clock_control_driver_api mcux_ccm_driver_api = {
	.on = mcux_ccm_on,
	.off = mcux_ccm_off,
	.get_rate = mcux_ccm_get_subsys_rate,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &mcux_ccm_driver_api);
