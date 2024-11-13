/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_rts5817_trng

#include <zephyr/device.h>
#include <zephyr/drivers/entropy.h>
#include <errno.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <string.h>
#include "entropy_rts5817.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_rts5817, CONFIG_ENTROPY_LOG_LEVEL);

struct trng_rts5817_cfg {
	mem_addr_t regs;
};

static int entropy_rts5817_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct trng_rts5817_cfg *cfg = dev->config;
	mem_addr_t regs = cfg->regs;
	uint32_t rng_data;
	uint32_t rngstatus;
	uint32_t chunk;
	uint32_t offset;
	uint32_t ret;

	if ((buffer == NULL) || (length == 0)) {
		return -EINVAL;
	}

	sys_write32(RNG_CONFIG_VAL, regs + R_RNG_CONFIG);
	sys_write32(RNG_FUN_EN | RNG_CLK_EN, regs + R_RNG_ENABLE);

	if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_IS_ENABLED) == 0x0, RNG_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("RNG enable timeout!");
		ret = -ETIMEDOUT;
		goto exit;
	}

	if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_HEALTH_TEST_ACTIVE) == 0x0,
		      RNG_TIMEOUT_US, k_yield())) {
		LOG_ERR("RNG health test timeout!");
		ret = -ETIMEDOUT;
		goto exit;
	}

	rngstatus = sys_read32(regs + R_RNG_STATUS);

	if ((rngstatus & RNG_HALTED) == RNG_HALTED) {
		sys_clear_bits(regs + R_RNG_ENABLE, RNG_FUN_EN);
		sys_set_bits(regs + R_RNG_FIFO_CLEAR, RNG_HT_CLR);

		if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & FIFO_CLEARED) != 0x0,
			      RNG_TIMEOUT_US, k_yield())) {
			LOG_ERR("RNG clear FIFO timeout!");
			ret = -ETIMEDOUT;
			goto exit;
		}

		/* Try again */
		sys_set_bits(regs + R_RNG_ENABLE, RNG_FUN_EN);

		if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_IS_ENABLED) == 0x0,
			      RNG_TIMEOUT_US, k_yield())) {
			LOG_ERR("RNG enable timeout!");
			ret = -ETIMEDOUT;
			goto exit;
		}

		if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_HEALTH_TEST_ACTIVE) == 0x0,
			      RNG_TIMEOUT_US, k_yield())) {
			LOG_ERR("RNG health test timeout!");
			ret = -ETIMEDOUT;
			goto exit;
		}

		rngstatus = sys_read32(regs + R_RNG_STATUS);
		if ((rngstatus & RNG_HALTED) == RNG_HALTED) {
			/* Halted agagin */
			LOG_ERR("Cannot get random data from %s!", dev->name);
			ret = -EIO;
			goto exit;
		}
	}

	offset = 0;
	if ((rngstatus & GENERATING_RANDOM_DATA) == GENERATING_RANDOM_DATA) {
		/* Collect rng data */
		while (offset < length) {
			rng_data = sys_read32(regs + R_RNG_DATA_OUT);
			chunk = MIN(4, length - offset);
			memcpy(&buffer[offset], &rng_data, chunk);
			offset += chunk;
		}
		ret = 0;
	} else {
		LOG_ERR("Cannot get random data from %s!", dev->name);
		ret = -EIO;
	}

exit:
	sys_clear_bits(regs + R_RNG_ENABLE, RNG_FUN_EN | RNG_CLK_EN);
	return ret;
}

static DEVICE_API(entropy, entropy_rts5817_api) = {
	.get_entropy = entropy_rts5817_get_entropy,
};

static const struct trng_rts5817_cfg trng_rts5817_cfg = {
	.regs = (mem_addr_t)DT_INST_REG_ADDR(0),
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, &trng_rts5817_cfg, PRE_KERNEL_1,
		      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_rts5817_api);
