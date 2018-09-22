/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <init.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <board.h>
#include <adc.h>
#include <arch/cpu.h>

#define SYS_LOG_DOMAIN "dev/adc_quark_d2000"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define MAX_CHANNELS			18

#define REG_CCU_PERIPH_CLK_GATE_CTL	(SCSS_REGISTER_BASE + 0x18)
#define CLK_PERIPH_CLK			BIT(1)
#define CLK_PERIPH_ADC			BIT(22)
#define CLK_PERIPH_ADC_REGISTER		BIT(23)

#define REG_CCU_PERIPH_CLK_DIV_CTL0	(SCSS_REGISTER_BASE + 0x1C)
#define CLK_DIV_ADC_POS			16
#define CLK_DIV_ADC_MASK		(0x3FF << CLK_DIV_ADC_POS)

#define REG_INT_ADC_PWR_MASK		(SCSS_REGISTER_BASE + 0x4CC)
#define REG_INT_ADC_CALIB_MASK		(SCSS_REGISTER_BASE + 0x4D0)

#define ADC_DIV_MAX			(1023)
#define ADC_DELAY_MAX			(0x1FFF)
#define ADC_CAL_MAX			(0x3F)
#define ADC_FIFO_LEN			(32)
#define ADC_FIFO_CLEAR			(0xFFFFFFFF)

/* ADC sequence table */
#define ADC_CAL_SEQ_TABLE_DEFAULT	(0x80808080)

/* ADC command */
#define ADC_CMD_SW_OFFSET		(24)
#define ADC_CMD_SW_MASK			(0xFF000000)
#define ADC_CMD_CAL_DATA_OFFSET		(16)
#define ADC_CMD_RESOLUTION_OFFSET	(14)
#define ADC_CMD_RESOLUTION_MASK		(0xC000)
#define ADC_CMD_NS_OFFSET		(4)
#define ADC_CMD_NS_MASK			(0x1F0)
#define ADC_CMD_IE_OFFSET		(3)
#define ADC_CMD_IE			BIT(3)

#define ADC_CMD_START_SINGLE		(0)
#define ADC_CMD_START_CONT		(1)
#define ADC_CMD_RESET_CAL		(2)
#define ADC_CMD_START_CAL		(3)
#define ADC_CMD_LOAD_CAL		(4)
#define ADC_CMD_STOP_CONT		(5)

/* Interrupt enable */
#define ADC_INTR_ENABLE_CC		BIT(0)
#define ADC_INTR_ENABLE_FO		BIT(1)
#define ADC_INTR_ENABLE_CONT_CC		BIT(2)

/* Interrupt status */
#define ADC_INTR_STATUS_CC		BIT(0)
#define ADC_INTR_STATUS_FO		BIT(1)
#define ADC_INTR_STATUS_CONT_CC		BIT(2)

/* Operating mode */
#define ADC_OP_MODE_IE			BIT(27)
#define ADC_OP_MODE_DELAY_OFFSET	(0x3)
#define ADC_OP_MODE_DELAY_MASK		(0xFFF8)
#define ADC_OP_MODE_OM_MASK		(0x7)

#define FIFO_INTR_THRESHOLD		(ADC_FIFO_LEN / 2)

enum {
	ADC_MODE_DEEP_PWR_DOWN,	/**< Deep power down mode. */
	ADC_MODE_PWR_DOWN,	/**< Power down mode. */
	ADC_MODE_STDBY,		/**< Standby mode. */
	ADC_MODE_NORM_CAL,	/**< Normal mode, with calibration. */
	ADC_MODE_NORM_NO_CAL	/**< Normal mode, no calibration. */
};

/** ADC register map */
typedef struct {
	u32_t seq[8];		/**< ADC Channel Sequence Table Entry 0 */
	u32_t cmd; 		/**< ADC Command Register  */
	u32_t intr_status;	/**< ADC Interrupt Status Register */
	u32_t intr_enable;	/**< ADC Interrupt Enable Register */
	u32_t sample;		/**< ADC Sample Register */
	u32_t calibration;	/**< ADC Calibration Data Register */
	u32_t fifo_count;	/**< ADC FIFO Count Register */
	u32_t op_mode;		/**< ADC Operating Mode Register */
} adc_reg_t;

struct adc_quark_d2000_config {
	adc_reg_t *reg_base;
	void (*config_func)(struct device *dev);
};

struct adc_quark_d2000_info {
	struct device *dev;
	struct adc_context ctx;
	u16_t *buffer;
	u32_t active_channels;
	u32_t channels;
	u8_t channel_id;

	/** Sequence entries array */
	const struct adc_sequence *entries;

	/** Sequence size */
	u8_t seq_size;

	/** Resolution value (mapped) */
	u8_t resolution;
};

static struct adc_quark_d2000_info adc_quark_d2000_data_0 = {
	ADC_CONTEXT_INIT_TIMER(adc_quark_d2000_data_0, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_quark_d2000_data_0, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_quark_d2000_data_0, ctx),
};

static void adc_quark_d2000_set_mode(struct device *dev, int mode)
{
	const struct adc_quark_d2000_config *config = dev->config->config_info;
	volatile adc_reg_t *adc_regs = config->reg_base;

	/* Set mode and wait for change */
	adc_regs->op_mode = mode;
	while ((adc_regs->op_mode & ADC_OP_MODE_OM_MASK) != mode)
		;

	/* Perform a dummy conversion if going into normal mode */
	if (mode >= ADC_MODE_NORM_CAL) {
		/* setup sequence table */
		adc_regs->seq[0] = ADC_CAL_SEQ_TABLE_DEFAULT;

		/* clear command complete interrupt */
		adc_regs->intr_status = ADC_INTR_STATUS_CC;

		/* run dummy conversion and wait for completion */
		adc_regs->cmd = (ADC_CMD_IE | ADC_CMD_START_SINGLE);
		while (!(adc_regs->intr_status & ADC_INTR_STATUS_CC))
			;

		/* flush FIFO */
		adc_regs->sample = ADC_FIFO_CLEAR;

		/* clear command complete interrupt (again) */
		adc_regs->intr_status = ADC_INTR_STATUS_CC;
	}
}

#ifdef CONFIG_ADC_INTEL_QUARK_D2000_CALIBRATION
static void adc_quark_d2000_goto_normal_mode(struct device *dev)
{
	const struct adc_quark_d2000_config *config = dev->config->config_info;
	volatile adc_reg_t *adc_regs = config->reg_base;

	/* Set controller mode*/
	adc_quark_d2000_set_mode(dev, ADC_MODE_NORM_CAL);

	/* Perform calibration */

	/* clear command complete interrupt */
	adc_regs->intr_status = ADC_INTR_STATUS_CC;

	/* start the calibration and wait for completion */
	adc_regs->cmd = (ADC_CMD_IE | ADC_CMD_START_CAL);
	while (!(adc_regs->intr_status & ADC_INTR_STATUS_CC))
		;

	/* clear command complete interrupt */
	adc_regs->intr_status = ADC_INTR_STATUS_CC;
}
#else
static void adc_quark_d2000_goto_normal_mode(struct device *dev)
{
	adc_quark_d2000_set_mode(dev, ADC_MODE_NORM_NO_CAL);
}
#endif

static void adc_quark_d2000_enable(struct device *dev)
{
	adc_quark_d2000_goto_normal_mode(dev);
}

static int adc_quark_d2000_channel_setup(struct device *dev,
				const struct adc_channel_cfg *channel_cfg)
{
	struct adc_quark_d2000_info *info = dev->driver_data;
	u8_t channel_id = channel_cfg->channel_id;

	if (channel_id > MAX_CHANNELS) {
		SYS_LOG_ERR("Channel %d is not valid", channel_id);
		return -EINVAL;
	}

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		SYS_LOG_ERR("Invalid channel acquisition time");
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		SYS_LOG_ERR("Differential channels are not supported");
		return -EINVAL;
	}

	if (channel_cfg->gain != ADC_GAIN_1) {
		SYS_LOG_ERR("Invalid channel gain");
		return -EINVAL;
	}

	if (channel_cfg->reference != ADC_REF_INTERNAL) {
		SYS_LOG_ERR("Invalid channel reference");
		return -EINVAL;
	}

	info->active_channels |= 1 << channel_id;
	return 0;
}

static int adc_quark_d2000_read_request(struct device *dev,
					const struct adc_sequence *seq_tbl)
{
	struct adc_quark_d2000_info *info = dev->driver_data;
	int error;
	u32_t utmp, num_channels;

	/* hardware requires minimum 10 us delay between consecutive samples */
	if (seq_tbl->options &&
	    seq_tbl->options->extra_samplings &&
	    seq_tbl->options->interval_us < 10) {
		return -EINVAL;
	}

	info->channels = seq_tbl->channels & info->active_channels;

	if (seq_tbl->channels != info->channels) {
		return -EINVAL;
	}

	/* make sure resolution is valid */
	switch (seq_tbl->resolution) {
	case 6:
	case 8:
	case 10:
	case 12:
		info->resolution = (seq_tbl->resolution / 2) - 3;
		break;
	default:
		return -EINVAL;
	}

	info->entries = seq_tbl;
	info->buffer = (u16_t *)seq_tbl->buffer;

	if (seq_tbl->options) {
		info->seq_size = seq_tbl->options->extra_samplings + 1;
	} else {
		info->seq_size = 1;
	}

	if (info->seq_size > ADC_FIFO_LEN) {
		return -EINVAL;
	}

	/* check if buffer has enough size */
	utmp = info->channels;
	num_channels = 0;
	while (utmp) {
		if (utmp & BIT(0)) {
			num_channels++;
		}
		utmp >>= 1;
	}
	utmp = info->seq_size * num_channels * sizeof(u16_t);
	if (utmp > seq_tbl->buffer_size) {
		return -EINVAL;
	}

	adc_context_start_read(&info->ctx, seq_tbl);
	error = adc_context_wait_for_completion(&info->ctx);
	adc_context_release(&info->ctx, error);

	return 0;
}

static int adc_quark_d2000_read(struct device *dev,
			   const struct adc_sequence *sequence)
{
	struct adc_quark_d2000_info *info = dev->driver_data;

	adc_context_lock(&info->ctx, false, NULL);

	return adc_quark_d2000_read_request(dev, sequence);
}

#ifdef CONFIG_ADC_ASYNC
static int adc_quark_d2000_read_async(struct device *dev,
				      const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	struct adc_quark_d2000_info *info = dev->driver_data;

	adc_context_lock(&info->ctx, true, async);

	return adc_quark_d2000_read_request(dev, sequence);
}
#endif

static void adc_quark_d2000_start_conversion(struct device *dev)
{
	struct adc_quark_d2000_info *info = dev->driver_data;
	const struct adc_quark_d2000_config *config =
		info->dev->config->config_info;
	const struct adc_sequence *entry = info->ctx.sequence;
	volatile adc_reg_t *adc_regs = config->reg_base;
	u32_t i, val, interval_us = 0;
	u32_t idx = 0, offset = 0;

	info->channel_id = find_lsb_set(info->channels) - 1;

	if (entry->options) {
		interval_us = entry->options->interval_us;
	}

	/* flush the FIFO */
	adc_regs->sample = ADC_FIFO_CLEAR;

	/* setup the sequence table */
	for (i = 0; i < info->seq_size; i++) {
		idx = i / 4;
		offset = (i % 4) * 8;

		val = adc_regs->seq[idx];

		/* clear last of sequence bit */
		val &= ~(1 << (offset + 7));

		/* set channel number */
		val |= (info->channel_id << offset);

		adc_regs->seq[idx] = val;
	}

	/* set last of sequence bit */
	if (info->seq_size > 1) {
		val = adc_regs->seq[idx];
		val |= (1 << (offset + 7));
		adc_regs->seq[idx] = val;
	}

	/* clear pending interrupts */
	adc_regs->intr_status = ADC_INTR_STATUS_CC;

	/* enable command completion interrupts */
	adc_regs->intr_enable = ADC_INTR_ENABLE_CC;

	/* issue command to start conversion */
	val = interval_us << ADC_CMD_SW_OFFSET;
	val |= info->resolution << ADC_CMD_RESOLUTION_OFFSET;
	val |= (ADC_CMD_IE | ADC_CMD_START_SINGLE);
	adc_regs->cmd = val;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_quark_d2000_info *info =
		CONTAINER_OF(ctx, struct adc_quark_d2000_info, ctx);

	info->channels = ctx->sequence->channels;

	adc_quark_d2000_start_conversion(info->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_quark_d2000_info *info =
		CONTAINER_OF(ctx, struct adc_quark_d2000_info, ctx);
	const struct adc_sequence *entry = ctx->sequence;

	if (repeat) {
		info->buffer = (u16_t *)entry->buffer;
	}
}

static int adc_quark_d2000_init(struct device *dev)
{
	const struct adc_quark_d2000_config *config =
		dev->config->config_info;
	struct adc_quark_d2000_info *info = dev->driver_data;
	u32_t val;

	/* Enable the ADC and set the clock divisor */
	val = sys_read32(REG_CCU_PERIPH_CLK_GATE_CTL);
	val |= (CLK_PERIPH_CLK | CLK_PERIPH_ADC | CLK_PERIPH_ADC_REGISTER);
	sys_write32(val, REG_CCU_PERIPH_CLK_GATE_CTL);

	/* ADC clock divider */
	val = sys_read32(REG_CCU_PERIPH_CLK_DIV_CTL0);
	val &= ~CLK_DIV_ADC_MASK;
	val |= ((CONFIG_ADC_INTEL_QUARK_D2000_CLOCK_RATIO - 1)
		<< CLK_DIV_ADC_POS) & CLK_DIV_ADC_MASK;
	sys_write32(val, REG_CCU_PERIPH_CLK_DIV_CTL0);

	/* Clear host interrupt mask */
	val = sys_read32(REG_INT_ADC_PWR_MASK);
	val &= ~1;
	sys_write32(val, REG_INT_ADC_PWR_MASK);
	val = sys_read32(REG_INT_ADC_CALIB_MASK);
	val &= ~1;
	sys_write32(val, REG_INT_ADC_CALIB_MASK);

	config->config_func(dev);
	info->dev = dev;

	adc_quark_d2000_enable(dev);
	adc_context_unlock_unconditionally(&info->ctx);

	return 0;
}

static void adc_quark_d2000_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct adc_quark_d2000_config *config = dev->config->config_info;
	struct adc_quark_d2000_info *info = dev->driver_data;
	volatile adc_reg_t *adc_regs = config->reg_base;
	u32_t intr_status;
	u32_t to_read, val;

	intr_status = adc_regs->intr_status;

	/* single conversion command completion */
	if (intr_status & ADC_INTR_STATUS_CC) {
		adc_regs->intr_status = ADC_INTR_STATUS_CC;

		to_read = adc_regs->fifo_count;

		while (to_read--) {
			/* read from FIFO */
			val = adc_regs->sample;

			/* sample is always 12-bit, so need to shift */
			val = val >> (2 * (3 - info->resolution));

			*info->buffer++ = val;
		}
	}

	/* setup for next conversion if needed */
	info->channels &= ~BIT(info->channel_id);

	if (info->channels) {
		adc_quark_d2000_start_conversion(dev);
	} else {
		adc_context_on_sampling_done(&info->ctx, dev);
	}
}

static const struct adc_driver_api adc_quark_d2000_driver_api = {
	.channel_setup = adc_quark_d2000_channel_setup,
	.read = adc_quark_d2000_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_quark_d2000_read_async,
#endif
};

#if CONFIG_ADC_0
static void adc_quark_d2000_config_func_0(struct device *dev);

static const struct adc_quark_d2000_config adc_quark_d2000_config_0 = {
	.reg_base = (adc_reg_t *)CONFIG_ADC_0_BASE_ADDRESS,
	.config_func = adc_quark_d2000_config_func_0,
};

DEVICE_AND_API_INIT(adc_quark_d2000_0, CONFIG_ADC_0_NAME,
		    &adc_quark_d2000_init, &adc_quark_d2000_data_0,
		    &adc_quark_d2000_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &adc_quark_d2000_driver_api);

static void adc_quark_d2000_config_func_0(struct device *dev)
{
	IRQ_CONNECT(CONFIG_ADC_0_IRQ, CONFIG_ADC_0_IRQ_PRI,
		    adc_quark_d2000_isr,
		    DEVICE_GET(adc_quark_d2000_0),
		    CONFIG_ADC_0_IRQ_FLAGS);

	irq_enable(CONFIG_ADC_0_IRQ);
}
#endif /* CONFIG_ADC_0 */
