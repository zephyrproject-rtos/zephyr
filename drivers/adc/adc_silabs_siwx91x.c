/*
 * Copyright (c) 2025 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_siwx91x_adc

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include "rsi_adc.h"
#include "rsi_bod.h"
#include "rsi_ipmu.h"
#include "rsi_system_config.h"
#include "aux_reference_volt_config.h"

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define SIWX91X_CHANNEL_COUNT        16
#define ADC_MAX_OP_VALUE             4095
#define SIGN_BIT                     BIT(11)
#define ADC_OPERATION_MODE           ADC_STATICMODE_ENABLE
#define ADC_SINGLE_ENDED_IN          0
#define ADC_DIFFERENTIAL_IN          1
#define MAX_IP_VOLT_SCDC             2.4f
#define DEFAULT_NEGATIVE_CHANNEL_POS 7
#define DEFAULT_VREF_VALUE           2.8f
#define CHIP_VOLTAGE_VALUE           3.2f
#define ADC_DEFAULT_CLK_DIV_FACTOR   4

struct adc_siwx91x_ch_config {
	uint8_t *input_type;
	uint8_t *pos_inp_sel;
	uint8_t *neg_inp_sel;
};

struct adc_siwx91x_common_config {
	uint32_t adc_clk_src;
	uint16_t adc_sing_offset;
	uint16_t adc_diff_offset;
	float adc_sing_gain;
	float adc_diff_gain;
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
	struct adc_siwx91x_common_config *adc_common_config;
	struct adc_siwx91x_ch_config *adc_channel_config;
	uint8_t *channel_init_status;
};

static inline float adc_siwx91x_ceilf(float x)
{
	int val_int = (int)x;

	return (x > (float)val_int) ? (float)(val_int + 1) : (float)val_int;
}

void adc_siwx91x_channel_config(const struct device *dev, uint8_t channel)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	float min_sampl_time = 100.0f / 1000000000.0f;
	uint32_t f_sample_rate_achive = max_sample_rate_achive(min_sampl_time);
	uint16_t total_clk;
	uint16_t on_clk;
	uint16_t min_total_clk;
	uint32_t rate = 0;

	total_clk = (uint16_t)adc_siwx91x_ceilf((1.0f / (float)f_sample_rate_achive) /
						(1.0f /
						(float)data->adc_common_config->adc_clk_src));

	on_clk = (uint16_t)adc_siwx91x_ceilf(min_sampl_time / (1.0f /
					     (float)data->adc_common_config->adc_clk_src));

	min_total_clk = (uint16_t)adc_siwx91x_ceilf(data->adc_common_config->adc_clk_src /
						    cfg->sampling_rate);

	/* Ensure total_clk and on_clk are not equal */
	if (total_clk == on_clk) {
		total_clk += 1;
	}

	/* Ensure total_clk meets minimum requirement based on sampling rate */
	if (total_clk < min_total_clk) {
		total_clk = min_total_clk;
	}

	/* MSB 16-bits contain on duration and LSB 16-bits contain total duration */
	rate = on_clk;
	rate = (rate << 16) | total_clk;

	clock_control_set_rate(cfg->clock_dev, cfg->clock_subsys, &rate);

	RSI_ADC_NoiseAvgMode(cfg->reg, ENABLE);

	RSI_ADC_Config(cfg->reg, DYNAMIC_MODE_DI, ADC_OPERATION_MODE, 0, 0, 0);

	RSI_ADC_StaticMode(cfg->reg, data->adc_channel_config->pos_inp_sel[channel],
			   data->adc_channel_config->neg_inp_sel[channel],
			   data->adc_channel_config->input_type[channel]);

	RSI_ADC_ChnlIntrUnMask(cfg->reg, 0, ADC_OPERATION_MODE);
}

static void adc_siwx91x_start_channel(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;

	/* Find the current channel which needs to be sampled */
	data->current_channel = find_lsb_set(data->channels) - 1;
	adc_siwx91x_channel_config(dev, data->current_channel);

	RSI_ADC_Start(cfg->reg, ADC_OPERATION_MODE);
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

		if (i >= SIWX91X_CHANNEL_COUNT) {
			return -EINVAL;
		}

		if (!data->channel_init_status[i]) {
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

	if (channel_cfg->channel_id >= SIWX91X_CHANNEL_COUNT) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		return -EINVAL;
	}

	data->channel_init_status[channel_cfg->channel_id] = false;
	data->adc_channel_config->pos_inp_sel[channel_cfg->channel_id] =
		channel_cfg->input_positive;

	if (channel_cfg->differential) {
		data->adc_channel_config->neg_inp_sel[channel_cfg->channel_id] =
			channel_cfg->input_negative;
		data->adc_channel_config->input_type[channel_cfg->channel_id] = ADC_DIFFERENTIAL_IN;
	} else {
		data->adc_channel_config->neg_inp_sel[channel_cfg->channel_id] =
			DEFAULT_NEGATIVE_CHANNEL_POS;
		data->adc_channel_config->input_type[channel_cfg->channel_id] = ADC_SINGLE_ENDED_IN;
	}

	data->channel_init_status[channel_cfg->channel_id] = true;

	return 0;
}

void adc_siwx91x_peripheral_init(struct adc_siwx91x_common_config *adc_common_config)
{
	uint32_t integer_val = 0;
	float frac = 0;

	/* Set analog reference voltage */
	RSI_AUX_RefVoltageConfig(DEFAULT_VREF_VALUE, CHIP_VOLTAGE_VALUE);

	RSI_ADC_Calibration();

	adc_common_config->adc_clk_src = system_clocks.ulpss_ref_clk;
	adc_common_config->adc_sing_offset = (uint16_t)RSI_IPMU_Auxadcoff_SeEfuse();
	adc_common_config->adc_diff_offset = (uint16_t)RSI_IPMU_Auxadcoff_DiffEfuse();

	/* Single ended gain */
	integer_val = RSI_IPMU_Auxadcgain_SeEfuse();
	/* Obtain the 14 MSBs */
	frac = (float)(integer_val & 0x3FFF);
	frac /= 1000;
	adc_common_config->adc_sing_gain = ((float)(integer_val >> 14) + frac);

	/* Differential ended gain */
	integer_val = RSI_IPMU_Auxadcgain_DiffEfuse();
	/* Obtain the 14 MSBs */
	frac = (float)(integer_val & 0x3FFF);
	frac /= 1000;
	adc_common_config->adc_diff_gain = ((float)(integer_val >> 14) + frac);
}

static int adc_siwx91x_init(const struct device *dev)
{
	const struct adc_siwx91x_config *cfg = dev->config;
	struct adc_siwx91x_data *data = dev->data;
	float chip_volt = 0;
	float ref_voltage = (float)cfg->ref_voltage / 1000;
	uint32_t total_duration = ADC_DEFAULT_CLK_DIV_FACTOR;
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

	adc_siwx91x_peripheral_init(data->adc_common_config);

	chip_volt = RSI_BOD_SoftTriggerGetBatteryStatus();
	if (chip_volt < MAX_IP_VOLT_SCDC) {
		RSI_IPMU_ProgramConfigData(hp_ldo_voltsel);
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
	struct adc_siwx91x_data *data = dev->data;
	int16_t adc_temp = 0;

	adc_temp = cfg->reg->AUXADC_DATA_b.AUXADC_DATA;

	if (adc_temp & SIGN_BIT) {
		adc_temp = adc_temp & (int16_t)(ADC_MASK_VALUE);
	} else {
		adc_temp = adc_temp | SIGN_BIT;
	}

	if (data->adc_channel_config->input_type[data->current_channel] == ADC_DIFFERENTIAL_IN) {
		adc_temp = (int16_t)((adc_temp - data->adc_common_config->adc_diff_offset) *
				     data->adc_common_config->adc_diff_gain);
	} else {
		adc_temp = (int16_t)((adc_temp - data->adc_common_config->adc_sing_offset) *
				     data->adc_common_config->adc_sing_gain);
	}

	if (adc_temp > ADC_MAX_OP_VALUE) {
		adc_temp = ADC_MAX_OP_VALUE;
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
	static uint8_t input_type_##inst[DT_CHILD_NUM(DT_DRV_INST(inst))];                         \
	static uint8_t pos_inp_sel_##inst[DT_CHILD_NUM(DT_DRV_INST(inst))];                        \
	static uint8_t neg_inp_sel_##inst[DT_CHILD_NUM(DT_DRV_INST(inst))];                        \
	static uint8_t channel_init_status_##inst[DT_CHILD_NUM(DT_DRV_INST(inst))];                \
                                                                                                   \
	static struct adc_siwx91x_ch_config adc_siwx91x_ch_config_##inst = {                       \
		.input_type = input_type_##inst,                                                   \
		.pos_inp_sel = pos_inp_sel_##inst,                                                 \
		.neg_inp_sel = neg_inp_sel_##inst,                                                 \
	};                                                                                         \
                                                                                                   \
	static struct adc_siwx91x_common_config adc_siwx91x_common_config_##inst;                  \
                                                                                                   \
	static struct adc_siwx91x_data adc_data_##inst = {                                         \
		ADC_CONTEXT_INIT_TIMER(adc_data_##inst, ctx),                                      \
		ADC_CONTEXT_INIT_LOCK(adc_data_##inst, ctx),                                       \
		ADC_CONTEXT_INIT_SYNC(adc_data_##inst, ctx),                                       \
		.adc_common_config = &adc_siwx91x_common_config_##inst,                            \
		.adc_channel_config = &adc_siwx91x_ch_config_##inst,                               \
		.channel_init_status = channel_init_status_##inst,                                 \
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
