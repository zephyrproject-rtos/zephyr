/*
 * Copyright (c) 2024 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_cc23x0_lgpt_pwm

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>

#include <driverlib/gpio.h>
#include <driverlib/clkctl.h>
#include <inc/hw_lgpt.h>
#include <inc/hw_lgpt1.h>
#include <inc/hw_lgpt3.h>
#include <inc/hw_types.h>
#include <inc/hw_evtsvt.h>
#include <inc/hw_memmap.h>

#include <zephyr/logging/log.h>
#define LOG_MODULE_NAME pwm_cc23x0_lgpt
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_PWM_LOG_LEVEL);

#define LGPT_CLK_PRESCALE(pres) ((pres + 1) << 8)

struct pwm_cc23x0_data {
	uint32_t prescale;
	uint32_t base_clk;
};

struct pwm_cc23x0_config {
	const uint32_t base; /* LGPT register base address */
	const struct pinctrl_dev_config *pcfg;
};

static inline void pwm_cc23x0_pm_policy_state_lock_get(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_get(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
	pm_policy_state_lock_get(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
#endif
}

static inline void pwm_cc23x0_pm_policy_state_lock_put(void)
{
#ifdef CONFIG_PM_DEVICE
	pm_policy_state_lock_put(PM_STATE_STANDBY, PM_ALL_SUBSTATES);
	pm_policy_state_lock_put(PM_STATE_RUNTIME_IDLE, PM_ALL_SUBSTATES);
#endif
}

static int pwm_cc23x0_set_cycles(const struct device *dev, uint32_t channel, uint32_t period,
				 uint32_t pulse, pwm_flags_t flags)
{
	const struct pwm_cc23x0_config *config = dev->config;

	LOG_DBG("set cycles period[%x] pulse[%x]", period, pulse);

	if (pulse == 0) {
		pwm_cc23x0_pm_policy_state_lock_get();
	}

	if ((config->base != LGPT3_BASE) && (pulse > 0xffff || period > 0xffff || pulse > period)) {
		/* LGPT0, LGPT1, LGPT2 - 16bit counters */
		LOG_ERR("Period of pulse out of range");
		return -EINVAL;
	} else if (pulse > 0xffffff || period > 0xffffff || pulse > period) {
		/* LGPT3 - 24bit counter */
		LOG_ERR("Period of pulse out of range");
		return -EINVAL;
	}

	if (channel == 0) {
		HWREG(config->base + LGPT_O_C0CC) = pulse;
		HWREG(config->base + LGPT_O_C0CFG) = 0x100 | 0xB;
	} else if (channel == 1) {
		HWREG(config->base + LGPT_O_C1CC) = pulse;
		HWREG(config->base + LGPT_O_C1CFG) = 0x200 | 0xB;
	} else if (channel == 2) {
		HWREG(config->base + LGPT_O_C2CC) = pulse;
		HWREG(config->base + LGPT_O_C2CFG) = 0x400 | 0xB;
	} else {
		LOG_ERR("Invalid chan ID");
		return -ENOTSUP;
	}

	/* get it from flags */
	HWREG(config->base + LGPT_O_CTL) = LGPT_CTL_MODE_UPDWN_PER;
	HWREG(config->base + LGPT_O_TGT) = period;

	/* Activate LGPT */
	HWREG(config->base + LGPT_O_STARTCFG) = 0x1;

	if (pulse > 0) {
		pwm_cc23x0_pm_policy_state_lock_put();
	}

	return 0;
}

static int pwm_cc23x0_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					 uint64_t *cycles)
{
	struct pwm_cc23x0_data *data = dev->data;

	*cycles = data->base_clk / (data->prescale + 1);

	return 0;
}

static const struct pwm_driver_api pwm_cc23x0_driver_api = {
	.set_cycles = pwm_cc23x0_set_cycles,
	.get_cycles_per_sec = pwm_cc23x0_get_cycles_per_sec,
};

static int pwm_cc23x0_clock_action(const struct device *dev, bool activate)
{
	const struct pwm_cc23x0_config *config = dev->config;
	struct pwm_cc23x0_data *data = dev->data;
	uint32_t lgpt_clk_id = 0;

	switch (config->base) {
	case LGPT0_BASE:
		lgpt_clk_id = CLKCTL_LGPT0;
		break;
	case LGPT1_BASE:
		lgpt_clk_id = CLKCTL_LGPT1;
		break;
	case LGPT2_BASE:
		lgpt_clk_id = CLKCTL_LGPT2;
		break;
	case LGPT3_BASE:
		lgpt_clk_id = CLKCTL_LGPT3;
		break;
	default:
		return -EINVAL;
	}

	if (activate) {
		CLKCTLEnable(CLKCTL_BASE, lgpt_clk_id);
		HWREG(config->base + LGPT_O_PRECFG) = LGPT_CLK_PRESCALE(data->prescale);
		HWREG(EVTSVT_BASE + EVTSVT_O_LGPTSYNCSEL) = EVTSVT_LGPTSYNCSEL_PUBID_SYSTIM0;
	} else {
		CLKCTLDisable(CLKCTL_BASE, lgpt_clk_id);
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE

static int pwm_cc23x0_pm_action(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		pwm_cc23x0_clock_action(dev, false);
		return 0;
	case PM_DEVICE_ACTION_RESUME:
		pwm_cc23x0_clock_action(dev, true);
		return 0;
	default:
		return -ENOTSUP;
	}
}

#endif /* CONFIG_PM_DEVICE */

#define DT_TIMER(idx)           DT_INST_PARENT(idx)
#define DT_TIMER_BASE_ADDR(idx) (DT_REG_ADDR(DT_TIMER(idx)))

static int pwm_cc23x0_init(const struct device *dev)
{
	const struct pwm_cc23x0_config *config = dev->config;

	int ret;

	LOG_DBG("PWM cc23x0 base=[%x]", config->base);

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("[ERR] failed to setup PWM pinctrl");
		return ret;
	}

	pwm_cc23x0_clock_action(dev, true);

	return 0;
}

#define PWM_DEVICE_INIT(idx)                                                                       \
	PM_DEVICE_DT_INST_DEFINE(idx, pwm_cc23x0_pm_action);                                       \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	LOG_INSTANCE_REGISTER(LOG_MODULE_NAME, idx, CONFIG_PWM_LOG_LEVEL);                         \
                                                                                                   \
	static const struct pwm_cc23x0_config pwm_cc23x0_##idx##_config = {                        \
		.base = DT_TIMER_BASE_ADDR(idx),                                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
	};                                                                                         \
                                                                                                   \
	static struct pwm_cc23x0_data pwm_cc23x0_##idx##_data = {                                  \
		.prescale = DT_PROP(DT_INST_PARENT(idx), clk_prescale),                            \
		.base_clk = DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency),                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, pwm_cc23x0_init, NULL, &pwm_cc23x0_##idx##_data,                \
			      &pwm_cc23x0_##idx##_config, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,   \
			      &pwm_cc23x0_driver_api)

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT);
