/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_adc

#include <soc.h>
#include <py32f0xx_ll_adc.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define PY32_ADC_CHANNEL_MASK           (GENMASK(9, 0) | BIT(11) | BIT(12))
#define PY32_ADC_CALIBRATION_TIMEOUT_US 10000U

struct adc_py32_config {
	ADC_TypeDef *adc;
	ADC_Common_TypeDef *common;
	const struct device *clk_dev;
	struct py32_pclken pclken;
	struct reset_dt_spec reset;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_configure)(const struct device *dev);
};

struct adc_py32_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t channels;
	uint32_t configured;
	uint32_t adc_rate;
	uint8_t sample_time[13];
};

static int adc_py32_acquisition_time(uint16_t acquisition_time, uint32_t adc_rate)
{
	/* Twice the sampling time, to retain the half-cycle hardware precision. */
	static const uint16_t half_cycles[] = {7U, 11U, 15U, 27U, 57U, 83U, 143U, 479U};
	uint64_t required;
	uint32_t value = ADC_ACQ_TIME_VALUE(acquisition_time);

	if ((acquisition_time == ADC_ACQ_TIME_DEFAULT) || (acquisition_time == ADC_ACQ_TIME_MAX)) {
		return ARRAY_SIZE(half_cycles) - 1U;
	}
	switch (ADC_ACQ_TIME_UNIT(acquisition_time)) {
	case ADC_ACQ_TIME_TICKS:
		required = value * 2U;
		break;
	case ADC_ACQ_TIME_MICROSECONDS:
		required = DIV_ROUND_UP((uint64_t)value * adc_rate * 2U, 1000000U);
		break;
	case ADC_ACQ_TIME_NANOSECONDS:
		required = DIV_ROUND_UP((uint64_t)value * adc_rate * 2U, 1000000000U);
		break;
	default:
		return -EINVAL;
	}
	for (size_t i = 0U; i < ARRAY_SIZE(half_cycles); i++) {
		if (required <= half_cycles[i]) {
			return i;
		}
	}
	return -EINVAL;
}

static int adc_py32_channel_setup(const struct device *dev, const struct adc_channel_cfg *channel)
{
	struct adc_py32_data *data = dev->data;
	int sample_time;

	if ((channel->channel_id >= ARRAY_SIZE(data->sample_time)) ||
	    ((BIT(channel->channel_id) & PY32_ADC_CHANNEL_MASK) == 0U)) {
		return -EINVAL;
	}
	if (channel->differential || (channel->gain != ADC_GAIN_1) ||
	    (channel->reference != ADC_REF_VDD_1)) {
		return -ENOTSUP;
	}
	sample_time = adc_py32_acquisition_time(channel->acquisition_time, data->adc_rate);
	if (sample_time < 0) {
		return sample_time;
	}
	/* Internal voltage-reference and temperature inputs require longer acquisition. */
	if (channel->channel_id >= 11U) {
		sample_time = 7;
	}
	adc_context_lock(&data->ctx, false, NULL);
	data->sample_time[channel->channel_id] = sample_time;
	data->configured |= BIT(channel->channel_id);
	adc_context_release(&data->ctx, 0);
	return 0;
}

static int adc_py32_calibrate(const struct device *dev)
{
	const struct adc_py32_config *cfg = dev->config;
	int ret = reset_line_toggle_dt(&cfg->reset);

	if (ret < 0) {
		return ret;
	}
	/* Calibration requires the undivided system clock. */
	LL_ADC_SetClock(cfg->adc, LL_ADC_CLOCK_SYNC_PCLK_DIV1);
	LL_ADC_SetCalibrationSamplingTime(cfg->adc, LL_ADC_CAL_SAMPLINGTIME_8CYCLES);
	LL_ADC_SetCalibrationMode(cfg->adc, LL_ADC_CAL_MODE_OFFSETANDLINEARITY);
	LL_ADC_ClearCalibrationStatus(cfg->adc);
	LL_ADC_StartCalibration(cfg->adc);
	if (!WAIT_FOR(LL_ADC_IsCalibrationOnGoing(cfg->adc) == 0U, PY32_ADC_CALIBRATION_TIMEOUT_US,
		      k_busy_wait(1))) {
		return -ETIMEDOUT;
	}
	if (LL_ADC_GetCalibrationStatus(cfg->adc) != LL_ADC_CAL_STATUS_SUCCESS) {
		return -EIO;
	}
	/* Allow at least four system clock cycles after calibration. */
	k_busy_wait(1);
	LL_ADC_SetClock(cfg->adc, LL_ADC_CLOCK_SYNC_PCLK_DIV4);
	LL_ADC_SetDataAlignment(cfg->adc, LL_ADC_DATA_ALIGN_RIGHT);
	LL_ADC_REG_SetTriggerSource(cfg->adc, LL_ADC_REG_TRIG_SOFTWARE);
	LL_ADC_REG_SetContinuousMode(cfg->adc, LL_ADC_REG_CONV_SINGLE);
	/* Internal paths can only be configured while the ADC is disabled. */
	LL_ADC_SetCommonPathInternalCh(cfg->common, LL_ADC_PATH_INTERNAL_TEMPSENSOR |
							    LL_ADC_PATH_INTERNAL_VREFINT);
	/* Cover the temperature sensor's 120 us startup and VREFINT stabilization. */
	k_busy_wait(120);
	return 0;
}

static void adc_py32_start_channel(const struct device *dev)
{
	const struct adc_py32_config *cfg = dev->config;
	struct adc_py32_data *data = dev->data;
	uint32_t channel = find_lsb_set(data->channels) - 1U;

	LL_ADC_REG_SetSequencerChannels(cfg->adc, BIT(channel));
	LL_ADC_ClearFlag_EOC(cfg->adc);
	LL_ADC_ClearFlag_EOS(cfg->adc);
	LL_ADC_ClearFlag_OVR(cfg->adc);
	LL_ADC_EnableIT_EOS(cfg->adc);
	LL_ADC_EnableIT_OVR(cfg->adc);
	/* Single conversions power down the ADC; enable it for every channel. */
	LL_ADC_Enable(cfg->adc);
	k_busy_wait(2);
	LL_ADC_REG_StartConversion(cfg->adc);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_py32_data *data = CONTAINER_OF(ctx, struct adc_py32_data, ctx);

	data->repeat_buffer = data->buffer;
	data->channels = ctx->sequence.channels;
	adc_py32_start_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_py32_data *data = CONTAINER_OF(ctx, struct adc_py32_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_py32_isr(const struct device *dev)
{
	const struct adc_py32_config *cfg = dev->config;
	struct adc_py32_data *data = dev->data;

	if (LL_ADC_IsActiveFlag_OVR(cfg->adc) != 0U) {
		LL_ADC_DisableIT_EOS(cfg->adc);
		LL_ADC_DisableIT_OVR(cfg->adc);
		LL_ADC_ClearFlag_OVR(cfg->adc);
		adc_context_complete(&data->ctx, -EIO);
		return;
	}
	if (LL_ADC_IsActiveFlag_EOS(cfg->adc) != 0U) {
		*data->buffer++ = LL_ADC_REG_ReadConversionData12(cfg->adc);
		LL_ADC_ClearFlag_EOC(cfg->adc);
		LL_ADC_ClearFlag_EOS(cfg->adc);
		data->channels &= data->channels - 1U;
		if (data->channels != 0U) {
			adc_py32_start_channel(dev);
		} else {
			LL_ADC_DisableIT_EOS(cfg->adc);
			LL_ADC_DisableIT_OVR(cfg->adc);
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	}
}

static int adc_py32_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_py32_config *cfg = dev->config;
	struct adc_py32_data *data = dev->data;
	size_t samples = POPCOUNT(sequence->channels);
	uint32_t resolution;
	uint8_t sample_time = 0U;
	int ret;

	if ((sequence->channels == 0U) || ((sequence->channels & ~PY32_ADC_CHANNEL_MASK) != 0U) ||
	    ((sequence->channels & ~data->configured) != 0U) || (sequence->buffer == NULL)) {
		return -EINVAL;
	}
	if (sequence->oversampling != 0U) {
		return -ENOTSUP;
	}
	switch (sequence->resolution) {
	case 6:
		resolution = LL_ADC_RESOLUTION_6B;
		break;
	case 8:
		resolution = LL_ADC_RESOLUTION_8B;
		break;
	case 10:
		resolution = LL_ADC_RESOLUTION_10B;
		break;
	case 12:
		resolution = LL_ADC_RESOLUTION_12B;
		break;
	default:
		return -EINVAL;
	}
	if (sequence->options != NULL) {
		samples *= 1U + sequence->options->extra_samplings;
	}
	if (sequence->buffer_size < samples * sizeof(uint16_t)) {
		return -ENOMEM;
	}
	if (sequence->calibrate) {
		ret = adc_py32_calibrate(dev);
		if (ret < 0) {
			return ret;
		}
	}
	for (size_t channel = 0U; channel < ARRAY_SIZE(data->sample_time); channel++) {
		if ((sequence->channels & BIT(channel)) != 0U) {
			sample_time = MAX(sample_time, data->sample_time[channel]);
		}
	}
	LL_ADC_SetResolution(cfg->adc, resolution);
	LL_ADC_SetSamplingTimeCommonChannels(cfg->adc, sample_time);
	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_py32_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_py32_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_py32_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_py32_read_async(const struct device *dev, const struct adc_sequence *sequence,
			       struct k_poll_signal *poll_signal)
{
	struct adc_py32_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, poll_signal);
	ret = adc_py32_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);
	return ret;
}
#endif

static int adc_py32_init(const struct device *dev)
{
	const struct adc_py32_config *cfg = dev->config;
	struct adc_py32_data *data = dev->data;
	uint32_t rate;
	int ret;

	if (!device_is_ready(cfg->clk_dev) || !device_is_ready(cfg->reset.dev)) {
		return -ENODEV;
	}
	ret = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken);
	if (ret < 0) {
		return ret;
	}
	ret = clock_control_get_rate(cfg->clk_dev, (clock_control_subsys_t)&cfg->pclken, &rate);
	if (ret < 0) {
		return ret;
	}
	data->adc_rate = rate / 4U;
	if (data->adc_rate > 12000000U) {
		return -EINVAL;
	}
	if (cfg->pinctrl != NULL) {
		ret = pinctrl_apply_state(cfg->pinctrl, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
	}
	ret = adc_py32_calibrate(dev);
	if (ret < 0) {
		return ret;
	}
	data->dev = dev;
	cfg->irq_configure(dev);
	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(adc, adc_py32_api) = {
	.channel_setup = adc_py32_channel_setup,
	.read = adc_py32_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_py32_read_async,
#endif
};

#define ADC_PY32_PINCTRL_DEFINE(inst)                                                              \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, pinctrl_0), (PINCTRL_DT_INST_DEFINE(inst);))

#define ADC_PY32_PINCTRL(inst)                                                                     \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, pinctrl_0), \
		    (PINCTRL_DT_INST_DEV_CONFIG_GET(inst)), (NULL))

#define ADC_PY32_DEFINE(inst)                                                                      \
	ADC_PY32_PINCTRL_DEFINE(inst)                                                              \
	static void adc_py32_irq_configure_##inst(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), adc_py32_isr,         \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}                                                                                          \
	static struct adc_py32_data adc_py32_data_##inst = {                                       \
		ADC_CONTEXT_INIT_TIMER(adc_py32_data_##inst, ctx),                                 \
		ADC_CONTEXT_INIT_LOCK(adc_py32_data_##inst, ctx),                                  \
		ADC_CONTEXT_INIT_SYNC(adc_py32_data_##inst, ctx),                                  \
	};                                                                                         \
	static const struct adc_py32_config adc_py32_config_##inst = {                             \
		.adc = (ADC_TypeDef *)DT_INST_REG_ADDR_BY_NAME(inst, adc),                         \
		.common = (ADC_Common_TypeDef *)DT_INST_REG_ADDR_BY_NAME(inst, common),            \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
		.pclken = {DT_INST_CLOCKS_CELL(inst, bus), DT_INST_CLOCKS_CELL(inst, bits)},       \
		.reset = RESET_DT_SPEC_INST_GET(inst),                                             \
		.pinctrl = ADC_PY32_PINCTRL(inst),                                                 \
		.irq_configure = adc_py32_irq_configure_##inst,                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, adc_py32_init, NULL, &adc_py32_data_##inst,                    \
			      &adc_py32_config_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,      \
			      &adc_py32_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_PY32_DEFINE)
