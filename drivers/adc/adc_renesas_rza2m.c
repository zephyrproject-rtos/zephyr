/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_adc

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(adc_renesas_rz, CONFIG_ADC_LOG_LEVEL);

struct adc_reg {
	uint8_t offset, size;
};

enum {
	RZA2M_ADCSR,      /* Control Register */
	RZA2M_ADANSA0,    /* Channel Select Register A0 */
	RZA2M_ADADS0,     /* Addition/Average Mode Select Register 0 */
	RZA2M_ADADC,      /* Addition/Average Count Select Register */
	RZA2M_ADCER,      /* Control Extended Register */
	RZA2M_ADSTRGR,    /* Start Trigger Select Register */
	RZA2M_ADANSB0,    /* Channel Select Register B0 */
	RZA2M_ADDBLDR,    /* Data Duplication Register */
	RZA2M_ADRD,       /* Self-Diagnosis Data Register */
	RZA2M_ADDR0,      /* Data Register 0 */
	RZA2M_ADDR1,      /* Data Register 1 */
	RZA2M_ADDR2,      /* Data Register 2 */
	RZA2M_ADDR3,      /* Data Register 3 */
	RZA2M_ADDR4,      /* Data Register 4 */
	RZA2M_ADDR5,      /* Data Register 5 */
	RZA2M_ADDR6,      /* Data Register 6 */
	RZA2M_ADDR7,      /* Data Register 7 */
	RZA2M_ADDISCR,    /* Disconnection Detection Control Register */
	RZA2M_ADGSPCR,    /* Group Scan Priority Control Register */
	RZA2M_ADDBLDRA,   /*  Data Duplication Register A */
	RZA2M_ADDBLDRB,   /* Data Duplication Register B */
	RZA2M_ADWINMON,   /* Compare Function AB Status Monitor Register */
	RZA2M_ADCMPCR,    /* Compare Control Register */
	RZA2M_ADCMPANSR0, /*  Compare Function Window-A Channel Selection Register 0 */
	RZA2M_ADCMPLR0,   /* Compare Function Window-A Comparison Condition Setting Register 0 */
	RZA2M_ADCMPDR0,   /* Compare Function Window-A Lower Level Setting Register */
	RZA2M_ADCMPDR1,   /* Compare Function Window-A Upper Level Setting Register */
	RZA2M_ADCMPSR0,   /* Compare Function Window-A Channel Status Register 0 */
	RZA2M_ADCMPBNSR,  /* Compare Function Window-B Channel Selection Register */
	RZA2M_ADWINLLB,   /* Compare Function Window-B Lower Level Setting Register */
	RZA2M_ADWINULB,   /* Compare Function Window-B Upper Level Setting Register */
	RZA2M_ADCMPBSR,   /* Compare Function Window-B Status Register */
	RZA2M_ADANSC0,    /* Channel Select Register C0 */
	RZA2M_ADGCTRGR,   /* Group C Trigger Select Register */
	RZA2M_ADSSTR0,    /* Sampling State Register 0 */
	RZA2M_ADSSTR1,    /* Sampling State Register 1 */
	RZA2M_ADSSTR2,    /* Sampling State Register 2 */
	RZA2M_ADSSTR3,    /* Sampling State Register 3 */
	RZA2M_ADSSTR4,    /* Sampling State Register 4 */
	RZA2M_ADSSTR5,    /* Sampling State Register 5 */
	RZA2M_ADSSTR6,    /* Sampling State Register 6 */
	RZA2M_ADSSTR7,    /* Sampling State Register 7 */

	RZA2M_ADC_NR_REGS, /* Total number of registers */
};

struct adc_rza2m_config {
	DEVICE_MMIO_ROM; /* Must be first */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	/* Pinctrl configs */
	const struct pinctrl_dev_config *pcfg;
	/* Structure that stores register offset and size information */
	const struct adc_reg *regs;
	/* Mask for channels existed in each board */
	uint32_t channel_available_mask;
};

struct adc_rza2m_data {
	DEVICE_MMIO_RAM; /* Must be first */
	/* Structure that handle state of ongoing read operation */
	struct adc_context ctx;
	/* Pointer to RZ ADC own device structure */
	const struct device *dev;
	/* Pointer to memory where next sample will be written */
	uint16_t *buf;
	/* Mask with channels that will be sampled */
	uint32_t channels;
	/* Mask of channels that have been configured via setup API */
	uint32_t configured_channels;
	/* Buffer id */
	uint16_t buf_id;
};

#define ADC_RZA2M_DEFAULT_ACQ_TIME 11

/* ADCSR (Control Register) */
#define ADCSR_ADST        BIT(15)         /* A/D Conversion Start */
#define ADCSR_ADCS_MASK   GENMASK(14, 13) /* Scan Mode Select */
#define ADCSR_ADIE        BIT(12)         /* Scan End Interrupt Enable */
#define ADCSR_TRGE        BIT(9)          /* Trigger Start Enable */
#define ADCSR_EXTRG       BIT(8)          /* Trigger Select */
#define ADCSR_DBLE        BIT(7)          /* Double Trigger Mode Select */
#define ADCSR_GBADIE      BIT(6)          /* Group B Scan End Interrupt Enable */
#define ADCSR_DBLANS_MASK GENMASK(4, 0)   /* Double Trigger Channel Select */

/* ADANSA0 (Channel Select Register A0) */
#define ADANSA0_ANSA0_MASK GENMASK(7, 0) /* A/D Conversion Channel Select */

/* ADADS0 (Addition/Average Mode Select Register 0) */
#define ADADS0_ADS0_MASK GENMASK(7, 0) /* A/D Addition/Average Channel Select */

/* ADADC (Addition/Average Count Select Register) */
#define ADADC_ADC_MASK GENMASK(2, 0) /* Addition Count Select */
#define ADADC_AVEE     BIT(7)        /* Average Mode Enable */

/* ADCER  (Control Extended Register) */
#define ADCER_ADRFMT       BIT(15)       /* Data Register Format Select */
#define ADCER_DIAGM        BIT(11)       /* Self-Diagnosis Enable */
#define ADCER_DIAGLD       BIT(10)       /* Self-Diagnosis Mode Select */
#define ADCER_DIAGVAL_MASK GENMASK(9, 8) /* Self-Diagnosis Conversion Voltage Select */
#define ADCER_ACE          BIT(5)        /* Data Register Automatic Clearing Enable */
#define ADCER_ADPRC_MASK   GENMASK(2, 1) /* Conversion Accuracy */

/* clang-format off */
/* Registers */
static const struct adc_reg rza2m_regs[RZA2M_ADC_NR_REGS] = {
			[RZA2M_ADCSR] = {0x00, 16},
			[RZA2M_ADANSA0] = {0x04, 16},
			[RZA2M_ADADS0] = {0x08, 16},
			[RZA2M_ADADC] = {0x0C, 8},
			[RZA2M_ADCER] = {0x0E, 16},
			[RZA2M_ADSTRGR] = {0x10, 16},
			[RZA2M_ADANSB0] = {0x14, 16},
			[RZA2M_ADDBLDR] = {0x18, 16},
			[RZA2M_ADRD] = {0x1E, 16},
			[RZA2M_ADDR0] = {0x20, 16},
			[RZA2M_ADDR1] = {0x22, 16},
			[RZA2M_ADDR2] = {0x24, 16},
			[RZA2M_ADDR3] = {0x26, 16},
			[RZA2M_ADDR4] = {0x28, 16},
			[RZA2M_ADDR5] = {0x2A, 16},
			[RZA2M_ADDR6] = {0x2C, 16},
			[RZA2M_ADDR7] = {0x2E, 16},
			[RZA2M_ADDISCR] = {0x7A, 8},
			[RZA2M_ADGSPCR] = {0x80, 16},
			[RZA2M_ADDBLDRA] = {0x84, 16},
			[RZA2M_ADDBLDRB] = {0x86, 16},
			[RZA2M_ADWINMON] = {0x8C, 8},
			[RZA2M_ADCMPCR] = {0x90, 16},
			[RZA2M_ADCMPANSR0] = {0x94, 16},
			[RZA2M_ADCMPLR0] = {0x98, 16},
			[RZA2M_ADCMPDR0] = {0x9C, 16},
			[RZA2M_ADCMPDR1] = {0x9E, 16},
			[RZA2M_ADCMPSR0] = {0xA0, 16},
			[RZA2M_ADCMPBNSR] = {0xA6, 8},
			[RZA2M_ADWINLLB] = {0xA8, 16},
			[RZA2M_ADWINULB] = {0xAA, 16},
			[RZA2M_ADCMPBSR] = {0xAC, 8},
			[RZA2M_ADANSC0] = {0xD4, 16},
			[RZA2M_ADGCTRGR] = {0xD9, 8},
			[RZA2M_ADSSTR0] = {0xE0, 8},
			[RZA2M_ADSSTR1] = {0xE1, 8},
			[RZA2M_ADSSTR2] = {0xE2, 8},
			[RZA2M_ADSSTR3] = {0xE3, 8},
			[RZA2M_ADSSTR4] = {0xE4, 8},
			[RZA2M_ADSSTR5] = {0xE5, 8},
			[RZA2M_ADSSTR6] = {0xE6, 8},
			[RZA2M_ADSSTR7] = {0xE7, 8},
};
/* clang-format on */

static __maybe_unused uint8_t adc_rza2m_read_8(const struct device *dev, uint32_t offs)
{
	const struct adc_rza2m_config *config = dev->config;
	uint32_t offset = config->regs[offs].offset;

	return sys_read8(DEVICE_MMIO_GET(dev) + offset);
}

static void adc_rza2m_write_8(const struct device *dev, uint32_t offs, uint8_t value)
{
	const struct adc_rza2m_config *config = dev->config;
	uint32_t offset = config->regs[offs].offset;

	sys_write8(value, DEVICE_MMIO_GET(dev) + offset);
}

static uint16_t adc_rza2m_read_16(const struct device *dev, uint32_t offs)
{
	const struct adc_rza2m_config *config = dev->config;
	uint32_t offset = config->regs[offs].offset;

	return sys_read16(DEVICE_MMIO_GET(dev) + offset);
}

static void adc_rza2m_write_16(const struct device *dev, uint32_t offs, uint16_t value)
{
	const struct adc_rza2m_config *config = dev->config;
	uint32_t offset = config->regs[offs].offset;

	sys_write16(value, DEVICE_MMIO_GET(dev) + offset);
}

/**
 * @brief Interrupt handler
 */
static void adc_rza2m_isr(const struct device *dev)
{
	struct adc_rza2m_data *data = dev->data;
	int16_t *sample_buffer = (int16_t *)data->buf;
	uint32_t channels = data->channels;
	uint8_t channel_id = 0;
	/* Read ADC results for all enabled channels in the current sequence mask */
	while (channels != 0U) {
		if ((channels & 0x01U) != 0U) {
			sample_buffer[data->buf_id] =
				adc_rza2m_read_16(dev, RZA2M_ADDR0 + channel_id);
			data->buf_id = data->buf_id + 1;
		}
		channels >>= 1;
		channel_id++;
	}
	adc_context_on_sampling_done(&data->ctx, dev);
}

/**
 * @brief Setup channels before starting to scan ADC
 */
static int adc_rza2m_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	struct adc_rza2m_data *data = dev->data;
	const struct adc_rza2m_config *config = dev->config;
	uint8_t acq_time = 0;

	if (!((config->channel_available_mask & BIT(channel_cfg->channel_id)) != 0)) {
		LOG_ERR("Unsupported channel id '%d'", channel_cfg->channel_id);
		return -ENOTSUP;
	}

	if (channel_cfg->acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		acq_time = ADC_RZA2M_DEFAULT_ACQ_TIME;

	} else {
		if (ADC_ACQ_TIME_UNIT(channel_cfg->acquisition_time) != ADC_ACQ_TIME_TICKS) {
			LOG_ERR("Acquisition time only support ADC_ACQ_TIME_TICKS unit");
			return -ENOTSUP;
		}

		if (!IN_RANGE(ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time), 5, 255)) {
			LOG_ERR("Acquisition time value %d is out of range (5~255 ticks)",
				channel_cfg->acquisition_time);
			return -ENOTSUP;
		}
		acq_time = ADC_ACQ_TIME_VALUE(channel_cfg->acquisition_time);
	}

	if (channel_cfg->differential) {
		LOG_ERR("Differential channels are not supported");
		return -ENOTSUP;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Unsupported channel gain %d", channel_cfg->gain);
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		LOG_ERR("Unsupported channel reference");
		return -EINVAL;
	}

	data->configured_channels |= (1U << channel_cfg->channel_id);

	/* Set acquisition time for each channel */
	adc_rza2m_write_8(dev, RZA2M_ADSSTR0 + channel_cfg->channel_id, acq_time);

	return 0;
}

/**
 * @brief Check if buffer in @p sequence is big enough to hold all ADC samples
 */
static int adc_rza2m_check_buffer_size(const struct device *dev,
				       const struct adc_sequence *sequence)
{
	uint8_t channels = 0;
	size_t needed;

	channels = POPCOUNT(sequence->channels);

	if (sequence->resolution == 8) {
		needed = channels * sizeof(uint8_t);
	} else {
		needed = channels * sizeof(uint16_t);
	}

	if (sequence->options) {
		needed *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < needed) {
		return -ENOMEM;
	}

	return 0;
}

/**
 * @brief Start processing read request
 */
static int adc_rza2m_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	const struct adc_rza2m_config *config = dev->config;
	struct adc_rza2m_data *data = dev->data;
	uint8_t adadc = 0;
	uint16_t adcer;
	int err;

	if (sequence->channels == 0) {
		LOG_ERR("No channel to read");
		return -EINVAL;
	}

	switch (sequence->resolution) {
	case 8:
		adcer = adc_rza2m_read_16(dev, RZA2M_ADCER);
		adcer &= ~ADCER_ADPRC_MASK;
		adcer |= FIELD_PREP(ADCER_ADPRC_MASK, 0x02);
		break;
	case 10:
		adcer = adc_rza2m_read_16(dev, RZA2M_ADCER);
		adcer &= ~ADCER_ADPRC_MASK;
		adcer |= FIELD_PREP(ADCER_ADPRC_MASK, 0x01);
		break;
	case 12:
		adcer = adc_rza2m_read_16(dev, RZA2M_ADCER);
		adcer &= ~ADCER_ADPRC_MASK;
		adcer |= FIELD_PREP(ADCER_ADPRC_MASK, 0x00);
		break;
	default:
		LOG_ERR("Invalid resolution value %d, (valid value: 8, 10, 12)",
			sequence->resolution);
		return -EINVAL;
	}

	switch (sequence->oversampling) {
	case 0:
		adadc = 0;
		break;
	case 1:
		adadc |= FIELD_PREP(ADADC_ADC_MASK, 0x01) | ADADC_AVEE;
		break;
	case 2:
		adadc |= FIELD_PREP(ADADC_ADC_MASK, 0x03) | ADADC_AVEE;
		break;
	default:
		LOG_ERR("Invalid oversampling value %d (valid value: 0, 1, 2)",
			sequence->oversampling);
		return -EINVAL;
	}

	if ((sequence->channels & ~config->channel_available_mask) != 0) {
		LOG_ERR("Unsupported channels in mask: 0x%08x", sequence->channels);
		return -ENOTSUP;
	}

	/* Check if channels have been configured via channel_setup */
	if ((sequence->channels & ~data->configured_channels) != 0) {
		LOG_ERR("Attempted to read from unconfigured channels in mask: 0x%08x",
			sequence->channels);
		return -EINVAL;
	}

	err = adc_rza2m_check_buffer_size(dev, sequence);

	if (err) {
		LOG_ERR("Buffer size too small");
		return err;
	}

	/* Select input channels for this sequence */
	adc_rza2m_write_16(dev, RZA2M_ADANSA0, sequence->channels);
	/* Set oversampling */
	adc_rza2m_write_16(dev, RZA2M_ADADS0, sequence->channels);
	adc_rza2m_write_8(dev, RZA2M_ADADC, adadc);
	/* Set resolution */
	adc_rza2m_write_16(dev, RZA2M_ADCER, adcer);

	data->buf_id = 0;
	data->buf = sequence->buffer;
	adc_context_start_read(&data->ctx, sequence);
	adc_context_wait_for_completion(&data->ctx);

	return 0;
}

/**
 * @brief Start processing read request asynchronously
 */
static int adc_rza2m_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	struct adc_rza2m_data *data = dev->data;
	int err;

	adc_context_lock(&data->ctx, async ? true : false, async);
	err = adc_rza2m_start_read(dev, sequence);
	adc_context_release(&data->ctx, err);

	return err;
}

/**
 * @brief Start processing read request synchronously
 */
static int adc_rza2m_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_rza2m_read_async(dev, sequence, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_rza2m_data *data = CONTAINER_OF(ctx, struct adc_rza2m_data, ctx);
	const struct device *dev = data->dev;
	uint16_t reg_16;

	data->channels = ctx->sequence.channels;
	/* Start conversion */
	reg_16 = adc_rza2m_read_16(dev, RZA2M_ADCSR);
	reg_16 |= ADCSR_ADST;
	adc_rza2m_write_16(dev, RZA2M_ADCSR, reg_16);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_rza2m_data *data = CONTAINER_OF(ctx, struct adc_rza2m_data, ctx);

	if (repeat_sampling) {
		data->buf_id = 0;
	}
}

/**
 * @brief Set the operational mode common to all channels
 */
static void adc_rza2m_configure(const struct device *dev)
{
	uint16_t adcsr = 0, adcer = 0;

	/* ADCSR:
	 * Single scan mode, scan end interrupt enable
	 */
	adcsr |= ADCSR_ADIE;
	adc_rza2m_write_16(dev, RZA2M_ADCSR, adcsr);

	/* ADCER:
	 * Resolution 12-bit, automatic clearing after read, right alignment data format
	 */
	adcer |= ADCER_ACE;
	adc_rza2m_write_16(dev, RZA2M_ADCER, adcer);

	/* Set default values for acquisition time */
	for (int i = 0; i < 8; i++) {
		adc_rza2m_write_8(dev, RZA2M_ADSSTR0 + i, ADC_RZA2M_DEFAULT_ACQ_TIME);
	}
}

/**
 * @brief Function called on init for each RZA2M ADC device
 */
static int adc_rza2m_init(const struct device *dev)
{
	const struct adc_rza2m_config *config = dev->config;
	struct adc_rza2m_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	adc_rza2m_configure(dev);

	/* Release context unconditionally */
	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

/**
 * ************************* DRIVER REGISTER SECTION ***************************
 */
#define ADC_RZA2M_IRQ_CONNECT(idx, irq_name, isr)                                                  \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, irq_name, irq) - GIC_SPI_INT_BASE,            \
			    DT_INST_IRQ_BY_NAME(idx, irq_name, priority), isr,                     \
			    DEVICE_DT_INST_GET(idx), DT_INST_IRQ_BY_NAME(idx, irq_name, flags));   \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, irq_name, irq) - GIC_SPI_INT_BASE);            \
	} while (0)

#define ADC_RZA2M_CONFIG_FUNC(idx) ADC_RZA2M_IRQ_CONNECT(idx, scanend, adc_rza2m_isr);

#define ADC_RZA2M_INIT(idx)                                                                        \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	uint32_t clock_subsys##n = DT_INST_CLOCKS_CELL(idx, clk_id);                               \
	static DEVICE_API(adc, adc_rza2m_api_##idx) = {                                            \
		.channel_setup = adc_rza2m_channel_setup,                                          \
		.read = adc_rza2m_read,                                                            \
		.ref_internal = DT_INST_PROP(idx, vref_mv),                                        \
		IF_ENABLED(CONFIG_ADC_ASYNC,                                                       \
					  (.read_async = adc_rza2m_read_async))};                  \
	static struct adc_rza2m_config adc_rza2m_config_##idx = {                                  \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(idx)),                                            \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(idx)),                              \
		.clock_subsys = (clock_control_subsys_t)(&clock_subsys##n),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.regs = rza2m_regs,                                                                \
		.channel_available_mask = DT_INST_PROP(idx, channel_available_mask),               \
	};                                                                                         \
	static struct adc_rza2m_data adc_rza2m_data_##idx = {                                      \
		ADC_CONTEXT_INIT_TIMER(adc_rza2m_data_##idx, ctx),                                 \
		ADC_CONTEXT_INIT_LOCK(adc_rza2m_data_##idx, ctx),                                  \
		ADC_CONTEXT_INIT_SYNC(adc_rza2m_data_##idx, ctx),                                  \
		.dev = DEVICE_DT_INST_GET(idx),                                                    \
	};                                                                                         \
	static int adc_rza2m_init_##idx(const struct device *dev)                                  \
	{                                                                                          \
		ADC_RZA2M_CONFIG_FUNC(idx)                                                         \
		return adc_rza2m_init(dev);                                                        \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(idx, adc_rza2m_init_##idx, NULL, &adc_rza2m_data_##idx,              \
			      &adc_rza2m_config_##idx, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,      \
			      &adc_rza2m_api_##idx)

DT_INST_FOREACH_STATUS_OKAY(ADC_RZA2M_INIT);
