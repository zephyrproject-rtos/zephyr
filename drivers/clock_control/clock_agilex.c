/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2021, Intel Corporation
 *
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_agilex_ll.h>
#include <zephyr/dt-bindings/clock/intel_socfpga_clock.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(clock_control, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static int clk_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	LOG_INF("Intel Clock driver initialized");
	return 0;
}

static int clk_get_rate(const struct device *dev,
			clock_control_subsys_t sub_system,
			uint32_t *rate)
{
	ARG_UNUSED(dev);

	struct clock_attr *attr = (struct clock_attr *)(sub_system);

	switch (attr->clock_id) {
	case INTEL_SOCFPGA_CLOCK_MPU:
		*rate = get_mpu_clk();
		break;
	case INTEL_SOCFPGA_CLOCK_WDT:
		*rate = get_wdt_clk();
		break;
	case INTEL_SOCFPGA_CLOCK_UART:
		*rate = get_uart_clk();
		break;
	case INTEL_SOCFPGA_CLOCK_MMC:
		*rate = get_mmc_clk();
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct clock_control_driver_api clk_api = {
	.get_rate = clk_get_rate
};

DEVICE_DT_DEFINE(DT_NODELABEL(clock), clk_init, NULL, NULL, NULL,
		 PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		 &clk_api);
