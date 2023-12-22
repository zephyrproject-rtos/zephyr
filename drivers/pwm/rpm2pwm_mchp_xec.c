/*
 * Copyright (c) 2023 Silicom Connectivity Solutions, Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <soc.h>

LOG_MODULE_REGISTER(rpm2pwm_mchp_xec, CONFIG_PWM_LOG_LEVEL);

struct rpm2pwm_regs {
	volatile uint16_t SETTING;
	volatile uint16_t CONFIG;
	volatile uint8_t  DIVIDE;
	volatile uint8_t  GAIN;
	volatile uint8_t  SPINUP;
	volatile uint8_t  STEP;
	volatile uint8_t  MINDRIVE;
	volatile uint8_t  VALIDCNT;
	volatile uint8_t  FAILBAND;
	volatile uint16_t TARGET;
	volatile uint16_t TACH;
	volatile uint8_t  PWMFREQ;
	volatile uint8_t  STATUS;
};

struct fan_config {
	const struct pwm_dt_spec *pwm;
	uint8_t edges;
};

struct rpm2pwm_xec_config {
	struct rpm2pwm_regs *const regs;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	const struct pinctrl_dev_config *pcfg;
	const struct fan_config *fan;
};

struct rpm2pwm_tach_xec_config {
	struct device * const parent;
};

struct rpm2pwm_tach_xec_data {
	uint16_t count;
};

struct rpm2pwm_xec_data {
	uint32_t config;
};

int rpm2pwm_tach_xec_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	ARG_UNUSED(chan);

	const struct rpm2pwm_tach_xec_config * const cfg = dev->config;
	struct device * const parent = cfg->parent;
	const struct rpm2pwm_xec_config * const parent_cfg = parent->config;
	struct rpm2pwm_regs * const regs = parent_cfg->regs;

	struct rpm2pwm_tach_xec_data * const data = dev->data;

	data->count = 3932160 / (regs->TACH >> 3);

	return 0;
}

static int rpm2pwm_tach_xec_channel_get(const struct device *dev,
					enum sensor_channel chan,
					struct sensor_value *val)
{
	struct rpm2pwm_tach_xec_data * const data = dev->data;

	if (chan != SENSOR_CHAN_RPM) {
		return -ENOTSUP;
	}

	val->val1 = data->count;
	val->val2 = 0U;

	return 0;
}

static const struct sensor_driver_api rpm2pwm_tach_xec_driver_api = {
	.sample_fetch = rpm2pwm_tach_xec_sample_fetch,
	.channel_get = rpm2pwm_tach_xec_channel_get,
};

static int rpm2pwm_tach_xec_init(const struct device *dev)
{
	const struct rpm2pwm_tach_xec_config *config = dev->config;

	if (!device_is_ready(config->parent))
		return -ENODEV;

	return 0;
}

static int rpm2pwm_xec_set_cycles_internal(const struct device *dev, uint32_t channel,
					   uint32_t period_count, uint32_t pulse_count,
					   pwm_flags_t flags)
{
	const struct rpm2pwm_xec_config * const cfg = dev->config;
	struct rpm2pwm_regs * const regs = cfg->regs;
	const struct fan_config * const fan = cfg->fan;
	uint8_t spinup;
	uint16_t config;

	spinup = regs->SPINUP;

	/*
	 * TODO: Figure out DTS flag options that are appropriate:
	 * if (flags & RPM2PWM_XEC_FLAG_SPIN_NOKICK)
	 */
	spinup |= (1 << 5);

	regs->SPINUP |= spinup;

	config = (((fan->edges / 2) - 1) << 3);

	regs->TARGET = (3932160 / pulse_count) << 3;

	regs->CONFIG |= (config | 0x80);

	return 0;
}

static int rpm2pwm_xec_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles,
				  pwm_flags_t flags)
{
	if (channel > 0) {
		return -EIO;
	}

	rpm2pwm_xec_set_cycles_internal(dev, channel, period_cycles, pulse_cycles, flags);
	return 0;
}

static int rpm2pwm_xec_get_cycles_per_sec(const struct device *dev,
				      uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(dev);

	if (channel > 0) {
		return -EIO;
	}

	if (cycles) {
		/* User does not have to know about lowest clock,
		 * the driver will select the most relevant one.
		 */
		*cycles = 32768;
	}

	return 0;
}

static const struct pwm_driver_api rpm2pwm_xec_driver_api = {
	.set_cycles = rpm2pwm_xec_set_cycles,
	.get_cycles_per_sec = rpm2pwm_xec_get_cycles_per_sec,
};

static int rpm2pwm_xec_init(const struct device *dev)
{
	const struct rpm2pwm_xec_config *const cfg = dev->config;
	struct rpm2pwm_regs *const regs = cfg->regs;

	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	regs->CONFIG &= ~(3 << 5);
	regs->MINDRIVE = 0x33;

	if (ret != 0) {
		LOG_ERR("XEC RPM2PWM pinctrl init failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define XEC_RPM2PWM_CONFIG(inst)							\
											\
	static const struct pwm_dt_spec fan_pwm_##inst =				\
		PWM_DT_SPEC_GET(DT_CHILD(DT_INST_CHILD(inst, fan), fan));		\
											\
	static const struct fan_config fan_##inst##_cfg = {				\
		.pwm = &fan_pwm_##inst,							\
		.edges = DT_PROP(DT_CHILD(DT_INST_CHILD(inst, fan), fan), edges),	\
	};										\
											\
	static struct rpm2pwm_xec_config rpm2pwm_xec_config_##inst = {			\
		.regs = (struct rpm2pwm_regs * const)DT_INST_REG_ADDR(inst),		\
		.pcr_idx = (uint8_t)DT_INST_PROP_BY_IDX(inst, pcrs, 0),			\
		.pcr_pos = (uint8_t)DT_INST_PROP_BY_IDX(inst, pcrs, 1),			\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),				\
		.fan = &fan_##inst##_cfg,						\
	};

#define XEC_RPM2PWM_DEVICE_INIT(index)					\
									\
	static struct rpm2pwm_xec_data rpm2pwm_xec_data_##index;	\
									\
	PINCTRL_DT_INST_DEFINE(index);					\
									\
	XEC_RPM2PWM_CONFIG(index);					\
									\
	DEVICE_DT_INST_DEFINE(index, &rpm2pwm_xec_init,			\
			      NULL,					\
			      &rpm2pwm_xec_data_##index,		\
			      &rpm2pwm_xec_config_##index, POST_KERNEL,	\
			      CONFIG_PWM_INIT_PRIORITY,			\
			      &rpm2pwm_xec_driver_api);

#define DT_DRV_COMPAT microchip_xec_rpm2pwm
DT_INST_FOREACH_STATUS_OKAY(XEC_RPM2PWM_DEVICE_INIT)
#undef DT_DRV_COMPAT

#define XEC_RPM2PWM_TACH_CONFIG(inst)							\
	static struct rpm2pwm_tach_xec_config rpm2pwm_tach_xec_config_##inst = {	\
	       .parent = (struct device *const)DEVICE_DT_GET(DT_INST_PARENT(inst)),	\
	};

#define XEC_RPM2PWM_TACH_DEVICE_INIT(index)					\
	static struct rpm2pwm_tach_xec_data rpm2pwm_tach_xec_data_##index;	\
										\
	XEC_RPM2PWM_TACH_CONFIG(index);						\
										\
	DEVICE_DT_INST_DEFINE(index, &rpm2pwm_tach_xec_init, NULL,		\
			      &rpm2pwm_tach_xec_data_##index,			\
			      &rpm2pwm_tach_xec_config_##index, POST_KERNEL,	\
			      CONFIG_PWM_INIT_PRIORITY,				\
			      &rpm2pwm_tach_xec_driver_api);

#define DT_DRV_COMPAT microchip_xec_rpm2pwm_tach
DT_INST_FOREACH_STATUS_OKAY(XEC_RPM2PWM_TACH_DEVICE_INIT)
#undef DT_DRV_COMPAT
