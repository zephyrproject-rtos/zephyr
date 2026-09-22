/*
 * Copyright (c) 2026 Aleksandr Senin <al@meshium.net>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gd_gd32_bldo

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/gd32.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <soc.h>
#include <gd32_backup_domain.h>

#define GD32_BLDO_READY_TIMEOUT_MS 100U

struct gd32_bldo_config {
	struct regulator_common_config common;
	uint32_t clkid;
};

struct gd32_bldo_data {
	struct regulator_common_data common;
};

static int gd32_bldo_enable(const struct device *dev)
{
	const struct gd32_bldo_config *cfg = dev->config;
	int ret;

	ret = clock_control_on(GD32_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	uint32_t start_ms = k_uptime_get_32();

	gd32_backup_domain_enable_access();
	PMU_CS |= PMU_CS_BLDOON;

	while ((PMU_CS & PMU_CS_BLDORF) == 0U) {
		if ((k_uptime_get_32() - start_ms) > GD32_BLDO_READY_TIMEOUT_MS) {
			PMU_CS &= ~PMU_CS_BLDOON;
			gd32_backup_domain_disable_access();
			return -ETIMEDOUT;
		}

		k_msleep(1);
	}

	gd32_backup_domain_disable_access();

	return 0;
}

static int gd32_bldo_disable(const struct device *dev)
{
	const struct gd32_bldo_config *cfg = dev->config;
	int ret;

	ret = clock_control_on(GD32_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	gd32_backup_domain_enable_access();
	PMU_CS &= ~PMU_CS_BLDOON;
	gd32_backup_domain_disable_access();

	return 0;
}

static DEVICE_API(regulator, gd32_bldo_api) = {
	.enable = gd32_bldo_enable,
	.disable = gd32_bldo_disable,
};

static int gd32_bldo_init(const struct device *dev)
{
	const struct gd32_bldo_config *cfg = dev->config;
	bool is_enabled;
	int ret;

	regulator_common_data_init(dev);

	ret = clock_control_on(GD32_CLOCK_CONTROLLER, (clock_control_subsys_t)&cfg->clkid);
	if (ret < 0 && ret != -EALREADY) {
		return ret;
	}

	is_enabled = (PMU_CS & PMU_CS_BLDORF) != 0U;

	return regulator_common_init(dev, is_enabled);
}

/* clang-format off */
#define GD32_BLDO_DEFINE(inst) \
	static struct gd32_bldo_data data##inst; \
	static const struct gd32_bldo_config config##inst = { \
		.common = REGULATOR_DT_INST_COMMON_CONFIG_INIT(inst), \
		.clkid = DT_INST_CLOCKS_CELL(inst, id), \
	}; \
	DEVICE_DT_INST_DEFINE(inst, gd32_bldo_init, NULL, &data##inst, &config##inst, \
			      POST_KERNEL, CONFIG_REGULATOR_GD32_BLDO_INIT_PRIORITY, \
			      &gd32_bldo_api);
/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(GD32_BLDO_DEFINE)
