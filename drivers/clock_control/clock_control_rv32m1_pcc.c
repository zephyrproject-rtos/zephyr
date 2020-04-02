/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_pcc
#include <errno.h>
#include <soc.h>
#include <drivers/clock_control.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(clock_control);

struct rv32m1_pcc_config {
	u32_t base_address;
};

#define DEV_CFG(dev)  ((struct rv32m1_pcc_config *)(dev->config->config_info))
#define DEV_BASE(dev) (DEV_CFG(dev)->base_address)

static inline clock_ip_name_t clock_ip(struct device *dev,
				       clock_control_subsys_t sub_system)
{
	u32_t offset = POINTER_TO_UINT(sub_system);

	return MAKE_PCC_REGADDR(DEV_BASE(dev), offset);
}

static int rv32m1_pcc_on(struct device *dev, clock_control_subsys_t sub_system)
{
	CLOCK_EnableClock(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_off(struct device *dev, clock_control_subsys_t sub_system)
{
	CLOCK_DisableClock(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_get_rate(struct device *dev,
			       clock_control_subsys_t sub_system,
			       u32_t *rate)
{
	*rate = CLOCK_GetIpFreq(clock_ip(dev, sub_system));
	return 0;
}

static int rv32m1_pcc_init(struct device *dev)
{
	return 0;
}

static const struct clock_control_driver_api rv32m1_pcc_api = {
	.on = rv32m1_pcc_on,
	.off = rv32m1_pcc_off,
	.get_rate = rv32m1_pcc_get_rate,
};

#if DT_HAS_DRV_INST(0)
static struct rv32m1_pcc_config rv32m1_pcc0_config = {
	.base_address = DT_INST_REG_ADDR(0)
};

DEVICE_AND_API_INIT(rv32m1_pcc0, DT_INST_LABEL(0),
		    &rv32m1_pcc_init,
		    NULL, &rv32m1_pcc0_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &rv32m1_pcc_api);
#endif

#if DT_HAS_DRV_INST(1)
static struct rv32m1_pcc_config rv32m1_pcc1_config = {
	.base_address = DT_INST_REG_ADDR(1)
};

DEVICE_AND_API_INIT(rv32m1_pcc1, DT_INST_LABEL(1),
		    &rv32m1_pcc_init,
		    NULL, &rv32m1_pcc1_config,
		    PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_OBJECTS,
		    &rv32m1_pcc_api);
#endif
