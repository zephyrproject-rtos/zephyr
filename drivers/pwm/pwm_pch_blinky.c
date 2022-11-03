/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_blinky_pwm

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>

#define PWM_CTRL_REG            0x304
#define PWM_ENABLE              0x80000000
#define PWM_SWUP                0x40000000
#define PWM_FREQ_INT_SHIFT      22
#define PWM_FREQ_MAX            0x100
#define PWM_DUTY_MAX            0x100

struct bk_intel_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t clock_freq;
	uint32_t max_pins;
};

struct bk_intel_runtime {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	uint32_t pwm_ctrl;
};

static int bk_intel_set_cycles(const struct device *dev, uint32_t pin,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	uint32_t ret = 0;
	uint32_t val;
	uint32_t period;
	uint32_t duty;
	struct bk_intel_runtime *rt = dev->data;
	const struct bk_intel_config *cfg = dev->config;

	if (pin > cfg->max_pins) {
		ret = -EINVAL;
		goto err;
	}

	period = (period_cycles * PWM_FREQ_MAX) / cfg->clock_freq;
	duty = (pulse_cycles * PWM_DUTY_MAX) / period_cycles;

	val = PWM_DUTY_MAX - duty;
	val = val | (period << PWM_FREQ_INT_SHIFT);
	val = val | PWM_ENABLE | PWM_SWUP;

	if (period >= PWM_FREQ_MAX) {
		ret = -EINVAL;
		goto err;
	}

	if (duty >= PWM_DUTY_MAX) {
		ret = -EINVAL;
		goto err;
	}

	sys_write32(val, rt->pwm_ctrl);
err:
	return ret;
}

static int bk_intel_get_cycles_per_sec(const struct device *dev, uint32_t pin,
				       uint64_t *cycles)
{
	const struct bk_intel_config *cfg = dev->config;

	if (pin > cfg->max_pins) {
		return -EINVAL;
	}

	*cycles = cfg->clock_freq;

	return 0;
}

static const struct pwm_driver_api api_funcs = {
	.set_cycles = bk_intel_set_cycles,
	.get_cycles_per_sec = bk_intel_get_cycles_per_sec,
};

static int bk_intel_init(const struct device *dev)
{
	struct bk_intel_runtime *runtime = dev->data;
	const struct bk_intel_config *config = dev->config;

	device_map(&runtime->reg_base,
		   config->reg_base.phys_addr & ~0xFFU,
		   config->reg_base.size,
		   K_MEM_CACHE_NONE);

	runtime->pwm_ctrl = runtime->reg_base + PWM_CTRL_REG;

	return 0;
}

#define BK_INTEL_DEV_CFG(n)						       \
	static const struct bk_intel_config bk_cfg_##n = {		       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	       \
		.max_pins = DT_INST_PROP(n, max_pins),			       \
		.clock_freq = DT_INST_PROP(n, clock_frequency),		       \
	};								       \
									       \
	static struct bk_intel_runtime bk_rt_##n;			       \
	DEVICE_DT_INST_DEFINE(n, &bk_intel_init, NULL,			       \
			      &bk_rt_##n, &bk_cfg_##n,			       \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &api_funcs);				       \

DT_INST_FOREACH_STATUS_OKAY(BK_INTEL_DEV_CFG)
