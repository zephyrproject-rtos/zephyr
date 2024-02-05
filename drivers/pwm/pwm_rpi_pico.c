/*
 * Copyright (c) 2022, Joep Buruma
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT raspberrypi_pico_pwm

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_rpi_pico, CONFIG_PWM_LOG_LEVEL);

/* pico-sdk includes */
#include <hardware/pwm.h>
#include <hardware/structs/pwm.h>

#define PWM_RPI_PICO_COUNTER_TOP_MAX    UINT16_MAX
#define PWM_RPI_NUM_CHANNELS            (16U)

struct pwm_rpi_slice_config {
	uint8_t integral;
	uint8_t frac;
	bool phase_correct;
};

struct pwm_rpi_config {
	/*
	 * pwm_controller is the start address of the pwm peripheral.
	 */
	pwm_hw_t *pwm_controller;
	struct pwm_rpi_slice_config slice_configs[NUM_PWM_SLICES];
	const struct pinctrl_dev_config *pcfg;
	const struct reset_dt_spec reset;
	const struct device *clk_dev;
	const clock_control_subsys_t clk_id;
};

static float pwm_rpi_get_clkdiv(const struct device *dev, int slice)
{
	const struct pwm_rpi_config *cfg = dev->config;

	/* the divider is a fixed point 8.4 convert to float for use in pico-sdk */
	return (float)cfg->slice_configs[slice].integral +
		(float)cfg->slice_configs[slice].frac / 16.0f;
}

static inline uint32_t pwm_rpi_channel_to_slice(uint32_t channel)
{
	return channel / 2;
}

static inline uint32_t pwm_rpi_channel_to_pico_channel(uint32_t channel)
{
	return channel % 2;
}

static int pwm_rpi_get_cycles_per_sec(const struct device *dev, uint32_t ch, uint64_t *cycles)
{
	const struct pwm_rpi_config *cfg = dev->config;
	int slice = pwm_rpi_channel_to_slice(ch);
	uint32_t pclk;
	int ret;

	if (ch >= PWM_RPI_NUM_CHANNELS) {
		return -EINVAL;
	}

	ret = clock_control_get_rate(cfg->clk_dev, cfg->clk_id, &pclk);
	if (ret < 0 || pclk == 0) {
		return -EINVAL;
	}

	/* No need to check for divide by 0 since the minimum value of
	 * pwm_rpi_get_clkdiv is 1
	 */
	*cycles = (uint64_t)((float)pclk / pwm_rpi_get_clkdiv(dev, slice));
	return 0;
}

/* The pico_sdk only allows setting the polarity of both channels at once.
 * This is a convenience function to make setting the polarity of a single
 * channel easier.
 */
static void pwm_rpi_set_channel_polarity(const struct device *dev, int slice,
					 int pico_channel, bool inverted)
{
	const struct pwm_rpi_config *cfg = dev->config;

	bool pwm_polarity_a = (cfg->pwm_controller->slice[slice].csr & PWM_CH0_CSR_A_INV_BITS) > 0;
	bool pwm_polarity_b = (cfg->pwm_controller->slice[slice].csr & PWM_CH0_CSR_B_INV_BITS) > 0;

	if (pico_channel == PWM_CHAN_A) {
		pwm_polarity_a = inverted;
	} else if (pico_channel == PWM_CHAN_B) {
		pwm_polarity_b = inverted;
	}

	pwm_set_output_polarity(slice, pwm_polarity_a, pwm_polarity_b);
}

static int pwm_rpi_set_cycles(const struct device *dev, uint32_t ch, uint32_t period_cycles,
			   uint32_t pulse_cycles, pwm_flags_t flags)
{
	if (ch >= PWM_RPI_NUM_CHANNELS) {
		return -EINVAL;
	}

	if (period_cycles - 1 > PWM_RPI_PICO_COUNTER_TOP_MAX ||
	    pulse_cycles > PWM_RPI_PICO_COUNTER_TOP_MAX) {
		return -EINVAL;
	}

	int slice = pwm_rpi_channel_to_slice(ch);

	/* this is the channel within a pwm slice */
	int pico_channel = pwm_rpi_channel_to_pico_channel(ch);

	pwm_rpi_set_channel_polarity(dev, slice, pico_channel,
				     (flags & PWM_POLARITY_MASK) == PWM_POLARITY_INVERTED);
	pwm_set_wrap(slice, period_cycles - 1);
	pwm_set_chan_level(slice, pico_channel, pulse_cycles);

	return 0;
};

struct pwm_driver_api pwm_rpi_driver_api = {
	.get_cycles_per_sec = pwm_rpi_get_cycles_per_sec,
	.set_cycles = pwm_rpi_set_cycles,
};

static int pwm_rpi_init(const struct device *dev)
{
	const struct pwm_rpi_config *cfg = dev->config;
	pwm_config slice_cfg;
	size_t slice_idx;
	int err;

	err = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		LOG_ERR("Failed to configure pins for PWM. err=%d", err);
		return err;
	}

	err = clock_control_on(cfg->clk_dev, cfg->clk_id);
	if (err < 0) {
		return err;
	}

	err = reset_line_toggle_dt(&cfg->reset);
	if (err < 0) {
		return err;
	}

	for (slice_idx = 0; slice_idx < NUM_PWM_SLICES; slice_idx++) {
		slice_cfg = pwm_get_default_config();
		pwm_config_set_clkdiv_mode(&slice_cfg, PWM_DIV_FREE_RUNNING);

		pwm_init(slice_idx, &slice_cfg, false);

		pwm_set_clkdiv_int_frac(slice_idx,
					cfg->slice_configs[slice_idx].integral,
					cfg->slice_configs[slice_idx].frac);
		pwm_set_enabled(slice_idx, true);
	}

	return 0;
}

#define PWM_INST_RPI_SLICE_DIVIDER(idx, n)				   \
	{								   \
		.integral = DT_INST_PROP(idx, UTIL_CAT(divider_int_, n)), \
		.frac = DT_INST_PROP(idx, UTIL_CAT(divider_frac_, n)),	   \
	}

#define PWM_RPI_INIT(idx)									   \
												   \
	PINCTRL_DT_INST_DEFINE(idx);								   \
	static const struct pwm_rpi_config pwm_rpi_config_##idx = {				   \
		.pwm_controller = (pwm_hw_t *)DT_INST_REG_ADDR(idx),				   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),					   \
		.slice_configs = {								   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 0),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 1),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 2),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 3),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 4),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 5),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 6),					   \
			PWM_INST_RPI_SLICE_DIVIDER(idx, 7),					   \
		},										   \
		.reset = RESET_DT_SPEC_INST_GET(idx),						   \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),				   \
		.clk_id = (clock_control_subsys_t)DT_INST_PHA_BY_IDX(idx, clocks, 0, clk_id),	   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(idx, pwm_rpi_init, NULL, NULL, &pwm_rpi_config_##idx, POST_KERNEL,   \
			      CONFIG_PWM_INIT_PRIORITY, &pwm_rpi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_RPI_INIT);
