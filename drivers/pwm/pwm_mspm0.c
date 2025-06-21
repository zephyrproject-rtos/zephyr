/*
 * Copyright (c) 2025, Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_timer_pwm

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* Driverlib includes */
#include <ti/driverlib/dl_timera.h>
#include <ti/driverlib/dl_timerg.h>
#include <ti/driverlib/dl_timer.h>

LOG_MODULE_REGISTER(pwm_mspm0, CONFIG_PWM_LOG_LEVEL);

/* capture and compare block count per timer */
#define MSPM0_TIMER_CC_COUNT		2
#define MSPM0_TIMER_CC_MAX		4

struct pwm_mspm0_config {
	const struct mspm0_sys_clock clock_subsys;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	GPTIMER_Regs *base;
	DL_Timer_ClockConfig clk_config;
	uint8_t	cc_idx[MSPM0_TIMER_CC_MAX];
	uint8_t cc_idx_cnt;
};

struct pwm_mspm0_data {
	uint32_t pulse_cycle[MSPM0_TIMER_CC_MAX];
	uint32_t period;
	struct k_mutex lock;

	DL_TIMER_PWM_MODE out_mode;
};

static void mspm0_setup_pwm_out(const struct pwm_mspm0_config *config,
				struct pwm_mspm0_data *data)
{
	int i;
	DL_Timer_PWMConfig pwmcfg = { 0 };
	uint8_t ccdir_mask = 0;

	pwmcfg.period = data->period;
	pwmcfg.pwmMode = data->out_mode;

	for (i = 0; i < config->cc_idx_cnt; i++) {
		if (config->cc_idx[i] >= MSPM0_TIMER_CC_COUNT) {
			pwmcfg.isTimerWithFourCC = true;
			break;
		}
	}

	DL_Timer_initPWMMode(config->base, &pwmcfg);

	for (i = 0; i < config->cc_idx_cnt; i++) {
		DL_Timer_setCaptureCompareValue(config->base,
						data->pulse_cycle[i],
						config->cc_idx[i]);
		ccdir_mask |= 1 << config->cc_idx[i];
	}

	DL_Timer_enableClock(config->base);
	DL_Timer_setCCPDirection(config->base, ccdir_mask);
	DL_Timer_startCounter(config->base);
}

static int mspm0_pwm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;

	if (channel >= MSPM0_TIMER_CC_MAX) {
		LOG_ERR("Invalid channel");
		return -EINVAL;
	}

	if (period_cycles > UINT16_MAX) {
		LOG_ERR("period cycles exceeds 16-bit timer limit");
		return -ENOTSUP;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->pulse_cycle[channel] = pulse_cycles;
	data->period = period_cycles;

	if (data->out_mode == DL_TIMER_PWM_MODE_CENTER_ALIGN) {
		data->period = period_cycles >> 1;
	}

	DL_Timer_setLoadValue(config->base, data->period);
	DL_Timer_setCaptureCompareValue(config->base,
					data->pulse_cycle[channel],
					config->cc_idx[channel]);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspm0_pwm_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	const struct pwm_mspm0_config *config = dev->config;
	DL_Timer_ClockConfig clkcfg;
	uint32_t clock_rate;
	int ret;

	if (cycles == NULL) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(config->clock_dev,
				(clock_control_subsys_t)&config->clock_subsys,
				&clock_rate);
	if (ret != 0) {
		LOG_ERR("clk get rate err %d", ret);
		return ret;
	}

	DL_Timer_getClockConfig(config->base, &clkcfg);
	*cycles = clock_rate /
			((clkcfg.divideRatio + 1) * (clkcfg.prescale + 1));

	return 0;
}

static int pwm_mspm0_init(const struct device *dev)
{
	const struct pwm_mspm0_config *config = dev->config;
	struct pwm_mspm0_data *data = dev->data;
	int err;

	k_mutex_init(&data->lock);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		return err;
	}

	DL_Timer_reset(config->base);
	if (!DL_Timer_isPowerEnabled(config->base)) {
		DL_Timer_enablePower(config->base);
	}

	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	DL_Timer_setClockConfig(config->base,
				(DL_Timer_ClockConfig *)&config->clk_config);

	mspm0_setup_pwm_out(config, data);

	return 0;
}

static const struct pwm_driver_api pwm_mspm0_driver_api = {
	.set_cycles = mspm0_pwm_set_cycles,
	.get_cycles_per_sec = mspm0_pwm_get_cycles_per_sec,
};

#define MSPM0_PWM_MODE(mode)		DT_CAT(DL_TIMER_PWM_MODE_, mode)
#define MSPM0_CLK_DIV(div)		DT_CAT(DL_TIMER_CLOCK_DIVIDE_, div)

#define MSPM0_CC_IDX_ARRAY(node_id, prop, idx)	\
				 DT_PROP_BY_IDX(node_id, prop, idx),

#define MSPM0_PWM_DATA(n)	\
	.out_mode = MSPM0_PWM_MODE(DT_STRING_TOKEN(DT_DRV_INST(n),		\
				   ti_pwm_mode)),

#define PWM_DEVICE_INIT_MSPM0(n)						\
	static struct pwm_mspm0_data pwm_mspm0_data_ ## n = {			\
		.period = DT_PROP(DT_DRV_INST(n), ti_period),			\
		COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(n), ti_pwm_mode),	\
			    (MSPM0_PWM_DATA(n)), ())				\
	};									\
	PINCTRL_DT_INST_DEFINE(n);						\
										\
	static const struct pwm_mspm0_config pwm_mspm0_config_ ## n = {		\
		.base = (GPTIMER_Regs *)DT_REG_ADDR(DT_INST_PARENT(n)),		\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(		\
						DT_INST_PARENT(n), 0)),		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),			\
		.clock_subsys = {						\
			.clk = DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, clk),\
		},								\
		.cc_idx = {							\
			DT_INST_FOREACH_PROP_ELEM(n, ti_cc_index,		\
						  MSPM0_CC_IDX_ARRAY)		\
		},								\
		.cc_idx_cnt = DT_INST_PROP_LEN(n, ti_cc_index),			\
		.clk_config = {							\
			.clockSel = MSPM0_CLOCK_PERIPH_REG_MASK(		\
				DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n),	\
						      0, clk)),			\
			.divideRatio = MSPM0_CLK_DIV(DT_PROP(DT_INST_PARENT(n),	\
						     ti_clk_div)),		\
			.prescale = DT_PROP(DT_INST_PARENT(n), ti_clk_prescaler),\
		},								\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      pwm_mspm0_init,					\
			      NULL,						\
			      &pwm_mspm0_data_ ## n,				\
			      &pwm_mspm0_config_ ## n,				\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,		\
			      &pwm_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_DEVICE_INIT_MSPM0)
