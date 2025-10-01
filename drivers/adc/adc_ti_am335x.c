/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am335x_adc

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(ti_adc);

#define TI_ADC_TOTAL_CHANNELS  (8)
#define TI_ADC_TOTAL_STEPS     (16)
#define TI_ADC_IDLE_TIMEOUT_MS (83)

struct ti_adc_regs {
	uint8_t RESERVED_1[0x28];     /**< Reserved, offset: 0xA0 - 0x28 */
	volatile uint32_t IRQ_STATUS; /**< Interrupt Status Register, offset: 0x28 */
	volatile uint32_t IRQ_ENABLE; /**< Interrupt Enable Register, offset: 0x2C */
	uint8_t RESERVED_2[0x10];     /**< Reserved, offset: 0x30 - 0x40 */
	volatile uint32_t CONTROL;    /**< Control Register, offset: 0x40 */
	volatile uint32_t SEQ_STATUS; /**< Sequencer Status Register, offset: 0x44 */
	uint8_t RESERVED_3[0xC];      /**< Reserved, offset: 0x48 - 0x54 */
	volatile uint32_t STEPENABLE; /**< Sequencer Step Enable Register, offset: 0x54 */
	uint8_t RESERVED_4[0xC];      /**< Reserved, offset: 0x58 - 0x64 */
	volatile struct {
		volatile uint32_t CONFIG; /**< Step Config Register, offset: 0x64 + (j x 0x8) */
		volatile uint32_t DELAY;  /**< Step Delay Register, offset: 0x68 + (j x 0x8) */
	} STEP[TI_ADC_TOTAL_STEPS];
	volatile uint32_t FIFO0WC;    /**< FIFO0 Word Count Register, offset: 0xE4 */
	volatile uint32_t FIFO0THRSH; /**< FIFO0 Threshold Register, offset: 0xE8 */
	uint8_t RESERVED_5[0x04];     /**< Reserved, offset: 0xEC - 0xF0 */
	volatile uint32_t FIFO1WC;    /**< FIFO1 Word Count Register, offset: 0xF0 */
	volatile uint32_t FIFO1THRSH; /**< FIFO1 Threshold Register, offset: 0xE8 */
	uint8_t RESERVED_6[0x08];     /**< Reserved, offset: 0xF8 - 0x100 */
	volatile uint32_t FIFO0DATA;  /**< FIFO0 Read Data Register, offset: 0x100 */
	uint8_t RESERVED_7[0xFC];     /**< Reserved, offset: 0x104 - 0x200 */
	volatile uint32_t FIFO1DATA;  /**< FIFO1 Read Data Register, offset: 0x200 */
};

enum ti_adc_irq {
	TI_ADC_IRQ_END_OF_SEQUENCE_MISSING = BIT(0), /* End of Sequence Missing */
	TI_ADC_IRQ_END_OF_SEQUENCE = BIT(1),         /* End of Sequence */
	TI_ADC_IRQ_FIFO0_THR = BIT(2),               /* FIFO0 Threshold */
	TI_ADC_IRQ_FIFO0_OVERFLOW = BIT(3),          /* FIFO0 Overflow */
	TI_ADC_IRQ_FIFO0_UNDERFLOW = BIT(4),         /* FIFO0 Underflow */
	TI_ADC_IRQ_FIFO1_THR = BIT(5),               /* FIFO1 Threshold */
	TI_ADC_IRQ_FIFO1_OVERFLOW = BIT(6),          /* FIFO1 Overflow */
	TI_ADC_IRQ_FIFO1_UNDERFLOW = BIT(7),         /* FIFO1 Underflow */
	TI_ADC_IRQ_OUT_OF_RANGE = BIT(8),            /* Out Of Range*/
};

/* ADC Control Register */
#define TI_ADC_CONTROL_ENABLE     BIT(0) /* Enable Sequencer */
#define TI_ADC_CONTROL_POWER_DOWN BIT(4) /* Power Off */

/* ADC Sequencer Status Register */
#define TI_ADC_SEQ_STATUS_FSM       BIT(5)        /* FSM Status */
#define TI_ADC_SEQ_STATUS_FSM_IDLE  (0x0U)        /* FSM Status - Idle */
#define TI_ADC_SEQ_STATUS_STEP      GENMASK(4, 0) /* Current Step */
#define TI_ADC_SEQ_STATUS_STEP_IDLE (0x10U)       /* Current Step - Idle */

/* ADC Sequencer Step Config Register */
#define TI_ADC_STEPCONFIG_MODE            GENMASK(1, 0)   /* Step Mode */
#define TI_ADC_STEPCONFIG_MODE_SINGLESHOT (0x0U)          /* Step Mode - Continuous */
#define TI_ADC_STEPCONFIG_MODE_CONTINUOUS (0x1U)          /* Step Mode - Singleshot */
#define TI_ADC_STEPCONFIG_AVERAGING       GENMASK(4, 2)   /* Step Averaging */
#define TI_ADC_STEPCONFIG_AVERAGING_MAX   (4U)            /* Step Averaging - Max */
#define TI_ADC_STEPCONFIG_SEL_INM         GENMASK(18, 15) /* Negative Input */
#define TI_ADC_STEPCONFIG_SEL_INM_REFN    (0x8U)          /* Negative Input - Reference */
#define TI_ADC_STEPCONFIG_SEL_INP         GENMASK(22, 19) /* Positive Input */
#define TI_ADC_STEPCONFIG_DIFFERENTIAL    BIT(25)         /* Step Differential */
#define TI_ADC_STEPCONFIG_FIFOSEL         BIT(26)         /* Selected FIFO */

/* ADC Sequencer Step Delay Register */
#define TI_ADC_STEPDELAY_OPENDELAY       GENMASK(17, 0)  /* Pre-Conversion Delay */
#define TI_ADC_STEPDELAY_OPENDELAY_MAX   (0x3FFFFU)      /* Pre-Conversion Delay - Max */
#define TI_ADC_STEPDELAY_SAMPLEDELAY     GENMASK(31, 24) /* Conversion Delay */
#define TI_ADC_STEPDELAY_SAMPLEDELAY_MAX (0xFFU)         /* Conversion Delay - Max */

/* FIFO Threshold Register */
#define TI_ADC_FIFO_THRESHOLD (40)

/* FIFO Data Register */
#define TI_ADC_FIFODATA_ADCDATA GENMASK(11, 0) /* FIFO Data Mask */

#define DEV_CFG(dev)  ((const struct ti_adc_cfg *)(dev)->config)
#define DEV_DATA(dev) ((struct ti_adc_data *)(dev)->data)
#define DEV_REGS(dev) ((struct ti_adc_regs *)DEVICE_MMIO_GET(dev))

struct ti_adc_cfg {
	DEVICE_MMIO_ROM;
	void (*irq_func)(const struct device *dev);
	uint32_t open_delay[TI_ADC_TOTAL_CHANNELS];
	uint8_t oversampling[TI_ADC_TOTAL_CHANNELS];
	uint8_t fifo;
};

struct ti_adc_data {
	DEVICE_MMIO_RAM;
	struct adc_context ctx;
	const struct device *dev;
	uint8_t steps[TI_ADC_TOTAL_CHANNELS];
	uint32_t fifo_irq_mask;
	volatile const uint32_t *fifo_wc_ptr;
	volatile const uint32_t *fifo_data_ptr;
	uint16_t *buffer;
	uint16_t *repeat_buffer;
	uint8_t chan_count;
	uint8_t step_count;
};

/* Forward Declaration */
static int ti_adc_sequencer_start(const struct device *dev);

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ti_adc_data *data = CONTAINER_OF(ctx, struct ti_adc_data, ctx);
	struct ti_adc_regs *regs = DEV_REGS(data->dev);

	regs->CONTROL &= ~TI_ADC_CONTROL_ENABLE;

	/* enable steps */
	for (int chan = 0; chan < TI_ADC_TOTAL_CHANNELS; chan++) {
		if (IS_BIT_SET(ctx->sequence.channels, chan)) {
			regs->STEPENABLE |= BIT(data->steps[chan] + 1);
		}
	}

	if (ti_adc_sequencer_start(data->dev) < 0) {
		LOG_ERR("Sequencer failed to start");
	};
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat)
{
	struct ti_adc_data *data = CONTAINER_OF(ctx, struct ti_adc_data, ctx);

	if (repeat) {
		data->buffer = data->repeat_buffer;
	} else {
		data->buffer += data->chan_count;
	}
}

static int ti_adc_sequencer_start(const struct device *dev)
{
	struct ti_adc_data *data = DEV_DATA(dev);
	struct ti_adc_regs *regs = DEV_REGS(dev);
	uint64_t timeout = k_uptime_get();
	uint32_t seq_status = regs->SEQ_STATUS;

	while (FIELD_GET(TI_ADC_SEQ_STATUS_FSM, seq_status) != TI_ADC_SEQ_STATUS_FSM_IDLE &&
	       FIELD_GET(TI_ADC_SEQ_STATUS_STEP, seq_status) != TI_ADC_SEQ_STATUS_STEP_IDLE) {
		/* Timeout */
		if (k_uptime_get() - timeout > (TI_ADC_IDLE_TIMEOUT_MS * data->chan_count)) {
			return -ETIMEDOUT;
		}

		k_sleep(K_USEC(10)); /* sleep for 10us */

		seq_status = regs->SEQ_STATUS;
	}

	regs->CONTROL |= TI_ADC_CONTROL_ENABLE;

	return 0;
}

static int ti_adc_channel_setup(const struct device *dev, const struct adc_channel_cfg *chan_cfg)
{
	const struct ti_adc_cfg *cfg = DEV_CFG(dev);
	struct ti_adc_data *data = DEV_DATA(dev);
	struct ti_adc_regs *regs = DEV_REGS(dev);
	const uint8_t chan = chan_cfg->channel_id;

	if (chan >= TI_ADC_TOTAL_CHANNELS) {
		LOG_ERR("Channel %d invalid, must be less than %d", chan, TI_ADC_TOTAL_CHANNELS);
		return -EINVAL;
	}

	if (chan_cfg->gain != ADC_GAIN_1) {
		LOG_ERR("Gain must be 1x");
		return -EINVAL;
	}

	if (cfg->oversampling[chan] > TI_ADC_STEPCONFIG_AVERAGING_MAX) {
		LOG_ERR("Invalid oversampling value");
		return -EINVAL;
	}

	if (cfg->open_delay[chan] > TI_ADC_STEPDELAY_OPENDELAY_MAX) {
		LOG_ERR("Invalid open delay");
		return -EINVAL;
	}

	if (chan_cfg->acquisition_time > TI_ADC_STEPDELAY_SAMPLEDELAY_MAX) {
		LOG_ERR("Invalid acquisition time (sample delay)");
		return -EINVAL;
	}

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
	if (!chan_cfg->differential && chan_cfg->input_negative != TI_ADC_STEPCONFIG_SEL_INM_REFN) {
		LOG_ERR("For single ended input, negative input must be REFN");
		return -EINVAL;
	}
#endif

	/* TODO: allow continuous mode when DMA is possible */
	regs->STEP[data->step_count].CONFIG =
		FIELD_PREP(TI_ADC_STEPCONFIG_MODE, TI_ADC_STEPCONFIG_MODE_SINGLESHOT) |
		FIELD_PREP(TI_ADC_STEPCONFIG_AVERAGING, cfg->oversampling[chan]) |
#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
		FIELD_PREP(TI_ADC_STEPCONFIG_SEL_INP, chan_cfg->input_positive) |
		FIELD_PREP(TI_ADC_STEPCONFIG_SEL_INM, chan_cfg->input_negative) |
#else
		FIELD_PREP(TI_ADC_STEPCONFIG_SEL_INP, chan) |
		FIELD_PREP(TI_ADC_STEPCONFIG_SEL_INM, TI_ADC_STEPCONFIG_SEL_INM_REFN) |
#endif
		FIELD_PREP(TI_ADC_STEPCONFIG_DIFFERENTIAL, chan_cfg->differential) |
		FIELD_PREP(TI_ADC_STEPCONFIG_FIFOSEL, cfg->fifo);

	regs->STEP[data->step_count].DELAY =
		FIELD_PREP(TI_ADC_STEPDELAY_OPENDELAY, cfg->open_delay[chan]) |
		FIELD_PREP(TI_ADC_STEPDELAY_SAMPLEDELAY, chan_cfg->acquisition_time);

	data->steps[chan] = data->step_count++;

	return 0;
}

static int ti_adc_read_start(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ti_adc_data *data = DEV_DATA(dev);
	const uint8_t samplings = (sequence->options ? sequence->options->extra_samplings + 1 : 1);
	uint32_t required_size = 0;

	data->chan_count = 0;

	/* count channels enabled in sequence */
	for (int chan = 0; chan < TI_ADC_TOTAL_CHANNELS; chan++) {
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

static int ti_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
			     struct k_poll_signal *async)
{
	struct ti_adc_data *data = DEV_DATA(dev);
	int error;

	adc_context_lock(&data->ctx, async ? true : false, async);
	error = ti_adc_read_start(dev, sequence);
	adc_context_release(&data->ctx, error);

	return error;
}

static int ti_adc_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ti_adc_read_async(dev, sequence, NULL);
}

static int ti_adc_init(const struct device *dev)
{
	const struct ti_adc_cfg *cfg = DEV_CFG(dev);
	struct ti_adc_data *data = DEV_DATA(dev);
	struct ti_adc_regs *regs = DEV_REGS(dev);

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	cfg->irq_func(dev);

	if (cfg->fifo == 0) {
		/* FIFO 0 */
		data->fifo_irq_mask = TI_ADC_IRQ_FIFO0_OVERFLOW | TI_ADC_IRQ_FIFO0_UNDERFLOW;
		data->fifo_wc_ptr = &regs->FIFO0WC;
		data->fifo_data_ptr = &regs->FIFO0DATA;
		regs->FIFO0THRSH = TI_ADC_FIFO_THRESHOLD;
	} else if (cfg->fifo == 1) {
		/* FIFO 1 */
		data->fifo_irq_mask = TI_ADC_IRQ_FIFO1_OVERFLOW | TI_ADC_IRQ_FIFO1_UNDERFLOW;
		data->fifo_wc_ptr = &regs->FIFO1WC;
		data->fifo_data_ptr = &regs->FIFO1DATA;
		regs->FIFO1THRSH = TI_ADC_FIFO_THRESHOLD;
	} else {
		LOG_ERR("FIFO must be 0 or 1");
		return -EINVAL;
	}

	/* clear interrupt status */
	regs->IRQ_STATUS = data->fifo_irq_mask | TI_ADC_IRQ_END_OF_SEQUENCE;

	/* power up if not already up */
	if (regs->CONTROL & TI_ADC_CONTROL_POWER_DOWN) {
		regs->CONTROL &= ~TI_ADC_CONTROL_POWER_DOWN;
		k_sleep(K_USEC(4)); /* wait at least 4us */
	}

	/* enable interrupts */
	regs->IRQ_ENABLE = data->fifo_irq_mask | TI_ADC_IRQ_END_OF_SEQUENCE;

	adc_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void ti_adc_isr(const struct device *dev)
{
	struct ti_adc_regs *regs = DEV_REGS(dev);
	struct ti_adc_data *data = DEV_DATA(dev);
	uint32_t status = regs->IRQ_STATUS;
	uint8_t count = 0;
	uint32_t fsm;

	/* fifo overflow or underflow */
	if (status & data->fifo_irq_mask) {
		/* stop the sequencer */
		regs->CONTROL &= ~TI_ADC_CONTROL_ENABLE;

		/* clear interrupt status */
		regs->IRQ_STATUS |= data->fifo_irq_mask;

		/* wait for current conversion to finish */
		do {
			fsm = FIELD_GET(TI_ADC_SEQ_STATUS_FSM, regs->SEQ_STATUS);
		} while (fsm != TI_ADC_SEQ_STATUS_FSM_IDLE && count++ < 100);

		/* start the sequencer */
		regs->CONTROL |= TI_ADC_CONTROL_ENABLE;

	} else if (status & TI_ADC_IRQ_END_OF_SEQUENCE) {
		uint16_t wc = *data->fifo_wc_ptr;
		uint16_t i;

		for (i = 0; i < wc; i++) {
			/* write from fifo to buffer */
			data->buffer[i] = FIELD_GET(TI_ADC_FIFODATA_ADCDATA, *data->fifo_data_ptr);
		}

		regs->IRQ_STATUS |= TI_ADC_IRQ_END_OF_SEQUENCE;

		adc_context_on_sampling_done(&data->ctx, dev);
	}
}

#define EXPLICIT_CHAN_PROP(node, prop) [DT_REG_ADDR(node)] = DT_PROP(node, prop)

#define CHAN_PROP_LIST(n, prop) {DT_INST_FOREACH_CHILD_SEP_VARGS(n, EXPLICIT_CHAN_PROP, (,), prop)}

#define TI_ADC_INIT(n)                                                                             \
	static DEVICE_API(adc, ti_adc_driver_api_##n) = {                                          \
		.channel_setup = ti_adc_channel_setup,                                             \
		.read = ti_adc_read,                                                               \
		.ref_internal = DT_INST_PROP(n, ti_vrefp),                                         \
		IF_ENABLED(CONFIG_ADC_ASYNC, (.read_async = ti_adc_read_async,)) };                \
                                                                                                   \
	static void ti_adc_irq_setup_##n(const struct device *dev)                                 \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), ti_adc_isr,                 \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
                                                                                                   \
	static const struct ti_adc_cfg ti_adc_cfg_##n = {                                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.irq_func = &ti_adc_irq_setup_##n,                                                 \
		.open_delay = CHAN_PROP_LIST(n, ti_open_delay),                                    \
		.oversampling = CHAN_PROP_LIST(n, zephyr_oversampling),                            \
		.fifo = DT_INST_PROP(n, ti_fifo),                                                  \
	};                                                                                         \
                                                                                                   \
	static struct ti_adc_data ti_adc_data_##n = {                                              \
		ADC_CONTEXT_INIT_TIMER(ti_adc_data_##n, ctx),                                      \
		ADC_CONTEXT_INIT_LOCK(ti_adc_data_##n, ctx),                                       \
		ADC_CONTEXT_INIT_SYNC(ti_adc_data_##n, ctx),                                       \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, ti_adc_init, NULL, &ti_adc_data_##n, &ti_adc_cfg_##n,             \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ti_adc_driver_api_##n);

DT_INST_FOREACH_STATUS_OKAY(TI_ADC_INIT)
