/*
 * Copyright (c) 2026 Gail Rojas <gailroco@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_msp432p4xx_adc

#include <errno.h>

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_msp432p4xx);

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <soc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* Maximum channel index supported by the ADC14 hardware */
#define MSP432_ADC_CHAN_MAX    32U

/* Sentinel stored in channel_vrsel[] for unconfigured channels */
#define MSP432_ADC_CH_UNINIT  0xFFU

/*
 * SHT fields in CTL0: value 7 selects 192 ADCCLK cycles.
 * At 5 MHz ADCOSC this equals 38.4 us, satisfying the 30 us minimum
 * required by the temperature sensor (TRM Table 6-5).
 */
#define MSP432_ADC_SHT_192 \
	(ADC14_CTL0_SHT0_7 | ADC14_CTL0_SHT1_7)

struct adc_msp432p4xx_data {
	struct adc_context ctx;
	const struct device *dev;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint32_t pending_mask;
	/* 0 = AVCC reference, 1 = internal 1.2 V reference */
	uint8_t  channel_vrsel[MSP432_ADC_CHAN_MAX];
};

struct adc_msp432p4xx_cfg {
	ADC14_Type *base;
	void (*irq_cfg_func)(void);
};

static void start_next_channel(const struct device *dev)
{
	struct adc_msp432p4xx_data *data = dev->data;
	const struct adc_msp432p4xx_cfg *cfg = dev->config;
	ADC14_Type *adc = cfg->base;
	uint8_t ch = (uint8_t)(find_lsb_set(data->pending_mask) - 1U);
	uint32_t vrsel = (data->channel_vrsel[ch] == 1U)
			 ? ADC14_MCTLN_VRSEL_1
			 : ADC14_MCTLN_VRSEL_0;

	/*
	 * MCTL[0] is read-only while ENC=1. In CONSEQ=0 mode ENC should
	 * auto-clear after the conversion, but on MSP432 a debug reset
	 * (SYSRESETREQ via OpenOCD) does not reset ADC14 registers. If a
	 * prior firmware left ENC=1, our init write to CTL0 is silently
	 * ignored and ENC persists across the reset. Clearing ENC here
	 * before every MCTL write is safe: any in-progress conversion has
	 * already completed by the time this function is called (either
	 * from adc_context_start_sampling at the start of a sequence, or
	 * from the ISR after the previous channel's IFG fires).
	 */
	adc->CTL0 &= ~ADC14_CTL0_ENC;
	adc->MCTL[0] = vrsel | (uint32_t)ch;
	adc->CLRIFGR0 = BIT(0);
	adc->IER0 |= BIT(0);
	adc->CTL0 |= ADC14_CTL0_ENC | ADC14_CTL0_SC;
}

static void adc_msp432p4xx_isr(const struct device *dev)
{
	struct adc_msp432p4xx_data *data = dev->data;
	const struct adc_msp432p4xx_cfg *cfg = dev->config;
	ADC14_Type *adc = cfg->base;
	uint8_t ch;

	adc->IER0 &= ~BIT(0);
	adc->CLRIFGR0 = BIT(0);

	ch = (uint8_t)(find_lsb_set(data->pending_mask) - 1U);
	*data->buffer++ = (uint16_t)(adc->MEM[0] & 0x3FFFU);
	data->pending_mask &= ~BIT(ch);

	if (data->pending_mask != 0U) {
		start_next_channel(dev);
		return;
	}

	adc_context_on_sampling_done(&data->ctx, dev);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_msp432p4xx_data *data =
		CONTAINER_OF(ctx, struct adc_msp432p4xx_data, ctx);

	data->pending_mask = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
	start_next_channel(data->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_msp432p4xx_data *data =
		CONTAINER_OF(ctx, struct adc_msp432p4xx_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	}
}

static int adc_msp432p4xx_channel_setup(
	const struct device *dev,
	const struct adc_channel_cfg *cfg)
{
	struct adc_msp432p4xx_data *data = dev->data;
	uint8_t ch = cfg->channel_id;

	if (ch >= MSP432_ADC_CHAN_MAX) {
		LOG_ERR("channel %u out of range (max %u)",
			ch, MSP432_ADC_CHAN_MAX - 1U);
		return -EINVAL;
	}
	if (cfg->differential) {
		LOG_ERR("differential mode not supported");
		return -ENOTSUP;
	}
	if (cfg->gain != ADC_GAIN_1) {
		LOG_ERR("only ADC_GAIN_1 is supported");
		return -ENOTSUP;
	}
	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("only ADC_ACQ_TIME_DEFAULT is supported");
		return -ENOTSUP;
	}

	switch (cfg->reference) {
	case ADC_REF_VDD_1:
		data->channel_vrsel[ch] = 0U;
		break;

	case ADC_REF_INTERNAL:
		/*
		 * Enable REF_A at 1.2 V. Required for the temperature
		 * sensor on A22/ch22 (output is 0.68-0.85 V at typical
		 * temps, safely within the 1.2 V range).
		 *
		 * Also clear TCOFF (bit 3, "temperature sensor disabled").
		 * A debug reset does not clear REF_A registers; a prior
		 * firmware that called REF_A_disableTempSensor() would leave
		 * TCOFF=1, disconnecting the sensor from the ADC mux even
		 * when TCMAP=1 in ADC14_CTL1.
		 */
		REF_A->CTL0 = (uint16_t)((REF_A->CTL0
				& (uint16_t)~(REF_A_CTL0_VSEL_MASK |
					      REF_A_CTL0_TCOFF))
				| REF_A_CTL0_VSEL_0
				| REF_A_CTL0_ON);
		/*
		 * TRM worst-case REF_A startup is 75 us.  Poll for up to
		 * 200 us so a hardware fault returns -ETIMEDOUT rather than
		 * hanging the caller's thread indefinitely.
		 */
		if (!WAIT_FOR((REF_A->CTL0 & REF_A_CTL0_GENRDY) != 0U,
			      200U, k_busy_wait(1U))) {
			return -ETIMEDOUT;
		}
		data->channel_vrsel[ch] = 1U;
		break;

	default:
		LOG_ERR("reference %d not supported", cfg->reference);
		return -ENOTSUP;
	}

	return 0;
}

static int adc_msp432p4xx_start_read(const struct device *dev,
				     const struct adc_sequence *seq)
{
	struct adc_msp432p4xx_data *data = dev->data;
	uint32_t ch_count = POPCOUNT(seq->channels);
	uint32_t mask;
	size_t required;

	if (seq->channels == 0U || seq->buffer == NULL) {
		return -EINVAL;
	}
	if (seq->resolution != 14U) {
		LOG_ERR("only 14-bit resolution supported");
		return -ENOTSUP;
	}
	if (seq->oversampling != 0U) {
		LOG_ERR("oversampling not supported");
		return -ENOTSUP;
	}

	mask = seq->channels;
	while (mask != 0U) {
		uint8_t ch = (uint8_t)(find_lsb_set(mask) - 1U);

		if (data->channel_vrsel[ch] == MSP432_ADC_CH_UNINIT) {
			LOG_ERR("channel %u not configured", ch);
			return -EINVAL;
		}
		mask &= ~BIT(ch);
	}

	required = ch_count * sizeof(uint16_t);
	if (seq->options != NULL) {
		required *= (1U + seq->options->extra_samplings);
	}
	if (seq->buffer_size < required) {
		return -ENOMEM;
	}

	data->buffer = seq->buffer;
	adc_context_start_read(&data->ctx, seq);
	return adc_context_wait_for_completion(&data->ctx);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_msp432p4xx_read_async(const struct device *dev,
				     const struct adc_sequence *seq,
				     struct k_poll_signal *async)
{
	struct adc_msp432p4xx_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, true, async);
	ret = adc_msp432p4xx_start_read(dev, seq);
	adc_context_release(&data->ctx, ret);
	return ret;
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_msp432p4xx_read(const struct device *dev,
				const struct adc_sequence *seq)
{
	struct adc_msp432p4xx_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);
	ret = adc_msp432p4xx_start_read(dev, seq);
	adc_context_release(&data->ctx, ret);
	return ret;
}

static int adc_msp432p4xx_init(const struct device *dev)
{
	struct adc_msp432p4xx_data *data = dev->data;
	const struct adc_msp432p4xx_cfg *cfg = dev->config;
	ADC14_Type *adc = cfg->base;

	data->dev = dev;

	for (int i = 0; i < (int)MSP432_ADC_CHAN_MAX; i++) {
		data->channel_vrsel[i] = MSP432_ADC_CH_UNINIT;
	}

	/*
	 * A debug reset (SYSRESETREQ via OpenOCD) does not clear ADC14
	 * registers on MSP432. If a prior firmware left ENC=1, writing
	 * CTL0 or CTL1 while ENC=1 is silently ignored. Clear ENC first
	 * so the subsequent full writes always take effect.
	 */
	adc->CTL0 &= ~ADC14_CTL0_ENC;

	/*
	 * Power on ADC14 before writing CTL1. The TI DriverLib always calls
	 * ADC14_enableModule() (ON=1) before ADC14_initModule() (which sets
	 * TCMAP and BATMAP in CTL1). Reversing the order leaves TCMAP and
	 * BATMAP written while ON=0, which prevents the internal signal mux
	 * from routing the temperature sensor and AVCC/2 to the internal mux.
	 *   SSEL=0  ADCOSC (5 MHz internal oscillator, no external dep)
	 *   SHP=1   sample timer drives SAMPCON
	 *   CONSEQ=0  single-channel, single-conversion
	 *   SHT0/1=7  192-cycle hold time (38.4 us; TRM requires >=30 us
	 *             for the temperature sensor)
	 */
	adc->CTL0 = ADC14_CTL0_ON | ADC14_CTL0_SHP | MSP432_ADC_SHT_192;

	/*
	 * CTL1: 14-bit resolution; TCMAP routes the temperature sensor to
	 * A(MAX-1); BATMAP routes AVCC/2 to A(MAX). On the 24-channel
	 * MSP432P401R (VQFN-80) MAX=23, so the effective channels are A22
	 * and A23 respectively. Written after ON=1 so the internal mux
	 * sees the module as powered up.
	 */
	adc->CTL1 = ADC14_CTL1_RES_3 |
		    ADC14_CTL1_TCMAP  |
		    ADC14_CTL1_BATMAP;

	cfg->irq_cfg_func();

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_msp432p4xx_driver_api) = {
	.channel_setup = adc_msp432p4xx_channel_setup,
	.read          = adc_msp432p4xx_read,
	IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_msp432p4xx_read_async,))
	.ref_internal  = 1200,
};

#define ADC_MSP432P4XX_INIT(n)						\
	static void adc_msp432p4xx_irq_cfg_##n(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    adc_msp432p4xx_isr,				\
			    DEVICE_DT_INST_GET(n), 0);			\
		irq_enable(DT_INST_IRQN(n));				\
	}								\
									\
	static const struct adc_msp432p4xx_cfg				\
		adc_msp432p4xx_cfg_##n = {				\
		.base         = (ADC14_Type *)DT_INST_REG_ADDR(n),	\
		.irq_cfg_func = adc_msp432p4xx_irq_cfg_##n,		\
	};								\
									\
	static struct adc_msp432p4xx_data adc_msp432p4xx_data_##n = {	\
		ADC_CONTEXT_INIT_TIMER(adc_msp432p4xx_data_##n, ctx),	\
		ADC_CONTEXT_INIT_LOCK(adc_msp432p4xx_data_##n, ctx),	\
		ADC_CONTEXT_INIT_SYNC(adc_msp432p4xx_data_##n, ctx),	\
	};								\
									\
	DEVICE_DT_INST_DEFINE(n, adc_msp432p4xx_init, NULL,		\
			      &adc_msp432p4xx_data_##n,			\
			      &adc_msp432p4xx_cfg_##n,			\
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,	\
			      &adc_msp432p4xx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_MSP432P4XX_INIT)
