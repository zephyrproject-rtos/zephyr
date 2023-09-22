/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT       ene_kb1200_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ene_kb1200);

#include <zephyr/drivers/adc.h>
#include <soc.h>
#include <errno.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define ADC_MAX_CHAN        ADC_Channel_N
#define ADC_VREF_ANALOG     3300
#define ADC_REG_BASE  ((ADC_T*)(DT_INST_REG_ADDR(0)))

struct adc_kb1200_data {
	struct adc_context ctx;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
};

static const uint16_t adc_gpio_pin[] = 
{
		ADC_ADC0_GPIO_Num, 
		ADC_ADC1_GPIO_Num, 
		ADC_ADC2_GPIO_Num, 
		ADC_ADC3_GPIO_Num,
		ADC_ADC4_GPIO_Num, 
		ADC_ADC5_GPIO_Num, 
		ADC_ADC6_GPIO_Num, 
		ADC_ADC7_GPIO_Num,
		ADC_ADC8_GPIO_Num,
		ADC_ADC9_GPIO_Num,
		ADC_ADC10_GPIO_Num,
		ADC_ADC11_GPIO_Num,
};

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_kb1200_data *data = CONTAINER_OF(ctx, struct adc_kb1200_data, ctx);
	ADC_T *adc_regs = ADC_REG_BASE;

	data->repeat_buffer = data->buffer;

	//printk("%s sequence.channels = 0x%04X\n", __func__, ctx->sequence.channels);
	adc_regs->ADCCFG = (adc_regs->ADCCFG & 0xFFFF) | (ctx->sequence.channels << 16);
	adc_regs->ADCCFG |= 1;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat_sampling)
{
	struct adc_kb1200_data *data = CONTAINER_OF(ctx, struct adc_kb1200_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_kb1200_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	//printk("%s channel_cfg->channel_id = 0x%04X\n", __func__, channel_cfg->channel_id);
	if (channel_cfg->channel_id >= ADC_MAX_CHAN) {
		return -EINVAL;
	}
	if (channel_cfg->channel_id < ARRAY_SIZE(adc_gpio_pin)) {
		uint16_t gpio_pin = adc_gpio_pin[channel_cfg->channel_id];
		PINMUX_DEV_T adc_pinmux_dev = gpio_pinmux(gpio_pin);
		gpio_pinmux_set(adc_pinmux_dev.port, adc_pinmux_dev.pin, PINMUX_FUNC_B);
		gpio_pinmux_pullup(adc_pinmux_dev.port, adc_pinmux_dev.pin, 0);
	}

	return 0;
}

static bool adc_kb1200_validate_buffer_size(const struct adc_sequence *sequence)
{
	int chan_count = 0;
	size_t buff_need;
	uint32_t chan_mask;

	for (chan_mask = 0x80; chan_mask != 0; chan_mask >>= 1) {
		if (chan_mask & sequence->channels) {
			chan_count++;
		}
	}

	buff_need = chan_count * sizeof(uint16_t);

	if (sequence->options) {
		buff_need *= 1 + sequence->options->extra_samplings;
	}

	if (buff_need > sequence->buffer_size) {
		return false;
	}

	return true;
}

static void adc_kb1200_isr(const struct device *dev);
static int adc_kb1200_start_read(const struct device *dev,
			      const struct adc_sequence *sequence)
{
	struct adc_kb1200_data *data = dev->data;
        ADC_T *adc_regs = ADC_REG_BASE;
    
	if (sequence->channels & ~BIT_MASK(ADC_MAX_CHAN)) {
		LOG_ERR("Incorrect channels, bitmask 0x%x", sequence->channels);
		return -EINVAL;
	}

	if (sequence->channels == 0UL) {
		LOG_ERR("No channel selected");
		return -EINVAL;
	}

	if (!adc_kb1200_validate_buffer_size(sequence)) {
		LOG_ERR("Incorrect buffer size");
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);

	// Since KB106X has no irq for ADC, force issue isr.
	int count = 0;
	while ((adc_regs->ADCCFG & 0x0080) && (count < 100))
	{
		k_usleep(100);
		count++;
	}
	if (count >= 100)
	{
		printk("ADC busy timeout...\n");
	}
	//else 
	//{
	//	printk("ADC count = %d...\n", count);
	//}
	adc_kb1200_isr(dev);
	adc_regs->ADCCFG &= ~1;

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_kb1200_read(const struct device *dev,
			const struct adc_sequence *sequence)
{
	struct adc_kb1200_data *data = dev->data;
	int error = 0;

	//printk("%s channles = 0x%08X\n", __func__, sequence->channels);
	adc_context_lock(&data->ctx, false, NULL);
	error = adc_kb1200_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

#if defined(CONFIG_ADC_ASYNC)
static int adc_kb1200_read_async(const struct device *dev,
			      const struct adc_sequence *sequence,
			      struct k_poll_signal *async)
{
	struct adc_kb1200_data *data = dev->data;
	int error = 0;

	//printk("%s channles = 0x%08X\n", __func__, sequence->channels);
	adc_context_lock(&data->ctx, true, async);
	error = adc_kb1200_start_read(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}
#endif /* CONFIG_ADC_ASYNC */

static void xec_adc_get_sample(const struct device *dev)
{
    ADC_T *adc_regs = ADC_REG_BASE;
	//volatile uint32_t *adccfg = (uint32_t *)(ADC_REG_BASE+ADCCFG_OFFSET);
	struct adc_kb1200_data *data = dev->data;
	uint32_t idx;
	uint32_t channels = (adc_regs->ADCCFG & 0x03FF0000) >> 16;
	uint32_t bit;

	/*
	 * Using the enabled channel bit set, from
	 * lowest channel number to highest, find out
	 * which channel is enabled and copy the ADC
	 * values from hardware registers to the data
	 * buffer.
	 */
	bit = find_lsb_set(channels);
	while (bit != 0) {
		idx = bit - 1;
		//printk("%s channel %d = %d\n", __func__, idx, value);
		*data->buffer = (uint16_t)(adc_regs->ADCxDATA[idx]);
		data->buffer++;

		channels &= ~BIT(idx);
		bit = find_lsb_set(channels);
	}

	/* Clear the status register */
	adc_regs->ADCCFG &= 0xFFFF;
}

static void adc_kb1200_isr(const struct device *dev)
{
	struct adc_kb1200_data *data = dev->data;

	xec_adc_get_sample(dev);
	adc_context_on_sampling_done(&data->ctx, dev);
	LOG_DBG("ADC ISR triggered.");
}

struct adc_driver_api adc_kb1200_api = {
	.channel_setup = adc_kb1200_channel_setup,
	.read = adc_kb1200_read,
#if defined(CONFIG_ADC_ASYNC)
	.read_async = adc_kb1200_read_async,
#endif
	.ref_internal = ADC_VREF_ANALOG,
};

///* ADC Config Register */
//#define XEC_ADC_CFG_CLK_VAL(clk_time)	(		
//	(clk_time << MCHP_ADC_CFG_CLK_LO_TIME_POS) |	
//	(clk_time << MCHP_ADC_CFG_CLK_HI_TIME_POS))

static int adc_kb1200_init(const struct device *dev)
{
	struct adc_kb1200_data *data = dev->data;
	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static struct adc_kb1200_data adc_kb1200_dev_data = {
	ADC_CONTEXT_INIT_TIMER(adc_kb1200_dev_data, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_kb1200_dev_data, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_kb1200_dev_data, ctx),
};

DEVICE_DT_INST_DEFINE(0, adc_kb1200_init, NULL,
		    &adc_kb1200_dev_data, NULL,
		    PRE_KERNEL_2, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adc_kb1200_api);
