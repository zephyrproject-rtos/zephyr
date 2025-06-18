/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "rsi_adc.h"
#include "rsi_bod.h"
#include "rsi_ipmu.h"
#include "rsi_system_config.h"
#include "aux_reference_volt_config.h"

LOG_MODULE_REGISTER(adc_silabs_siwx91x, CONFIG_ADC_LOG_LEVEL);

#include "adc_context.h"

struct adc_siwx91x_chan_data {
	uint8_t input_type;
	uint8_t pos_inp_sel;
	uint8_t neg_inp_sel;
	uint8_t channel_init_status;
};

struct adc_siwx91x_config {
	AUX_ADC_DAC_COMP_Type *reg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	int ref_voltage;
	uint32_t sampling_rate;
	void (*irq_configure)(void);
};

struct adc_siwx91x_data {
	const struct device *dev;
	struct adc_context ctx;
	int16_t *buffer;
	int16_t *repeat_buffer;
	uint32_t channels;
	uint8_t current_channel;
	uint8_t channel_en_count;
	struct adc_siwx91x_chan_data *chan_data;
};

int adc_siwx91x_channel_config(const struct device *dev, uint8_t channel)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	float min_sampl_time = 100e-9f;
	uint32_t f_sample_rate = max_sample_rate_achive(min_sampl_time);
	uint16_t total_clk;
	uint16_t on_clk;
	uint16_t min_total_clk;
	int ret;

	total_clk = system_clocks.ulpss_ref_clk / f_sample_rate;

	on_clk = min_sampl_time * system_clocks.ulpss_ref_clk;

	min_total_clk = system_clocks.ulpss_ref_clk / cfg->sampling_rate;

	if (total_clk < min_total_clk) {
		total_clk = min_total_clk;
	}

	if (total_clk == on_clk) {
		total_clk += 1;
	}

	/* MSB 16-bits contain on duration and LSB 16-bits contain total duration */
	on_clk = FIELD_PREP(0xFFFF0000, on_clk) | total_clk;

	ret = clock_control_set_rate(cfg->clock_dev, cfg->clock_subsys, &on_clk);
	if (ret) {
		return ret;
	}

	RSI_ADC_NoiseAvgMode(cfg->reg, ENABLE);

	RSI_ADC_Config(cfg->reg, DYNAMIC_MODE_DI, ADC_STATICMODE_ENABLE, 0, 0, 0);

	RSI_ADC_StaticMode(cfg->reg, data->chan_data[channel].pos_inp_sel,
			   data->chan_data[channel].neg_inp_sel,
			   data->chan_data[channel].input_type);

	RSI_ADC_ChnlIntrUnMask(cfg->reg, 0, ADC_STATICMODE_ENABLE);

	return 0;
}

static void adc_siwx91x_start_channel(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	int ret;

	/* Find the current channel which needs to be sampled */
	data->current_channel = find_lsb_set(data->channels) - 1;

	ret = adc_siwx91x_channel_config(dev, data->current_channel);
	if (ret) {
		data->channels = 0;
		data->current_channel = 0;
		adc_context_complete(&data->ctx, -EINVAL);
		return;
	}

	RSI_ADC_Start(cfg->reg, ADC_STATICMODE_ENABLE);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_siwx91x_data *data = CONTAINER_OF(ctx, struct adc_siwx91x_data, ctx);

	data->channels = ctx->sequence.channels;
	adc_siwx91x_start_channel(data->dev);
}

static int adc_siwx91x_check_buffer_size(const struct adc_sequence *sequence,
					 uint8_t active_channels)
{
	size_t needed_buffer_size = active_channels * sizeof(uint16_t);

	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		return -ENOMEM;
	}

	return 0;
}

static int adc_siwx91x_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_siwx91x_data *data = dev->data;
	uint8_t channel_count;
	uint32_t channels;
	int ret;
	int i;

	if (sequence->channels == 0) {
		return -EINVAL;
	}

	if (sequence->oversampling) {
		return -ENOTSUP;
	}

	/* Find the total channel count */
	channels = sequence->channels;
	channel_count = 0;
	while (channels) {
		i = find_lsb_set(channels) - 1;

		if (i >= data->channel_en_count) {
			return -EINVAL;
		}

		if (!data->chan_data[i].channel_init_status) {
			return -EINVAL;
		}
		channel_count++;
		channels &= ~BIT(i);
	}

	ret = adc_siwx91x_check_buffer_size(sequence, channel_count);
	if (ret < 0) {
		return ret;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	ret = adc_context_wait_for_completion(&data->ctx);

	return ret;
}

static int adc_siwx91x_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_siwx91x_data *data = dev->data;
	int ret;

	if (sequence->resolution != 12) {
		return -ENOTSUP;
	}

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_siwx91x_start_read(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

static int adc_siwx91x_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	struct adc_siwx91x_data *data = dev->data;

	if (channel_cfg->channel_id >= data->channel_en_count) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		return -EINVAL;
	}

	data->chan_data[channel_cfg->channel_id].channel_init_status = false;
	data->chan_data[channel_cfg->channel_id].pos_inp_sel = channel_cfg->input_positive;

	if (channel_cfg->differential) {
		data->chan_data[channel_cfg->channel_id].neg_inp_sel = channel_cfg->input_negative;
		/* Differential input */
		data->chan_data[channel_cfg->channel_id].input_type = 1;
	} else {
		/* Update negative input to default value */
		data->chan_data[channel_cfg->channel_id].neg_inp_sel = 7;
		/* Single ended input */
		data->chan_data[channel_cfg->channel_id].input_type = 0;
	}

	data->chan_data[channel_cfg->channel_id].channel_init_status = true;

	return 0;
}

static int adc_siwx91x_init(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	float chip_volt;
	float ref_voltage = cfg->ref_voltage / 1000.;
	uint32_t total_duration = 4; /* Default clock division factor */
	int ret;

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret) {
		return ret;
	}

	ret = clock_control_set_rate(cfg->clock_dev, cfg->clock_subsys, &total_duration);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	/* Set default analog reference voltage to 2.8 V from 3.2 V chip voltage */
	RSI_AUX_RefVoltageConfig(2.8, 3.2);

	RSI_ADC_Calibration();

	chip_volt = RSI_BOD_SoftTriggerGetBatteryStatus();
	if (chip_volt < 2.4f) {
		RSI_IPMU_HP_LDO_Enable();
	}

	ret = RSI_AUX_RefVoltageConfig(ref_voltage, chip_volt);
	if (ret) {
		return -EIO;
	}

	adc_context_unlock_unconditionally(&data->ctx);

	cfg->irq_configure();

	data->dev = dev;

	return 0;
}

int16_t adc_siwx91x_read_data(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	int adc_temp = cfg->reg->AUXADC_DATA_b.AUXADC_DATA;
	struct adc_siwx91x_data *data = dev->data;
	uint32_t integer_val;
	float frac;
	float gain;

	/* Check for negative values */
	if (adc_temp & BIT(11)) {
		adc_temp = adc_temp & (int16_t)(ADC_MASK_VALUE);
	} else {
		adc_temp = adc_temp | BIT(11);
	}

	if (data->chan_data[data->current_channel].input_type == 1) {
		integer_val = RSI_IPMU_Auxadcgain_DiffEfuse();
	} else {
		integer_val = RSI_IPMU_Auxadcgain_SeEfuse();
	}
	/* Extract the fractional part from the lower 14 bits of the value */
	frac = (float)FIELD_GET(0x3FFF, integer_val);
	/* Convert the extracted integer fraction to a decimal  */
	frac /= 1000;
	if (frac > 1) {
		LOG_ERR("Invalid gain value");
	}

	/* Extract the integer part from the upper bits and add the fraction */
	gain = (float)(integer_val >> 14) + frac;

	if (data->chan_data[data->current_channel].input_type == 1) {
		adc_temp = (int16_t)((adc_temp - (uint16_t)RSI_IPMU_Auxadcoff_DiffEfuse()) * gain);
	} else {
		adc_temp = (int16_t)((adc_temp - (uint16_t)RSI_IPMU_Auxadcoff_SeEfuse()) * gain);
	}

	/* Maximum value for 12-bit ADC */
	if (adc_temp > 4095) {
		adc_temp = 4095;
	}

	if (adc_temp <= 0) {
		adc_temp = 0;
	}

	if (adc_temp >= 2048) {
		adc_temp = adc_temp - 2048;
	} else {
		adc_temp = adc_temp + 2048;
	}

	return adc_temp;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_siwx91x_data *data = CONTAINER_OF(ctx, struct adc_siwx91x_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_siwx91x_isr(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	uint32_t intr_status;
	int16_t sample;

	intr_status = RSI_ADC_ChnlIntrStatus(cfg->reg);
	if (intr_status & ADC_STATIC_MODE_INTR &&
	    cfg->reg->INTR_MASK_REG_b.ADC_STATIC_MODE_DATA_INTR_MASK == 0) {
		RSI_ADC_ChnlIntrMask(cfg->reg, 0, ADC_STATICMODE_ENABLE);

		sample = adc_siwx91x_read_data(dev);

		*data->buffer++ = sample;
		data->channels &= ~BIT(data->current_channel);

		if (data->channels) {
			adc_siwx91x_start_channel(dev);
		} else {
			adc_context_on_sampling_done(&data->ctx, dev);
		}
	} else {
		adc_context_complete(&data->ctx, -EIO);
	}
}

static DEVICE_API(adc, adc_siwx91x_driver_api) = {
	.channel_setup = adc_siwx91x_channel_setup,
	.read = adc_siwx91x_read,
};

#define SIWX91X_ADC_INIT(inst)                                                                     \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static struct adc_siwx91x_chan_data adc_chan_data_##inst[DT_CHILD_NUM(DT_DRV_INST(inst))]; \
                                                                                                   \
	static struct adc_siwx91x_data adc_data_##inst = {                                         \
		ADC_CONTEXT_INIT_TIMER(adc_data_##inst, ctx),                                      \
		ADC_CONTEXT_INIT_LOCK(adc_data_##inst, ctx),                                       \
		ADC_CONTEXT_INIT_SYNC(adc_data_##inst, ctx),                                       \
		.channel_en_count = DT_CHILD_NUM(DT_DRV_INST(inst)),                               \
		.chan_data = adc_chan_data_##inst,                                                 \
	};                                                                                         \
                                                                                                   \
	static void siwx91x_adc_irq_configure_##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ(inst, irq), DT_INST_IRQ(inst, priority), adc_siwx91x_isr,  \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQ(inst, irq));                                                \
	}                                                                                          \
                                                                                                   \
	static const struct adc_siwx91x_config adc_cfg_##inst = {                                  \
		.reg = (AUX_ADC_DAC_COMP_Type *)DT_INST_REG_ADDR(inst),                            \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_PHA(inst, clocks, clkid),          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.irq_configure = siwx91x_adc_irq_configure_##inst,                                 \
		.ref_voltage = DT_INST_PROP(inst, silabs_adc_ref_voltage),                         \
		.sampling_rate = DT_INST_PROP(inst, silabs_adc_sampling_rate),                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, adc_siwx91x_init, NULL, &adc_data_##inst, &adc_cfg_##inst,     \
			      PRE_KERNEL_1, CONFIG_ADC_INIT_PRIORITY, &adc_siwx91x_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SIWX91X_ADC_INIT)
