/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_ftm_pwm

#include <drivers/clock_control.h>
#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <fsl_ftm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_mcux_ftm);

#define MAX_CHANNELS ARRAY_SIZE(FTM0->CONTROLS)

struct mcux_ftm_config {
	FTM_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	ftm_clock_source_t ftm_clock_source;
	ftm_clock_prescale_t prescale;
	uint8_t channel_count;
	ftm_pwm_mode_t mode;
};

struct mcux_ftm_data {
	uint32_t clock_freq;
	uint32_t period_cycles;
	ftm_chnl_pwm_config_param_t channel[MAX_CHANNELS];
};

static int mcux_ftm_pin_set(struct device *dev, uint32_t pwm,
			    uint32_t period_cycles, uint32_t pulse_cycles,
			    pwm_flags_t flags)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	status_t status;

	if ((period_cycles == 0U) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination: period_cycles=%d, "
			    "pulse_cycles=%d", period_cycles, pulse_cycles);
		return -EINVAL;
	}

	if (pwm >= config->channel_count) {
		LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

	data->channel[pwm].dutyValue = pulse_cycles;

	if ((flags & PWM_POLARITY_INVERTED) == 0) {
		data->channel[pwm].level = kFTM_HighTrue;
	} else {
		data->channel[pwm].level = kFTM_LowTrue;
	}

	LOG_DBG("pulse_cycles=%d, period_cycles=%d, flags=%d",
		pulse_cycles, period_cycles, flags);

	if (period_cycles != data->period_cycles) {
		if (data->period_cycles != 0) {
			/* Only warn when not changing from zero */
			LOG_WRN("Changing period cycles from %d to %d"
				" affects all %d channels in %s",
				data->period_cycles, period_cycles,
				config->channel_count, dev->name);
		}

		data->period_cycles = period_cycles;

		FTM_StopTimer(config->base);
		FTM_SetTimerPeriod(config->base, period_cycles);

		FTM_SetSoftwareTrigger(config->base, true);
		FTM_StartTimer(config->base, config->ftm_clock_source);

	}

	status = FTM_SetupPwmMode(config->base, data->channel,
				  config->channel_count, config->mode);
	if (status != kStatus_Success) {
		LOG_ERR("Could not set up pwm");
		return -ENOTSUP;
	}
	FTM_SetSoftwareTrigger(config->base, true);

	return 0;
}

static int mcux_ftm_get_cycles_per_sec(struct device *dev, uint32_t pwm,
				       uint64_t *cycles)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int mcux_ftm_init(struct device *dev)
{
	const struct mcux_ftm_config *config = dev->config;
	struct mcux_ftm_data *data = dev->data;
	ftm_chnl_pwm_config_param_t *channel = data->channel;
	struct device *clock_dev;
	ftm_config_t ftm_config;
	int i;

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	clock_dev = device_get_binding(config->clock_name);
	if (clock_dev == NULL) {
		LOG_ERR("Could not get clock device");
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
		channel->level = kFTM_NoPwmSignal;
		channel->dutyValue = 0;
		channel->firstEdgeValue = 0;
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

#define TO_FTM_PRESCALE_DIVIDE(val) _DO_CONCAT(kFTM_Prescale_Divide_, val)

#define FTM_DEVICE(n) \
	static const struct mcux_ftm_config mcux_ftm_config_##n = { \
		.base = (FTM_Type *)DT_INST_REG_ADDR(n),\
		.clock_name = DT_INST_CLOCKS_LABEL(n), \
		.clock_subsys = (clock_control_subsys_t) \
			DT_INST_CLOCKS_CELL(n, name), \
		.ftm_clock_source = kFTM_FixedClock, \
		.prescale = TO_FTM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)),\
		.channel_count = FSL_FEATURE_FTM_CHANNEL_COUNTn((FTM_Type *) \
			DT_INST_REG_ADDR(n)), \
		.mode = kFTM_EdgeAlignedPwm, \
	}; \
	static struct mcux_ftm_data mcux_ftm_data_##n; \
	DEVICE_AND_API_INIT(mcux_ftm_##n, DT_INST_LABEL(n), \
			    &mcux_ftm_init, &mcux_ftm_data_##n, \
			    &mcux_ftm_config_##n, \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			    &mcux_ftm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(FTM_DEVICE)
