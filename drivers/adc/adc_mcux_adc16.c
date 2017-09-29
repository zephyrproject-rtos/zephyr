/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <adc.h>
#include <soc.h>
#include <fsl_adc16.h>

struct mcux_adc16_config {
	ADC_Type *base;
	void (*irq_config_func)(struct device *dev);
};

struct mcux_adc16_data {
	struct k_sem sync;
	u32_t channel_group;
	u32_t result;
};

static void mcux_adc16_enable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static void mcux_adc16_disable(struct device *dev)
{
	ARG_UNUSED(dev);
}

static int mcux_adc16_read(struct device *dev, struct adc_seq_table *seq_table)
{
	const struct mcux_adc16_config *config = dev->config->config_info;
	struct mcux_adc16_data *data = dev->driver_data;
	ADC_Type *base = config->base;

	struct adc_seq_entry *entry = seq_table->entries;
	adc16_channel_config_t channel_config;
	u32_t channel_group = 0;
	int i;

	channel_config.enableInterruptOnConversionCompleted = true;
#if defined(FSL_FEATURE_ADC16_HAS_DIFF_MODE) && FSL_FEATURE_ADC16_HAS_DIFF_MODE
	channel_config.enableDifferentialConversion = false;
#endif

	for (i = 0; i < seq_table->num_entries; i++) {
		if (entry->buffer_length < sizeof(data->result)) {
			return -EINVAL;
		}

		channel_config.channelNumber = entry->channel_id;
		ADC16_SetChannelConfig(base, channel_group, &channel_config);

		data->channel_group = channel_group;

		k_sem_take(&data->sync, K_FOREVER);

		memcpy(entry->buffer, &data->result, sizeof(data->result));

		entry++;
	}

	return 0;
}

static void mcux_adc16_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_adc16_config *config = dev->config->config_info;
	struct mcux_adc16_data *data = dev->driver_data;
	ADC_Type *base = config->base;
	u32_t channel_group = data->channel_group;

	data->result = ADC16_GetChannelConversionValue(base, channel_group);

	k_sem_give(&data->sync);
}

static int mcux_adc16_init(struct device *dev)
{
	const struct mcux_adc16_config *config = dev->config->config_info;
	struct mcux_adc16_data *data = dev->driver_data;
	ADC_Type *base = config->base;
	adc16_config_t adc_config;

	k_sem_init(&data->sync, 0, UINT_MAX);

	ADC16_GetDefaultConfig(&adc_config);
	ADC16_Init(base, &adc_config);

	ADC16_EnableHardwareTrigger(base, false);
	ADC16_SetHardwareAverage(base, kADC16_HardwareAverageCount4);

	config->irq_config_func(dev);

	return 0;
}

static const struct adc_driver_api mcux_adc16_driver_api = {
	.enable = mcux_adc16_enable,
	.disable = mcux_adc16_disable,
	.read = mcux_adc16_read,
};

#if CONFIG_ADC_0
static void mcux_adc16_config_func_0(struct device *dev);

static const struct mcux_adc16_config mcux_adc16_config_0 = {
	.base = (ADC_Type *)CONFIG_ADC_0_BASE_ADDRESS,
	.irq_config_func = mcux_adc16_config_func_0,
};

static struct mcux_adc16_data mcux_adc16_data_0;

DEVICE_AND_API_INIT(mcux_adc16_0, CONFIG_ADC_0_NAME, &mcux_adc16_init,
		    &mcux_adc16_data_0, &mcux_adc16_config_0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_adc16_driver_api);

static void mcux_adc16_config_func_0(struct device *dev)
{
	IRQ_CONNECT(CONFIG_ADC_0_IRQ, CONFIG_ADC_0_IRQ_PRI,
		    mcux_adc16_isr, DEVICE_GET(mcux_adc16_0), 0);

	irq_enable(CONFIG_ADC_0_IRQ);
}
#endif /* CONFIG_ADC_0 */

#if CONFIG_ADC_1
static void mcux_adc16_config_func_1(struct device *dev);

static const struct mcux_adc16_config mcux_adc16_config_1 = {
	.base = (ADC_Type *)CONFIG_ADC_1_BASE_ADDRESS,
	.irq_config_func = mcux_adc16_config_func_1,
};

static struct mcux_adc16_data mcux_adc16_data_1;

DEVICE_AND_API_INIT(mcux_adc16_1, CONFIG_ADC_1_NAME, &mcux_adc16_init,
		    &mcux_adc16_data_1, &mcux_adc16_config_1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &mcux_adc16_driver_api);

static void mcux_adc16_config_func_1(struct device *dev)
{
	IRQ_CONNECT(CONFIG_ADC_1_IRQ, CONFIG_ADC_1_IRQ_PRI,
		    mcux_adc16_isr, DEVICE_GET(mcux_adc16_1), 0);

	irq_enable(CONFIG_ADC_1_IRQ);
}
#endif /* CONFIG_ADC_1 */
