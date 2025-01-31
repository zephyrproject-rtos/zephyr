/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT silabs_siwx91x_rng

#include <zephyr/drivers/entropy.h>

#include "rsi_rom_rng.h"
#include "rsi_rom_clks.h"

static int siwx91x_get_entropy_isr(const struct device *dev, uint8_t *buffer,
				   uint16_t length, uint32_t flags)
{
	uint32_t u32_count = length / sizeof(uint32_t);
	uint32_t u8_count = u32_count * sizeof(uint32_t);
	uint32_t u8_remainder = length - u8_count;
	uint32_t swap_space;

	if (!(flags & ENTROPY_BUSYWAIT)) {
		return -ENOTSUP;
	}
	RSI_RNG_GetBytes((void *)dev->config, (uint32_t *)buffer, u32_count);
	if (length % sizeof(uint32_t)) {
		RSI_RNG_GetBytes((void *)dev->config, &swap_space, 1);
		memcpy(buffer + u8_count, &swap_space, u8_remainder);
	}

	return 0;
}

static int siwx91x_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	return siwx91x_get_entropy_isr(dev, buffer, length, ENTROPY_BUSYWAIT);
}

static int siwx91x_init(const struct device *dev)
{
	int ret;

	ret = RSI_CLK_PeripheralClkEnable1(M4CLK, HWRNG_PCLK_ENABLE);
	if (ret) {
		return -EIO;
	}
	ret = RSI_RNG_Start((void *)dev->config, RSI_RNG_TRUE_RANDOM);
	if (ret) {
		return -EIO;
	}
	return 0;
}

static DEVICE_API(entropy, siwx91x_api) = {
	.get_entropy = siwx91x_get_entropy,
	.get_entropy_isr = siwx91x_get_entropy_isr,
};

#define SIWX91X_RNG_INIT(idx)                                                               \
	DEVICE_DT_INST_DEFINE(idx, siwx91x_init, NULL, NULL, (void *)DT_INST_REG_ADDR(idx), \
			      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &siwx91x_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_RNG_INIT)
