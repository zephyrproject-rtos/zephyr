/*
 * Copyright (c) 2025 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT neorv32_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(pwm_neorv32, CONFIG_PWM_LOG_LEVEL);

/* NEORV32 PWM CHANNEL_CFG[0..15] register bits */
#define NEORV32_PWM_CFG_EN   BIT(31)
#define NEORV32_PWM_CFG_PRSC GENMASK(30, 28)
#define NEORV32_PWM_CFG_POL  BIT(27)
#define NEORV32_PWM_CFG_CDIV GENMASK(17, 8)
#define NEORV32_PWM_CFG_DUTY GENMASK(7, 0)

/* Maximum number of PWMs supported */
#define NEORV32_PWM_CHANNELS 16

struct neorv32_pwm_config {
	const struct device *syscon;
	mm_reg_t base;
};

static inline void neorv32_pwm_write_channel_cfg(const struct device *dev, uint8_t channel,
						 uint32_t cfg)
{
	const struct neorv32_pwm_config *config = dev->config;

	__ASSERT_NO_MSG(channel < NEORV32_PWM_CHANNELS);

	sys_write32(cfg, config->base + (channel * sizeof(uint32_t)));
}

static int neorv32_pwm_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	static const uint16_t cdiv_max = FIELD_GET(NEORV32_PWM_CFG_CDIV, NEORV32_PWM_CFG_CDIV);
	static const uint16_t steps = FIELD_GET(NEORV32_PWM_CFG_DUTY, NEORV32_PWM_CFG_DUTY) + 1U;
	static const uint16_t prsc_tbl[] = {2, 4, 8, 64, 128, 1024, 2048, 4096};
	uint32_t cfg = 0U;
	uint8_t duty = 0U;
	uint16_t cdiv;
	uint8_t prsc;

	if (channel >= NEORV32_PWM_CHANNELS) {
		LOG_ERR("invalid PWM channel %u", channel);
		return -EINVAL;
	}

	if (pulse_cycles == 0U) {
		/* Constant inactive level */
		if ((flags & PWM_POLARITY_INVERTED) != 0U) {
			cfg |= NEORV32_PWM_CFG_POL;
		}
	} else if (pulse_cycles == period_cycles) {
		/* Constant active level */
		if ((flags & PWM_POLARITY_INVERTED) == 0U) {
			cfg |= NEORV32_PWM_CFG_POL;
		}
	} else {
		/* PWM enabled */
		if ((flags & PWM_POLARITY_INVERTED) != 0U) {
			cfg |= NEORV32_PWM_CFG_POL;
		}

		for (prsc = 0; prsc < ARRAY_SIZE(prsc_tbl); prsc++) {
			if (period_cycles / prsc_tbl[prsc] <= steps * (1U + cdiv_max)) {
				break;
			}
		}

		cdiv = DIV_ROUND_CLOSEST(DIV_ROUND_CLOSEST(period_cycles, prsc_tbl[prsc]), steps) -
		       1U;

		duty = CLAMP(DIV_ROUND_CLOSEST((uint64_t)(pulse_cycles * steps), period_cycles),
			     1U, steps - 1U);

		cfg |= NEORV32_PWM_CFG_EN | FIELD_PREP(NEORV32_PWM_CFG_PRSC, prsc) |
		       FIELD_PREP(NEORV32_PWM_CFG_CDIV, cdiv) |
		       FIELD_PREP(NEORV32_PWM_CFG_DUTY, duty);
	}

	neorv32_pwm_write_channel_cfg(dev, channel, cfg);

	return 0;
}

static int neorv32_pwm_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					  uint64_t *cycles)
{
	const struct neorv32_pwm_config *config = dev->config;
	uint32_t clk;
	int err;

	if (channel >= NEORV32_PWM_CHANNELS) {
		LOG_ERR("invalid PWM channel %u", channel);
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_CLK, &clk);
	if (err < 0) {
		LOG_ERR("failed to determine clock rate (err %d)", err);
		return -EIO;
	}

	*cycles = clk;

	return 0;
}

static int neorv32_pwm_init(const struct device *dev)
{
	const struct neorv32_pwm_config *config = dev->config;
	uint32_t features;
	int err;

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("syscon device not ready");
		return -EINVAL;
	}

	err = syscon_read_reg(config->syscon, NEORV32_SYSINFO_SOC, &features);
	if (err < 0) {
		LOG_ERR("failed to determine implemented features (err %d)", err);
		return -EIO;
	}

	if ((features & NEORV32_SYSINFO_SOC_IO_PWM) == 0) {
		LOG_ERR("neorv32 pwm not supported");
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(pwm, neorv32_pwm_driver_api) = {
	.set_cycles = neorv32_pwm_set_cycles,
	.get_cycles_per_sec = neorv32_pwm_get_cycles_per_sec,
};

#define NEORV32_PWM_INIT(n)                                                                        \
	static const struct neorv32_pwm_config neorv32_pwm_##n##_config = {                        \
		.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(n, syscon)),                               \
		.base = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, neorv32_pwm_init, NULL, NULL, &neorv32_pwm_##n##_config,          \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &neorv32_pwm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NEORV32_PWM_INIT)
