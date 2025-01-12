/*
 * Copyright (c) 2017 Vitor Massaru Iha <vitor@massaru.org>
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_ledc

#include <hal/ledc_hal.h>
#include <hal/ledc_types.h>
#include <esp_clk_tree.h>

#include <soc.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_ledc_esp32, CONFIG_PWM_LOG_LEVEL);

static const int global_clks[] = LEDC_LL_GLOBAL_CLOCKS;
#if SOC_LEDC_HAS_TIMER_SPECIFIC_MUX
static const int timer_specific_clks[] = LEDC_LL_TIMER_SPECIFIC_CLOCKS;
static int lowspd_clks[ARRAY_SIZE(global_clks) + ARRAY_SIZE(timer_specific_clks)];
#endif
#if SOC_LEDC_SUPPORT_HS_MODE
static const int highspd_clks[] = {LEDC_APB_CLK, LEDC_REF_TICK};
#endif

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
	uint32_t clock_src_hz;
	uint32_t duty_val;
	bool inverted;
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
	struct pwm_ledc_esp32_config *config = (struct pwm_ledc_esp32_config *)dev->config;

	for (uint8_t i = 0; i < config->channel_len; i++) {
		if (config->channel_config[i].idx == channel_id) {
			return &config->channel_config[i];
		}
	}
	return NULL;
}

static void pwm_led_esp32_start(struct pwm_ledc_esp32_data *data,
				struct pwm_ledc_esp32_channel_config *channel)
{
	ledc_hal_set_sig_out_en(&data->hal, channel->channel_num, true);
	ledc_hal_set_duty_start(&data->hal, channel->channel_num, true);

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_ls_channel_update(&data->hal, channel->channel_num);
	}
}

static void pwm_led_esp32_stop(struct pwm_ledc_esp32_data *data,
			       struct pwm_ledc_esp32_channel_config *channel, bool idle_level)
{
	ledc_hal_set_idle_level(&data->hal, channel->channel_num, idle_level);
	ledc_hal_set_sig_out_en(&data->hal, channel->channel_num, false);
	ledc_hal_set_duty_start(&data->hal, channel->channel_num, false);

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_ls_channel_update(&data->hal, channel->channel_num);
	}
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
}

static int pwm_led_esp32_calculate_max_resolution(struct pwm_ledc_esp32_channel_config *channel)
{
	/**
	 * Max duty resolution can be obtained with
	 * max_res = log2(CLK_FREQ/FREQ)
	 */
	uint32_t max_precision_n = channel->clock_src_hz / channel->freq;

	for (uint8_t i = 0; i <= SOC_LEDC_TIMER_BIT_WIDTH; i++) {
		max_precision_n /= 2;
		if (!max_precision_n) {
			channel->resolution = i;
			return 0;
		}
	}
	return -EINVAL;
}

static int pwm_led_esp32_timer_config(struct pwm_ledc_esp32_channel_config *channel)
{
	const int *clock_src;
	int clock_src_num;

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
#ifdef SOC_LEDC_HAS_TIMER_SPECIFIC_MUX
		clock_src = lowspd_clks;
		clock_src_num = ARRAY_SIZE(lowspd_clks);
#else
		clock_src = global_clks;
		clock_src_num = ARRAY_SIZE(global_clks);
#endif
	}
#ifdef SOC_LEDC_SUPPORT_HS_MODE
	else {
		clock_src = highspd_clks;
		clock_src_num = ARRAY_SIZE(highspd_clks);
	}
#endif

	/**
	 * Calculate max resolution based on the given frequency and the pwm clock.
	 * Try each clock source available depending on the device and channel type.
	 */
	for (int i = 0; i < clock_src_num; i++) {
		channel->clock_src = clock_src[i];
		esp_clk_tree_src_get_freq_hz(channel->clock_src,
					     ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
					     &channel->clock_src_hz);
		if (!pwm_led_esp32_calculate_max_resolution(channel)) {
			return 0;
		}
	}

	/* Frequency is too low for this device, so even though best precision can't
	 * be achieved we can set max resolution and consider that the previous
	 * loop selects clock from fastest to slowest, so this is the best
	 * configuration achievable.
	 */
	channel->resolution = SOC_LEDC_TIMER_BIT_WIDTH;

	return 0;
}

static int pwm_led_esp32_timer_set(const struct device *dev,
				   struct pwm_ledc_esp32_channel_config *channel)
{
	int prescaler = 0;
	uint32_t precision = (0x1 << channel->resolution);
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;

	__ASSERT_NO_MSG(channel->freq > 0);

	prescaler = ((uint64_t)channel->clock_src_hz << 8) / channel->freq / precision;

	if (prescaler < 0x100 || prescaler > 0x3FFFF) {
		LOG_ERR("Prescaler out of range: %#X", prescaler);
		return -EINVAL;
	}

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_set_slow_clk_sel(&data->hal, channel->clock_src);
	}

	ledc_hal_set_clock_divider(&data->hal, channel->timer_num, prescaler);
	ledc_hal_set_duty_resolution(&data->hal, channel->timer_num, channel->resolution);
	ledc_hal_set_clock_source(&data->hal, channel->timer_num, channel->clock_src);

	if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
		ledc_hal_ls_timer_update(&data->hal, channel->timer_num);
	}

	LOG_DBG("channel_num=%d, speed_mode=%d, timer_num=%d, clock_src=%d, prescaler=%d, "
		"resolution=%d\n",
		channel->channel_num, channel->speed_mode, channel->timer_num, channel->clock_src,
		prescaler, channel->resolution);

	return 0;
}

static int pwm_led_esp32_get_cycles_per_sec(const struct device *dev, uint32_t channel_idx,
					    uint64_t *cycles)
{
	struct pwm_ledc_esp32_channel_config *channel = get_channel_config(dev, channel_idx);

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	*cycles = (uint64_t)channel->clock_src_hz;

	return 0;
}

static int pwm_led_esp32_channel_update_frequency(const struct device *dev,
						  struct pwm_ledc_esp32_channel_config *channel,
						  uint32_t period_cycles)
{
	const struct pwm_ledc_esp32_config *config = dev->config;
	uint32_t current_freq = channel->freq;
	uint64_t clk_freq;
	int ret;

	ret = pwm_led_esp32_get_cycles_per_sec(dev, channel->idx, &clk_freq);

	if (ret < 0) {
		return ret;
	}

	channel->freq = (uint32_t)(clk_freq / period_cycles);

	if (!channel->freq) {
		channel->freq = 1;
	}

	if (channel->freq == current_freq) {
		/* No need to reconfigure timer */
		return 0;
	}

	/* Check whether another channel is using the same timer.
	 * Timers can only be shared if the same frequency is used, so
	 * first set operation will take precedence.
	 */
	for (int i = 0; i < config->channel_len; ++i) {
		struct pwm_ledc_esp32_channel_config *ch = &config->channel_config[i];

		if (ch->freq && (channel->channel_num != ch->channel_num) &&
			(channel->timer_num == ch->timer_num) &&
			(channel->speed_mode == ch->speed_mode) &&
			(channel->freq != ch->freq)) {
			LOG_ERR("Timer can't be shared and different frequency be "
				"requested");
			channel->freq = 0;
			return -EINVAL;
		}
	}

	pwm_led_esp32_timer_config(channel);

	ret = pwm_led_esp32_timer_set(dev, channel);

	if (ret < 0) {
		LOG_ERR("Error setting timer for channel %d", channel->idx);
		return ret;
	}

	return 0;
}

static int pwm_led_esp32_set_cycles(const struct device *dev, uint32_t channel_idx,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	struct pwm_ledc_esp32_data *data = (struct pwm_ledc_esp32_data *const)(dev)->data;
	struct pwm_ledc_esp32_channel_config *channel = get_channel_config(dev, channel_idx);
	int ret = 0;

	if (!channel) {
		LOG_ERR("Error getting channel %d", channel_idx);
		return -EINVAL;
	}

	k_sem_take(&data->cmd_sem, K_FOREVER);

	if (flags & PWM_POLARITY_INVERTED) {
		pulse_cycles = period_cycles - pulse_cycles;
		channel->inverted = true;
	} else {
		channel->inverted = false;
	}

	ledc_hal_init(&data->hal, channel->speed_mode);

	if ((pulse_cycles == period_cycles) || (pulse_cycles == 0)) {
		/* For duty 0% and 100% stop PWM, set output level and return */
		pwm_led_esp32_stop(data, channel, (pulse_cycles == period_cycles));
		goto sem_give;
	}

	ret = pwm_led_esp32_channel_update_frequency(dev, channel, period_cycles);

	if (ret < 0) {
		LOG_ERR("Error updating frequency of channel %d", channel_idx);
		goto sem_give;
	}

	/* Update PWM duty  */

	double duty_cycle = (double)pulse_cycles / (double)period_cycles;

	channel->duty_val = (uint32_t)((double)(1 << channel->resolution) * duty_cycle);

	pwm_led_esp32_duty_set(dev, channel);

	pwm_led_esp32_start(data, channel);

sem_give:
	k_sem_give(&data->cmd_sem);

	return ret;
}

int pwm_led_esp32_init(const struct device *dev)
{
	const struct pwm_ledc_esp32_config *config = dev->config;
	struct pwm_ledc_esp32_data *data = dev->data;
	struct pwm_ledc_esp32_channel_config *channel;
	int ret = 0;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable peripheral */
	clock_control_on(config->clock_dev, config->clock_subsys);

#if SOC_LEDC_HAS_TIMER_SPECIFIC_MUX
	/* Combine clock sources to include timer specific sources */
	memcpy(lowspd_clks, global_clks, sizeof(global_clks));
	memcpy(&lowspd_clks[ARRAY_SIZE(global_clks)], timer_specific_clks,
	       sizeof(timer_specific_clks));
#endif

	for (int i = 0; i < config->channel_len; ++i) {
		channel = &config->channel_config[i];

		ledc_hal_init(&data->hal, channel->speed_mode);

		if (channel->speed_mode == LEDC_LOW_SPEED_MODE) {
			channel->clock_src = global_clks[0];
			ledc_hal_set_slow_clk_sel(&data->hal, channel->clock_src);
		}
#ifdef SOC_LEDC_SUPPORT_HS_MODE
		else {
			channel->clock_src = highspd_clks[0];
		}
#endif
		ledc_hal_set_clock_source(&data->hal, channel->timer_num, channel->clock_src);

		esp_clk_tree_src_get_freq_hz(channel->clock_src,
					     ESP_CLK_TREE_SRC_FREQ_PRECISION_CACHED,
					     &channel->clock_src_hz);

		ledc_hal_bind_channel_timer(&data->hal, channel->channel_num, channel->timer_num);
		pwm_led_esp32_stop(data, channel, channel->inverted);
		ledc_hal_timer_rst(&data->hal, channel->timer_num);
	}

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);

	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(pwm, pwm_led_esp32_api) = {
	.set_cycles = pwm_led_esp32_set_cycles,
	.get_cycles_per_sec = pwm_led_esp32_get_cycles_per_sec,
};

PINCTRL_DT_INST_DEFINE(0);

#define CHANNEL_CONFIG(node_id)                                                                    \
	{                                                                                          \
		.idx = DT_REG_ADDR(node_id),                                                       \
		.channel_num = DT_REG_ADDR(node_id) % 8,                                           \
		.timer_num = DT_PROP(node_id, timer),                                              \
		.speed_mode = DT_REG_ADDR(node_id) < SOC_LEDC_CHANNEL_NUM ? LEDC_LOW_SPEED_MODE    \
									  : !LEDC_LOW_SPEED_MODE,  \
		.inverted = DT_PROP(node_id, inverted),                                            \
	},

static struct pwm_ledc_esp32_channel_config channel_config[] = {
	DT_INST_FOREACH_CHILD(0, CHANNEL_CONFIG)};

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

DEVICE_DT_INST_DEFINE(0, &pwm_led_esp32_init, NULL, &pwm_ledc_esp32_data, &pwm_ledc_esp32_config,
		      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_led_esp32_api);
