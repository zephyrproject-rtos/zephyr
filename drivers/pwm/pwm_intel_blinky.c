/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_blinky_pwm

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>

#define PWM_ENABLE              0x80000000
#define PWM_SWUP                0x40000000
#define PWM_FREQ_INT_SHIFT      8
#define PWM_BASE_UNIT_FRACTION	14
#define PWM_FREQ_MAX            0x100
#define PWM_DUTY_MAX            0x100

struct bk_intel_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint32_t reg_offset;
	uint32_t clock_freq;
	uint32_t max_pins;
};

struct bk_intel_runtime {
	DEVICE_MMIO_NAMED_RAM(reg_base);
};

static int bk_intel_set_cycles(const struct device *dev, uint32_t pin,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	struct bk_intel_runtime *rt = dev->data;
	const struct bk_intel_config *cfg = dev->config;
	uint32_t ret = 0;
	uint32_t val = 0;
	uint32_t duty;
	float period;
	float out_freq;
	uint32_t base_unit;

	if (pin >= cfg->max_pins) {
		ret = -EINVAL;
		goto err;
	}

	out_freq = cfg->clock_freq / (float) period_cycles;
	period = (out_freq * PWM_FREQ_MAX) / cfg->clock_freq;
	base_unit = (uint32_t) (period * (1 << PWM_BASE_UNIT_FRACTION));
	duty = (pulse_cycles * PWM_DUTY_MAX) / period_cycles;

	if (duty) {
		val = PWM_DUTY_MAX - duty;
		val |= (base_unit << PWM_FREQ_INT_SHIFT);
	} else {
		val = PWM_DUTY_MAX - 1;
	}

	val |= PWM_ENABLE | PWM_SWUP;

	if (period >= PWM_FREQ_MAX) {
		ret = -EINVAL;
		goto err;
	}

	if (duty > PWM_DUTY_MAX) {
		ret = -EINVAL;
		goto err;
	}

	sys_write32(val, rt->reg_base + cfg->reg_offset);
err:
	return ret;
}

static int bk_intel_get_cycles_per_sec(const struct device *dev, uint32_t pin,
				       uint64_t *cycles)
{
	const struct bk_intel_config *cfg = dev->config;

	if (pin >= cfg->max_pins) {
		return -EINVAL;
	}

	*cycles = cfg->clock_freq;

	return 0;
}

static DEVICE_API(pwm, api_funcs) = {
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

	return 0;
}

#define BK_INTEL_DEV_CFG(n)						       \
	static const struct bk_intel_config bk_cfg_##n = {		       \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),	       \
		.reg_offset = DT_INST_PROP(n, reg_offset),		       \
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
