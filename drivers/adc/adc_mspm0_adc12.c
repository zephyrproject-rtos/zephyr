/*
 * Copyright (c) 2024 Texas Instruments
 * copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT ti_mspm0_adc12

#include <errno.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_mspm0);

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/regulator.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mspm0_clock.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>

#include <ti/driverlib/dl_adc12.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_MSPM0_CHANNEL_MAX            (ADC_SYS_NUM_ANALOG_CHAN)
#define ADC_MSPM0_CHANNEL_NO_INIT        (0xFF)
#define ADC_MSPM0_NUM_SAMPLE_TIMERS      2
#define ADC_MSPM0_CHANNEL_CFG_STIME_MASK BIT(0)
#define ADC_MSPM0_CHANNEL_CFG_REF_SHIFT  1
#define ADC_MSPM0_REF_MASK_AFTER_SHIFT   (BIT(1) | BIT(0))
#define ADC_MSPM0_REF_VDD                0
#define ADC_MSPM0_REF_INTERNAL           1
#define ADC_MSPM0_REF_EXTERNAL           2
#define ADC_MSPM0_MAX_OVERSAMPLING       7
#define ADC_MSPM0_RESOLUTION_14          14
#define ADC_MSPM0_RESOLUTION_12          12
#define ADC_MSPM0_RESOLUTION_10          10
#define ADC_MSPM0_RESOLUTION_8           8
#define ADC_MSPM0_VREF_MULTIPLIER        1000
#define ADC_MSPM0_SCOMP0                 0
#define ADC_MSPM0_SCOMP1                 1
#define ADC_MSPM0_WAKEUP_TIME_NS         5000
#define ADC_MSPM0_NS_PER_SEC             1000000000
#define INT_VREF                         BIT(0)
#define EXT_VREF                         BIT(1)

struct adc_mspm0_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint16_t sample_time[ADC_MSPM0_NUM_SAMPLE_TIMERS];
	uint8_t channel_mem_ctl[ADC_MSPM0_CHANNEL_MAX];
	uint8_t channel_eoc;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	uint8_t vref_flags;
#endif
};

struct adc_mspm0_cfg {
	DL_ADC12_ClockConfig clock_config;
	ADC12_Regs *base;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_cfg_func)(void);
	const struct device *vref_config;
	const struct mspm0_sys_clock *clock_subsys;
	const uint8_t divider;
	const uint8_t max_result;
	bool auto_pwdn;
};

static void adc_mspm0_isr(const struct device *dev)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	uint8_t mem_ix;

	DL_ADC12_disableConversions(config->base);

	switch (DL_ADC12_getPendingInterrupt(config->base)) {
	case DL_ADC12_IIDX_MEM0_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM1_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM2_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM3_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM4_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM5_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM6_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM7_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM8_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM9_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM10_RESULT_LOADED:
	case DL_ADC12_IIDX_MEM11_RESULT_LOADED:
		for (mem_ix = 0; mem_ix < data->channel_eoc; mem_ix++) {
			*data->buffer++ = DL_ADC12_getMemResult(config->base, mem_ix);
		}
		*data->buffer = DL_ADC12_getMemResult(config->base, mem_ix);
		DL_ADC12_disableInterrupt(config->base, ((1 << (data->channel_eoc)) <<
					  ADC12_CPU_INT_IMASK_MEMRESIFG0_OFS));
		break;
	default:
		break;
	}
	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_mspm0_data *data = CONTAINER_OF(ctx, struct adc_mspm0_data, ctx);
	const struct device *dev = data->dev;
	const struct adc_mspm0_cfg *config = dev->config;

	DL_ADC12_clearInterruptStatus(config->base, ((1 << data->channel_eoc) <<
				      ADC12_CPU_INT_IMASK_MEMRESIFG0_OFS));
	DL_ADC12_enableInterrupt(config->base, ((1 << data->channel_eoc) <<
				 ADC12_CPU_INT_IMASK_MEMRESIFG0_OFS));
	DL_ADC12_enableConversions(config->base);
	DL_ADC12_startConversion(config->base);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_mspm0_data *data = CONTAINER_OF(ctx, struct adc_mspm0_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer++;
	}
}

static int adc_mspm0_validate_sampling_time(const struct adc_mspm0_cfg *config, uint16_t acq_time)
{
	uint32_t clock_rate;
	uint32_t period_ns;
	int ret;
	uint8_t wakeup_cycles;

	ret = clock_control_get_rate(DEVICE_DT_GET(DT_NODELABEL(ckm)),
				     (struct mspm0_sys_clock *)config->clock_subsys, &clock_rate);
	if (ret < 0) {
		return ret;
	}

	if (!config->auto_pwdn) {
		if (acq_time == ADC_ACQ_TIME_DEFAULT) {
			return 0;
		}

		if ((ADC_ACQ_TIME_UNIT(acq_time) == ADC_ACQ_TIME_TICKS) &&
		    (ADC_ACQ_TIME_VALUE(acq_time) <= ADC12_SCOMP0_VAL_MASK)) {
			return ADC_ACQ_TIME_VALUE(acq_time);
		} else {
			return -EINVAL;
		}
	} else {
		period_ns = ADC_MSPM0_NS_PER_SEC / (clock_rate / config->divider);
		/* Wakup time is 5us for both l and g mspm0 series */
		wakeup_cycles = ADC_MSPM0_WAKEUP_TIME_NS / period_ns;

		if (acq_time == ADC_ACQ_TIME_DEFAULT) {
			return wakeup_cycles;
		}

		acq_time = acq_time + wakeup_cycles;
		if ((ADC_ACQ_TIME_UNIT(acq_time) == ADC_ACQ_TIME_TICKS) &&
		    (ADC_ACQ_TIME_VALUE(acq_time) <= ADC12_SCOMP0_VAL_MASK)) {
			return ADC_ACQ_TIME_VALUE(acq_time);
		} else {
			return -EINVAL;
		}
	}
}

static int adc_mspm0_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	const uint8_t ch = channel_cfg->channel_id;
	int sampling_time;
	int ret = 0;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	const struct adc_driver_api *api = dev->api;
	uint32_t vref = ((uint32_t)api->ref_internal) * ADC_MSPM0_VREF_MULTIPLIER;
#endif

	if (ch > ADC_MSPM0_CHANNEL_MAX) {
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	sampling_time = adc_mspm0_validate_sampling_time(dev->config, channel_cfg->acquisition_time);
	if (sampling_time < 0) {
		return sampling_time;
	}

	adc_context_lock(&data->ctx, false, NULL);

	if (data->sample_time[ADC_MSPM0_SCOMP0] == UINT16_MAX) {
		DL_ADC12_setSampleTime0(config->base, sampling_time);
		data->sample_time[ADC_MSPM0_SCOMP0] = sampling_time;
		data->channel_mem_ctl[ch] = ADC_MSPM0_SCOMP0;
	} else if (data->sample_time[ADC_MSPM0_SCOMP0] == sampling_time) {
		data->channel_mem_ctl[ch] = ADC_MSPM0_SCOMP0;
	} else if (data->sample_time[ADC_MSPM0_SCOMP1] == UINT16_MAX) {
		DL_ADC12_setSampleTime1(config->base, sampling_time);
		data->sample_time[ADC_MSPM0_SCOMP1] = sampling_time;
		data->channel_mem_ctl[ch] = ADC_MSPM0_SCOMP1;
	} else if (data->sample_time[ADC_MSPM0_SCOMP1] == sampling_time) {
		data->channel_mem_ctl[ch] = ADC_MSPM0_SCOMP1;
	} else {
		ret = -EINVAL;
		goto unlock;
	}

	switch (channel_cfg->reference) {
	case ADC_REF_VDD_1:
		data->channel_mem_ctl[ch] |= (ADC_MSPM0_REF_VDD << ADC_MSPM0_CHANNEL_CFG_REF_SHIFT);
		break;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
	case ADC_REF_INTERNAL:
		data->channel_mem_ctl[ch] |= (ADC_MSPM0_REF_INTERNAL <<
					      ADC_MSPM0_CHANNEL_CFG_REF_SHIFT);

		if ((data->vref_flags & EXT_VREF) || !(config->vref_config)) {
			ret = -EINVAL;
			goto unlock;
		}

		if (!(data->vref_flags & INT_VREF)) {
			if (regulator_set_voltage(config->vref_config, vref, vref) < 0) {
				ret = -ENODEV;
				goto unlock;
			}
			regulator_enable(config->vref_config);
			data->vref_flags |= INT_VREF;
		}
		break;
	case ADC_REF_EXTERNAL0:
		data->channel_mem_ctl[ch] |= (ADC_MSPM0_REF_EXTERNAL <<
					      ADC_MSPM0_CHANNEL_CFG_REF_SHIFT);

		if ((data->vref_flags & INT_VREF) || !(config->vref_config)) {
			ret = -EINVAL;
			goto unlock;
		}

		if (!(data->vref_flags & EXT_VREF)) {
			regulator_disable(config->vref_config);
			data->vref_flags |= EXT_VREF;
		}
		break;
#endif
	default:
		ret = -EINVAL;
		goto unlock;
	}

unlock:
	adc_context_release(&data->ctx, 0);
	return ret;
}

static int adc_mspm0_configSequence(const struct device *dev, const struct adc_sequence *seq)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	uint32_t channels = seq->channels;
	uint32_t resolution;
	uint32_t avg_enabled = 0;
	uint32_t avg_acc = 0;
	uint32_t avg_div = 0;
	uint32_t vrsel;
	uint32_t stime;
	uint8_t mem_ctl_count = 0;
	uint8_t ch;
	uint8_t ref_index;

	switch (seq->resolution) {
	case ADC_MSPM0_RESOLUTION_14:
	case ADC_MSPM0_RESOLUTION_12:
		resolution = DL_ADC12_SAMP_CONV_RES_12_BIT;
		break;
	case ADC_MSPM0_RESOLUTION_10:
		resolution = DL_ADC12_SAMP_CONV_RES_10_BIT;
		break;
	case ADC_MSPM0_RESOLUTION_8:
		resolution = DL_ADC12_SAMP_CONV_RES_8_BIT;
		break;
	default:
		return -EINVAL;
	}

	switch (seq->oversampling) {
	case 0:
		avg_enabled = DL_ADC12_AVERAGING_MODE_DISABLED;
		break;
	case 1:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_2;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_2;
		break;
	case 2:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_4;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_4;
		break;
	case 3:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_8;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_8;
		break;
	case 4:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_16;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_16;
		break;
	case 5:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_32;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_32;
		break;
	case 6:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_64;
		avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_64;
		break;
	case 7:
		avg_enabled = DL_ADC12_AVERAGING_MODE_ENABLED;
		avg_acc = DL_ADC12_HW_AVG_NUM_ACC_128;
		if (seq->resolution == 14) {
			avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_32;
		} else {
			avg_div = DL_ADC12_HW_AVG_DEN_DIV_BY_128;
		}
		break;
	default:
		return -EINVAL;
	}

	while (channels) {
		ch = find_lsb_set(channels) - 1;
		if (ch >= ADC_MSPM0_CHANNEL_MAX) {
			return -EINVAL;
		}

		if (data->channel_mem_ctl[ch] == ADC_MSPM0_CHANNEL_NO_INIT) {
			return -EINVAL;
		}

		ref_index = (data->channel_mem_ctl[ch] >> ADC_MSPM0_CHANNEL_CFG_REF_SHIFT) &
			     ADC_MSPM0_REF_MASK_AFTER_SHIFT;

		switch (ref_index) {
		case ADC_MSPM0_REF_VDD:
			vrsel = DL_ADC12_REFERENCE_VOLTAGE_VDDA;
			break;
#ifdef CONFIG_REGULATOR_MSPM0_VREF
		case ADC_MSPM0_REF_INTERNAL:
			vrsel = DL_ADC12_REFERENCE_VOLTAGE_INTREF;
			break;
		case ADC_MSPM0_REF_EXTERNAL:
			vrsel = DL_ADC12_REFERENCE_VOLTAGE_EXTREF;
			break;
#endif
		default:
			LOG_ERR("Invalid reference index for channel %d", ch);
			return -EINVAL;
		}

		stime = (data->channel_mem_ctl[ch] & ADC_MSPM0_CHANNEL_CFG_STIME_MASK)
			? DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP1
			: DL_ADC12_SAMPLE_TIMER_SOURCE_SCOMP0;

		if (mem_ctl_count < config->max_result) {
			DL_ADC12_configConversionMem(config->base, mem_ctl_count,
						     ch, vrsel, stime,
						     avg_enabled, DL_ADC12_BURN_OUT_SOURCE_DISABLED,
						     DL_ADC12_TRIGGER_MODE_AUTO_NEXT,
						     DL_ADC12_WINDOWS_COMP_MODE_DISABLED);
		} else {
			return -EINVAL;
		}

		mem_ctl_count++;
		channels &= ~BIT(ch);
	}

	if (avg_enabled == DL_ADC12_AVERAGING_MODE_ENABLED) {
		DL_ADC12_configHwAverage(config->base, avg_acc, avg_div);
	} else {
		DL_ADC12_configHwAverage(config->base, DL_ADC12_HW_AVG_NUM_ACC_DISABLED,
					 DL_ADC12_HW_AVG_DEN_DIV_BY_1);
	}

	if (mem_ctl_count - 1 != data->channel_eoc) {
		return -EINVAL;
	}

	DL_ADC12_initSeqSample(config->base, DL_ADC12_REPEAT_MODE_ENABLED,
			       DL_ADC12_SAMPLING_SOURCE_AUTO, DL_ADC12_TRIG_SRC_SOFTWARE,
			       DL_ADC12_SEQ_START_ADDR_00,
			       (data->channel_eoc) << ADC12_CTL2_ENDADD_OFS, resolution,
			       DL_ADC12_SAMP_CONV_DATA_FORMAT_UNSIGNED);

	return 0;
}

static int adc_mspm0_read_internal(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	int sequence_ret;
	uint8_t ch_count = POPCOUNT(sequence->channels);
	size_t exp_size;

	if ((sequence->resolution != ADC_MSPM0_RESOLUTION_14) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_12) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_10) &&
	    (sequence->resolution != ADC_MSPM0_RESOLUTION_8)) {
		return -EINVAL;
	}

	if (ch_count == 0 || ch_count > config->max_result) {
		return -EINVAL;
	}

	if (sequence->resolution == ADC_MSPM0_RESOLUTION_14 &&
	    sequence->oversampling != ADC_MSPM0_MAX_OVERSAMPLING) {
		return -EINVAL;
	}

	if (sequence->oversampling > ADC_MSPM0_MAX_OVERSAMPLING) {
		return -EINVAL;
	}

	if (sequence->calibrate) {
		return -ENOTSUP;
	}

	exp_size = ch_count * sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		return -ENOMEM;
	}

	data->channel_eoc = ch_count - 1;
	data->buffer = sequence->buffer;
	data->repeat_buffer = data->buffer;

	sequence_ret = adc_mspm0_configSequence(dev, sequence);
	if (sequence_ret < 0) {
		return sequence_ret;
	}

	adc_context_start_read(&data->ctx, sequence);
	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_mspm0_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_mspm0_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_mspm0_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_mspm0_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_mspm0_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_mspm0_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int adc_mspm0_init(const struct device *dev)
{
	struct adc_mspm0_data *data = dev->data;
	const struct adc_mspm0_cfg *config = dev->config;
	int ret;

	data->dev = dev;

	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	DL_ADC12_enablePower(config->base);
	delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	DL_ADC12_setClockConfig(config->base, &config->clock_config);

	if (config->auto_pwdn) {
		DL_ADC12_setPowerDownMode(config->base, DL_ADC12_POWER_DOWN_MODE_AUTO);
	} else {
		DL_ADC12_setPowerDownMode(config->base, DL_ADC12_POWER_DOWN_MODE_MANUAL);
	}

	data->sample_time[ADC_MSPM0_SCOMP0] = UINT16_MAX;
	data->sample_time[ADC_MSPM0_SCOMP1] = UINT16_MAX;

	for (int i = 0; i < ADC_MSPM0_CHANNEL_MAX; i++) {
		data->channel_mem_ctl[i] = ADC_MSPM0_CHANNEL_NO_INIT;
	}
	config->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

#define ADC_CLOCK_DIV(x)    DT_INST_PROP(x, ti_clk_divider)
#define ADC_DT_CLOCK_DIV(x) _CONCAT(DL_ADC12_CLOCK_DIVIDE_, ADC_CLOCK_DIV(x))

#define ADC_DT_CLOCK_RANGE(x) DT_INST_PROP(x, ti_clk_range)

#define MSPM0_ADC_INIT(index)                                                                      \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
                                                                                                   \
	static void adc_mspm0_cfg_func_##index(void);                                              \
	static const struct mspm0_sys_clock mspm0_adc_sys_clock##index =                           \
						MSPM0_CLOCK_SUBSYS_FN(index);			   \
                                                                                                   \
	static const struct adc_mspm0_cfg adc_mspm0_cfg_##index = {                                \
		.base = (ADC12_Regs *)DT_INST_REG_ADDR(index),                                     \
		.irq_cfg_func = adc_mspm0_cfg_func_##index,                                        \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                                  \
		.clock_subsys = &mspm0_adc_sys_clock##index,					   \
		.divider = ADC_CLOCK_DIV(index),						   \
		.clock_config = {.clockSel =							   \
				     MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(index, clk)), \
				 .freqRange = ADC_DT_CLOCK_RANGE(index),                           \
				 .divideRatio = ADC_DT_CLOCK_DIV(index)},                          \
		.max_result = DT_INST_PROP(index, max_result_reg),                                 \
		.auto_pwdn = DT_INST_PROP_OR(index, auto_powerdown, false),                        \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(index, vref),					   \
		(.vref_config = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(index), vref)),),		   \
		(.vref_config = NULL)) };							   \
	static DEVICE_API(adc, mspm0_driver_api##index) = {                                        \
		.channel_setup = adc_mspm0_channel_setup,                                          \
		.read = adc_mspm0_read,                                                            \
		.ref_internal = DT_INST_PROP(index, vref_mv),                                      \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_mspm0_read_async,))};		   \
	static struct adc_mspm0_data adc_mspm0_data_##index = {                                    \
		ADC_CONTEXT_INIT_TIMER(adc_mspm0_data_##index, ctx),                               \
		ADC_CONTEXT_INIT_LOCK(adc_mspm0_data_##index, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_mspm0_data_##index, ctx),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(index, &adc_mspm0_init, NULL, &adc_mspm0_data_##index,               \
			      &adc_mspm0_cfg_##index, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,       \
			      &mspm0_driver_api##index);                                           \
                                                                                                   \
	static void adc_mspm0_cfg_func_##index(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), adc_mspm0_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}
DT_INST_FOREACH_STATUS_OKAY(MSPM0_ADC_INIT)
