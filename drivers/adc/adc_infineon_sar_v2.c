/*
 * Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_sar_v2

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_ifx.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sar_infineon_cat1, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <cy_sar2.h>

#define IFX_SAR_RESOLUTION	12
#define IFX_SAR_MIN_FREQ_HZ	2000000
#define IFX_SAR_MAX_FREQ_HZ	26670000
#define IFX_SAR_MIN_SAMP_CLK	1
#define IFX_SAR_MAX_SAMP_CLK	4095

struct ifx_cat1_sar_config {
	PASS_SAR_Type *base;
	const struct device *clk_dev;
	struct ifx_clk_peri clk_info;
	uint32_t frequency;
	void (*irq_configure)(void);
	uint8_t num_channels;
};

struct ifx_cat1_sar_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t configured_channels;
};

static void ifx_cat1_sar_isr(const struct device *dev)
{
	const struct ifx_cat1_sar_config *config = dev->config;
	struct ifx_cat1_sar_data *data = dev->data;
	uint32_t channels = data->ctx.sequence.channels;
	uint32_t last_ch = find_msb_set(channels) - 1;
	uint32_t ch = 0;
	uint32_t status = Cy_SAR2_Channel_GetInterruptStatus(config->base, last_ch);
	Cy_SAR2_Channel_ClearInterrupt(config->base, last_ch, CY_SAR2_INT_GRP_DONE);

	if (!(status & CY_SAR2_INT_GRP_DONE) && (channels == 0)) {
		return;
	}

	while (channels != 0) {
		ch = find_lsb_set(channels) - 1;

		*data->buffer++ = Cy_SAR2_Channel_GetResult(config->base, ch, NULL);
		channels &= ~BIT(ch);
	}

	data->buffer--;

	/* Clear GROUP_END on last channel after group scan completes */
	config->base->CH[last_ch].TR_CTL &= ~PASS_SAR_CH_TR_CTL_GROUP_END_Msk;

	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct ifx_cat1_sar_data *data = CONTAINER_OF(ctx, struct ifx_cat1_sar_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer++;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ifx_cat1_sar_data *data = CONTAINER_OF(ctx, struct ifx_cat1_sar_data, ctx);
	const struct device *dev = data->dev;
	const struct ifx_cat1_sar_config *config = dev->config;
	uint8_t last_ch = find_msb_set(ctx->sequence.channels) - 1;
	uint8_t first_ch = find_lsb_set(ctx->sequence.channels) - 1;

	/* Mark last channel as group end */
	config->base->CH[last_ch].TR_CTL |= PASS_SAR_CH_TR_CTL_GROUP_END_Msk;

	Cy_SAR2_Channel_ClearInterrupt(config->base, last_ch, CY_SAR2_INT_GRP_DONE);
	Cy_SAR2_Channel_SetInterruptMask(config->base, last_ch, CY_SAR2_INT_GRP_DONE);

	/* Software trigger on first channel starts the group scan */
	Cy_SAR2_Channel_SoftwareTrigger(config->base, first_ch);
}

static int ifx_cat1_sar_channel_setup(const struct device *dev,
				      const struct adc_channel_cfg *channel_cfg)
{
	const struct ifx_cat1_sar_config *config = dev->config;
	struct ifx_cat1_sar_data *data = dev->data;
	cy_stc_sar2_channel_config_t cy_ch_cfg = {0};
	cy_en_sar2_status_t status;
	uint16_t sample_time = 0;
	uint8_t channel_id = channel_cfg->channel_id;
	int ret = 0;

	if (channel_id >= config->num_channels) {
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		return -ENOTSUP;
	}

	if (channel_cfg->differential) {
		return -ENOTSUP;
	}

	if (channel_cfg->reference != ADC_REF_VDD_1) {
		return -ENOTSUP;
	}

	cy_ch_cfg.channelHwEnable = true;
	cy_ch_cfg.preenptionType = CY_SAR2_PREEMPTION_FINISH_RESUME;
	cy_ch_cfg.pinAddress = channel_cfg->input_positive;
	cy_ch_cfg.portAddress = CY_SAR2_PORT_ADDRESS_SARMUX0;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		switch (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time)) {
		case ADC_ACQ_TIME_TICKS:
			sample_time = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
			break;
		case ADC_ACQ_TIME_MICROSECONDS:
			sample_time = DIV_ROUND_UP(ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time) *
						   config->frequency, USEC_PER_SEC);
			break;
		case ADC_ACQ_TIME_NANOSECONDS:
			sample_time = DIV_ROUND_UP(
				      (uint64_t)ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time) *
				      config->frequency, NSEC_PER_SEC);
			break;
		default:
			LOG_ERR("Invalid acquisition time unit");
			return -EINVAL;
		}
	} else {
		sample_time = IFX_SAR_MIN_SAMP_CLK;
	}
	sample_time = CLAMP(sample_time, IFX_SAR_MIN_SAMP_CLK, IFX_SAR_MAX_SAMP_CLK);
	cy_ch_cfg.sampleTime = sample_time;
	cy_ch_cfg.averageCount = 1U;
	cy_ch_cfg.rangeDetectionMode = CY_SAR2_RANGE_DETECTION_MODE_BELOW_LO;
	cy_ch_cfg.rangeDetectionLoThreshold = 0U;
	cy_ch_cfg.rangeDetectionHiThreshold = 0xFFFU;

	adc_context_lock(&data->ctx, false, NULL);
	status = Cy_SAR2_Channel_Init(config->base, channel_id, &cy_ch_cfg);
	if (status != CY_SAR2_SUCCESS) {
		ret = -EIO;
	} else {
		data->configured_channels |= BIT(channel_id);
	}
	adc_context_release(&data->ctx, 0);

	return ret;
}

static int ifx_cat1_sar_validate_sequence(const struct device *dev,
					  const struct adc_sequence *sequence)
{
	struct ifx_cat1_sar_data *data = dev->data;
	size_t exp_size;
	uint8_t count = POPCOUNT(sequence->channels);

	if (sequence->resolution != IFX_SAR_RESOLUTION) {
		LOG_ERR("Unsupported resolution %u", sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling != 0) {
		LOG_ERR("Oversampling not supported");
		return -ENOTSUP;
	}

	if (count == 0) {
		LOG_ERR("No channels selected");
		return -EINVAL;
	}

	if ((sequence->channels & ~data->configured_channels) != 0U) {
		LOG_ERR("Configured channels are not matched with requested channels");
		return -EINVAL;
	}

	exp_size = count * sizeof(uint16_t);
	if (sequence->options) {
		exp_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < exp_size) {
		LOG_ERR("Buffer too small: need %zu, have %zu",
			exp_size, sequence->buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int ifx_cat1_sar_read_internal(const struct device *dev,
				      const struct adc_sequence *sequence)
{
	struct ifx_cat1_sar_data *data = dev->data;
	int ret;

	ret = ifx_cat1_sar_validate_sequence(dev, sequence);
	if (ret < 0) {
		return ret;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = data->buffer;
	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int ifx_cat1_sar_read(const struct device *dev,
			     const struct adc_sequence *sequence)
{
	struct ifx_cat1_sar_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = ifx_cat1_sar_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int ifx_cat1_sar_read_async(const struct device *dev,
				   const struct adc_sequence *sequence,
				   struct k_poll_signal *async)
{
	struct ifx_cat1_sar_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = ifx_cat1_sar_read_internal(dev, sequence);
	adc_context_release(&data->ctx, ret);

	return ret;
}
#endif

static int ifx_cat1_sar_init(const struct device *dev)
{
	const struct ifx_cat1_sar_config *config = dev->config;
	struct ifx_cat1_sar_data *data = dev->data;
	int ret;
	const cy_stc_sar2_config_t sar_cfg = {
		.preconditionTime = 0U,
		.powerupTime = 0U,
		.enableIdlePowerDown = false,
		.msbStretchMode = CY_SAR2_MSB_STRETCH_MODE_2CYCLE,
		.enableHalfLsbConv = true,
		.sarMuxEnable = true,
		.adcEnable = true,
		.sarIpEnable = true,
		.channelConfig = {NULL}
	};

	data->dev = dev;

	if (config->frequency < IFX_SAR_MIN_FREQ_HZ ||
	    config->frequency > IFX_SAR_MAX_FREQ_HZ) {
		LOG_ERR("SAR frequency %u out of range", config->frequency);
		return -EINVAL;
	}

	ret = clock_control_set_rate(config->clk_dev,
				    (void *)&config->clk_info,
				    (void *)&config->frequency);
	if (ret < 0) {
		return ret;
	}

        if (Cy_SAR2_Init(config->base, &sar_cfg) != CY_SAR2_SUCCESS) {
                return -EIO;
        }

	config->irq_configure();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, ifx_cat1_driver_api) = {
	.channel_setup = ifx_cat1_sar_channel_setup,
	.read = ifx_cat1_sar_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ifx_cat1_sar_read_async,
#endif
};

#define ADC_PERI_CLOCK_INIT(n)								\
	.clk_info = {									\
		.rootclk_id = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, rootclk_id),		\
		.divider_type = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, divider_type), 	\
		.divider_inst = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, divider_inst), 	\
	},

#define IRQ_CONFIGURE(n, inst)								\
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, n),					\
		    DT_INST_IRQ_BY_IDX(inst, n, priority),				\
		    ifx_cat1_sar_isr, DEVICE_DT_INST_GET(inst), 0);			\
	irq_enable(DT_INST_IRQN_BY_IDX(inst, n));

#define IFX_SAR_ADC_INIT(n)								\
											\
	static void ifx_cat1_adc_irq_configure##n(void);				\
											\
	static struct ifx_cat1_sar_data ifx_cat1_sar_data_##n = {			\
		ADC_CONTEXT_INIT_TIMER(ifx_cat1_sar_data_##n, ctx),			\
		ADC_CONTEXT_INIT_LOCK(ifx_cat1_sar_data_##n, ctx),			\
		ADC_CONTEXT_INIT_SYNC(ifx_cat1_sar_data_##n, ctx),			\
	};										\
											\
	static const struct ifx_cat1_sar_config						\
		ifx_cat1_sar_cfg_##n = {						\
		.base = (PASS_SAR_Type *)DT_INST_REG_ADDR(n),				\
		.irq_configure = ifx_cat1_adc_irq_configure##n,				\
		.frequency = DT_INST_PROP_OR(n, clock_frequency, SAR_MAX_FREQ_HZ),	\
		.num_channels = DT_INST_PROP(n, total_channels),			\
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),			\
		ADC_PERI_CLOCK_INIT(n)							\
	};										\
											\
	static void ifx_cat1_adc_irq_configure##n(void)					\
	{										\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), IRQ_CONFIGURE, (), n)		\
	}										\
											\
	DEVICE_DT_INST_DEFINE(n, &ifx_cat1_sar_init, NULL,				\
			      &ifx_cat1_sar_data_##n,					\
			      &ifx_cat1_sar_cfg_##n,					\
			      POST_KERNEL,						\
			      CONFIG_ADC_INIT_PRIORITY,					\
			      &ifx_cat1_driver_api);

DT_INST_FOREACH_STATUS_OKAY(IFX_SAR_ADC_INIT)
