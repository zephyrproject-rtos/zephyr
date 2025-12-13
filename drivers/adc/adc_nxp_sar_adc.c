/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define DT_DRV_COMPAT nxp_sar_adc

LOG_MODULE_REGISTER(nxp_sar_adc, CONFIG_ADC_LOG_LEVEL);

struct nxp_sar_adc_config {
	ADC_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t precision_count;
	uint8_t standard_count;
	bool overwrite;
	bool auto_clock_off;
#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct nxp_sar_adc_data {
	const struct device *dev;
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t enabled_channels;
#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
	struct k_sem done;
#endif
};

static inline int nxp_sar_adc_logical_to_hw(const struct nxp_sar_adc_config *config, uint8_t logical,
					    uint8_t *group, uint8_t *index)
{
	uint8_t precision_cnt = MIN(config->precision_count, 32U);
	uint8_t standard_cnt = MIN(config->standard_count, 32U);

	if (logical < precision_cnt) {
		*group = 0U;
		*index = logical;
		return 0;
	}

	if (logical < (uint8_t)(precision_cnt + standard_cnt)) {
		*group = 1U;
		*index = (uint8_t)(logical - precision_cnt);
		return 0;
	}

	return -EINVAL;
}

#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
static void nxp_sar_adc_isr(const struct device *dev)
{
	ADC_Type *base = ((const struct nxp_sar_adc_config *)dev->config)->base;
	struct nxp_sar_adc_data *data = dev->data;
	uint32_t status = base->ISR;

	if ((status & ADC_ISR_ECH_MASK) != 0U) {
		base->ISR = ADC_ISR_ECH_MASK;
		base->IMR &= ~ADC_ISR_ECH_MASK;
		k_sem_give(&data->done);
	}
}
#else
/* Polling mode uses an inlined ECH wait loop in adc_context_start_sampling(). */
#endif

/* Build the HW conversion chain by enabling NCMR0 and NCMR1 masks
 * through the sequence channels bitmap.
 */
static int nxp_sar_adc_build_hw_conversion_chain(const struct device *dev, uint32_t logical_mask,
					uint32_t *ncmr0, uint32_t *ncmr1, uint8_t *enabled_count)
{
	const struct nxp_sar_adc_config *config = dev->config;
	uint32_t group0 = 0U;
	uint32_t group1 = 0U;
	uint8_t count = 0U;

	for (uint8_t ch = 0U; ch < 32U; ch++) {
		if ((logical_mask & BIT(ch)) == 0U) {
			continue;
		}

		uint8_t group;
		uint8_t index;
		if (nxp_sar_adc_logical_to_hw(config, ch, &group, &index) != 0) {
			return -EINVAL;
		}

		if (group == 0U) {
			group0 |= BIT(index);
		} else {
			group1 |= BIT(index);
		}
		count++;
	}

	*ncmr0 = group0;
	*ncmr1 = group1;
	*enabled_count = count;

	return 0;
}

static void nxp_sar_adc_start_hw_conversion_chain(const struct nxp_sar_adc_config *config,
			ADC_Type *base, uint32_t ncmr0, uint32_t ncmr1, bool enable_ech_irq)
{
	ARG_UNUSED(config);

	/* One-shot mode. */
	base->MCR &= ~ADC_MCR_MODE_MASK;

	/* Disable per-channel interrupts. */
	base->CIMR0 = 0U;
	base->CIMR1 = 0U;

	/* Clear global flags and data-available flags before starting. */
	base->ISR = ADC_ISR_EOC_MASK | ADC_ISR_ECH_MASK | ADC_ISR_JEOC_MASK | ADC_ISR_JECH_MASK;
	base->CEOCFR0 = UINT32_MAX;
	base->CEOCFR1 = UINT32_MAX;

	/* Program selected channels for this chain. */
	base->NCMR0 = ncmr0;
	base->NCMR1 = ncmr1;

#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
	if (enable_ech_irq) {
		base->IMR |= ADC_ISR_ECH_MASK;
	} else {
		base->IMR &= ~ADC_ISR_ECH_MASK;
	}
#else
	ARG_UNUSED(enable_ech_irq);
#endif

	base->MCR |= ADC_MCR_NSTART_MASK;
}

/* Setup HW channel chain based on sequence->channels bit mask,
 * and then start context sampling.
 *
 * Current does not support DMA branch.
 * For polling branch, start conversion and poll for ECH flag until timeout.
 * For interrupt branch, enable ECH interrupt and wait for completion semaphore.
 *
 * After conversion completes, copy results to user buffer in logical channel order.
 * Finally, call adc_context_on_sampling_done().
 */
static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct nxp_sar_adc_data *data = CONTAINER_OF(ctx, struct nxp_sar_adc_data, ctx);
	const struct adc_sequence *sequence = &ctx->sequence;
	const struct device *dev = data->dev;
	const struct nxp_sar_adc_config *config = dev->config;
	ADC_Type *base = config->base;
	uint8_t enabled_channels = 0U;
	uint32_t ncmr0 = 0U;
	uint32_t ncmr1 = 0U;
	int err;

	/* Keep the start pointer so ADC_ACTION_REPEAT can rewrite the same samples. */
	data->repeat_buffer = data->buffer;

	/* Build HW conversion chain. */
	err = nxp_sar_adc_build_hw_conversion_chain(dev, sequence->channels,
				&ncmr0, &ncmr1, &enabled_channels);
	if (err != 0) {
		adc_context_complete(ctx, err);
		return;
	}

	data->enabled_channels = enabled_channels;

#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
	k_sem_reset(&data->done);
	/* Start conversion chain with ECH interrupt enabled. */
	nxp_sar_adc_start_hw_conversion_chain(config, base, ncmr0, ncmr1, true);

	k_timeout_t timeout = K_FOREVER;
	if (CONFIG_ADC_NXP_SAR_ADC_INTERRUPT_TIMEOUT > 0) {
		timeout = K_USEC(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT_TIMEOUT);
	}

	err = k_sem_take(&data->done, timeout);
	if (err != 0) {
		base->MCR |= ADC_MCR_ABORTCHAIN_MASK;
		base->IMR &= ~ADC_ISR_ECH_MASK;
		base->ISR = ADC_ISR_ECH_MASK;
		adc_context_complete(ctx, -ETIMEDOUT);
		return;
	}
#else
	/* Start conversion chain without ECH interrupt (polling mode). */
	nxp_sar_adc_start_hw_conversion_chain(config, base, ncmr0, ncmr1, false);
	/* Poll until the normal conversion chain completes (ISR[ECH]) or timeout. */
	err = -ETIMEDOUT;
	for (uint32_t i = 0U; i < CONFIG_ADC_NXP_SAR_ADC_POLL_TIMEOUT; i++) {
		if ((base->ISR & ADC_ISR_ECH_MASK) != 0U) {
			err = 0;
			break;
		}
		k_busy_wait(1);
	}
	if (err != 0) {
		base->MCR |= ADC_MCR_ABORTCHAIN_MASK;
		adc_context_complete(ctx, err);
		return;
	}
	/* Clear ECH flag */
	base->ISR = ADC_ISR_ECH_MASK;
#endif

	/* Copy results to the user buffer in logical channel order (ascending channel_id). */
	for (uint8_t ch = 0U; ch < 32U; ch++) {
		if ((sequence->channels & BIT(ch)) == 0U) {
			continue;
		}

		uint8_t group;
		uint8_t index;
		if (nxp_sar_adc_logical_to_hw(config, ch, &group, &index) != 0) {
			adc_context_complete(ctx, -EINVAL);
			return;
		}
		/*
		 * Read conversion result from the register bank selected by hardware group.
		 * - group 0 (precision): PCDR[index]
		 * - group 1 (standard):  ICDR[index]
		 */
		if (group == 0U) {
			uint32_t raw = base->PCDR[index];

			*data->buffer++ = (uint16_t)(raw & ADC_PCDR_CDATA_MASK);
		} else {
			uint32_t raw = base->ICDR[index];

			*data->buffer++ = (uint16_t)(raw & ADC_ICDR_CDATA_MASK);
		}
	}

	adc_context_on_sampling_done(ctx, dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct nxp_sar_adc_data *data = CONTAINER_OF(ctx, struct nxp_sar_adc_data, ctx);

	/* The driver advances data->buffer while copying results.
	 * When repeating a sampling, reset it to the start of the previous sample set.
	 */
	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

/* Configure sequence resolution, buffer, oversampling. Then submit to context. */
static int nxp_sar_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct nxp_sar_adc_data *data = dev->data;

	if ((sequence->channels == 0U) || (sequence->buffer == NULL)) {
		return -EINVAL;
	}

	/* Count how many logical channels are enabled in the mask (0..31). */
	uint8_t enabled_channels_count = 0U;
	uint32_t mask = sequence->channels;

	while (mask != 0U) {
		enabled_channels_count += (uint8_t)(mask & 1U);
		mask >>= 1U;
	}

	if ((size_t)enabled_channels_count * sizeof(uint16_t) > sequence->buffer_size) {
		return -ENOMEM;
	}

	if ((sequence->options != NULL) && (sequence->options->interval_us != 0U)) {
		return -ENOTSUP;
	}

	if (sequence->resolution != 12U && sequence->resolution != 15U) {
		return -ENOTSUP;
	}

#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(hw_average_support)
	ADC_Type *base = ((const struct nxp_sar_adc_config *)dev->config)->base;

	switch (sequence->oversampling) {
	case 0U:
		base->MCR &= ~(ADC_MCR_AVGEN_MASK | ADC_MCR_AVGS_MASK);
		break;
	case 2U:
		base->MCR = ((base->MCR & ~ADC_MCR_AVGS_MASK) | ADC_MCR_AVGEN_MASK);
		break;
	case 3U:
		base->MCR = ((base->MCR & ~ADC_MCR_AVGS_MASK) | (ADC_MCR_AVGEN_MASK |
			      ADC_MCR_AVGS(1U)));
		break;
	case 4U:
		base->MCR = ((base->MCR & ~ADC_MCR_AVGS_MASK) | (ADC_MCR_AVGEN_MASK |
			      ADC_MCR_AVGS(2U)));
		break;
	case 5U:
		base->MCR = ((base->MCR & ~ADC_MCR_AVGS_MASK) | (ADC_MCR_AVGEN_MASK |
			      ADC_MCR_AVGS(3U)));
		break;
	default:
		return -ENOTSUP;
	}
#endif

	adc_context_lock(&data->ctx, false, NULL);
	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	int err = adc_context_wait_for_completion(&data->ctx);

	adc_context_release(&data->ctx, err);

	return err;
}

/* Configure an ADC channel from a struct adc_dt_spec, including
 * acquisition time, reference, gain and differential mode.
 */
static int nxp_sar_adc_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	const struct nxp_sar_adc_config *config = dev->config;
	uint8_t group;
	uint8_t index;

	if (channel_cfg->channel_id >= 32U) {
		LOG_ERR("channel %u out of range", channel_cfg->channel_id);
		return -EINVAL;
	}

	if ((channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) ||
	    (channel_cfg->reference != ADC_REF_VDD_1) ||
	    (channel_cfg->gain != ADC_GAIN_1) ||
	    (channel_cfg->differential)) {
		LOG_ERR("channel %u configuration not supported", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (nxp_sar_adc_logical_to_hw(config, channel_cfg->channel_id, &group, &index) != 0) {
		LOG_ERR("channel %u not mapped", channel_cfg->channel_id);
		return -EINVAL;
	}
	ARG_UNUSED(group);
	ARG_UNUSED(index);

	return 0;
}

static int nxp_sar_adc_init(const struct device *dev)
{
	const struct nxp_sar_adc_config *config = dev->config;
	struct nxp_sar_adc_data *data = dev->data;
	ADC_Type *base = config->base;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	int err = clock_control_on(config->clock_dev, config->clock_subsys);

	if (err != 0) {
		return err;
	}

	base->MCR = ((base->MCR & ~(ADC_MCR_PWDN_MASK | ADC_MCR_OWREN_MASK |
		      ADC_MCR_ACKO_MASK)) | ADC_MCR_OWREN(config->overwrite ? 1U : 0U) |
		      ADC_MCR_ACKO(config->auto_clock_off ? 1U : 0U));

	/* Disable global and all channels' interrupt. */
	base->IMR = 0U;
	base->CIMR0 = 0U;
	base->CIMR1 = 0U;

	/* Disable all channels. */
	base->NCMR0 = 0U;
	base->NCMR1 = 0U;

	/* Clear global interrupt flags. */
	base->ISR = ADC_ISR_EOC_MASK | ADC_ISR_ECH_MASK | ADC_ISR_JEOC_MASK |
		    ADC_ISR_JECH_MASK;

	/* Clear all channels' interrupt flags. */
	base->CEOCFR0 = UINT32_MAX;
	base->CEOCFR1 = UINT32_MAX;

	data->dev = dev;
	adc_context_init(&data->ctx);
	adc_context_unlock_unconditionally(&data->ctx);

#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
	k_sem_init(&data->done, 0, 1);
	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
	}
#endif

	return 0;
}

static const struct adc_driver_api nxp_sar_adc_api = {
	.channel_setup = nxp_sar_adc_channel_setup,
	.read = nxp_sar_adc_read,
};

#if IS_ENABLED(CONFIG_ADC_NXP_SAR_ADC_INTERRUPT)
#define NXP_SAR_ADC_IRQ_CONFIG(inst)								\
	static void nxp_sar_adc_irq_config_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),			\
			nxp_sar_adc_isr, DEVICE_DT_INST_GET(inst), 0);				\
		irq_enable(DT_INST_IRQN(inst));							\
	}

#define NXP_SAR_ADC_IRQ_FUNC(inst) .irq_config_func = nxp_sar_adc_irq_config_##inst,
#else
#define NXP_SAR_ADC_IRQ_CONFIG(inst)
#define NXP_SAR_ADC_IRQ_FUNC(inst)
#endif

#define NXP_SAR_ADC_INIT(inst)									\
	NXP_SAR_ADC_IRQ_CONFIG(inst)								\
												\
	static const struct nxp_sar_adc_config nxp_sar_adc_config_##inst = {			\
		.base = (ADC_Type *)DT_INST_REG_ADDR(inst),					\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),				\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),	\
		.precision_count = DT_INST_PROP(inst, num_precision_channels),			\
		.standard_count = DT_INST_PROP(inst, num_standard_channels),			\
		.overwrite = DT_INST_PROP(inst, overwrite),					\
		.auto_clock_off = DT_INST_PROP(inst, auto_clock_off),				\
		NXP_SAR_ADC_IRQ_FUNC(inst)							\
	};											\
												\
	static struct nxp_sar_adc_data nxp_sar_adc_data_##inst;					\
												\
	DEVICE_DT_INST_DEFINE(inst, nxp_sar_adc_init, NULL, &nxp_sar_adc_data_##inst,		\
			&nxp_sar_adc_config_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,	\
			&nxp_sar_adc_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SAR_ADC_INIT)
