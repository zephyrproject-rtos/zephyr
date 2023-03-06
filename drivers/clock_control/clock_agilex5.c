/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2022-2023, Intel Corporation
 *
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_agilex5_ll.h>
#include <zephyr/dt-bindings/clock/intel_socfpga_clock.h>
#include <zephyr/logging/log.h>

#define DT_DRV_COMPAT intel_agilex5_clock

LOG_MODULE_REGISTER(clock_control_agilex5, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

struct clock_control_config {
	DEVICE_MMIO_ROM;
};

struct clock_control_data {
	DEVICE_MMIO_RAM;
};

static int clock_init(const struct device *dev)
{
	if (!dev) {
		LOG_ERR("Intel Agilex5 clock driver failed to initialize!");
		return -ENODEV;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Initialize the low layer clock driver */
	clock_agilex5_ll_init(DEVICE_MMIO_GET(dev));

	LOG_INF("Intel Agilex5 clock driver initialized!");

	return 0;
}

static int clock_get_rate(const struct device *dev,
				clock_control_subsys_t sub_system,
				uint32_t *rate)
{

	ARG_UNUSED(dev);

	switch ((intptr_t) sub_system) {
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

	case INTEL_SOCFPGA_CLOCK_TIMER:
		*rate = get_timer_clk();
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
static const struct clock_control_driver_api clock_api = {
	.get_rate = clock_get_rate
};

#define CLOCK_CONTROL_DEVICE(_inst)						\
	\
	static struct clock_control_data clock_control_data_##_inst; \
	\
	static struct clock_control_config clock_control_config_##_inst = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(_inst)), \
	}; \
	\
	DEVICE_DT_INST_DEFINE(_inst, \
		clock_init, \
		NULL, \
		&clock_control_data_##_inst, \
		&clock_control_config_##_inst, \
		PRE_KERNEL_1, \
		CONFIG_CLOCK_CONTROL_INIT_PRIORITY, \
		&clock_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_DEVICE)
