/*
 * Copyright (c) 2024 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_si32_ahb

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>

#include <SI32_CLKCTRL_A_Type.h>
#include <SI32_FLASHCTRL_A_Type.h>
#include <si32_device.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ahb);

struct clock_control_si32_ahb_config {
	const struct device *clock_dev;
	uint32_t freq;
};

static int clock_control_si32_ahb_on(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int clock_control_si32_ahb_off(const struct device *dev, clock_control_subsys_t sys)
{

	return -ENOTSUP;
}

static int clock_control_si32_ahb_get_rate(const struct device *dev, clock_control_subsys_t sys,
					   uint32_t *rate)
{
	const struct clock_control_si32_ahb_config *config = dev->config;
	*rate = config->freq;
	return 0;
}

static struct clock_control_driver_api clock_control_si32_ahb_api = {
	.on = clock_control_si32_ahb_on,
	.off = clock_control_si32_ahb_off,
	.get_rate = clock_control_si32_ahb_get_rate,
};

static int clock_control_si32_ahb_init(const struct device *dev)
{
	const struct clock_control_si32_ahb_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (config->freq != 20000000) {
		uint32_t freq = config->freq;

		ret = clock_control_set_rate(config->clock_dev, NULL, &freq);
		if (ret) {
			LOG_ERR("failed to set parent clock rate: %d", ret);
			return ret;
		}

		ret = clock_control_on(config->clock_dev, NULL);
		if (ret) {
			LOG_ERR("failed to enable parent clock: %d", ret);
			return ret;
		}

		uint32_t spmd;

		if (config->freq > 80000000) {
			spmd = 3;
		} else if (config->freq > 53000000) {
			spmd = 2;
		} else if (config->freq > 26000000) {
			spmd = 1;
		} else {
			spmd = 0;
		}
		SI32_FLASHCTRL_A_select_flash_speed_mode(SI32_FLASHCTRL_0, spmd);

		/* TODO: support other clock sources */
		SI32_CLKCTRL_A_select_ahb_source_pll(SI32_CLKCTRL_0);
	}

	return 0;
}

static const struct clock_control_si32_ahb_config config = {
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.freq = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),
};

DEVICE_DT_INST_DEFINE(0, clock_control_si32_ahb_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_si32_ahb_api);
