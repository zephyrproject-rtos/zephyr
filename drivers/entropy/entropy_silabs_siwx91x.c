/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_rng

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include "rsi_rom_rng.h"

struct rng_siwx91x_config {
	HWRNG_Type *reg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static int rng_siwx91x_get_entropy_isr(const struct device *dev, uint8_t *buffer,
				       uint16_t length, uint32_t flags)
{
	const struct rng_siwx91x_config *config = dev->config;
	uint32_t u32_count = length / sizeof(uint32_t);
	uint32_t u8_count = u32_count * sizeof(uint32_t);
	uint32_t u8_remainder = length - u8_count;
	uint32_t swap_space;
	int ret;

	if (!(flags & ENTROPY_BUSYWAIT)) {
		return -ENOTSUP;
	}

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		return ret;
	}

	RSI_RNG_GetBytes(config->reg, (uint32_t *)buffer, u32_count);
	if (length % sizeof(uint32_t)) {
		RSI_RNG_GetBytes(config->reg, &swap_space, 1);
		memcpy(buffer + u8_count, &swap_space, u8_remainder);
	}

	(void)pm_device_runtime_put(dev);

	return 0;
}

static int rng_siwx91x_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	return rng_siwx91x_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);
}

static int rng_siwx91x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct rng_siwx91x_config *config = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_TURN_ON:
		ret = clock_control_on(config->clock_dev, config->clock_subsys);
		if (ret && ret != -EALREADY) {
			return ret;
		}
		ret = RSI_RNG_Start(config->reg, RSI_RNG_TRUE_RANDOM);
		if (ret) {
			return -EIO;
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		ret = clock_control_off(config->clock_dev, config->clock_subsys);
		if (ret < 0 && ret != -EALREADY) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rng_siwx91x_init(const struct device *dev)
{
	return pm_device_driver_init(dev, rng_siwx91x_pm_action);
}

static DEVICE_API(entropy, rng_siwx91x_api) = {
	.get_entropy = rng_siwx91x_get_entropy,
	.get_entropy_isr = rng_siwx91x_get_entropy_isr,
};

#define SIWX91X_RNG_INIT(n)                                                                     \
	static const struct rng_siwx91x_config rng_siwx91x_cfg##n = {                           \
		.reg = (HWRNG_Type *)DT_INST_REG_ADDR(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(n, clocks, clkid),          \
	};                                                                                      \
	PM_DEVICE_DT_INST_DEFINE(n, rng_siwx91x_pm_action);                                     \
	DEVICE_DT_INST_DEFINE(n, rng_siwx91x_init, PM_DEVICE_DT_INST_GET(n), NULL,              \
			      &rng_siwx91x_cfg##n, PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY,  \
			      &rng_siwx91x_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_RNG_INIT)
