/*
 * Copyright (c) 2017 Vitor Massaru Iha <vitor@massaru.org>
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ledc

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/ledc_hal.h>
#include <hal/ledc_types.h>

#include <soc.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ledc_esp32, CONFIG_PWM_LOG_LEVEL);

struct pwm_ledc_esp32_data {
	ledc_hal_context_t hal;
	struct k_sem cmd_sem;
};

struct pwm_ledc_esp32_channel_config {
	const uint8_t idx;
	const uint8_t channel_num;
	const uint8_t timer_num;
	uint32_t freq;
	const ledc_mode_t speed_mode;
	uint8_t resolution;
	ledc_clk_src_t clock_src;
	uint32_t duty_val;
};

struct pwm_ledc_esp32_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	struct pwm_ledc_esp32_channel_config *channel_config;
	const int channel_len;
};

static struct pwm_ledc_esp32_channel_config *get_channel_config(const struct device *dev,
	int channel_id)
{
	struct pwm_ledc_esp32_config *config =
		(struct pwm_ledc_esp32_config *) dev->config;

	for (uint8_t i = 0; i < config->channel_len; i++) {
		if (config->channel_config[i].idx == channel_id) {
			return &config->channel_config[i];
		}
	}
	return NULL;
}

static void pwm_led_esp32_low_speed_update(const struct device *dev, int speed_mode, int channel)
{
	uint32_t reg_addr;
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	if (speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_ls_channel_update(&data->hal, channel);
	}
}

static void pwm_led_esp32_update_duty(const struct device *dev, int speed_mode, int channel)
{
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	ledc_hal_set_sig_out_en(&data->hal, channel, true);
	ledc_hal_set_duty_start(&data->hal, channel, true);

	pwm_led_esp32_low_speed_update(dev, speed_mode, channel);
}

static void pwm_led_esp32_duty_set(const struct device *dev,
	struct pwm_ledc_esp32_channel_config *channel)
{
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	ledc_hal_set_hpoint(&data->hal, channel->channel_num, 0);
	ledc_hal_set_duty_int_part(&data->hal, channel->channel_num, channel->duty_val);
	ledc_hal_set_duty_direction(&data->hal, channel->channel_num, 1);
	ledc_hal_set_duty_num(&data->hal, channel->channel_num, 1);
	ledc_hal_set_duty_cycle(&data->hal, channel->channel_num, 1);
	ledc_hal_set_duty_scale(&data->hal, channel->channel_num, 0);
	pwm_led_esp32_low_speed_update(dev, channel->speed_mode, channel->channel_num);
	pwm_led_esp32_update_duty(dev, channel->speed_mode, channel->channel_num);
}

static int pwm_led_esp32_configure_pinctrl(const struct device *dev)
{
	int ret;
	struct pwm_ledc_esp32_config *config = (struct pwm_ledc_esp32_config *) dev->config;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}
	return 0;
}

static void pwm_led_esp32_bind_channel_timer(const struct device *dev,
	struct pwm_ledc_esp32_channel_config *channel)
{
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	ledc_hal_bind_channel_timer(&data->hal, channel->channel_num, channel->timer_num);

	pwm_led_esp32_low_speed_update(dev, channel->speed_mode, channel->channel_num);
}

static int pwm_led_esp32_calculate_max_resolution(struct pwm_ledc_esp32_channel_config *channel)
{
	/**
	 * Max duty resolution can be obtained with
	 * max_res = log2(CLK_FREQ/FREQ)
	 */
	uint64_t clock_freq = channel->clock_src == LEDC_APB_CLK ? APB_CLK_FREQ : REF_CLK_FREQ;
	uint32_t max_precision_n = clock_freq/channel->freq;

	for (uint8_t i = 0; i <= SOC_LEDC_TIMER_BIT_WIDE_NUM; i++) {
		max_precision_n /= 2;
		if (!max_precision_n) {
			channel->resolution =  i;
			return 0;
		}
	}
	return -EINVAL;

}

static int pwm_led_esp32_timer_config(struct pwm_ledc_esp32_channel_config *channel)
{
	/**
	 * Calculate max resolution based on the given frequency and the pwm clock.
	 *
	 * There are 2 clock resources for PWM:
	 *
	 * 1. APB_CLK (80MHz)
	 * 2. REF_TICK (1MHz)
	 *
	 * The low speed timers can be sourced from:
	 *
	 * 1. APB_CLK (80MHz)
	 * 2. RTC_CLK (8Mhz)
	 *
	 * The APB_CLK is mostly used
	 *
	 * First we try to find the largest resolution using the APB_CLK source.
	 * If the given frequency doesn't support it, we move to the next clock source.
	 */

	channel->clock_src = LEDC_APB_CLK;
	if (!pwm_led_esp32_calculate_max_resolution(channel)) {
		return 0;
	}

	channel->clock_src = LEDC_REF_TICK;
	if (!pwm_led_esp32_calculate_max_resolution(channel)) {
		return 0;
	}

	return -EINVAL;
}

static int pwm_led_esp32_timer_set(const struct device *dev,
	struct pwm_ledc_esp32_channel_config *channel)
{
	int prescaler = 0;
	uint32_t precision = (0x1 << channel->resolution);
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	__ASSERT_NO_MSG(channel->freq > 0);

	switch (channel->clock_src) {
	case LEDC_APB_CLK:
		/** This expression comes from ESP32 Espressif's Technical Reference
		 * Manual chapter 13.2.2 Timers.
		 * div_num is a fixed point value (Q10.8).
		 */
		prescaler = ((uint64_t) APB_CLK_FREQ << 8) / channel->freq / precision;
	break;
	case LEDC_REF_TICK:
		prescaler = ((uint64_t) REF_CLK_FREQ << 8) / channel->freq / precision;
	break;
	default:
		LOG_ERR("Invalid clock source");
		return -EINVAL;
	}

	if (prescaler < 0x100 || prescaler > 0x3FFFF) {
		return -EINVAL;
	}

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_set_slow_clk(&data->hal, channel->clock_src);
	}

	ledc_hal_set_clock_divider(&data->hal, channel->timer_num, prescaler);
	ledc_hal_set_duty_resolution(&data->hal, channel->timer_num, channel->resolution);
	ledc_hal_set_clock_source(&data->hal, channel->timer_num, channel->clock_src);

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_ls_timer_update(&data->hal, channel->timer_num);
	}

	/* reset low speed timer */
	ledc_hal_timer_rst(&data->hal, channel->timer_num);

	return 0;
}

static int pwm_led_esp32_get_cycles_per_sec(const struct device *dev,
					    uint32_t channel_idx, uint64_t *cycles)
{
	struct pwm_ledc_esp32_config *config = (struct pwm_ledc_esp32_config *) dev->config;
	struct pwm_ledc_esp32_channel_config *channel = get_channel_config(dev, channel_idx);

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	*cycles = channel->clock_src == LEDC_APB_CLK ? APB_CLK_FREQ : REF_CLK_FREQ;

	return 0;
}

static int pwm_led_esp32_set_cycles(const struct device *dev, uint32_t channel_idx,
				    uint32_t period_cycles,
				    uint32_t pulse_cycles, pwm_flags_t flags)
{
	int ret;
	uint64_t clk_freq;
	struct pwm_ledc_esp32_config *config = (struct pwm_ledc_esp32_config *) dev->config;
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;
	struct pwm_ledc_esp32_channel_config *channel = get_channel_config(dev, channel_idx);

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	/* Update PWM frequency according to period_cycles */
	pwm_led_esp32_get_cycles_per_sec(dev, channel_idx, &clk_freq);

	channel->freq = (uint32_t) (clk_freq/period_cycles);
	if (!channel->freq) {
		channel->freq = 1;
	}

	k_sem_take(&data->cmd_sem, K_FOREVER);

	ledc_hal_init(&data->hal, channel->speed_mode);

	ret = pwm_led_esp32_timer_config(channel);
	if (ret < 0) {
		k_sem_give(&data->cmd_sem);
		return ret;
	}

	ret = pwm_led_esp32_timer_set(dev, channel);
	if (ret < 0) {
		k_sem_give(&data->cmd_sem);
		return ret;
	}

	pwm_led_esp32_bind_channel_timer(dev, channel);

	/* Update PWM duty  */

	double duty_cycle = (double) pulse_cycles / (double) period_cycles;

	channel->duty_val = (uint32_t)((double) (1 << channel->resolution) * duty_cycle);

	pwm_led_esp32_duty_set(dev, channel);

	ret = pwm_led_esp32_configure_pinctrl(dev);
	if (ret < 0) {
		k_sem_give(&data->cmd_sem);
		return ret;
	}

	k_sem_give(&data->cmd_sem);

	return ret;
}


int pwm_led_esp32_init(const struct device *dev)
{
	int ret;
	const struct pwm_ledc_esp32_config *config = dev->config;
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable peripheral */
	clock_control_on(config->clock_dev, config->clock_subsys);

	return 0;
}

static const struct pwm_driver_api pwm_led_esp32_api = {
	.set_cycles = pwm_led_esp32_set_cycles,
	.get_cycles_per_sec = pwm_led_esp32_get_cycles_per_sec,
};

PINCTRL_DT_INST_DEFINE(0);

#define CHANNEL_CONFIG(node_id)                                                \
	{                                                                      \
		.idx = DT_REG_ADDR(node_id),                                   \
		.channel_num = DT_REG_ADDR(node_id) % 8,                       \
		.timer_num = DT_PROP(node_id, timer),                          \
		.speed_mode = DT_REG_ADDR(node_id) < SOC_LEDC_CHANNEL_NUM      \
				      ? LEDC_LOW_SPEED_MODE                    \
				      : !LEDC_LOW_SPEED_MODE,                  \
		.clock_src = LEDC_APB_CLK,                                     \
	},

static struct pwm_ledc_esp32_channel_config channel_config[] = {
	DT_INST_FOREACH_CHILD(0, CHANNEL_CONFIG)
};

static struct pwm_ledc_esp32_config pwm_ledc_esp32_config = {
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.channel_config = channel_config,
	.channel_len = ARRAY_SIZE(channel_config),
};

static struct pwm_ledc_esp32_data pwm_ledc_esp32_data = {
	.hal = {
		.dev = (ledc_dev_t *) DT_INST_REG_ADDR(0),
	},
	.cmd_sem = Z_SEM_INITIALIZER(pwm_ledc_esp32_data.cmd_sem, 1, 1),
};

DEVICE_DT_INST_DEFINE(0, &pwm_led_esp32_init, NULL,
			&pwm_ledc_esp32_data,
			&pwm_ledc_esp32_config,
			POST_KERNEL,
			CONFIG_PWM_INIT_PRIORITY,
			&pwm_led_esp32_api);
