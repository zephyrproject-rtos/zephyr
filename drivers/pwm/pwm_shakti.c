/*
 * Copyright (c) 2026 RISE Lab, IIT Madras
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT shakti_pwm

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>

#define PWM_PRESCALER			0x00	/* shared clock divider */
#define PWM_MODULE_BASE			0x04	/* module 0 starts here */
#define PWM_MODULE_OFFSET		0x10	/* stride between modules */

/* Register offsets within one module */
#define PWM_CONTROL				0x00	/* control (16-bit) */
#define PWM_PERIOD				0x04	/* period in PWM clock ticks (32-bit) */
#define PWM_DUTY				0x08	/* duty in PWM clock ticks (32-bit) */
#define PWM_DEADBAND			0x0C	/* deadband delay (16-bit) */

/*
 * registers have mixed widths: clock, control and deadband are 16-bit, period
 * and duty are 32-bit access width must match the register or the bus never
 * acknowledges
 */
#define REG16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define REG32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

/* Control register bits */
#define PWM_ENABLE				BIT(0)
#define PWM_START				BIT(1)
#define PWM_OUTPUT_ENABLE		BIT(2)
#define PWM_OUTPUT_POLARITY		BIT(3)
#define PWM_UPDATE_ENABLE		BIT(12)	/* latch new period/duty values */

#define PWM_SHAKTI_NUM_CHANNELS	6
#define PWM_SHAKTI_PRESCALER	50	/* PWM clock = input clock / 50 (800 kHz for 40 MHz) */

struct pwm_shakti_config {
	uintptr_t base;
	uint32_t clock_freq;
};

static int pwm_shakti_set_cycles(const struct device *dev, uint32_t channel,
				 uint32_t period_cycles, uint32_t pulse_cycles,
				 pwm_flags_t flags)
{
	const struct pwm_shakti_config *cfg = dev->config;

	if (channel >= PWM_SHAKTI_NUM_CHANNELS) {
		return -EINVAL;
	}

	if (period_cycles == 0U) {
		return -ENOTSUP;
	}

	uintptr_t cbase = cfg->base + PWM_MODULE_BASE + (channel * PWM_MODULE_OFFSET);
	uint32_t control = PWM_ENABLE | PWM_START | PWM_OUTPUT_ENABLE | PWM_UPDATE_ENABLE;

	if (flags & PWM_POLARITY_INVERTED) {
		control |= PWM_OUTPUT_POLARITY;
	}

	REG32(cbase + PWM_PERIOD) = period_cycles;
	REG32(cbase + PWM_DUTY) = pulse_cycles;
	REG16(cbase + PWM_DEADBAND) = 0;
	REG16(cbase + PWM_CONTROL) = (uint16_t)control;

	return 0;
}

static int pwm_shakti_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	const struct pwm_shakti_config *cfg = dev->config;

	if (channel >= PWM_SHAKTI_NUM_CHANNELS) {
		return -EINVAL;
	}

	*cycles = (uint64_t)(cfg->clock_freq / PWM_SHAKTI_PRESCALER);

	return 0;
}

static int pwm_shakti_init(const struct device *dev)
{
	const struct pwm_shakti_config *cfg = dev->config;

	/* prescaler value set in upper bit */
	REG16(cfg->base + PWM_PRESCALER) = (uint16_t)(PWM_SHAKTI_PRESCALER << 1);

	return 0;
}

static DEVICE_API(pwm, pwm_shakti_driver_api) = {
	.set_cycles = pwm_shakti_set_cycles,
	.get_cycles_per_sec = pwm_shakti_get_cycles_per_sec,
};

#define PWM_SHAKTI_INIT(n)                                          \
	static const struct pwm_shakti_config pwm_shakti_cfg_##n = {    \
		.base = DT_INST_REG_ADDR(n),                                \
		.clock_freq = DT_INST_PROP(n, clock_frequency),             \
	};                                                              \
	DEVICE_DT_INST_DEFINE(n, pwm_shakti_init, NULL,                 \
			      NULL, &pwm_shakti_cfg_##n,                        \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,            \
			      &pwm_shakti_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_SHAKTI_INIT)
