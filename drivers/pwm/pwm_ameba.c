/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_pwm

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ameba, CONFIG_PWM_LOG_LEVEL);

/* Ameba PWM private flags
 * - Bit 9  : One-pulse mode
 * - Bit 10 : External trigger single edge
 * - Bit 11 : External trigger both edges
 * - Bit 12 : Default output level
 */
#define AMEBA_PWM_MODE               (1U << 9)
#define AMEBA_OPMODE_ETP_ACTIVE_EDGE (1U << 10)
#define AMEBA_OPMODE_ETP_BOTH_ACTIVE (1U << 11)
#define AMEBA_OPMODE_DEFAULT_LEVEL   (1U << 12)

#ifdef CONFIG_PWM_CAPTURE
#define SKIP_IRQ_NUM 3U
struct pwm_ameba_capture_data {
	uint8_t skip_irq;
	uint16_t value[SKIP_IRQ_NUM];
	uint16_t period;
	uint16_t pulse;
	bool capture_period;
	bool capture_pulse;
	pwm_capture_callback_handler_t callback;
	void *user_data;
};
#endif /* CONFIG_PWM_CAPTURE */
struct pwm_ameba_data {
	uint16_t prescale;
	bool CC_polarity;
	uint32_t channel_idx;
	uint32_t port_count;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_ameba_capture_data capture[PWM_CHAN_MAX];
#endif /* CONFIG_PWM_CAPTURE */
};

struct pwm_ameba_config {
	RTIM_TypeDef *pwm_timer;
	uint32_t clock_frequency;
	int irq_source;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	uint8_t pwm_index;
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
	const struct pinctrl_dev_config *pincfg;
};

static int pwm_ameba_get_cycles_per_sec(const struct device *dev, uint32_t channel_idx,
					uint64_t *cycles)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;

	if (dev == NULL || cycles == NULL) {
		return -EINVAL;
	}

	*cycles = (uint64_t)(config->clock_frequency / data->prescale);

	return 0;
}

static int pwm_ameba_set_cycles(const struct device *dev, uint32_t channel_idx,
				uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;
	TIM_CCInitTypeDef TIM_CCInitStruct;

	if (channel_idx >= data->port_count) {
		LOG_ERR("invalid channel_idx!");
		return -EINVAL;
	}

	data->CC_polarity = (flags & PWM_POLARITY_MASK);
	data->channel_idx = channel_idx;
	RTIM_CCStructInit(&TIM_CCInitStruct);
	if (flags & PWM_POLARITY_MASK) {
		TIM_CCInitStruct.TIM_CCPolarity = TIM_CCPolarity_Low;
	}
	if (flags & AMEBA_PWM_OCPROTECTION) {
		TIM_CCInitStruct.TIM_OCProtection = TIM_OCPreload_Disable;
	}
	TIM_CCInitStruct.TIM_OCPulse = pulse_cycles;
	RTIM_CCxInit(config->pwm_timer, &(TIM_CCInitStruct), channel_idx);
	RTIM_ChangePeriodImmediate(config->pwm_timer, period_cycles);

	if (flags & AMEBA_PWM_MODE) {
		if (flags & AMEBA_OPMODE_ETP_BOTH_ACTIVE) {
			RTIM_SetOnePulseOutputMode(config->pwm_timer, TIM_OPMode_Single,
						   TIM_OPMode_ETP_bothedge);
		} else if (flags & AMEBA_OPMODE_ETP_ACTIVE_EDGE) {
			RTIM_SetOnePulseOutputMode(config->pwm_timer, TIM_OPMode_Single,
						   TIM_OPMode_ETP_negative);
		} else {
			RTIM_SetOnePulseOutputMode(config->pwm_timer, TIM_OPMode_Single,
						   TIM_OPMode_ETP_positive);
		}
		if (flags & AMEBA_OPMODE_DEFAULT_LEVEL) {
			RTIM_SetOnePulseDefaultLevel(config->pwm_timer, channel_idx,
						     TIMPWM_DefaultLevel_High);
		}
	}
	RTIM_CCxCmd(config->pwm_timer, channel_idx, TIM_CCx_Enable);

	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static int pwm_ameba_configure_capture(const struct device *dev, uint32_t channel_idx,
				       pwm_flags_t flags, pwm_capture_callback_handler_t cb,
				       void *user_data)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;
	TIM_CCInitTypeDef TIM_CCInitStruct;

	if (channel_idx >= data->port_count) {
		LOG_ERR("invalid channel_idx!");
		return -EINVAL;
	}

	if (RTIM_IsChannelCaptureEnabled(config->pwm_timer, channel_idx) == RTK_SUCCESS) {
		LOG_ERR("pwm capture is busy!");
		return -EBUSY;
	}

	UPS_SrcConfig(UPS_SRC_GPIO, (uint8_t)(config->pwm_index));
	UPS_DstConfig(UPS_DST_PWM_TRIG, (uint8_t)(config->pwm_index));

	RTIM_CCStructInit(&TIM_CCInitStruct);
	TIM_CCInitStruct.TIM_CCMode = TIM_CCMode_Inputcapture;
	if (flags & PWM_POLARITY_MASK) {
		TIM_CCInitStruct.TIM_CCPolarity = TIM_CCPolarity_Low;
	}
	if (flags & AMEBA_PWM_OCPROTECTION) {
		TIM_CCInitStruct.TIM_OCProtection = TIM_OCPreload_Disable;
	}
	RTIM_CCxInit(config->pwm_timer, &(TIM_CCInitStruct), channel_idx);

	data->CC_polarity = (flags & PWM_POLARITY_MASK);
	data->capture[channel_idx].callback = cb;
	data->capture[channel_idx].user_data = user_data;
	data->capture[channel_idx].capture_period = (flags & PWM_CAPTURE_TYPE_PERIOD);
	data->capture[channel_idx].capture_pulse = (flags & PWM_CAPTURE_TYPE_PULSE);

	return 0;
}

static int pwm_ameba_disable_capture(const struct device *dev, uint32_t channel_idx)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;

	if (channel_idx >= data->port_count) {
		LOG_ERR("invalid channel_idx!");
		return -EINVAL;
	}

	RTIM_CCxCmd(config->pwm_timer, channel_idx, TIM_CCx_Disable);

	return 0;
}

static int pwm_ameba_enable_capture(const struct device *dev, uint32_t channel_idx)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;

	if (!IS_TIM_PWM_TIM(config->pwm_timer)) {
		LOG_ERR("illegal device !!!\n");
		return -EINVAL;
	}

	if (!IS_TIM_CHANNEL(channel_idx)) {
		LOG_ERR("channel_idx is illegal\n");
		return -EINVAL;
	}

	if (!data->capture[channel_idx].callback) {
		LOG_ERR("Capture not configured");
		return -EINVAL;
	}

	if (RTIM_IsChannelCaptureEnabled(config->pwm_timer, channel_idx) == RTK_SUCCESS) {
		LOG_ERR("pwm capture is busy!");
		return -EBUSY;
	}

	RTIM_INTConfig(config->pwm_timer, TIM_IT_CC0 << channel_idx, ENABLE);
	RTIM_CCxCmd(config->pwm_timer, channel_idx, TIM_CCx_Enable);
	data->capture[channel_idx].skip_irq = 0;

	return 0;
}

static void pwm_ameba_isr(const struct device *dev)
{
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;
	int channel_idx = data->channel_idx;

	if (channel_idx >= data->port_count) {
		LOG_ERR("Invalid channel_idx %u (max: %u)", channel_idx, data->port_count);
		return;
	}

	if (data->capture[channel_idx].skip_irq < SKIP_IRQ_NUM - 1) {
		data->capture[channel_idx].value[data->capture[channel_idx].skip_irq] =
			RTIM_CCRxGet(config->pwm_timer, channel_idx);
		/*Invert polarity*/
		if (data->CC_polarity) {
			RTIM_CCxPolarityConfig(config->pwm_timer, TIM_CCPolarity_High, channel_idx);
		} else {
			RTIM_CCxPolarityConfig(config->pwm_timer, TIM_CCPolarity_Low, channel_idx);
		}
		data->CC_polarity = !data->CC_polarity;
		data->capture[channel_idx].skip_irq++;
	} else {
		data->capture[channel_idx].value[data->capture[channel_idx].skip_irq] =
			RTIM_CCRxGet(config->pwm_timer, channel_idx);
		/*Invert polarity*/
		if (data->CC_polarity) {
			RTIM_CCxPolarityConfig(config->pwm_timer, TIM_CCPolarity_High, channel_idx);
		} else {
			RTIM_CCxPolarityConfig(config->pwm_timer, TIM_CCPolarity_Low, channel_idx);
		}
		data->CC_polarity = !data->CC_polarity;
		data->capture[channel_idx].period =
			data->capture[channel_idx].value[2] - data->capture[channel_idx].value[0];
		data->capture[channel_idx].pulse =
			(data->capture[channel_idx].value[1] - data->capture[channel_idx].value[0]);
		data->capture[channel_idx].value[0] = data->capture[channel_idx].value[2];
		data->capture[channel_idx].skip_irq = 1;
		if (data->capture[channel_idx].callback) {
			data->capture[channel_idx].callback(dev, channel_idx,
							    data->capture[channel_idx].period,
							    data->capture[channel_idx].pulse, 0,
							    data->capture[channel_idx].user_data);
		}
	}
	RTIM_INTClear(config->pwm_timer);
}
#endif /* CONFIG_PWM_CAPTURE */

int pwm_ameba_init(const struct device *dev)
{
	int ret;
	const struct pwm_ameba_config *config = dev->config;
	struct pwm_ameba_data *data = dev->data;
	RTIM_TimeBaseInitTypeDef TIM_InitStruct;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable peripheral */
	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}
	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}
	RTIM_TimeBaseStructInit(&TIM_InitStruct);
	TIM_InitStruct.TIM_Prescaler = data->prescale;
	RTIM_TimeBaseInit(config->pwm_timer, &TIM_InitStruct, config->irq_source, NULL,
			  (uint32_t)&TIM_InitStruct);
#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
#endif /* CONFIG_PWM_CAPTURE */
	RTIM_Cmd(config->pwm_timer, ENABLE);
	return 0;
}

static DEVICE_API(pwm, pwm_ameba_api) = {
	.set_cycles = pwm_ameba_set_cycles,
	.get_cycles_per_sec = pwm_ameba_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_ameba_configure_capture,
	.enable_capture = pwm_ameba_enable_capture,
	.disable_capture = pwm_ameba_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
#define IRQ_CONFIG_FUNC(n)                                                                         \
	static void pwm_ameba_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), pwm_ameba_isr,              \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}
#define CAPTURE_INIT(n) .irq_config_func = pwm_ameba_irq_config_func_##n
#else
#define IRQ_CONFIG_FUNC(n)
#define CAPTURE_INIT(n)
#endif /* CONFIG_PWM_CAPTURE */

#define AMEBA_PWM_INIT(n)                                                                          \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	IRQ_CONFIG_FUNC(n);                                                                        \
	static struct pwm_ameba_data pwm_ameba_data_##n = {                                        \
		.prescale = DT_INST_PROP(n, prescale),                                             \
		.port_count = DT_INST_PROP(n, channel_count),                                      \
	};                                                                                         \
	static const struct pwm_ameba_config pwm_ameba_config_##n = {                              \
		.pwm_timer = (RTIM_TypeDef *)DT_INST_REG_ADDR(n),                                  \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, idx),               \
		.clock_frequency = DT_INST_PROP(n, clock_frequency),                               \
		.pwm_index = DT_INST_PROP(n, index),                                               \
		CAPTURE_INIT(n)};                                                                  \
	DEVICE_DT_INST_DEFINE(n, &pwm_ameba_init, NULL, &pwm_ameba_data_##n,                       \
			      &pwm_ameba_config_##n, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,        \
			      &pwm_ameba_api);

DT_INST_FOREACH_STATUS_OKAY(AMEBA_PWM_INIT)
