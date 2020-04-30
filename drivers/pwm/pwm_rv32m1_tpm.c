/*
 * Copyright 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 *
 * Heavily based on pwm_mcux_ftm.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT openisa_rv32m1_tpm

#include <drivers/clock_control.h>
#include <errno.h>
#include <drivers/pwm.h>
#include <soc.h>
#include <fsl_tpm.h>
#include <fsl_clock.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_rv32m1_tpm);

#define MAX_CHANNELS ARRAY_SIZE(TPM0->CONTROLS)

struct rv32m1_tpm_config {
	TPM_Type *base;
	char *clock_name;
	clock_control_subsys_t clock_subsys;
	tpm_clock_source_t tpm_clock_source;
	tpm_clock_prescale_t prescale;
	uint8_t channel_count;
	tpm_pwm_mode_t mode;
};

struct rv32m1_tpm_data {
	uint32_t clock_freq;
	uint32_t period_cycles;
	tpm_chnl_pwm_signal_param_t channel[MAX_CHANNELS];
};

static int rv32m1_tpm_pin_set(const struct device *dev, uint32_t pwm,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct rv32m1_tpm_config *config = dev->config;
	struct rv32m1_tpm_data *data = dev->data;
	uint8_t duty_cycle;

	if ((period_cycles == 0U) || (pulse_cycles > period_cycles)) {
		LOG_ERR("Invalid combination: period_cycles=%d, "
			    "pulse_cycles=%d", period_cycles, pulse_cycles);
		return -EINVAL;
	}

	if (pwm >= config->channel_count) {
		LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

	duty_cycle = pulse_cycles * 100U / period_cycles;
	data->channel[pwm].dutyCyclePercent = duty_cycle;

	if ((flags & PWM_POLARITY_INVERTED) == 0) {
		data->channel[pwm].level = kTPM_HighTrue;
	} else {
		data->channel[pwm].level = kTPM_LowTrue;
	}

	LOG_DBG("pulse_cycles=%d, period_cycles=%d, duty_cycle=%d, flags=%d",
		pulse_cycles, period_cycles, duty_cycle, flags);

	if (period_cycles != data->period_cycles) {
		uint32_t pwm_freq;
		status_t status;

		if (data->period_cycles != 0) {
			/* Only warn when not changing from zero */
			LOG_WRN("Changing period cycles from %d to %d"
				" affects all %d channels in %s",
				data->period_cycles, period_cycles,
				config->channel_count, dev->name);
		}

		data->period_cycles = period_cycles;

		pwm_freq = (data->clock_freq >> config->prescale) /
			   period_cycles;

		LOG_DBG("pwm_freq=%d, clock_freq=%d", pwm_freq,
			data->clock_freq);

		if (pwm_freq == 0U) {
			LOG_ERR("Could not set up pwm_freq=%d", pwm_freq);
			return -EINVAL;
		}

		TPM_StopTimer(config->base);

		status = TPM_SetupPwm(config->base, data->channel,
				      config->channel_count, config->mode,
				      pwm_freq, data->clock_freq);

		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}
		TPM_StartTimer(config->base, config->tpm_clock_source);
	} else {
		TPM_UpdateChnlEdgeLevelSelect(config->base, pwm,
					      data->channel[pwm].level);
		TPM_UpdatePwmDutycycle(config->base, pwm, config->mode,
				       duty_cycle);
	}

	return 0;
}

static int rv32m1_tpm_get_cycles_per_sec(const struct device *dev,
					 uint32_t pwm,
					 uint64_t *cycles)
{
	const struct rv32m1_tpm_config *config = dev->config;
	struct rv32m1_tpm_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int rv32m1_tpm_init(const struct device *dev)
{
	const struct rv32m1_tpm_config *config = dev->config;
	struct rv32m1_tpm_data *data = dev->data;
	tpm_chnl_pwm_signal_param_t *channel = data->channel;
	const struct device *clock_dev;
	tpm_config_t tpm_config;
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

	if (clock_control_on(clock_dev, config->clock_subsys)) {
		LOG_ERR("Could not turn on clock");
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
		channel->level = kTPM_NoPwmSignal;
		channel->dutyCyclePercent = 0;
		channel->firstEdgeDelayPercent = 0;
		channel++;
	}

	TPM_GetDefaultConfig(&tpm_config);
	tpm_config.prescale = config->prescale;

	TPM_Init(config->base, &tpm_config);

	return 0;
}

static const struct pwm_driver_api rv32m1_tpm_driver_api = {
	.pin_set = rv32m1_tpm_pin_set,
	.get_cycles_per_sec = rv32m1_tpm_get_cycles_per_sec,
};

#define TPM_DEVICE(n) \
	static const struct rv32m1_tpm_config rv32m1_tpm_config_##n = { \
		.base =	(TPM_Type *) \
			DT_INST_REG_ADDR(n), \
		.clock_name = \
			DT_INST_CLOCKS_LABEL(n), \
		.clock_subsys = (clock_control_subsys_t) \
			DT_INST_CLOCKS_CELL(n, name), \
		.tpm_clock_source = kTPM_SystemClock, \
		.prescale = kTPM_Prescale_Divide_16, \
		.channel_count = FSL_FEATURE_TPM_CHANNEL_COUNTn((TPM_Type *) \
			DT_INST_REG_ADDR(n)), \
		.mode = kTPM_EdgeAlignedPwm, \
	}; \
	static struct rv32m1_tpm_data rv32m1_tpm_data_##n; \
	DEVICE_AND_API_INIT(rv32m1_tpm_##n, \
			    DT_INST_LABEL(n), \
			    &rv32m1_tpm_init, &rv32m1_tpm_data_##n, \
			    &rv32m1_tpm_config_##n, \
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			    &rv32m1_tpm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TPM_DEVICE)
