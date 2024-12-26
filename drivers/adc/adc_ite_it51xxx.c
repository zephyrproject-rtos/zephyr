/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_adc

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adc_ite_it51xxx);

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/pinctrl.h>
#include <soc.h>
#include <soc_dt.h>
#include <errno.h>
#include <assert.h>
#include <zephyr/irq.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ADC internal reference voltage (Unit:mV) */
#ifdef CONFIG_ADC_IT51XXX_VOL_FULL_SCALE
#define IT51XXX_ADC_VREF_VOL 3300
#else
#define IT51XXX_ADC_VREF_VOL 3000
#endif
/* ADC channels disabled */
#define IT51XXX_ADC_CHANNEL_DISABLED    0x1F
/* ADC sample time delay (Unit:us) */
#define IT51XXX_ADC_SAMPLE_TIME_US      500
/* Wait next clock rising (Clock source 32.768K) */
#define IT51XXX_WAIT_NEXT_CLOCK_TIME_US 31

/* ADC channels disabled */
#define IT51XXX_ADC_CHANNEL_DISABLED 0x1F

#ifdef CONFIG_ADC_IT51XXX_VOL_FULL_SCALE
#define ADC_0_7_FULL_SCALE_MASK GENMASK(7, 0)
#endif

/* List of ADC channels. */
enum chip_adc_channel {
	CHIP_ADC_CH0 = 0,
	CHIP_ADC_CH1,
	CHIP_ADC_CH2,
	CHIP_ADC_CH3,
	CHIP_ADC_CH4,
	CHIP_ADC_CH5,
	CHIP_ADC_CH6,
	CHIP_ADC_CH7,
	CHIP_ADC_COUNT,
};

/* (19xxh) Analog to Digital Converter (ADC) registers */
/* 0x00: ADC Status */
#define ADCSTS      0x00
/* 0x01: ADC Configuration */
#define ADCCFG      0x01
/* 0x02: ADC Clock Control */
#define ADCCTL      0x02
/* 0x03: General Control */
#define ADCGCR      0x03
/* 0x04: Voltage Channel 0 Control */
#define VCH0CTL     0x04
/* 0x05: Calibration Data Control */
#define KDCTL       0x05
/* 0x18: Voltage Channel 0 Data Buffer LSB */
#define VCH0DATL    0x18
/* 0x19: Voltage Channel 0 Data Buffer MSB */
#define VCH0DATM    0x19
/* 0x44: ADC Data Valid Status */
#define ADCDVSTS    0x44
/* 0x55: ADC Input Voltage Mapping Full-Scale Code Selection 1 */
#define ADCIVMFSCS1 0x55

/* ADC conversion time select 1 */
#define IT51XXX_ADC_ADCCTS1 BIT(7)
/* Analog accuracy initialization */
#define IT51XXX_ADC_AINITB  BIT(3)
/* ADC conversion time select 0 */
#define IT51XXX_ADC_ADCCTS0 BIT(5)
/* ADC module enable */
#define IT51XXX_ADC_ADCEN   BIT(0)
/* ADC data buffer keep enable */
#define IT51XXX_ADC_DBKEN   BIT(7)
/* W/C data valid flag */
#define IT51XXX_ADC_DATVAL  BIT(7)
/* Data valid interrupt of adc */
#define IT51XXX_ADC_INTDVEN BIT(5)
/* Automatic hardware calibration enable */
#define IT51XXX_ADC_AHCE    BIT(7)

struct adc_it51xxx_data {
	struct adc_context ctx;
	struct k_sem sem;
	/* Channel ID */
	uint32_t ch;
	/* Save ADC result to the buffer. */
	uint16_t *buffer;
	/*
	 * The sample buffer pointer should be prepared
	 * for writing of next sampling results.
	 */
	uint16_t *repeat_buffer;
};

/*
 * Structure adc_it51xxx_cfg is about the setting of adc
 * this config will be used at initial time
 */
struct adc_it51xxx_cfg {
	mm_reg_t base;
	/* ADC alternate configuration */
	const struct pinctrl_dev_config *pcfg;
};

static int adc_it51xxx_channel_setup(const struct device *dev,
				     const struct adc_channel_cfg *channel_cfg)
{
	uint8_t channel_id = channel_cfg->channel_id;

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Selected ADC acquisition time is not valid");
		return -EINVAL;
	}

	/* Support channels 0~7 */
	if (!(channel_id >= 0 && channel_id <= 7)) {
		LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	LOG_DBG("Channel setup succeeded!");
	return 0;
}

static void adc_disable_measurement(const struct device *dev)
{
	const struct adc_it51xxx_cfg *config = dev->config;

	/*
	 * Disable measurement.
	 * bit(4:0) = 0x1f : channel disable
	 */
	sys_write8(IT51XXX_ADC_DATVAL | IT51XXX_ADC_CHANNEL_DISABLED, config->base + VCH0CTL);

	/* ADC module disable */
	sys_write8(sys_read8(config->base + ADCCFG) & ~IT51XXX_ADC_ADCEN, config->base + ADCCFG);

	/* disable adc interrupt */
	irq_disable(DT_INST_IRQN(0));
}

static int adc_data_valid(const struct device *dev)
{
	const struct adc_it51xxx_cfg *config = dev->config;

	return sys_read8(config->base + VCH0CTL) & IT51XXX_ADC_DATVAL;
}

/* Get result for each ADC selected channel. */
static void adc_it51xxx_get_sample(const struct device *dev)
{
	struct adc_it51xxx_data *data = dev->data;
	const struct adc_it51xxx_cfg *config = dev->config;

	if (adc_data_valid(dev)) {
		/* Read adc raw data of msb and lsb */
		*data->buffer++ = sys_read8(config->base + VCH0DATM) << 8 |
				  sys_read8(config->base + VCH0DATL);
	} else {
		LOG_WRN("ADC failed to read (regs=%x, ch=%d)", sys_read8(config->base + ADCDVSTS),
			data->ch);
	}

	adc_disable_measurement(dev);
}

static void adc_poll_valid_data(void)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	int valid = 0;

	/*
	 * If the polling waits for a valid data longer than
	 * the sampling time limit, the program will return.
	 */
	for (int i = 0U; i < (IT51XXX_ADC_SAMPLE_TIME_US / IT51XXX_WAIT_NEXT_CLOCK_TIME_US); i++) {
		/* Wait next clock time (1/32.768K~=30.5us) */
		k_busy_wait(IT51XXX_WAIT_NEXT_CLOCK_TIME_US);

		if (adc_data_valid(dev)) {
			valid = 1;
			break;
		}
	}

	if (valid) {
		adc_it51xxx_get_sample(dev);
	} else {
		LOG_ERR("Sampling timeout.");
		return;
	}
}

static void adc_enable_measurement(uint32_t ch)
{
	const struct device *const dev = DEVICE_DT_INST_GET(0);
	const struct adc_it51xxx_cfg *config = dev->config;
	struct adc_it51xxx_data *data = dev->data;

	/* Select and enable a voltage channel input for measurement */
	sys_write8((IT51XXX_ADC_DATVAL | IT51XXX_ADC_INTDVEN) + ch, config->base + VCH0CTL);

	/* ADC module enable */
	sys_write8(sys_read8(config->base + ADCCFG) | IT51XXX_ADC_ADCEN, config->base + ADCCFG);

	/*
	 * In the sampling process, it is possible to read multiple channels
	 * at a time. The ADC sampling of it51xxx needs to read each channel
	 * in sequence, so it needs to wait for an interrupt to read data in
	 * the loop through k_sem_take(). But k_timer_start() is used in the
	 * interval test in test_adc.c, so we need to use polling wait instead
	 * of k_sem_take() to wait, otherwise it will cause kernel panic.
	 *
	 * k_is_in_isr() can determine whether to use polling or k_sem_take()
	 * at present.
	 */
	if (k_is_in_isr()) {
		/* polling wait for a valid data */
		adc_poll_valid_data();
	} else {
		/* Enable adc interrupt */
		irq_enable(DT_INST_IRQN(0));
		/* Wait for an interrupt to read valid data. */
		k_sem_take(&data->sem, K_FOREVER);
	}
}

static int check_buffer_size(const struct adc_sequence *sequence, uint8_t active_channels)
{
	size_t needed_buffer_size;

	needed_buffer_size = active_channels * sizeof(uint16_t);
	if (sequence->options) {
		needed_buffer_size *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed_buffer_size) {
		LOG_ERR("Provided buffer is too small (%u/%u)", sequence->buffer_size,
			needed_buffer_size);
		return -ENOMEM;
	}

	return 0;
}

static int adc_it51xxx_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_it51xxx_data *data = dev->data;
	uint32_t channel_mask = sequence->channels;

	if (!channel_mask || channel_mask & ~BIT_MASK(CHIP_ADC_COUNT)) {
		LOG_ERR("Invalid selection of channels");
		return -EINVAL;
	}

	if (!sequence->resolution) {
		LOG_ERR("ADC resolution is not valid");
		return -EINVAL;
	}
	LOG_DBG("Configure resolution=%d", sequence->resolution);

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_it51xxx_data *data = CONTAINER_OF(ctx, struct adc_it51xxx_data, ctx);
	uint32_t channels = ctx->sequence.channels;
	uint8_t channel_count = 0;

	data->repeat_buffer = data->buffer;

	/*
	 * The ADC sampling of it51xxx needs to read each channel
	 * in sequence.
	 */
	while (channels) {
		data->ch = find_lsb_set(channels) - 1;
		channels &= ~BIT(data->ch);

		adc_enable_measurement(data->ch);

		channel_count++;
	}

	if (check_buffer_size(&ctx->sequence, channel_count)) {
		return;
	}

	adc_context_on_sampling_done(&data->ctx, DEVICE_DT_INST_GET(0));
}

static int adc_it51xxx_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_it51xxx_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, false, NULL);
	err = adc_it51xxx_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

#ifdef CONFIG_ADC_ASYNC
static int adc_it51xxx_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	struct adc_it51xxx_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, true, async);
	err = adc_it51xxx_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}
#endif /* CONFIG_ADC_ASYNC */

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_it51xxx_data *data = CONTAINER_OF(ctx, struct adc_it51xxx_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_it51xxx_isr(const struct device *dev)
{
	struct adc_it51xxx_data *data = dev->data;

	LOG_DBG("ADC ISR triggered.");

	adc_it51xxx_get_sample(dev);

	k_sem_give(&data->sem);
}

static DEVICE_API(adc, api_it51xxx_driver_api) = {
	.channel_setup = adc_it51xxx_channel_setup,
	.read = adc_it51xxx_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_it51xxx_read_async,
#endif
	.ref_internal = IT51XXX_ADC_VREF_VOL,
};

/*
 * ADC analog accuracy initialization (only once after VSTBY power on)
 *
 * Write 1 to this bit and write 0 to this bit immediately once and
 * only once during the firmware initialization and do not write 1 again
 * after initialization since IT51XXX takes much power consumption
 * if this bit is set as 1
 */
static void adc_accuracy_initialization(const struct device *dev)
{
	const struct adc_it51xxx_cfg *config = dev->config;

	/* Start adc accuracy initialization */
	sys_write8(sys_read8(config->base + ADCSTS) | IT51XXX_ADC_AINITB, config->base + ADCSTS);
	/* Enable automatic HW calibration. */
	sys_write8(sys_read8(config->base + KDCTL) | IT51XXX_ADC_AHCE, config->base + KDCTL);
	/* Stop adc accuracy initialization */
	sys_write8(sys_read8(config->base + ADCSTS) & ~IT51XXX_ADC_AINITB, config->base + ADCSTS);
}

static int adc_it51xxx_init(const struct device *dev)
{
	const struct adc_it51xxx_cfg *config = dev->config;
	struct adc_it51xxx_data *data = dev->data;
	int status;

#ifdef CONFIG_ADC_IT51XXX_VOL_FULL_SCALE
	/* ADC input voltage 0V ~ AVCC (3.3V) is mapped into 0h-3FFh */
	sys_write8(ADC_0_7_FULL_SCALE_MASK, config->base + ADCIVMFSCS1);
#endif
	/* ADC analog accuracy initialization */
	adc_accuracy_initialization(dev);

	/* Set the pin to ADC alternate function. */
	status = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (status < 0) {
		LOG_ERR("Failed to configure ADC pins");
		return status;
	}

	/*
	 * The ADC channel conversion time is 30.8*(SCLKDIV+1) us.
	 * (Current setting is 61.6us)
	 *
	 * NOTE: A sample time delay (60us) also need to be included in
	 * conversion time.
	 * In addition, the ADC has a waiting time of 202.8us for
	 * voltage stabilization.
	 *
	 * So the final ADC sample time result is ~= 324.4us.
	 */
	sys_write8(sys_read8(config->base + ADCSTS) & ~IT51XXX_ADC_ADCCTS1, config->base + ADCSTS);
	sys_write8(sys_read8(config->base + ADCCFG) & ~IT51XXX_ADC_ADCCTS0, config->base + ADCCFG);

	/*
	 * bit[5-0]@ADCCTL : SCLKDIV
	 * SCLKDIV has to be equal to or greater than 1h;
	 */
	sys_write8(1, config->base + ADCCTL);

	/*
	 * Enable this bit, and data of VCHxDATL/VCHxDATM will be
	 * kept until data valid is cleared.
	 */
	sys_write8(sys_read8(config->base + ADCGCR) | IT51XXX_ADC_DBKEN, config->base + ADCGCR);

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), adc_it51xxx_isr,
		    DEVICE_DT_INST_GET(0), 0);

	k_sem_init(&data->sem, 0, 1);
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static struct adc_it51xxx_data adc_it51xxx_data_0 = {
	ADC_CONTEXT_INIT_TIMER(adc_it51xxx_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_it51xxx_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_it51xxx_data_0, ctx),
};

PINCTRL_DT_INST_DEFINE(0);

static const struct adc_it51xxx_cfg adc_it51xxx_cfg_0 = {
	.base = DT_INST_REG_ADDR(0),
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

DEVICE_DT_INST_DEFINE(0, adc_it51xxx_init, NULL, &adc_it51xxx_data_0, &adc_it51xxx_cfg_0,
		      PRE_KERNEL_1, CONFIG_ADC_INIT_PRIORITY, &api_it51xxx_driver_api);