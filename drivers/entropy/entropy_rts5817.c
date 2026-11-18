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
#include <string.h>
#include "entropy_rts5817.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy_rts5817, CONFIG_ENTROPY_LOG_LEVEL);

struct trng_rts5817_cfg {
	mem_addr_t regs;
	const struct device *otp_dev;
};

struct trng_rts5817_data {
	struct k_mutex mutex;
};

static int rts5817_rng_wait_ready(mem_addr_t regs)
{
	/* RNG_IS_ENABLED clears once the RNG is enabled */
	if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_IS_ENABLED) == 0x0, RNG_TIMEOUT_US,
		      k_yield())) {
		LOG_ERR("RNG enable timeout!");
		return -ETIMEDOUT;
	}

	if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & RNG_HEALTH_TEST_ACTIVE) == 0x0,
		      RNG_TIMEOUT_US, k_yield())) {
		LOG_ERR("RNG health test timeout!");
		return -ETIMEDOUT;
	}

	return 0;
}

static int entropy_rts5817_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct trng_rts5817_cfg *cfg = dev->config;
	struct trng_rts5817_data *data = dev->data;
	mem_addr_t regs = cfg->regs;
	uint32_t rngstatus;
	int ret;

	if ((buffer == NULL) || (length == 0)) {
		return -EINVAL;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	sys_write32(RNG_CONFIG_PATH_RNG, regs + R_RNG_CONFIG);
	sys_write32(RNG_FUN_EN | RNG_CLK_EN, regs + R_RNG_ENABLE);

	ret = rts5817_rng_wait_ready(regs);
	if (ret != 0) {
		goto exit;
	}

	rngstatus = sys_read32(regs + R_RNG_STATUS);

	if ((rngstatus & RNG_HALTED) != 0) {
		sys_clear_bits(regs + R_RNG_ENABLE, RNG_FUN_EN);
		sys_write32(RNG_HT_CLR, regs + R_RNG_FIFO_CLEAR);

		if (!WAIT_FOR((sys_read32(regs + R_RNG_STATUS) & FIFO_CLEARED) != 0x0,
			      RNG_TIMEOUT_US, k_yield())) {
			LOG_ERR("RNG clear FIFO timeout!");
			ret = -ETIMEDOUT;
			goto exit;
		}

		/* Try again */
		sys_set_bits(regs + R_RNG_ENABLE, RNG_FUN_EN);

		ret = rts5817_rng_wait_ready(regs);
		if (ret != 0) {
			goto exit;
		}

		rngstatus = sys_read32(regs + R_RNG_STATUS);
	}

	if (((rngstatus & RNG_HALTED) != 0) || ((rngstatus & GENERATING_RANDOM_DATA) == 0)) {
		LOG_ERR("Cannot get random data from %s!", dev->name);
		ret = -EIO;
		goto exit;
	}

	/* Reads stall until the next word is ready */
	for (uint32_t offset = 0; offset < length;) {
		uint32_t rng_data = sys_read32(regs + R_RNG_DATA_OUT);
		uint32_t chunk = MIN(sizeof(rng_data), length - offset);

		memcpy(&buffer[offset], &rng_data, chunk);
		offset += chunk;
	}
	ret = 0;

exit:
	sys_clear_bits(regs + R_RNG_ENABLE, RNG_FUN_EN | RNG_CLK_EN);
	k_mutex_unlock(&data->mutex);
	return ret;
}

static int entropy_rts5817_init(const struct device *dev)
{
	const struct trng_rts5817_cfg *cfg = dev->config;
	struct trng_rts5817_data *data = dev->data;

	if (!device_is_ready(cfg->otp_dev)) {
		LOG_ERR("OTP device not ready");
		return -ENODEV;
	}

	/* RNG_CLK_EN comes up set out of the PUF reset */
	sys_clear_bits(cfg->regs + R_RNG_ENABLE, RNG_FUN_EN | RNG_CLK_EN);

	k_mutex_init(&data->mutex);

	return 0;
}

static DEVICE_API(entropy, entropy_rts5817_api) = {
	.get_entropy = entropy_rts5817_get_entropy,
};

/* The TRNG is in the OTP power/clock domain, so init after the OTP driver */
#define TRNG_RTS5817_INIT(inst)                                                                    \
	static struct trng_rts5817_data trng_rts5817_data_##inst;                                  \
	static const struct trng_rts5817_cfg trng_rts5817_cfg_##inst = {                           \
		.regs = (mem_addr_t)DT_INST_REG_ADDR(inst),                                        \
		.otp_dev = DEVICE_DT_GET(DT_INST_PHANDLE(inst, ocotp)),                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, entropy_rts5817_init, NULL, &trng_rts5817_data_##inst,         \
			      &trng_rts5817_cfg_##inst, POST_KERNEL,                               \
			      UTIL_INC(CONFIG_OTP_INIT_PRIORITY), &entropy_rts5817_api);

DT_INST_FOREACH_STATUS_OKAY(TRNG_RTS5817_INIT)
