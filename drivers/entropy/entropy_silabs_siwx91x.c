/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_rng

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/entropy.h>

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

	if (!(flags & ENTROPY_BUSYWAIT)) {
		return -ENOTSUP;
	}
	RSI_RNG_GetBytes(config->reg, (uint32_t *)buffer, u32_count);
	if (length % sizeof(uint32_t)) {
		RSI_RNG_GetBytes(config->reg, &swap_space, 1);
		memcpy(buffer + u8_count, &swap_space, u8_remainder);
	}

	return 0;
}

static int rng_siwx91x_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	return rng_siwx91x_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);
}

static int rng_siwx91x_init(const struct device *dev)
{
	const struct rng_siwx91x_config *config = dev->config;
	int ret;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret && ret != -EALREADY) {
		return ret;
	}
	ret = RSI_RNG_Start(config->reg, RSI_RNG_TRUE_RANDOM);
	if (ret) {
		return -EIO;
	}
	return 0;
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
	DEVICE_DT_INST_DEFINE(n, rng_siwx91x_init, NULL, NULL, &rng_siwx91x_cfg##n,             \
			      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &rng_siwx91x_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_RNG_INIT)
