/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * Based on clock_control_rv32m1_pcc.c, which is:
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_pcc

#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_mcux_pcc);

struct mcux_pcc_config {
	uint32_t base_address;
};

#define DEV_BASE(dev) (((struct mcux_pcc_config *)(dev->config))->base_address)
#ifndef MAKE_PCC_REGADDR
#define MAKE_PCC_REGADDR(base, offset) ((base) + (offset))
#endif

static inline clock_ip_name_t clock_ip(const struct device *dev,
				       clock_control_subsys_t sub_system)
{
	uint32_t offset = POINTER_TO_UINT(sub_system);

	return MAKE_PCC_REGADDR(DEV_BASE(dev), offset);
}

static int mcux_pcc_on(const struct device *dev,
		       clock_control_subsys_t sub_system)
{
	CLOCK_EnableClock(clock_ip(dev, sub_system));
	return 0;
}

static int mcux_pcc_off(const struct device *dev,
			clock_control_subsys_t sub_system)
{
	CLOCK_DisableClock(clock_ip(dev, sub_system));
	return 0;
}

static int mcux_pcc_get_rate(const struct device *dev,
			       clock_control_subsys_t sub_system,
			       uint32_t *rate)
{
	*rate = CLOCK_GetIpFreq(clock_ip(dev, sub_system));
	return 0;
}

static const struct clock_control_driver_api mcux_pcc_api = {
	.on = mcux_pcc_on,
	.off = mcux_pcc_off,
	.get_rate = mcux_pcc_get_rate,
};

#define MCUX_PCC_INIT(inst)						\
	static const struct mcux_pcc_config mcux_pcc##inst##_config = {	\
		.base_address = DT_INST_REG_ADDR(inst)			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    NULL,					\
			    NULL,					\
			    NULL, &mcux_pcc##inst##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		\
			    &mcux_pcc_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_PCC_INIT)
