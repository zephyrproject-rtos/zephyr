/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_clock

#include <zephyr/drivers/clock_control.h>

#include <Clock_Ip.h>

#define NXP_S32_CLOCK_CONFIG_IDX CONFIG_CLOCK_CONTROL_NXP_S32_CLOCK_CONFIG_IDX

BUILD_ASSERT(CLOCK_IP_GET_FREQUENCY_API == STD_ON,
	     "Clock Get Frequency API must be enabled");

static int nxp_s32_clock_on(const struct device *dev,
			    clock_control_subsys_t sub_system)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

	Clock_Ip_EnableModuleClock(clock_name);

	return 0;
}

static int nxp_s32_clock_off(const struct device *dev,
			     clock_control_subsys_t sub_system)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

	Clock_Ip_DisableModuleClock(clock_name);

	return 0;
}

static int nxp_s32_clock_get_rate(const struct device *dev,
				  clock_control_subsys_t sub_system,
				  uint32_t *rate)
{
	Clock_Ip_NameType clock_name = (Clock_Ip_NameType)sub_system;

	if ((clock_name <= CLOCK_IS_OFF) || (clock_name >= RESERVED_CLK)) {
		return -EINVAL;
	}

	*rate = Clock_Ip_GetClockFrequency(clock_name);

	return 0;
}

static int nxp_s32_clock_init(const struct device *dev)
{
	Clock_Ip_StatusType status;

	status = Clock_Ip_Init(&Clock_Ip_aClockConfig[NXP_S32_CLOCK_CONFIG_IDX]);

	return (status == CLOCK_IP_SUCCESS ? 0 : -EIO);
}

static DEVICE_API(clock_control, nxp_s32_clock_driver_api) = {
	.on = nxp_s32_clock_on,
	.off = nxp_s32_clock_off,
	.get_rate = nxp_s32_clock_get_rate,
};

DEVICE_DT_INST_DEFINE(0,
		      nxp_s32_clock_init,
		      NULL, NULL, NULL,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &nxp_s32_clock_driver_api);
