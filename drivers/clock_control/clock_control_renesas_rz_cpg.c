/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/renesas_rzg_clock.h>
#include <zephyr/kernel.h>

#define DT_DRV_COMPAT renesas_rz_cpg

static int clock_control_renesas_rz_on(const struct device *dev, clock_control_subsys_t sys)
{
	if (!dev || !sys) {
		return -EINVAL;
	}

	uint32_t *clock_id = (uint32_t *)sys;

	uint32_t ip = (*clock_id & RZ_IP_MASK) >> RZ_IP_SHIFT;
	uint32_t ch = (*clock_id & RZ_IP_CH_MASK) >> RZ_IP_CH_SHIFT;

	switch (ip) {
	case RZ_IP_GTM:
		R_BSP_MODULE_START(FSP_IP_GTM, ch);
		break;
	case RZ_IP_GPT:
		R_BSP_MODULE_START(FSP_IP_GPT, ch);
		break;
	case RZ_IP_SCIF:
		R_BSP_MODULE_START(FSP_IP_SCIF, ch);
		break;
	case RZ_IP_RIIC:
		R_BSP_MODULE_START(FSP_IP_RIIC, ch);
		break;
	case RZ_IP_RSPI:
		R_BSP_MODULE_START(FSP_IP_RSPI, ch);
		break;
	case RZ_IP_MHU:
		R_BSP_MODULE_START(FSP_IP_MHU, ch);
		break;
	case RZ_IP_DMAC:
		R_BSP_MODULE_START(FSP_IP_DMAC, ch);
		break;
	case RZ_IP_CANFD:
		R_BSP_MODULE_START(FSP_IP_CANFD, ch);
		break;
	case RZ_IP_ADC:
		R_BSP_MODULE_START(FSP_IP_ADC, ch);
		break;
	default:
		return -EINVAL; /* Invalid FSP IP Module */
	}

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

	switch (ip) {
	case RZ_IP_GTM:
		R_BSP_MODULE_STOP(FSP_IP_GTM, ch);
		break;
	case RZ_IP_GPT:
		R_BSP_MODULE_STOP(FSP_IP_GPT, ch);
		break;
	case RZ_IP_SCIF:
		R_BSP_MODULE_STOP(FSP_IP_SCIF, ch);
		break;
	case RZ_IP_RIIC:
		R_BSP_MODULE_STOP(FSP_IP_RIIC, ch);
		break;
	case RZ_IP_RSPI:
		R_BSP_MODULE_STOP(FSP_IP_RSPI, ch);
		break;
	case RZ_IP_MHU:
		R_BSP_MODULE_STOP(FSP_IP_MHU, ch);
		break;
	case RZ_IP_DMAC:
		R_BSP_MODULE_STOP(FSP_IP_DMAC, ch);
		break;
	case RZ_IP_CANFD:
		R_BSP_MODULE_STOP(FSP_IP_CANFD, ch);
		break;
	case RZ_IP_ADC:
		R_BSP_MODULE_STOP(FSP_IP_ADC, ch);
		break;
	default:
		return -EINVAL; /* Invalid */
	}
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
	uint32_t clk_div = (*clock_id & RZ_CLOCK_DIV_MASK) >> RZ_CLOCK_DIV_SHIFT;

	uint32_t clk_hz = R_FSP_SystemClockHzGet(clk_src);
	*rate = clk_hz / clk_div;
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
