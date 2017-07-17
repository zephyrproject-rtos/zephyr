/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <pwm.h>
#include <soc.h>
#include <fsl_ftm.h>
#include <fsl_clock.h>

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_PWM_LEVEL
#include <logging/sys_log.h>

#define MAX_CHANNELS ARRAY_SIZE(FTM0->CONTROLS)

struct mcux_ftm_config {
	FTM_Type *base;
	clock_name_t clock_source;
	ftm_clock_source_t ftm_clock_source;
	ftm_clock_prescale_t prescale;
	u8_t channel_count;
	ftm_pwm_mode_t mode;
};

struct mcux_ftm_data {
	u32_t period_cycles;
	ftm_chnl_pwm_signal_param_t channel[MAX_CHANNELS];
};

static int mcux_ftm_pin_set(struct device *dev, u32_t pwm,
			    u32_t period_cycles, u32_t pulse_cycles)
{
	const struct mcux_ftm_config *config = dev->config->config_info;
	struct mcux_ftm_data *data = dev->driver_data;
	u8_t duty_cycle;

	if ((period_cycles == 0) || (pulse_cycles > period_cycles)) {
		SYS_LOG_ERR("Invalid combination: period_cycles=%d, "
			    "pulse_cycles=%d", period_cycles, pulse_cycles);
		return -EINVAL;
	}

	if (pwm >= config->channel_count) {
		SYS_LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

	duty_cycle = 100 * pulse_cycles / period_cycles;
	data->channel[pwm].dutyCyclePercent = duty_cycle;

	SYS_LOG_DBG("pulse_cycles=%d, period_cycles=%d, duty_cycle=%d",
		    pulse_cycles, period_cycles, duty_cycle);

	if (period_cycles != data->period_cycles) {
		u32_t clock_freq;
		u32_t pwm_freq;
		status_t status;

		SYS_LOG_WRN("Changing period cycles from %d to %d"
			    " affects all %d channels in %s",
			    data->period_cycles, period_cycles,
			    config->channel_count, dev->config->name);

		data->period_cycles = period_cycles;

		clock_freq = CLOCK_GetFreq(config->clock_source);
		pwm_freq = (clock_freq >> config->prescale) / period_cycles;

		SYS_LOG_DBG("pwm_freq=%d, clock_freq=%d", pwm_freq, clock_freq);

		if (pwm_freq == 0) {
			SYS_LOG_ERR("Could not set up pwm_freq=%d", pwm_freq);
			return -EINVAL;
		}

		FTM_StopTimer(config->base);

		status = FTM_SetupPwm(config->base, data->channel,
				      config->channel_count, config->mode,
				      pwm_freq, clock_freq);

		if (status != kStatus_Success) {
			SYS_LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}
		FTM_SetSoftwareTrigger(config->base, true);
		FTM_StartTimer(config->base, config->ftm_clock_source);

	} else {
		FTM_UpdatePwmDutycycle(config->base, pwm, config->mode,
				       duty_cycle);
		FTM_SetSoftwareTrigger(config->base, true);
	}

	return 0;
}

static int mcux_ftm_get_cycles_per_sec(struct device *dev, u32_t pwm,
				       u64_t *cycles)
{
	const struct mcux_ftm_config *config = dev->config->config_info;

	*cycles = CLOCK_GetFreq(config->clock_source) >> config->prescale;

	return 0;
}

static int mcux_ftm_init(struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config->config_info;
	struct mcux_ftm_data *data = dev->driver_data;
	ftm_chnl_pwm_signal_param_t *channel = data->channel;
	ftm_config_t ftm_config;
	int i;

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		SYS_LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
		channel->level = kFTM_LowTrue;
		channel->dutyCyclePercent = 0;
		channel->firstEdgeDelayPercent = 0;
		channel++;
	}

	FTM_GetDefaultConfig(&ftm_config);
	ftm_config.prescale = config->prescale;

	FTM_Init(config->base, &ftm_config);

	return 0;
}

static const struct pwm_driver_api mcux_ftm_driver_api = {
	.pin_set = mcux_ftm_pin_set,
	.get_cycles_per_sec = mcux_ftm_get_cycles_per_sec,
};

#ifdef CONFIG_PWM_0
static const struct mcux_ftm_config mcux_ftm_config_0 = {
	.base = (FTM_Type *)CONFIG_FTM_0_BASE_ADDRESS,
	.clock_source = kCLOCK_McgFixedFreqClk,
	.ftm_clock_source = kFTM_FixedClock,
	.prescale = kFTM_Prescale_Divide_16,
	.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn(FTM0),
	.mode = kFTM_EdgeAlignedPwm,
};

static struct mcux_ftm_data mcux_ftm_data_0;

DEVICE_AND_API_INIT(mcux_ftm_0, CONFIG_FTM_0_NAME, &mcux_ftm_init,
		    &mcux_ftm_data_0, &mcux_ftm_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ftm_driver_api);
#endif /* CONFIG_PWM_0 */

#ifdef CONFIG_PWM_1
static const struct mcux_ftm_config mcux_ftm_config_1 = {
	.base = (FTM_Type *)CONFIG_FTM_1_BASE_ADDRESS,
	.clock_source = kCLOCK_McgFixedFreqClk,
	.ftm_clock_source = kFTM_FixedClock,
	.prescale = kFTM_Prescale_Divide_16,
	.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn(FTM1),
	.mode = kFTM_EdgeAlignedPwm,
};

static struct mcux_ftm_data mcux_ftm_data_1;

DEVICE_AND_API_INIT(mcux_ftm_1, CONFIG_FTM_1_NAME, &mcux_ftm_init,
		    &mcux_ftm_data_1, &mcux_ftm_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ftm_driver_api);
#endif /* CONFIG_PWM_1 */

#ifdef CONFIG_PWM_2
static const struct mcux_ftm_config mcux_ftm_config_2 = {
	.base = (FTM_Type *)CONFIG_FTM_2_BASE_ADDRESS,
	.clock_source = kCLOCK_McgFixedFreqClk,
	.ftm_clock_source = kFTM_FixedClock,
	.prescale = kFTM_Prescale_Divide_16,
	.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn(FTM2),
	.mode = kFTM_EdgeAlignedPwm,
};

static struct mcux_ftm_data mcux_ftm_data_2;

DEVICE_AND_API_INIT(mcux_ftm_2, CONFIG_FTM_2_NAME, &mcux_ftm_init,
		    &mcux_ftm_data_2, &mcux_ftm_config_2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ftm_driver_api);
#endif /* CONFIG_PWM_2 */

#ifdef CONFIG_PWM_3
static const struct mcux_ftm_config mcux_ftm_config_3 = {
	.base = (FTM_Type *)CONFIG_FTM_3_BASE_ADDRESS,
	.clock_source = kCLOCK_McgFixedFreqClk,
	.ftm_clock_source = kFTM_FixedClock,
	.prescale = kFTM_Prescale_Divide_16,
	.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn(FTM3),
	.mode = kFTM_EdgeAlignedPwm,
};

static struct mcux_ftm_data mcux_ftm_data_3;

DEVICE_AND_API_INIT(mcux_ftm_3, CONFIG_FTM_3_NAME, &mcux_ftm_init,
		    &mcux_ftm_data_3, &mcux_ftm_config_3,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_ftm_driver_api);
#endif /* CONFIG_PWM_3 */
