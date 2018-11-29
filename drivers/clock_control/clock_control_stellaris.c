/*
 * Copyright (c) 2018 Zilogic Systems.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <clock_control.h>
#include <misc/util.h>
#include <sys_io.h>
#include <clock_control/stellaris_clock_control.h>

enum clock_control_regs {
	RCGC_OFFSET = 0x100,
};

#define CCM_REG_ADDR(offset)	(DT_CLOCK_CONTROL_BASE_ADDR + offset)

static u32_t get_bus_addr(u32_t bus)
{
	u32_t addr;

	addr = CCM_REG_ADDR(RCGC_OFFSET) + (bus * 4);
	return addr;
}

static inline int stellaris_clock_control_on(struct device *dev,
					   clock_control_subsys_t sub_system)
{
	struct stellaris_clock_t *pclk =
		(struct stellaris_clock_t *) (sub_system);

	sys_set_bit(get_bus_addr(pclk->bus), pclk->en);
	return 0;
}


static inline int stellaris_clock_control_off(struct device *dev,
					    clock_control_subsys_t sub_system)
{
	struct stellaris_clock_t *pclk =
		(struct stellaris_clock_t *)(sub_system);

	sys_clear_bit(get_bus_addr(pclk->bus), pclk->en);
	return 0;
}


static int stellaris_clock_control_get_subsys_rate(
	struct device *clock, clock_control_subsys_t sub_system, u32_t *rate)
{
	*rate = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
	return 0;
}

static int stellaris_clock_control_init(struct device *dev)
{
	return 0;
}

static struct clock_control_driver_api stellaris_clock_control_api = {
	.on = stellaris_clock_control_on,
	.off = stellaris_clock_control_off,
	.get_rate = stellaris_clock_control_get_subsys_rate,
};

DEVICE_AND_API_INIT(clock_stellaris, DT_CLOCK_CONTROL_LABEL,
		    &stellaris_clock_control_init,
		    NULL, NULL,
		    PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &stellaris_clock_control_api);
