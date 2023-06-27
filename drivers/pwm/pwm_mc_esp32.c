/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_mcpwm

#include <hal/mcpwm_hal.h>
#include <hal/mcpwm_ll.h>
#include <driver/mcpwm.h>

#include <soc.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#ifdef CONFIG_PWM_CAPTURE
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif /* CONFIG_PWM_CAPTURE */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mcpwm_esp32, CONFIG_PWM_LOG_LEVEL);

#define SOC_MCPWM_BASE_CLK_HZ (160000000U)
#ifdef CONFIG_PWM_CAPTURE
#define SKIP_IRQ_NUM 4U
#define MCPWM_INTR_CAP0  BIT(0)
#define MCPWM_INTR_CAP1  BIT(1)
#define MCPWM_INTR_CAP2  BIT(2)
#define MCPWM_CHANNEL_NUM   8U
#define CAPTURE_CHANNEL_IDX 6U
#else
#define MCPWM_CHANNEL_NUM 6U
#endif /* CONFIG_PWM_CAPTURE */

struct mcpwm_esp32_data {
	mcpwm_hal_context_t hal;
	mcpwm_hal_init_config_t init_config;
	struct k_sem cmd_sem;
};

#ifdef CONFIG_PWM_CAPTURE
struct capture_data {
	uint32_t value;
	mcpwm_capture_on_edge_t edge;
};

struct mcpwm_esp32_capture_config {
	uint8_t capture_signal;
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint32_t period;
	uint32_t pulse;
	uint32_t overflows;
	uint8_t skip_irq;
	bool capture_period;
	bool capture_pulse;
	bool continuous;
	struct capture_data capture_data[SKIP_IRQ_NUM];
};
#endif /* CONFIG_PWM_CAPTURE */

struct mcpwm_esp32_channel_config {
	uint8_t idx;
	uint8_t timer_id;
	uint8_t operator_id;
	uint8_t generator_id;
	uint32_t freq;
	uint32_t duty;
	uint8_t prescale;
	bool inverted;
#ifdef CONFIG_PWM_CAPTURE
	struct mcpwm_esp32_capture_config capture;
#endif /* CONFIG_PWM_CAPTURE */
};

struct mcpwm_esp32_config {
	const uint8_t index;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	uint8_t prescale;
	uint8_t prescale_timer0;
	uint8_t prescale_timer1;
	uint8_t prescale_timer2;
	struct mcpwm_esp32_channel_config channel_config[MCPWM_CHANNEL_NUM];
#ifdef CONFIG_PWM_CAPTURE
	void (*irq_config_func)(const struct device *dev);
#endif /* CONFIG_PWM_CAPTURE */
};

static void mcpwm_esp32_duty_set(const struct device *dev,
				 struct mcpwm_esp32_channel_config *channel)
{
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	mcpwm_duty_type_t duty_type;
	uint32_t set_duty;

	if (channel->inverted) {
		duty_type = channel->duty == 0 ?
			MCPWM_HAL_GENERATOR_MODE_FORCE_HIGH : channel->duty == 100 ?
			MCPWM_HAL_GENERATOR_MODE_FORCE_LOW  : MCPWM_DUTY_MODE_1;
	} else {
		duty_type = channel->duty == 0 ?
			MCPWM_HAL_GENERATOR_MODE_FORCE_LOW  : channel->duty == 100 ?
			MCPWM_HAL_GENERATOR_MODE_FORCE_HIGH : MCPWM_DUTY_MODE_0;
	}

	set_duty = mcpwm_ll_timer_get_peak(data->hal.dev, channel->timer_id, false) *
		   channel->duty / 100;
	mcpwm_ll_operator_connect_timer(data->hal.dev, channel->operator_id, channel->timer_id);
	mcpwm_ll_operator_set_compare_value(data->hal.dev, channel->operator_id,
					    channel->generator_id, set_duty);
	mcpwm_ll_operator_enable_update_compare_on_tez(data->hal.dev, channel->operator_id,
						       channel->generator_id, true);

	if (duty_type == MCPWM_DUTY_MODE_0) {
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH);
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_GEN_ACTION_KEEP);
		mcpwm_ll_generator_set_action_on_compare_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, channel->generator_id, MCPWM_ACTION_FORCE_LOW);
	} else if (duty_type == MCPWM_DUTY_MODE_1) {
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_LOW);
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_ACTION_NO_CHANGE);
		mcpwm_ll_generator_set_action_on_compare_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, channel->generator_id, MCPWM_ACTION_FORCE_HIGH);
	} else if (duty_type == MCPWM_HAL_GENERATOR_MODE_FORCE_LOW) {
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_ACTION_FORCE_LOW);
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_ACTION_FORCE_LOW);
		mcpwm_ll_generator_set_action_on_compare_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, channel->generator_id, MCPWM_ACTION_FORCE_LOW);
	} else if (duty_type == MCPWM_HAL_GENERATOR_MODE_FORCE_HIGH) {
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_ACTION_FORCE_HIGH);
		mcpwm_ll_generator_set_action_on_timer_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_FULL, MCPWM_ACTION_FORCE_HIGH);
		mcpwm_ll_generator_set_action_on_compare_event(
			data->hal.dev, channel->operator_id, channel->generator_id,
			MCPWM_TIMER_DIRECTION_UP, channel->generator_id, MCPWM_ACTION_FORCE_HIGH);
	}
}

static int mcpwm_esp32_configure_pinctrl(const struct device *dev)
{
	int ret;
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}
	return 0;
}

static int mcpwm_esp32_timer_set(const struct device *dev,
				 struct mcpwm_esp32_channel_config *channel)
{
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;

	__ASSERT_NO_MSG(channel->freq > 0);

	mcpwm_ll_timer_set_clock_prescale(data->hal.dev, channel->timer_id, channel->prescale);
	mcpwm_ll_timer_set_count_mode(data->hal.dev, channel->timer_id, MCPWM_TIMER_COUNT_MODE_UP);
	mcpwm_ll_timer_update_period_at_once(data->hal.dev, channel->timer_id);
	int real_group_prescale = mcpwm_ll_group_get_clock_prescale(data->hal.dev);
	uint32_t real_timer_clk_hz =
		SOC_MCPWM_BASE_CLK_HZ / real_group_prescale /
		mcpwm_ll_timer_get_clock_prescale(data->hal.dev, channel->timer_id);
	mcpwm_ll_timer_set_peak(data->hal.dev, channel->timer_id, real_timer_clk_hz / channel->freq,
				false);

	return 0;
}

static int mcpwm_esp32_get_cycles_per_sec(const struct device *dev, uint32_t channel_idx,
					  uint64_t *cycles)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_channel_config *channel = &config->channel_config[channel_idx];

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

#ifdef CONFIG_PWM_CAPTURE
	if (channel->idx >= CAPTURE_CHANNEL_IDX) {
		*cycles = (uint64_t)APB_CLK_FREQ;
		return 0;
	}
#endif /* CONFIG_PWM_CAPTURE */

	*cycles =
		(uint64_t)SOC_MCPWM_BASE_CLK_HZ / (config->prescale + 1) / (channel->prescale + 1);

	return 0;
}

static int mcpwm_esp32_set_cycles(const struct device *dev, uint32_t channel_idx,
				  uint32_t period_cycles, uint32_t pulse_cycles, pwm_flags_t flags)
{
	int ret = 0;
	uint64_t clk_freq;
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel = &config->channel_config[channel_idx];

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	/* Update PWM frequency according to period_cycles */
	mcpwm_esp32_get_cycles_per_sec(dev, channel_idx, &clk_freq);

	channel->freq = (uint32_t)(clk_freq / period_cycles);
	if (!channel->freq) {
		return -EINVAL;
	}

	k_sem_take(&data->cmd_sem, K_FOREVER);

	ret = mcpwm_esp32_timer_set(dev, channel);
	if (ret < 0) {
		k_sem_give(&data->cmd_sem);
		return ret;
	}

	double duty_cycle = (double)pulse_cycles * 100 / (double)period_cycles;

	channel->duty = (uint32_t)duty_cycle;

	channel->inverted = (flags & PWM_POLARITY_INVERTED);

	mcpwm_esp32_duty_set(dev, channel);

	ret = mcpwm_esp32_configure_pinctrl(dev);
	if (ret < 0) {
		k_sem_give(&data->cmd_sem);
		return ret;
	}

	mcpwm_ll_timer_set_start_stop_command(data->hal.dev, channel->timer_id,
					      MCPWM_TIMER_START_NO_STOP);

	k_sem_give(&data->cmd_sem);

	return ret;
}

#ifdef CONFIG_PWM_CAPTURE
static int mcpwm_esp32_configure_capture(const struct device *dev, uint32_t channel_idx,
					 pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					 void *user_data)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel = &config->channel_config[channel_idx];
	struct mcpwm_esp32_capture_config *capture = &channel->capture;

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	if ((channel->idx < CAPTURE_CHANNEL_IDX) || (channel->idx > CAPTURE_CHANNEL_IDX + 2)) {
		LOG_ERR("PWM capture only supported on channels 6, 7 and 8");
		return -EINVAL;
	}

	if (data->hal.dev->cap_chn_cfg[capture->capture_signal].capn_en) {
		LOG_ERR("PWM Capture already in progress");
		return -EBUSY;
	}

	if (!(flags & PWM_CAPTURE_TYPE_MASK)) {
		LOG_ERR("No PWM capture type specified");
		return -EINVAL;
	}

	channel->inverted = (flags & PWM_POLARITY_INVERTED);
	capture->capture_signal = channel->idx - CAPTURE_CHANNEL_IDX;
	capture->callback = cb;
	capture->user_data = user_data;
	capture->capture_period = (flags & PWM_CAPTURE_TYPE_PERIOD);
	capture->capture_pulse = (flags & PWM_CAPTURE_TYPE_PULSE);
	capture->continuous = (flags & PWM_CAPTURE_MODE_CONTINUOUS);

	return 0;
}

static int mcpwm_esp32_disable_capture(const struct device *dev, uint32_t channel_idx)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel = &config->channel_config[channel_idx];
	struct mcpwm_esp32_capture_config *capture = &channel->capture;

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	if ((channel->idx < CAPTURE_CHANNEL_IDX) || (channel->idx > CAPTURE_CHANNEL_IDX + 2)) {
		LOG_ERR("PWM capture only supported on channels 6, 7 and 8");
		return -EINVAL;
	}

	mcpwm_ll_capture_enable_channel(data->hal.dev, capture->capture_signal, false);
	mcpwm_ll_intr_enable(data->hal.dev, MCPWM_LL_EVENT_CAPTURE(capture->capture_signal), false);

	return 0;
}

static int mcpwm_esp32_enable_capture(const struct device *dev, uint32_t channel_idx)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel = &config->channel_config[channel_idx];
	struct mcpwm_esp32_capture_config *capture = &channel->capture;

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	if (!capture->callback) {
		LOG_ERR("Capture not configured");
		return -EINVAL;
	}

	if ((channel->idx < CAPTURE_CHANNEL_IDX) || (channel->idx > CAPTURE_CHANNEL_IDX + 2)) {
		LOG_ERR("PWM capture only supported on channels 6, 7 and 8");
		return -EINVAL;
	}

	if (data->hal.dev->cap_chn_cfg[capture->capture_signal].capn_en) {
		LOG_ERR("PWM Capture already in progress");
		return -EBUSY;
	}

	/**
	 * Capture prescale is different from other modules as it is applied to the input
	 * signal, not the timer source. It is disabled by default.
	 */
	mcpwm_capture_config_t cap_conf = {
		.cap_edge = MCPWM_BOTH_EDGE,
		.cap_prescale = 1,
	};

	mcpwm_hal_init(&data->hal, &data->init_config);
	mcpwm_ll_group_set_clock_prescale(data->hal.dev, config->prescale);
	mcpwm_ll_group_enable_shadow_mode(data->hal.dev);
	mcpwm_ll_group_flush_shadow(data->hal.dev);

	mcpwm_ll_capture_enable_timer(data->hal.dev, true);
	mcpwm_ll_capture_enable_channel(data->hal.dev, capture->capture_signal, true);
	mcpwm_ll_capture_enable_negedge(data->hal.dev, capture->capture_signal,
					cap_conf.cap_edge & MCPWM_NEG_EDGE);
	mcpwm_ll_capture_enable_posedge(data->hal.dev, capture->capture_signal,
					cap_conf.cap_edge & MCPWM_POS_EDGE);
	mcpwm_ll_capture_set_prescale(data->hal.dev, capture->capture_signal,
				      cap_conf.cap_prescale);

	mcpwm_ll_intr_enable(data->hal.dev, MCPWM_LL_EVENT_CAPTURE(capture->capture_signal), true);
	mcpwm_ll_intr_clear_capture_status(data->hal.dev, 1 << capture->capture_signal);

	capture->skip_irq = 0;

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static void channel_init(const struct device *dev)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_channel_config *channel;

	for (uint8_t i = 0; i < MCPWM_CHANNEL_NUM; i++) {
		channel = &config->channel_config[i];
		channel->idx = i;
		channel->timer_id = i < 2 ? 0 : i < 4 ? 1 : 2;
		channel->operator_id = i < 2 ? 0 : i < 4 ? 1 : 2;
		channel->generator_id = i % 2 ? 1 : 0;
		channel->prescale = i < 2   ? config->prescale_timer0
				    : i < 4 ? config->prescale_timer1
					    : config->prescale_timer2;
	}
}

int mcpwm_esp32_init(const struct device *dev)
{
	int ret;
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel;

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

	channel_init(dev);

	mcpwm_hal_init(&data->hal, &data->init_config);
	mcpwm_ll_group_set_clock_prescale(data->hal.dev, config->prescale);
	mcpwm_ll_group_enable_shadow_mode(data->hal.dev);
	mcpwm_ll_group_flush_shadow(data->hal.dev);

#ifdef CONFIG_PWM_CAPTURE
	config->irq_config_func(dev);
#endif /* CONFIG_PWM_CAPTURE */
	return 0;
}

#ifdef CONFIG_PWM_CAPTURE
static void IRAM_ATTR mcpwm_esp32_isr(const struct device *dev)
{
	struct mcpwm_esp32_config *config = (struct mcpwm_esp32_config *)dev->config;
	struct mcpwm_esp32_data *data = (struct mcpwm_esp32_data *const)(dev)->data;
	struct mcpwm_esp32_channel_config *channel;
	struct mcpwm_esp32_capture_config *capture;
	uint32_t mcpwm_intr_status;
	struct capture_data cap_data;

	mcpwm_intr_status = mcpwm_ll_intr_get_capture_status(data->hal.dev);

	mcpwm_ll_intr_clear_capture_status(data->hal.dev, mcpwm_intr_status);

	if (mcpwm_intr_status & MCPWM_INTR_CAP0) {
		channel = &config->channel_config[CAPTURE_CHANNEL_IDX];
	} else if (mcpwm_intr_status & MCPWM_INTR_CAP1) {
		channel = &config->channel_config[CAPTURE_CHANNEL_IDX + 1];
	} else if (mcpwm_intr_status & MCPWM_INTR_CAP2) {
		channel = &config->channel_config[CAPTURE_CHANNEL_IDX + 2];
	} else {
		return;
	}

	if (!channel) {
		return;
	}

	capture = &channel->capture;

	/* We need to wait at least 4 (2 positive edges and 2 negative edges) interrupts to
	 * calculate the period
	 */
	if (capture->skip_irq < SKIP_IRQ_NUM) {
		capture->capture_data[capture->skip_irq].value =
			mcpwm_ll_capture_get_value(data->hal.dev, capture->capture_signal);
		capture->capture_data[capture->skip_irq].edge =
			mcpwm_ll_capture_get_edge(data->hal.dev, capture->capture_signal) ==
					MCPWM_CAP_EDGE_NEG
				? MCPWM_NEG_EDGE
				: MCPWM_POS_EDGE;
		capture->skip_irq++;

	} else {
		/**
		 * The capture timer is a 32-bit counter incrementing continuously, once enabled.
		 * On the input it has an APB clock running typically at 80 MHz
		 */
		capture->period = channel->inverted ?
			capture->capture_data[0].edge == MCPWM_NEG_EDGE
				? (capture->capture_data[2].value - capture->capture_data[0].value)
				: (capture->capture_data[3].value - capture->capture_data[1].value)
			: capture->capture_data[0].edge == MCPWM_POS_EDGE
				? (capture->capture_data[2].value - capture->capture_data[0].value)
				: (capture->capture_data[3].value - capture->capture_data[1].value);

		capture->pulse = channel->inverted ?
			capture->capture_data[0].edge == MCPWM_NEG_EDGE
				? (capture->capture_data[1].value - capture->capture_data[0].value)
				: (capture->capture_data[2].value - capture->capture_data[1].value)
			: capture->capture_data[0].edge == MCPWM_POS_EDGE
				? (capture->capture_data[1].value - capture->capture_data[0].value)
				: (capture->capture_data[2].value - capture->capture_data[1].value);

		capture->skip_irq = 0;
		if (!capture->continuous) {
			mcpwm_esp32_disable_capture(dev, channel->idx);
		}

		if (capture->callback) {
			capture->callback(dev, capture->capture_signal + CAPTURE_CHANNEL_IDX,
					  capture->capture_period ? capture->period : 0u,
					  capture->capture_pulse ? capture->pulse : 0u, 0u,
					  capture->user_data);
		}
	}
}
#endif /* CONFIG_PWM_CAPTURE */

static const struct pwm_driver_api mcpwm_esp32_api = {
	.set_cycles = mcpwm_esp32_set_cycles,
	.get_cycles_per_sec = mcpwm_esp32_get_cycles_per_sec,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = mcpwm_esp32_configure_capture,
	.enable_capture = mcpwm_esp32_enable_capture,
	.disable_capture = mcpwm_esp32_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

#ifdef CONFIG_PWM_CAPTURE
#define IRQ_CONFIG_FUNC(idx)                                                                       \
	static void mcpwm_esp32_irq_config_func_##idx(const struct device *dev)                    \
	{                                                                                          \
		esp_intr_alloc(DT_INST_IRQN(idx), 0, (intr_handler_t)mcpwm_esp32_isr, (void *)dev, \
			       NULL);                                                              \
	}
#define CAPTURE_INIT(idx) .irq_config_func = mcpwm_esp32_irq_config_func_##idx
#else
#define IRQ_CONFIG_FUNC(idx)
#define CAPTURE_INIT(idx)
#endif /* CONFIG_PWM_CAPTURE */

#define ESP32_MCPWM_INIT(idx)                                                                      \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	IRQ_CONFIG_FUNC(idx);                                                                      \
	static struct mcpwm_esp32_data mcpwm_esp32_data_##idx = {                                  \
		.hal =                                                                             \
			{                                                                          \
				.dev = (mcpwm_dev_t *)DT_INST_REG_ADDR(idx),                       \
			},                                                                         \
		.init_config =                                                                     \
			{                                                                          \
				.group_id = idx,                                                   \
			},                                                                         \
		.cmd_sem = Z_SEM_INITIALIZER(mcpwm_esp32_data_##idx.cmd_sem, 1, 1),                \
	};                                                                                         \
                                                                                                   \
	static struct mcpwm_esp32_config mcpwm_esp32_config_##idx = {                              \
		.index = idx,                                                                      \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(idx, offset),          \
		.prescale = DT_INST_PROP(idx, prescale),                                           \
		.prescale_timer0 = DT_INST_PROP_OR(idx, prescale_timer0, 0),                       \
		.prescale_timer1 = DT_INST_PROP_OR(idx, prescale_timer1, 0),                       \
		.prescale_timer2 = DT_INST_PROP_OR(idx, prescale_timer2, 0),                       \
		CAPTURE_INIT(idx)};                                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &mcpwm_esp32_init, NULL, &mcpwm_esp32_data_##idx,               \
			      &mcpwm_esp32_config_##idx, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY,    \
			      &mcpwm_esp32_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_MCPWM_INIT)
