/*
 * Copyright (c) 2015-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel Quark SE C1000 Sensor Subsystem ADC Driver
 *
 * This is the driver for the ADC block in the Intel Quark SE C1000
 * Sensor Subsystem.
 */

#include <errno.h>

#include <init.h>
#include <kernel.h>
#include <string.h>
#include <stdlib.h>
#include <soc.h>
#include <adc.h>
#include <arch/cpu.h>
#include <misc/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include "adc_intel_quark_se_c1000_ss.h"

#define LOG_LEVEL CONFIG_ADC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(adc_intel_quark_se_c1000_ss);

/* MST */
#define ADC_CLOCK_GATE		BIT(31)
#define ADC_CAL_REQ		BIT(16)
#define ADC_DEEP_POWER_DOWN	0x01
#define ADC_POWER_DOWN		0x01
#define ADC_STANDBY		0x02
#define ADC_NORMAL_WITH_CALIB	0x03
#define ADC_NORMAL_WO_CALIB	0x04
#define ADC_MODE_MASK		0x07
#define ADC_DELAY_MASK		(0xFFF8)
#define ADC_DELAY_POS		3
#define ADC_DELAY_32MHZ		(160 << ADC_DELAY_POS)

/* SLV0 */
#define ADC_CAL_ACK		BIT(4)
#define ADC_PWR_MODE_STS	BIT(3)

#define ONE_BIT_SET		0x1
#define THREE_BITS_SET		0x7
#define FIVE_BITS_SET		0x1f
#define SIX_BITS_SET		0x3f
#define SEVEN_BITS_SET		0xef
#define ELEVEN_BITS_SET		0x7ff

#define CAPTURE_MODE_POS	6
#define OUTPUT_MODE_POS		7
#define SERIAL_DELAY_POS	8
#define SEQUENCE_MODE_POS	13
#define SEQ_ENTRIES_POS		16
#define THRESHOLD_POS		24

#define SEQ_MUX_EVEN_POS	0
#define SEQ_DELAY_EVEN_POS	5
#define SEQ_MUX_ODD_POS		16
#define SEQ_DELAY_ODD_POS	21

#define ADC_NONE_CALIBRATION	(0x80)

#ifdef CONFIG_SOC_QUARK_SE_C1000_SS
#define int_unmask(__mask)                                             \
	sys_write32(sys_read32((__mask)) & ENABLE_SSS_INTERRUPTS, (__mask))
#else
#define int_unmask(...) { ; }
#endif
static void adc_config_irq(void);


struct adc_info adc_info_dev = {
	ADC_CONTEXT_INIT_TIMER(adc_info_dev, ctx),
	ADC_CONTEXT_INIT_LOCK(adc_info_dev, ctx),
	ADC_CONTEXT_INIT_SYNC(adc_info_dev, ctx),
	.state = ADC_STATE_IDLE,
#ifdef CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_CALIBRATION
	.calibration_value = ADC_NONE_CALIBRATION,
#endif
};

static inline void wait_slv0_bit_set(u32_t bit_mask)
{
	u32_t reg_value;

	do {
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
	} while ((reg_value & bit_mask) == 0U);
}

static void set_power_mode_inner(u32_t mode)
{
	u32_t reg_value;
	u32_t state;

	state = irq_lock();

	/* Request Power Down first before transitioning */
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);

	reg_value &= ~(ADC_MODE_MASK);
	reg_value |= mode;

	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);

	irq_unlock(state);

	/* Wait for power mode to be set */
	wait_slv0_bit_set(ADC_PWR_MODE_STS);
}

static void set_power_mode(u32_t mode)
{
	u32_t reg_value;

	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);

	/* no need to set if power mode is the same as requested */
	if ((reg_value & ADC_MODE_MASK) == mode) {
		return;
	}

	/* Request Power Down first before transitioning */
	set_power_mode_inner(ADC_POWER_DOWN);

	/* Then set to the desired mode */
	set_power_mode_inner(mode);
}

/*
 * A dummy conversion is needed after coming out of deep
 * power down, or else the first conversion would not be
 * correct.
 */
static void dummy_conversion(struct device *dev)
{
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;
	u32_t reg_value;

	/* Flush FIFO */
	reg_value = sys_in32(adc_base + ADC_SET);
	reg_value |= ADC_SET_FLUSH_RX;
	sys_out32(reg_value, adc_base + ADC_SET);

	/* Reset sequence table */
	reg_value = sys_in32(adc_base + ADC_CTRL);
	reg_value |= ADC_CTRL_SEQ_TABLE_RST;
	sys_out32(reg_value, adc_base + ADC_CTRL);

	/* Setup sequence table for dummy */
	sys_out32((10 << 5), adc_base + ADC_SEQ);

	/*
	 * Set number of entries in sequencer (n-1)
	 * and threshold to generate interrupt.
	 */
	reg_value = sys_in32(adc_base + ADC_SET);
	reg_value &= ~(ADC_SET_SEQ_ENTRIES_MASK | ADC_SET_THRESHOLD_MASK);
	sys_out32(reg_value, adc_base + ADC_SET);

	/*
	 * Reset sequence pointer,
	 * Clear and mask interrupts,
	 * Enable ADC and start sequencer.
	 */
	reg_value = sys_in32(adc_base + ADC_CTRL);
	reg_value |= ADC_CTRL_SEQ_PTR_RST | ADC_CTRL_INT_CLR_ALL |
		     ADC_CTRL_INT_MASK_ALL | ADC_CTRL_ENABLE |
		     ADC_CTRL_SEQ_START;
	sys_out32(reg_value, adc_base + ADC_CTRL);

	/* Wait for data available */
	do {
		reg_value = sys_in32(adc_base + ADC_INTSTAT);
	} while ((reg_value & ADC_INTSTAT_DATA_A) == 0U);

	/* Flush FIFO */
	reg_value = sys_in32(adc_base + ADC_SET);
	reg_value |= ADC_SET_FLUSH_RX;
	sys_out32(reg_value, adc_base + ADC_SET);

	/* Clear data available interrupt and disable ADC */
	reg_value = sys_in32(adc_base + ADC_CTRL);
	reg_value |= ADC_CTRL_CLR_DATA_A;
	reg_value &= ~ADC_CTRL_ENABLE;
	sys_out32(reg_value, adc_base + ADC_CTRL);
}

#ifdef CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_CALIBRATION
static void calibration_command(u8_t command)
{
	u32_t state;
	u32_t reg_value;

	/* Set Calibration Request */
	state = irq_lock();
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value |= (command & THREE_BITS_SET) << 17;
	reg_value |= ADC_CAL_REQ;
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
	irq_unlock(state);

	/* Waiting for calibration ack */
	wait_slv0_bit_set(ADC_CAL_ACK);

	/* Clear Calibration Request once done */
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value &= ~ADC_CAL_REQ;
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
}

static void adc_goto_normal_mode(struct device *dev)
{
	struct adc_info *info = dev->driver_data;
	u8_t calibration_value;
	u32_t reg_value;

	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);

	if ((reg_value & ADC_MODE_MASK) != ADC_NORMAL_WITH_CALIB) {

		/* Request Normal With Calibration Mode */
		set_power_mode(ADC_NORMAL_WITH_CALIB);

		/* Poll waiting for normal mode with calibration */
		wait_slv0_bit_set(ADC_PWR_MODE_STS);

		if (info->calibration_value == ADC_NONE_CALIBRATION) {
			/* Reset Calibration */
			calibration_command(ADC_CMD_RESET_CALIBRATION);
			/* Request Calibration */
			calibration_command(ADC_CMD_START_CALIBRATION);
			reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
			calibration_value = (reg_value >> 5) & SEVEN_BITS_SET;
			info->calibration_value = calibration_value;
		}

		/* Load Calibration */
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
		reg_value |= (info->calibration_value << 20);
		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
		calibration_command(ADC_CMD_LOAD_CALIBRATION);
	}

	dummy_conversion(dev);
}

#else
static void adc_goto_normal_mode(struct device *dev)
{
	ARG_UNUSED(dev);

	/* Request Normal Without Calibration Mode */
	set_power_mode(ADC_NORMAL_WO_CALIB);

	dummy_conversion(dev);
}
#endif

static int set_resolution(struct device *dev,
			   const struct adc_sequence *sequence)
{
	u32_t tmp_val;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ~FIVE_BITS_SET;

	switch (sequence->resolution) {
	case 6:
		break;
	case 8:
		tmp_val |= 1 & FIVE_BITS_SET;
		break;
	case 10:
		tmp_val |= 2 & FIVE_BITS_SET;
		break;
	case 12:
		tmp_val |= 3 & FIVE_BITS_SET;
		break;
	default:
		return -EINVAL;
	}

	sys_out32(tmp_val, adc_base + ADC_SET);

	return 0;
}

/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_quark_se_ss_channel_setup(
	struct device *dev,
	const struct adc_channel_cfg *channel_cfg
	)
{
	u8_t channel_id = channel_cfg->channel_id;
	struct adc_info *info = dev->driver_data;

	if (channel_id >= DW_CHANNEL_COUNT) {
		LOG_ERR("Invalid channel id");
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

	if (channel_cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("Invalid channel acquisition time");
		return -EINVAL;
	}

	if (info->state != ADC_STATE_IDLE) {
		LOG_ERR("ADC is busy or in error state");
		return -EAGAIN;
	}

	info->active_channels |= 1 << channel_id;
	return 0;
}

static int adc_quark_se_ss_read_request(struct device *dev,
					const struct adc_sequence *seq_tbl)
{
	struct adc_info *info = dev->driver_data;
	int error = 0;
	u32_t utmp, num_channels, interval = 0U;

	info->channels = seq_tbl->channels & info->active_channels;

	if (seq_tbl->channels != info->channels) {
		return -EINVAL;
	}

	error = set_resolution(dev, seq_tbl);
	if (error) {
		return error;
	}

	/*
	 * Make sure the requested interval is longer than the time
	 * needed to do one conversion.
	 */
	if (seq_tbl->options &&
	    (seq_tbl->options->interval_us > 0)) {
		/*
		 * System clock is 32MHz, which means 1us == 32 cycles
		 * if divider is 1.
		 */
		interval = seq_tbl->options->interval_us * 32U /
			   CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_CLOCK_RATIO;

		if (interval < (seq_tbl->resolution + 2)) {
			return -EINVAL;
		}
	}

	info->entries = seq_tbl;
	info->buffer = (u16_t *)seq_tbl->buffer;

	/* check if buffer has enough size */
	utmp = info->channels;
	num_channels = 0U;
	while (utmp) {
		if (utmp & BIT(0)) {
			num_channels++;
		}
		utmp >>= 1;
	}
	utmp = num_channels * sizeof(u16_t);

	if (seq_tbl->options) {
		utmp *= (1 + seq_tbl->options->extra_samplings);
	}

	if (utmp > seq_tbl->buffer_size) {
		return -ENOMEM;
	}

	info->state = ADC_STATE_SAMPLING;

	adc_context_start_read(&info->ctx, seq_tbl);
	error = adc_context_wait_for_completion(&info->ctx);

	if (info->state == ADC_STATE_ERROR) {
		info->state = ADC_STATE_IDLE;
		return -EIO;
	}

	return error;
}

static int adc_quark_se_ss_read(struct device *dev,
				const struct adc_sequence *seq_tbl)
{
	struct adc_info *info = dev->driver_data;
	int ret;

	adc_context_lock(&info->ctx, false, NULL);
	ret = adc_quark_se_ss_read_request(dev, seq_tbl);
	adc_context_release(&info->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
/* Implementation of the ADC driver API function: adc_read_async. */
static int adc_quark_se_ss_read_async(struct device *dev,
				      const struct adc_sequence *sequence,
				      struct k_poll_signal *async)
{
	struct adc_info *info = dev->driver_data;
	int ret;

	adc_context_lock(&info->ctx, true, async);
	ret = adc_quark_se_ss_read_request(dev, sequence);
	adc_context_release(&info->ctx, ret);

	return ret;
}
#endif

static void adc_quark_se_ss_start_conversion(struct device *dev)
{
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = info->dev->config->config_info;
	const struct adc_sequence *entry = &info->ctx.sequence;
	u32_t adc_base = config->reg_base;
	u32_t ctrl, tmp_val, sample_window;

	info->channel_id = find_lsb_set(info->channels) - 1;

	/* Flush FIFO */
	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val |= ADC_SET_FLUSH_RX;
	sys_out32(tmp_val, adc_base + ADC_SET);

	/* Reset sequence table */
	ctrl = sys_in32(adc_base + ADC_CTRL);
	ctrl |= ADC_CTRL_SEQ_TABLE_RST;
	sys_out32(ctrl, adc_base + ADC_CTRL);

	/*
	 * Hardware requires min (resolution + 2) cycles,
	 * or will emit SEQERROR.
	 */
	sample_window = entry->resolution + 2;
	tmp_val = (sample_window & ELEVEN_BITS_SET) << SEQ_DELAY_EVEN_POS;
	tmp_val |= (info->channel_id & FIVE_BITS_SET);

	sys_out32(tmp_val, adc_base + ADC_SEQ);

	/*
	 * Clear number of entries in sequencer and threshold to generate
	 * interrupt, since only 1 conversion is needed and fields are
	 * zero-based.
	 */
	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ~(ADC_SET_SEQ_ENTRIES_MASK | ADC_SET_THRESHOLD_MASK);
	sys_out32(tmp_val, adc_base + ADC_SET);

	/*
	 * Reset sequence pointer,
	 * Clear and unmask interrupts,
	 * Enable ADC and start sequencer.
	 */
	ctrl = sys_in32(adc_base + ADC_CTRL);
	ctrl &= ~ADC_CTRL_INT_MASK_ALL;
	ctrl |= ADC_CTRL_SEQ_PTR_RST | ADC_CTRL_INT_CLR_ALL |
		ADC_CTRL_ENABLE | ADC_CTRL_SEQ_START;
	sys_out32(ctrl, adc_base + ADC_CTRL);
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_info *info = CONTAINER_OF(ctx, struct adc_info, ctx);

	info->channels = ctx->sequence.channels;

	adc_quark_se_ss_start_conversion(info->dev);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	struct adc_info *info = CONTAINER_OF(ctx, struct adc_info, ctx);
	const struct adc_sequence *entry = &ctx->sequence;

	if (repeat) {
		info->buffer = (u16_t *)entry->buffer;
	}
}

int adc_quark_se_ss_init(struct device *dev)
{
	u32_t val;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;
	struct adc_info *info = dev->driver_data;

	/* Disable clock gating */
	val = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	val &= ~(ADC_CLOCK_GATE);
	sys_out32(val, PERIPH_ADDR_BASE_CREG_MST0);

	/* Mask all interrupts and enable clock */
	val = ADC_CTRL_INT_MASK_ALL | ADC_CTRL_CLK_ENABLE;
	sys_out32(val, adc_base + ADC_CTRL);

	/* Configure common properties */
	val = ((config->capture_mode & ONE_BIT_SET) << CAPTURE_MODE_POS);
	val |= ((config->out_mode & ONE_BIT_SET) << OUTPUT_MODE_POS);
	val |= ((config->serial_dly & FIVE_BITS_SET) << SERIAL_DELAY_POS);
	val |= ((config->seq_mode & ONE_BIT_SET) << SEQUENCE_MODE_POS);
	val &= ~(ADC_SET_INPUT_MODE_MASK);
	sys_out32(val, adc_base + ADC_SET);

	/* Set the clock ratio */
	sys_out32(config->clock_ratio & ADC_DIVSEQSTAT_CLK_RATIO_MASK,
		  adc_base + ADC_DIVSEQSTAT);

	config->config_func();

	int_unmask(config->reg_irq_mask);
	int_unmask(config->reg_err_mask);

	info->dev = dev;

	adc_goto_normal_mode(dev);

	info->state = ADC_STATE_IDLE;

	adc_context_unlock_unconditionally(&info->ctx);
	return 0;
}

static void adc_quark_se_ss_rx_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	struct adc_sequence *seq = &info->ctx.sequence;
	u32_t adc_base = config->reg_base;
	u32_t reg_val;

	/* Pop data from FIFO and put it into buffer */
	reg_val = sys_in32(adc_base + ADC_SET);
	reg_val |= ADC_SET_POP_RX;
	sys_out32(reg_val, adc_base + ADC_SET);

	/* Sample is always 12-bit, so need to shift */
	*info->buffer++ = sys_in32(adc_base + ADC_SAMPLE) >>
			  (12 - seq->resolution);

	/* Clear data available register */
	reg_val = sys_in32(adc_base + ADC_CTRL);
	reg_val |= ADC_CTRL_CLR_DATA_A;
	sys_out32(reg_val, adc_base + ADC_CTRL);

	/* Stop sequencer and mask all interrupts */
	reg_val = sys_in32(adc_base + ADC_CTRL);
	reg_val &= ~ADC_CTRL_SEQ_START;
	reg_val |= ADC_CTRL_INT_MASK_ALL;
	sys_out32(reg_val, adc_base + ADC_CTRL);

	/* Disable ADC */
	reg_val = sys_in32(adc_base + ADC_CTRL);
	reg_val &= ~ADC_CTRL_ENABLE;
	sys_out32(reg_val, adc_base + ADC_CTRL);

	info->state = ADC_STATE_IDLE;
	info->channels &= ~BIT(info->channel_id);

	if (info->channels) {
		adc_quark_se_ss_start_conversion(dev);
	} else {
		adc_context_on_sampling_done(&info->ctx, dev);
	}
}

static void adc_quark_se_ss_err_isr(void *arg)
{
	struct device *dev = (struct device *) arg;
	const struct adc_config *config = dev->config->config_info;
	struct adc_info *info = dev->driver_data;
	u32_t adc_base = config->reg_base;
	u32_t reg_val = sys_in32(adc_base + ADC_SET);

	/*
	 * Stop sequencer, mask/clear all interrupts,
	 * and disable ADC.
	 */
	reg_val = sys_in32(adc_base + ADC_CTRL);
	reg_val &= ~(ADC_CTRL_SEQ_START | ADC_CTRL_ENABLE);
	reg_val |= ADC_CTRL_INT_MASK_ALL;
	reg_val |= ADC_CTRL_INT_CLR_ALL;
	sys_out32(reg_val, adc_base + ADC_CTRL);

	info->state = ADC_STATE_ERROR;
	adc_context_on_sampling_done(&info->ctx, dev);
}

static const struct adc_driver_api api_funcs = {
	.channel_setup = adc_quark_se_ss_channel_setup,
	.read          = adc_quark_se_ss_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_quark_se_ss_read_async,
#endif
};

const static struct adc_config adc_config_dev = {
	.reg_base = DT_ADC_0_BASE_ADDRESS,
	.reg_irq_mask = SCSS_REGISTER_BASE + INT_SS_ADC_IRQ_MASK,
	.reg_err_mask = SCSS_REGISTER_BASE + INT_SS_ADC_ERR_MASK,
#ifdef CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_SERIAL
	.out_mode     = 0,
#elif CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_PARALLEL
	.out_mode     = 1,
#endif
	.seq_mode = 0,

#ifdef CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_RISING_EDGE
	.capture_mode = 0,
#elif CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_FALLING_EDGE
	.capture_mode = 1,
#endif
	.clock_ratio  = CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_CLOCK_RATIO,
	.serial_dly   = CONFIG_ADC_INTEL_QUARK_SE_C1000_SS_SERIAL_DELAY,
	.config_func  = adc_config_irq,
};

DEVICE_AND_API_INIT(adc_quark_se_ss, DT_ADC_0_NAME, &adc_quark_se_ss_init,
		    &adc_info_dev, &adc_config_dev,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);

static void adc_config_irq(void)
{
	IRQ_CONNECT(DT_ADC_0_IRQ, DT_ADC_0_IRQ_PRI, adc_quark_se_ss_rx_isr,
		    DEVICE_GET(adc_quark_se_ss), 0);
	irq_enable(DT_ADC_0_IRQ);

	IRQ_CONNECT(DT_ADC_IRQ_ERR, DT_ADC_0_IRQ_PRI,
		    adc_quark_se_ss_err_isr, DEVICE_GET(adc_quark_se_ss), 0);
	irq_enable(DT_ADC_IRQ_ERR);
}
