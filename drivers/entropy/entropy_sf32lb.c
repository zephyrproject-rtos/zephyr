/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT sifli_sf32lb_trng

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control/sf32lb.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(entropy_sf32lb, CONFIG_ENTROPY_LOG_LEVEL);

#define TRNG_CTRL offsetof(TRNG_TypeDef, CTRL)
#define TRNG_STAT offsetof(TRNG_TypeDef, STAT)
#define TRNG_RAND offsetof(TRNG_TypeDef, RAND_NUM0)

#define TRNG_RAND_NUM_MAX (8U)

#define TRNG_RAND_MASK (TRNG_RAND_NUM_MAX - 1U)

struct entropy_sf32lb_config {
	uintptr_t base;
	struct sf32lb_clock_dt_spec clock;
};

static int entropy_sf32lb_get_entropy(const struct device *dev, uint8_t *buffer, uint16_t length)
{
	const struct entropy_sf32lb_config *config = dev->config;
	uint32_t buf[TRNG_RAND_NUM_MAX];
	uint16_t bytes;

	while (length) {
		sys_set_bit(config->base + TRNG_CTRL, TRNG_CTRL_GEN_SEED_START_Pos);
		while (!sys_test_bit(config->base + TRNG_STAT, TRNG_STAT_SEED_VALID_Pos)) {
		}

		/* Generate random data */
		sys_set_bit(config->base + TRNG_CTRL, TRNG_CTRL_GEN_RAND_NUM_START_Pos);
		while (!sys_test_bit(config->base + TRNG_STAT, TRNG_STAT_RAND_NUM_VALID_Pos)) {
		}

		for (uint8_t i = 0U; i < TRNG_RAND_NUM_MAX; i++) {
			buf[i] = sys_read32(config->base + TRNG_RAND + (i * 4U));
		}

		bytes = MIN(length, sizeof(buf));

		memcpy(buffer, buf, bytes);

		length -= bytes;
		buffer += bytes;
	}

	return 0;
}

static DEVICE_API(entropy, entropy_sf32lb_api) = {
	.get_entropy = entropy_sf32lb_get_entropy,
};

static int entropy_sf32lb_init(const struct device *dev)
{
	const struct entropy_sf32lb_config *config = dev->config;

	if (!sf32lb_clock_is_ready_dt(&config->clock)) {
		return -ENODEV;
	}

	return sf32lb_clock_control_on_dt(&config->clock);
}

#define ENTROPY_SF32LB_DEFINE(n)                                                                   \
	static const struct entropy_sf32lb_config entropy_sf32lb_config_##n = {                    \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.clock = SF32LB_CLOCK_DT_INST_SPEC_GET(n),                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, entropy_sf32lb_init, NULL, NULL,                                  \
			      &entropy_sf32lb_config_##n, PRE_KERNEL_1,                            \
			      CONFIG_ENTROPY_INIT_PRIORITY, &entropy_sf32lb_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_SF32LB_DEFINE)
