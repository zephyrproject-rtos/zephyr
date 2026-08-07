/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/renesas_rztn_clock.h>
#include <zephyr/kernel.h>
#include "bsp_api.h"

#define DT_DRV_COMPAT renesas_rz_cgc

static int clock_control_renesas_rz_on(const struct device *dev, clock_control_subsys_t sys)
{
	if (!dev || !sys) {
		return -EINVAL;
	}

	uint32_t *clock_id = (uint32_t *)sys;

	uint32_t ip = (*clock_id & RZ_IP_MASK) >> RZ_IP_SHIFT;
	uint32_t ch = (*clock_id & RZ_IP_CH_MASK) >> RZ_IP_CH_SHIFT;

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	switch (ip) {
	case RZ_IP_BSC:
		R_BSP_MODULE_START(FSP_IP_BSC, ch);
		break;
	case RZ_IP_XSPI:
		R_BSP_MODULE_START(FSP_IP_XSPI, ch);
		break;
	case RZ_IP_SCI:
		R_BSP_MODULE_START(FSP_IP_SCI, ch);
		break;
	case RZ_IP_IIC:
		R_BSP_MODULE_START(FSP_IP_IIC, ch);
		break;
	case RZ_IP_SPI:
		R_BSP_MODULE_START(FSP_IP_SPI, ch);
		break;
	case RZ_IP_GPT:
		R_BSP_MODULE_START(FSP_IP_GPT, ch);
		break;
	case RZ_IP_ADC12:
		R_BSP_MODULE_START(FSP_IP_ADC12, ch);
		break;
	case RZ_IP_CMT:
		R_BSP_MODULE_START(FSP_IP_CMT, ch);
		break;
	case RZ_IP_CMTW:
		R_BSP_MODULE_START(FSP_IP_CMTW, ch);
		break;
	case RZ_IP_CANFD:
		R_BSP_MODULE_START(FSP_IP_CANFD, ch);
		break;
	case RZ_IP_GMAC:
		R_BSP_MODULE_START(FSP_IP_GMAC, ch);
		break;
	case RZ_IP_ETHSW:
		R_BSP_MODULE_START(FSP_IP_ETHSW, ch);
		break;
	case RZ_IP_USBHS:
		R_BSP_MODULE_START(FSP_IP_USBHS, ch);
		break;
	case RZ_IP_RTC:
		R_BSP_MODULE_START(FSP_IP_RTC, ch);
		break;
	default:
		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);
		return -EINVAL; /* Invalid FSP IP Module */
	}
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);

	return 0;
}

static int clock_control_renesas_rz_off(const struct device *dev, clock_control_subsys_t sys)
{
	if (!dev || !sys) {
		return -EINVAL;
	}

	uint32_t *clock_id = (uint32_t *)sys;

	uint32_t ip = (*clock_id & RZ_IP_MASK) >> RZ_IP_SHIFT;
	uint32_t ch = (*clock_id & RZ_IP_CH_MASK) >> RZ_IP_CH_SHIFT;

	R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_RESET);
	switch (ip) {
	case RZ_IP_BSC:
		R_BSP_MODULE_STOP(FSP_IP_BSC, ch);
		break;
	case RZ_IP_XSPI:
		R_BSP_MODULE_STOP(FSP_IP_XSPI, ch);
		break;
	case RZ_IP_SCI:
		R_BSP_MODULE_STOP(FSP_IP_SCI, ch);
		break;
	case RZ_IP_IIC:
		R_BSP_MODULE_STOP(FSP_IP_IIC, ch);
		break;
	case RZ_IP_SPI:
		R_BSP_MODULE_STOP(FSP_IP_SPI, ch);
		break;
	case RZ_IP_GPT:
		R_BSP_MODULE_STOP(FSP_IP_GPT, ch);
		break;
	case RZ_IP_ADC12:
		R_BSP_MODULE_STOP(FSP_IP_ADC12, ch);
		break;
	case RZ_IP_CMT:
		R_BSP_MODULE_STOP(FSP_IP_CMT, ch);
		break;
	case RZ_IP_CMTW:
		R_BSP_MODULE_STOP(FSP_IP_CMTW, ch);
		break;
	case RZ_IP_CANFD:
		R_BSP_MODULE_STOP(FSP_IP_CANFD, ch);
		break;
	case RZ_IP_GMAC:
		R_BSP_MODULE_STOP(FSP_IP_GMAC, ch);
		break;
	case RZ_IP_ETHSW:
		R_BSP_MODULE_STOP(FSP_IP_ETHSW, ch);
		break;
	case RZ_IP_USBHS:
		R_BSP_MODULE_STOP(FSP_IP_USBHS, ch);
		break;
	case RZ_IP_RTC:
		R_BSP_MODULE_STOP(FSP_IP_RTC, ch);
		break;
	default:
		R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);
		return -EINVAL; /* Invalid FSP IP Module */
	}
	R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_RESET);

	return 0;
}

static int clock_control_renesas_rz_get_rate(const struct device *dev, clock_control_subsys_t sys,
					     uint32_t *rate)
{
	if (!dev || !sys || !rate) {
		return -EINVAL;
	}

	uint32_t *clock_id = (uint32_t *)sys;
	fsp_priv_clock_t clk_src = (*clock_id & RZ_CLOCK_MASK) >> RZ_CLOCK_SHIFT;
	uint32_t clk_hz = R_FSP_SystemClockHzGet(clk_src);

	*rate = clk_hz;

	return 0;
}

static DEVICE_API(clock_control, rz_clock_control_driver_api) = {
	.on = clock_control_renesas_rz_on,
	.off = clock_control_renesas_rz_off,
	.get_rate = clock_control_renesas_rz_get_rate,
};

static int clock_control_rz_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

DEVICE_DT_INST_DEFINE(0, clock_control_rz_init, NULL, NULL, NULL, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &rz_clock_control_driver_api);
