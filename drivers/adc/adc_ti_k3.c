/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_adc_k3

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(adc_k3);

#define ADC_K3_TOTAL_CHANNELS 8
#define ADC_K3_TOTAL_STEPS    16

struct adc_k3_regs {
	uint8_t RESERVED_1[0x28]; /**< Reserved, offset: 0xA0 - 0x28 */
	__IO uint32_t IRQ_STATUS; /**< Interrupt Status Register, offset: 0x28 */
	__IO uint32_t IRQ_ENABLE; /**< Interrupt Enable Register, offset: 0x2C */
	uint8_t RESERVED_2[0x10]; /**< Reserved, offset: 0x30 - 0x40 */
	__IO uint32_t CONTROL;    /**< Control Register, offset: 0x40 */
	__IO uint32_t SEQ_STATUS; /**< Sequencer Status Register, offset: 0x44 */
	uint8_t RESERVED_3[0xC];  /**< Reserved, offset: 0x48 - 0x54 */
	__IO uint32_t STEPENABLE; /**< Sequencer Step Enable Register, offset: 0x54 */
	uint8_t RESERVED_4[0xC];  /**< Reserved, offset: 0x58 - 0x64 */
	__IO struct {
		__IO uint32_t CONFIG; /**< Step Configuration Register, offset: 0x64 + (j x 0x8) */
		__IO uint32_t DELAY;  /**< Step Delay Register, offset: 0x68 + (j x 0x8) */
	} STEP[ADC_K3_TOTAL_STEPS];
	__IO uint32_t FIFO0WC;    /**< FIFO0 Word Count Register, offset: 0xE4 */
	__IO uint32_t FIFO0THRSH; /**< FIFO0 Threshold Register, offset: 0xE8 */
	uint8_t RESERVED_5[0x04]; /**< Reserved, offset: 0xEC - 0xF0 */
	__IO uint32_t FIFO1WC;    /**< FIFO1 Word Count Register, offset: 0xF0 */
	__IO uint32_t FIFO1THRSH; /**< FIFO1 Threshold Register, offset: 0xE8 */
	uint8_t RESERVED_6[0x08]; /**< Reserved, offset: 0xF8 - 0x100 */
	__IO uint32_t FIFO0DATA;  /**< FIFO0 Read Data Register, offset: 0x100 */
	uint8_t RESERVED_7[0xFC]; /**< Reserved, offset: 0x104 - 0x200 */
	__IO uint32_t FIFO1DATA;  /**< FIFO1 Read Data Register, offset: 0x200 */
};

enum adc_k3_irq {
	ADC_K3_IRQ_END_OF_SEQUENCE_MISSING = BIT(0), /* End of Sequence Missing */
	ADC_K3_IRQ_END_OF_SEQUENCE = BIT(1),         /* End of Sequence */
	ADC_K3_IRQ_FIFO0_THR = BIT(2),               /* FIFO0 Threshold */
	ADC_K3_IRQ_FIFO0_OVERFLOW = BIT(3),          /* FIFO0 Overflow */
	ADC_K3_IRQ_FIFO0_UNDERFLOW = BIT(4),         /* FIFO0 Underflow */
	ADC_K3_IRQ_FIFO1_THR = BIT(5),               /* FIFO1 Threshold */
	ADC_K3_IRQ_FIFO1_OVERFLOW = BIT(6),          /* FIFO1 Overflow */
	ADC_K3_IRQ_FIFO1_UNDERFLOW = BIT(7),         /* FIFO1 Underflow */
	ADC_K3_IRQ_OUT_OF_RANGE = BIT(8),            /* Out Of Range*/
};

/* ADC Control Register */
#define ADC_K3_CONTROL_ENABLE     BIT(0) /* Enable Sequencer */
#define ADC_K3_CONTROL_POWER_DOWN BIT(4) /* Power Off */

/* ADC Sequencer Status Register */
#define ADC_K3_SEQ_STATUS_FSM       BIT(5)        /* FSM Status */
#define ADC_K3_SEQ_STATUS_FSM_IDLE  (0x0U)        /* FSM Status - Idle */
#define ADC_K3_SEQ_STATUS_STEP      GENMASK(4, 0) /* Current Step */
#define ADC_K3_SEQ_STATUS_STEP_IDLE (0x10U)       /* Current Step - Idle */

/* ADC Sequencer Step Config Register */
#define ADC_K3_STEPCONFIG_MODE            GENMASK(1, 0)   /* Step Mode */
#define ADC_K3_STEPCONFIG_MODE_SINGLESHOT (0x0U)          /* Step Mode - Continuous */
#define ADC_K3_STEPCONFIG_MODE_CONTINUOUS (0x1U)          /* Step Mode - Singleshot */
#define ADC_K3_STEPCONFIG_AVERAGING       GENMASK(4, 2)   /* Step Averaging */
#define ADC_K3_STEPCONFIG_AVERAGING_MAX   (4U)            /* Step Averaging - Max */
#define ADC_K3_STEPCONFIG_SEL_INM         GENMASK(18, 15) /* Negative Input */
#define ADC_K3_STEPCONFIG_SEL_INM_REFN    (0x8U)          /* Negative Input - Reference */
#define ADC_K3_STEPCONFIG_SEL_INP         GENMASK(22, 19) /* Positive Input */
#define ADC_K3_STEPCONFIG_DIFFERENTIAL    BIT(25)         /* Step Differential */
#define ADC_K3_STEPCONFIG_FIFOSEL         BIT(26)         /* Selected FIFO */

/* ADC Sequencer Step Delay Register */
#define ADC_K3_STEPDELAY_OPENDELAY       GENMASK(17, 0)  /* Pre-Conversion Delay */
#define ADC_K3_STEPDELAY_OPENDELAY_MAX   (0x3FFFFU)      /* Pre-Conversion Delay - Max */
#define ADC_K3_STEPDELAY_SAMPLEDELAY     GENMASK(31, 24) /* Conversion Delay */
#define ADC_K3_STEPDELAY_SAMPLEDELAY_MAX (0xFFU)         /* Conversion Delay - Max */

/* FIFO Threshold Register */
#define ADC_K3_FIFO_THRESHOLD (40)

/* FIFO Data Register */
#define ADC_K3_FIFODATA_ADCDATA GENMASK(11, 0) /* FIFO Data Mask */

#define DEV_CFG(dev)  ((const struct adc_k3_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct adc_k3_data *)(dev)->data)

struct adc_k3_cfg {
	struct adc_k3_regs *regs;
	void (*irq_func)(const struct device *dev);
	uint32_t open_delay[ADC_K3_TOTAL_CHANNELS];
	uint8_t oversampling[ADC_K3_TOTAL_CHANNELS];
	uint8_t fifo;
};

struct adc_k3_data {
	struct adc_context ctx;
	const struct device *dev;
	uint8_t steps[ADC_K3_TOTAL_CHANNELS];
	uint32_t fifo_irq_mask;
	__I uint32_t *fifo_wc_ptr;
	__I uint32_t *fifo_data_ptr;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t chan_count;
	uint8_t step_count;
};

/* Forward Declaration */
static int adc_k3_sequencer_start(const struct device *dev);

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_k3_data *data = CONTAINER_OF(ctx, struct adc_k3_data, ctx);
	struct adc_k3_regs *regs = DEV_CFG(data->dev)->regs;

	regs->CONTROL &= ~ADC_K3_CONTROL_ENABLE;

	/* enable steps */
	for (int chan = 0; chan < ADC_K3_TOTAL_CHANNELS; chan++) {
		if (IS_BIT_SET(ctx->sequence.channels, chan)) {
			regs->STEPENABLE |= BIT(data->steps[chan] + 1);
		}
	}

	if (adc_k3_sequencer_start(data->dev) < 0) {
		LOG_ERR("Sequencer failed to start");
	};
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct adc_k3_data *data = CONTAINER_OF(ctx, struct adc_k3_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->chan_count;
	}
}

static int adc_k3_sequencer_start(const struct device *dev)
{
	struct adc_k3_regs *regs = DEV_CFG(dev)->regs;
	uint64_t timeout = k_uptime_get();

	while (FIELD_GET(ADC_K3_SEQ_STATUS_FSM, regs->SEQ_STATUS) != ADC_K3_SEQ_STATUS_FSM_IDLE &&
	       FIELD_GET(ADC_K3_SEQ_STATUS_STEP, regs->SEQ_STATUS) != ADC_K3_SEQ_STATUS_STEP_IDLE) {
		/* 100ms Timeout */
		if (k_uptime_get() - timeout < 100) {
			return -ETIMEDOUT;
		}

		k_busy_wait(10000); /* 10ms */
	}

	regs->CONTROL |= ADC_K3_CONTROL_ENABLE;

	return 0;
}

static int adc_k3_channel_setup(const struct device *dev, const struct adc_channel_cfg *chan_cfg)
{
	const struct adc_k3_cfg *cfg = DEV_CFG(dev);
	struct adc_k3_data *data = DEV_DATA(dev);
	struct adc_k3_regs *regs = cfg->regs;
	const uint8_t chan = chan_cfg->channel_id;

	if (chan >= ADC_K3_TOTAL_CHANNELS) {
		LOG_ERR("Channel %d invalid, must be less than %d", chan, ADC_K3_TOTAL_CHANNELS);
		return -EINVAL;
	}

	if (chan_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain must be 1x");
		return -EINVAL;
	}

	if (cfg->oversampling[chan] > ADC_K3_STEPCONFIG_AVERAGING_MAX) {
		LOG_ERR("Invalid oversampling value");
		return -EINVAL;
	}

	if (cfg->open_delay[chan] > ADC_K3_STEPDELAY_OPENDELAY_MAX) {
		LOG_ERR("Invalid open delay");
		return -EINVAL;
	}

	if (chan_cfg->acquisition_time > ADC_K3_STEPDELAY_SAMPLEDELAY_MAX) {
		LOG_ERR("Invalid acquisition time (sample delay)");
		return -EINVAL;
	}

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
	if (!chan_cfg->differential && chan_cfg->input_negative != ADC_K3_STEPCONFIG_SEL_INM_REFN) {
		LOG_ERR("For single ended input, negative input must be REFN");
		return -EINVAL;
	}
#endif

	/* TODO: allow continuous mode when DMA is possible */
	regs->STEP[data->step_count].CONFIG |=
		FIELD_PREP(ADC_K3_STEPCONFIG_MODE, ADC_K3_STEPCONFIG_MODE_SINGLESHOT) |
		FIELD_PREP(ADC_K3_STEPCONFIG_AVERAGING, cfg->oversampling[chan]) |
#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
		FIELD_PREP(ADC_K3_STEPCONFIG_SEL_INP, chan_cfg->input_positive) |
		FIELD_PREP(ADC_K3_STEPCONFIG_SEL_INM, chan_cfg->input_negative) |
#else
		FIELD_PREP(ADC_K3_STEPCONFIG_SEL_INP, chan) |
		FIELD_PREP(ADC_K3_STEPCONFIG_SEL_INM, ADC_K3_STEPCONFIG_SEL_INM_REFN) |
#endif
		FIELD_PREP(ADC_K3_STEPCONFIG_DIFFERENTIAL, chan_cfg->differential) |
		FIELD_PREP(ADC_K3_STEPCONFIG_FIFOSEL, cfg->fifo);

	regs->STEP[data->step_count].DELAY |=
		FIELD_PREP(ADC_K3_STEPDELAY_OPENDELAY, cfg->open_delay[chan]) |
		FIELD_PREP(ADC_K3_STEPDELAY_SAMPLEDELAY, chan_cfg->acquisition_time);

	data->steps[chan] = data->step_count++;

	return 0;
}

static int adc_k3_read_start(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_k3_data *data = DEV_DATA(dev);
	const uint8_t samplings = (sequence->options ? sequence->options->extra_samplings + 1 : 1);
	uint32_t required_size = 0;

	data->chan_count = 0;

	/* count channels enabled in sequence */
	for (int chan = 0; chan < ADC_K3_TOTAL_CHANNELS; chan++) {
		if (IS_BIT_SET(sequence->channels, chan)) {
			data->chan_count++;
		}
	};

	required_size = sizeof(uint16_t) * data->chan_count * samplings;

	if (sequence->buffer_size < required_size) {
		LOG_ERR("Buffer size is too small (%u/%u)", sequence->buffer_size, required_size);
		return -ENOMEM;
	}

	data->buffer = sequence->buffer;
	data->repeat_buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int adc_k3_read_async(const struct device *dev, const struct adc_sequence *sequence,
			     struct k_poll_signal *async)
{
	struct adc_k3_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = adc_k3_read_start(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int adc_k3_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return adc_k3_read_async(dev, sequence, NULL);
}

static int adc_k3_init(const struct device *dev)
{
	const struct adc_k3_cfg *cfg = DEV_CFG(dev);
	struct adc_k3_data *data = DEV_DATA(dev);
	struct adc_k3_regs *regs = cfg->regs;

	DEV_CFG(dev)->irq_func(dev);

	if (cfg->fifo == 0) {
		/* FIFO 0 */
		data->fifo_irq_mask = ADC_K3_IRQ_FIFO0_OVERFLOW | ADC_K3_IRQ_FIFO0_UNDERFLOW;
		data->fifo_wc_ptr = &regs->FIFO0WC;
		data->fifo_data_ptr = &regs->FIFO0DATA;
		regs->FIFO0THRSH = ADC_K3_FIFO_THRESHOLD;
	} else if (cfg->fifo == 1) {
		/* FIFO 1 */
		data->fifo_irq_mask = ADC_K3_IRQ_FIFO1_OVERFLOW | ADC_K3_IRQ_FIFO1_UNDERFLOW;
		data->fifo_wc_ptr = &regs->FIFO1WC;
		data->fifo_data_ptr = &regs->FIFO1DATA;
		regs->FIFO1THRSH = ADC_K3_FIFO_THRESHOLD;
	} else {
		LOG_ERR("FIFO must be 0 or 1");
		return -EINVAL;
	}

	/* clear interrupt status */
	regs->IRQ_STATUS = data->fifo_irq_mask | ADC_K3_IRQ_END_OF_SEQUENCE;

	/* power up if not already up */
	if (regs->CONTROL & ADC_K3_CONTROL_POWER_DOWN) {
		regs->CONTROL &= ~ADC_K3_CONTROL_POWER_DOWN;
		k_sleep(K_USEC(10)); /* wait at least 4us */
	}

	/* enable interrupts */
	regs->IRQ_ENABLE = data->fifo_irq_mask | ADC_K3_IRQ_END_OF_SEQUENCE;

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void adc_k3_isr(const struct device *dev)
{
	struct adc_k3_regs *regs = DEV_CFG(dev)->regs;
	struct adc_k3_data *data = DEV_DATA(dev);
	uint32_t status = regs->IRQ_STATUS;

	/* fifo overflow or underflow */
	if (status & data->fifo_irq_mask) {
		/* stop the sequencer */
		regs->CONTROL &= ~ADC_K3_CONTROL_ENABLE;

		/* clear interrupt status */
		regs->IRQ_STATUS |= data->fifo_irq_mask;

		/* start the sequencer */
		if (adc_k3_sequencer_start(dev) < 0) {
			LOG_ERR("Sequencer failed to start");
		};
	} else if (status & ADC_K3_IRQ_END_OF_SEQUENCE) {
		uint16_t wc = *data->fifo_wc_ptr, i;

		for (i = 0; i < wc; i++) {
			/* write from fifo to buffer */
			data->buffer[i] = FIELD_GET(ADC_K3_FIFODATA_ADCDATA, *data->fifo_data_ptr);
		}

		regs->IRQ_STATUS |= ADC_K3_IRQ_END_OF_SEQUENCE;

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

#define EXPLICIT_CHAN_PROP(node, prop) [DT_REG_ADDR(node)] = DT_PROP(node, prop)

#define CHAN_PROP_LIST(n, prop) {DT_INST_FOREACH_CHILD_SEP_VARGS(n, EXPLICIT_CHAN_PROP, (,), prop)}

#define ADC_K3_INIT(n)                                                                             \
	static DEVICE_API(adc, adc_k3_driver_api_##n) = {                                          \
		.channel_setup = adc_k3_channel_setup,                                             \
		.read = adc_k3_read,                                                               \
		.ref_internal = DT_INST_PROP(n, vref_mv),                                          \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = adc_k3_read_async,)) };                \
                                                                                                   \
	static void adc_k3_irq_setup_##n(const struct device *dev)                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), adc_k3_isr,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct adc_k3_cfg adc_k3_cfg_##n = {                                          \
		.regs = (struct adc_k3_regs *)DT_INST_REG_ADDR(n),                                 \
		.irq_func = &adc_k3_irq_setup_##n,                                                 \
		.open_delay = CHAN_PROP_LIST(n, open_delay),                                       \
		.oversampling = CHAN_PROP_LIST(n, zephyr_oversampling),                            \
		.fifo = DT_INST_PROP(n, fifo),                                                     \
	};                                                                                         \
                                                                                                   \
	static struct adc_k3_data adc_k3_data_##n = {                                              \
		ADC_CONTEXT_INIT_TIMER(adc_k3_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_LOCK(adc_k3_data_##n, ctx),                                       \
		ADC_CONTEXT_INIT_SYNC(adc_k3_data_##n, ctx),                                       \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &adc_k3_init, NULL, &adc_k3_data_##n, &adc_k3_cfg_##n,            \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_k3_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(ADC_K3_INIT)
