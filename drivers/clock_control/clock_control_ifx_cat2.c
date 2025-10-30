/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_cat2_clock

#include <zephyr/drivers/clock_control.h>
#include <cy_sysclk.h>

static int clock_control_infineon_cat2_init(const struct device *dev)
{
	ARG_UNUSED(dev);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
	if (!Cy_SysClk_ImoIsEnabled()) {
		Cy_SysClk_ImoEnable();
	}
	Cy_SysClk_ImoInit();
#endif

	return 0;
}

static int clock_control_infineon_cat2_on_off(const struct device *dev,
		clock_control_subsys_t sys)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);

	return -ENOSYS;
}

static const struct clock_control_driver_api clock_control_infineon_cat2_api = {
	.on = clock_control_infineon_cat2_on_off,
	.off = clock_control_infineon_cat2_on_off,
};

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(clk_imo))
DEVICE_DT_DEFINE(DT_NODELABEL(clk_imo),
		clock_control_infineon_cat2_init,
		NULL, NULL, NULL,
		PRE_KERNEL_1,
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		&clock_control_infineon_cat2_api);
#endif
