/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_pcc
#include <errno.h>
#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

struct rv32m1_pcc_config {
	uint32_t base_address;
};

#define DEV_BASE(dev) \
	(((struct rv32m1_pcc_config *)(dev->config))->base_address)

static inline clock_ip_name_t clock_ip(const struct device *dev,
				       clock_control_subsys_t sub_system)
{
	uint32_t offset = POINTER_TO_UINT(sub_system);

	return MAKE_PCC_REGADDR(DEV_BASE(dev), offset);
}

static int rv32m1_pcc_on(const struct device *dev,
			 clock_control_subsys_t sub_system)
{
	CLOCK_EnableClock(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_off(const struct device *dev,
			  clock_control_subsys_t sub_system)
{
	CLOCK_DisableClock(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_get_rate(const struct device *dev,
			       clock_control_subsys_t sub_system,
			       uint32_t *rate)
{
	*rate = CLOCK_GetIpFreq(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_init(const struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api rv32m1_pcc_api = {
	.on = rv32m1_pcc_on,
	.off = rv32m1_pcc_off,
	.get_rate = rv32m1_pcc_get_rate,
};

#define RV32M1_PCC_INIT(inst)						\
	static struct rv32m1_pcc_config rv32m1_pcc##inst##_config = {	\
		.base_address = DT_INST_REG_ADDR(inst)			\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    &rv32m1_pcc_init,				\
			    NULL,					\
			    NULL, &rv32m1_pcc##inst##_config,		\
			    PRE_KERNEL_1,				\
			    CONFIG_CLOCK_CONTROL_INIT_PRIORITY,		\
			    &rv32m1_pcc_api);

DT_INST_FOREACH_STATUS_OKAY(RV32M1_PCC_INIT)
