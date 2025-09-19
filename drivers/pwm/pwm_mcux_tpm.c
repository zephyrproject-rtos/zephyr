/*
 * Copyright 2019 Henrik Brix Andersen <henrik@brixandersen.dk>
 * Copyright 2020, 2024-2025 NXP
 *
 * Heavily based on pwm_mcux_ftm.c, which is:
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_tpm

#include <zephyr/drivers/clock_control.h>
#include <errno.h>
#include <zephyr/drivers/pwm.h>
#include <soc.h>
#include <fsl_tpm.h>
#include <fsl_clock.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>

#if defined(CONFIG_SOC_MIMX9596)
#include <zephyr/dt-bindings/clock/imx95_clock.h>
#endif

LOG_MODULE_REGISTER(pwm_mcux_tpm, CONFIG_PWM_LOG_LEVEL);

#if defined(TPM0)
#define MAX_CHANNELS ARRAY_SIZE(TPM0->CONTROLS)
#else
#define MAX_CHANNELS ARRAY_SIZE(TPM1->CONTROLS)
#endif

#define DEV_CFG(_dev) ((const struct mcux_tpm_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct mcux_tpm_data *)(_dev)->data)
#define TPM_TYPE_BASE(dev, name) ((TPM_Type *)DEVICE_MMIO_NAMED_GET(dev, name))

struct mcux_tpm_config {
	DEVICE_MMIO_NAMED_ROM(base);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	tpm_clock_source_t tpm_clock_source;
	tpm_clock_prescale_t prescale;
	uint8_t channel_count;
	tpm_pwm_mode_t mode;
	const struct pinctrl_dev_config *pincfg;
};

struct mcux_tpm_data {
	DEVICE_MMIO_NAMED_RAM(base);
	uint32_t clock_freq;
	uint32_t period_cycles;
	tpm_chnl_pwm_signal_param_t channel[MAX_CHANNELS];
};

static int mcux_tpm_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;
	TPM_Type *base = TPM_TYPE_BASE(dev, base);

	if (channel >= config->channel_count) {
		LOG_ERR("Invalid channel");
		return -ENOTSUP;
	}

	if (period_cycles == 0 || period_cycles == TPM_MAX_COUNTER_VALUE(base)) {
		return -ENOTSUP;
	}

	if ((TPM_MAX_COUNTER_VALUE(base) == 0xFFFFU) &&
	    (pulse_cycles > TPM_MAX_COUNTER_VALUE(base))) {
		return -ENOTSUP;
	}

	LOG_DBG("pulse_cycles=%d, period_cycles=%d, flags=%d", pulse_cycles, period_cycles, flags);

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

		TPM_StopTimer(base);

		/* Set counter back to zero */
		base->CNT = 0;

		status = TPM_SetupPwm(base, data->channel,
				      config->channel_count, config->mode,
				      pwm_freq, data->clock_freq);

		if (status != kStatus_Success) {
			LOG_ERR("Could not set up pwm");
			return -ENOTSUP;
		}
		TPM_StartTimer(base, config->tpm_clock_source);
	}

	if ((flags & PWM_POLARITY_INVERTED) == 0 &&
		   data->channel[channel].level != kTPM_HighTrue) {
		data->channel[channel].level = kTPM_HighTrue;
		TPM_UpdateChnlEdgeLevelSelect(base, channel, kTPM_HighTrue);
	} else if ((flags & PWM_POLARITY_INVERTED) != 0 &&
		   data->channel[channel].level != kTPM_LowTrue) {
		data->channel[channel].level = kTPM_LowTrue;
		TPM_UpdateChnlEdgeLevelSelect(base, channel, kTPM_LowTrue);
	}

	if (pulse_cycles == period_cycles) {
		pulse_cycles = period_cycles + 1U;
	}

	base->CONTROLS[channel].CnV = pulse_cycles;

	return 0;
}

static int mcux_tpm_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;

	*cycles = data->clock_freq >> config->prescale;

	return 0;
}

static int mcux_tpm_init(const struct device *dev)
{
	const struct mcux_tpm_config *config = dev->config;
	struct mcux_tpm_data *data = dev->data;
	tpm_chnl_pwm_signal_param_t *channel = data->channel;
	tpm_config_t tpm_config;
	TPM_Type *base;
	int i;
	int err;

	DEVICE_MMIO_NAMED_MAP(dev, base, K_MEM_CACHE_NONE | K_MEM_DIRECT_MAP);

	base = TPM_TYPE_BASE(dev, base);

	if (config->channel_count > ARRAY_SIZE(data->channel)) {
		LOG_ERR("Invalid channel count");
		return -EINVAL;
	}

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

#if defined(CONFIG_SOC_MIMX9596)
	/* IMX9596 AON and WAKEUP clocks aren't controllable */
	if (config->clock_subsys != (clock_control_subsys_t)IMX95_CLK_BUSWAKEUP &&
	    config->clock_subsys != (clock_control_subsys_t)IMX95_CLK_BUSAON) {
#endif
		if (clock_control_on(config->clock_dev, config->clock_subsys)) {
			LOG_ERR("Could not turn on clock");
			return -EINVAL;
		}
#if defined(CONFIG_SOC_MIMX9596)
	}
#endif

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	for (i = 0; i < config->channel_count; i++) {
		channel->chnlNumber = i;
#if !(defined(FSL_FEATURE_TPM_HAS_PAUSE_LEVEL_SELECT) && FSL_FEATURE_TPM_HAS_PAUSE_LEVEL_SELECT)
		channel->level = kTPM_NoPwmSignal;
#else
		channel->level = kTPM_HighTrue;
		channel->pauseLevel = kTPM_ClearOnPause;
#endif
		channel->dutyCyclePercent = 0;
#if defined(FSL_FEATURE_TPM_HAS_COMBINE) && FSL_FEATURE_TPM_HAS_COMBINE
		channel->firstEdgeDelayPercent = 0;
#endif
		channel++;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err) {
		return err;
	}

	TPM_GetDefaultConfig(&tpm_config);
	tpm_config.prescale = config->prescale;

	TPM_Init(base, &tpm_config);

	return 0;
}

static DEVICE_API(pwm, mcux_tpm_driver_api) = {
	.set_cycles = mcux_tpm_set_cycles,
	.get_cycles_per_sec = mcux_tpm_get_cycles_per_sec,
};

#define TO_TPM_PRESCALE_DIVIDE(val) _DO_CONCAT(kTPM_Prescale_Divide_, val)

#define TPM_DEVICE(n) \
	PINCTRL_DT_INST_DEFINE(n); \
	static const struct mcux_tpm_config mcux_tpm_config_##n = { \
		DEVICE_MMIO_NAMED_ROM_INIT(base, DT_DRV_INST(n)), \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)), \
		.clock_subsys = (clock_control_subsys_t) \
			DT_INST_CLOCKS_CELL(n, name), \
		.tpm_clock_source = kTPM_SystemClock, \
		.prescale = TO_TPM_PRESCALE_DIVIDE(DT_INST_PROP(n, prescaler)), \
		.channel_count = FSL_FEATURE_TPM_CHANNEL_COUNTn((TPM_Type *) \
			DT_INST_REG_ADDR(n)), \
		.mode = kTPM_EdgeAlignedPwm, \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
	}; \
	static struct mcux_tpm_data mcux_tpm_data_##n; \
	DEVICE_DT_INST_DEFINE(n, &mcux_tpm_init, NULL, \
			    &mcux_tpm_data_##n, \
			    &mcux_tpm_config_##n, \
			    POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			    &mcux_tpm_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TPM_DEVICE)
