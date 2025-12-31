/*
 * Copyright (c) 2025 James Bennion-Pedley
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_rng

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(entropy, CONFIG_ENTROPY_LOG_LEVEL);

#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/drivers/clock_control.h>

#include <hal_ch32fun.h>

struct entropy_wch_config {
	RNG_TypeDef *regs;
	const struct device *clk_dev;
	uint8_t clk_id;
};

static inline uint32_t entropy_wch_get_u32(const struct device *dev)
{
	const struct entropy_wch_config *config = dev->config;

	if (!(config->regs->SR & RNG_SR_DRDY)) {
		LOG_WRN("Invalid RNG Data");
		return 0;
	}

	return config->regs->DR;
}

static int entropy_wch_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	uint8_t *buf_bytes = buf;

	while (len > 0) {
		uint32_t word = entropy_wch_get_u32(dev);
		uint32_t to_copy = MIN(sizeof(word), len);

		memcpy(buf_bytes, &word, to_copy);
		buf_bytes += to_copy;
		len -= to_copy;
	}

	return 0;
}

static int entropy_wch_init(const struct device *dev)
{
	const struct entropy_wch_config *config = dev->config;
	clock_control_subsys_t clock_sys;
	int ret = 0;

	clock_sys = (clock_control_subsys_t *)(uintptr_t)config->clk_id;
	ret = clock_control_on(config->clk_dev, clock_sys);

	if (ret < 0) {
		LOG_ERR("Could not enable RNG Clock");
		return ret;
	}

	config->regs->CR |= RNG_CR_RNGEN;
	return ret;
}

static DEVICE_API(entropy, entropy_wch_api_funcs) = {.get_entropy = entropy_wch_get_entropy};

#define ENTROPY_WCH_DEVICE(inst)                                                                   \
	static const struct entropy_wch_config entropy_wch_config_##inst = {                       \
		.regs = (RNG_TypeDef *)DT_INST_REG_ADDR(inst),                                     \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_IDX(inst, 0)),                     \
		.clk_id = DT_INST_CLOCKS_CELL_BY_IDX(inst, 0, id),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, entropy_wch_init, NULL, NULL, &entropy_wch_config_##inst,      \
			      PRE_KERNEL_1, CONFIG_ENTROPY_INIT_PRIORITY, &entropy_wch_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_WCH_DEVICE)
