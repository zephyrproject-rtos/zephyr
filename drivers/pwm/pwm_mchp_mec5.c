/*
 * Copyright (c) 2024 Microchip Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_pwm

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <soc.h>

/* MEC5 HAL */
#include <device_mec5.h>
#include <mec_ecia_api.h>
#include <mec_pwm_api.h>

LOG_MODULE_REGISTER(pwm_mchp_mec5, CONFIG_PWM_LOG_LEVEL);

struct pwm_mec5_dev_cfg {
	struct pwm_regs * const regs;
	const struct pinctrl_dev_config *pcfg;
};

struct pwm_mec5_dev_data {
	uint8_t enabled;
};

/* Set the period and pulse width for a single PWM output.
 *
 * The PWM period and pulse width will synchronously be set to the new values
 * without glitches in the PWM signal, but the call will not block for the
 * change to take effect.
 *
 * Not all PWM controllers support synchronous, glitch-free updates of the
 * PWM period and pulse width. Depending on the hardware, changing the PWM
 * period and/or pulse width may cause a glitch in the generated PWM signal.
 *
 * Passing 0 as @p pulse will cause the pin to be driven to a constant
 * inactive level.
 * Passing a non-zero @p pulse equal to @p period will cause the pin
 * to be driven to a constant active level.
 *
 * parameters:
 * dev is PWM device instance.
 * channel is PWM channel.
 * period_cycles is Period (in clock cycles) set to the PWM. HW specific.
 * pulse_cycles is Pulse width (in clock cycles) set to the PWM. HW specific.
 * flags are flags for pin configuration.
 *
 * returns:
 * 0 If successful.
 * -EINVAL If pulse > period.
 * -EIO on failure.
 */
static int pwm_mec5_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mec5_dev_cfg * const devcfg = dev->config;
	struct pwm_regs * const regs = devcfg->regs;
	int ret = 0;
	uint8_t pwm_pol = 0; /* non-inverted */

	if (channel > 0) {
		return -EIO;
	}

	if (pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	if (flags & PWM_POLARITY_INVERTED) {
		pwm_pol = 1u;
	}

	mec_pwm_set_polarity(regs, pwm_pol);

	ret = mec_pwm_set_freq_out(regs, period_cycles, pulse_cycles);
	if (ret == MEC_RET_OK) {
		ret = 0;
	} else {
		ret = -EIO;
	}

	mec_pwm_enable(regs, 1);

	return ret;
}

/*
 * Return the maximum cycles per second the PWM is capable of.
 * All instances of MCHP PWM use the same input clock.
 * Each PWM instance implements one channel.
 */
static int pwm_mec5_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	ARG_UNUSED(dev);

	if (channel > 0) {
		return -EIO;
	}

	if (cycles) { /* return the highest frequency PWM is capable of */
		*cycles = mec_pwm_hi_freq_input();
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int pwm_mec5_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pwm_mec5_dev_cfg *const devcfg = dev->config;
	struct pwm_regs * const regs = devcfg->regs;
	struct pwm_mec5_data * const data = dev->data;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			LOG_ERR("MEC PWM pinctrl PM resume failed (%d)", ret);
		}

		if (data->enabled) { /* previously on? */
			mec_pwm_enable(regs, 1);
		}
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		if (mec_pwm_is_enabled(regs)) {
			data->enabled = 1u
			mec_pwm_enable(regs, 0);
		} else {
			data->enabled = 0;
		}

		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
		if (ret == -ENOENT) { /* pinctrl-1 does not exist. */
			ret = 0;
		}
	break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static const struct pwm_driver_api pwm_mec5_driver_api = {
	.set_cycles = pwm_mec5_set_cycles,
	.get_cycles_per_sec = pwm_mec5_get_cycles_per_sec,
};

static int pwm_mec5_dev_init(const struct device *dev)
{
	const struct pwm_mec5_dev_cfg * const devcfg = dev->config;
	struct pwm_regs * const regs = devcfg->regs;
	int ret = 0;

	ret = mec_pwm_init(regs, 0, 0, 0);
	if (ret != MEC_RET_OK) {
		LOG_ERR("MEC5 PWM HAL init failed (%d)", ret);
		return -EIO;
	}

	ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("MEC5 PWM pinctrl init failed (%d)", ret);
		return ret;
	}

	return 0;
}

#define PWM_MEC5_DEVICE_INIT(n)						\
	PINCTRL_DT_INST_DEFINE(n);					\
									\
	PM_DEVICE_DT_INST_DEFINE(n, pwm_mec5_pm_action);		\
									\
	static struct pwm_mec5_dev_data pwm_mec5_data_ ## n;		\
									\
	static const struct pwm_mec5_dev_cfg pwm_mec5_dcfg_ ## n = {	\
		.regs = (struct pwm_regs *)DT_INST_REG_ADDR(n),		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, &pwm_mec5_dev_init,			\
			      PM_DEVICE_DT_INST_GET(n),			\
			      &pwm_mec5_data_ ## n,			\
			      &pwm_mec5_dcfg_ ## n,			\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,	\
			      &pwm_mec5_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_MEC5_DEVICE_INIT)
