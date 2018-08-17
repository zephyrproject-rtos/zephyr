/* adc_dw.c - Designware ADC driver */

/*
 * Copyright (c) 2015 Intel Corporation
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
#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"
#include "adc_dw.h"
#include <logging/sys_log.h>

#define ADC_CLOCK_GATE      (1 << 31)
#define ADC_POWER_DOWN       0x01
#define ADC_STANDBY          0x02
#define ADC_NORMAL_WITH_CALIB  0x03
#define ADC_NORMAL_WO_CALIB    0x04
#define ADC_MODE_MASK        0x07

#define ONE_BIT_SET     0x1
#define THREE_BITS_SET   0x7
#define FIVE_BITS_SET   0x1f
#define SIX_BITS_SET    0x3f
#define SEVEN_BITS_SET  0xef
#define ELEVEN_BITS_SET 0x7ff

#define INPUT_MODE_POS     5
#define CAPTURE_MODE_POS   6
#define OUTPUT_MODE_POS    7
#define SERIAL_DELAY_POS   8
#define SEQUENCE_MODE_POS  13
#define SEQ_ENTRIES_POS    16
#define THRESHOLD_POS      24

#define SEQ_DELAY_EVEN_POS 5
#define SEQ_MUX_ODD_POS    16
#define SEQ_DELAY_ODD_POS  21

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
#ifdef CONFIG_ADC_DW_CALIBRATION
		.calibration_value = ADC_NONE_CALIBRATION,
#endif
	};

#ifdef CONFIG_ADC_DW_CALIBRATION
static void calibration_command(u8_t command)
{
	u32_t state;
	u32_t reg_value;

	state = irq_lock();
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value |= (command & THREE_BITS_SET) << 17;
	reg_value |= 0x10000;
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
	irq_unlock(state);
	/*Poll waiting for command*/
	do {
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
	} while ((reg_value & 0x10) == 0);

	/*Clear Calibration Request*/
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value &= ~(0x10000);
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
}

static void adc_goto_normal_mode(struct device *dev)
{
	struct adc_info *info = dev->driver_data;
	u8_t calibration_value;
	u32_t reg_value;
	u32_t state;

	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);

	if (((reg_value & 0xE) >> 1) != ADC_NORMAL_WITH_CALIB) {

		state = irq_lock();
		/*Request  Normal With Calibration Mode*/
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
		reg_value &= ~(ADC_MODE_MASK);
		reg_value |= ADC_NORMAL_WITH_CALIB;
		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
		irq_unlock(state);

		/*Poll waiting for normal mode*/
		do {
			reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
		} while ((reg_value & 0x1) == 0);

		if (info->calibration_value == ADC_NONE_CALIBRATION) {
			/*Reset Calibration*/
			calibration_command(ADC_CMD_RESET_CALIBRATION);
			/*Request Calibration*/
			calibration_command(ADC_CMD_START_CALIBRATION);
			reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
			calibration_value = (reg_value >> 5) & SEVEN_BITS_SET;
			info->calibration_value = calibration_value;
		}

		/*Load Calibration*/
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
		reg_value |= (info->calibration_value << 20);
		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
		calibration_command(ADC_CMD_LOAD_CALIBRATION);
	}
}

#else
static void adc_goto_normal_mode(struct device *dev)
{
	u32_t reg_value;
	u32_t state;

	ARG_UNUSED(dev);
	reg_value = sys_in32(
		PERIPH_ADDR_BASE_CREG_SLV0 + SLV_OBSR);

	if (((reg_value & 0xE) >> 1) == ADC_NORMAL_WO_CALIB) {
		state = irq_lock();
		/*Request  Power Down*/
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);

		reg_value &= ~(ADC_MODE_MASK);
		reg_value |= ADC_POWER_DOWN;

		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
		irq_unlock(state);

		do {
			reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
		} while ((reg_value & 0x1) == 0);
	}

	/*Request  Normal With Calibration Mode*/
	state = irq_lock();
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value &= ~(ADC_MODE_MASK);
	reg_value |= ADC_NORMAL_WO_CALIB;
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
	irq_unlock(state);

	/*Poll waiting for normal mode*/
	do {
		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
	} while ((reg_value & 0x1) == 0);
}
#endif

static void adc_goto_deep_power_down(void)
{
	u32_t reg_value;
	u32_t state;
	int i = 0;

	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
	if ((reg_value & 0xE >> 1) != 0) {
		state = irq_lock();

		reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);

		reg_value &= ~(ADC_MODE_MASK);

		reg_value |= 0 | ADC_CLOCK_GATE;
		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);

		irq_unlock(state);
		do {
			reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_SLV0);
			/*Don't hang system if power down fails*/
			if (i++ >= 10) {
				SYS_LOG_ERR("DW adc power down failed\n");
				break;
			}
		} while ((reg_value & 0x1) == 0);
	}

}

static void set_resolution(struct device *dev,
			  const struct adc_sequence *sequence)
{
	u32_t tmp_val;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ~FIVE_BITS_SET;

	tmp_val |= sequence->resolution & FIVE_BITS_SET;

	sys_out32(tmp_val, adc_base + ADC_SET);
}

static void adc_dw_enable(struct device *dev)
{
	u32_t reg_value;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;

	/*Go to Normal Mode*/
	sys_out32(ADC_INT_DSB|ENABLE_ADC, adc_base + ADC_CTRL);
	adc_goto_normal_mode(dev);

	/*Clock Gate*/
	reg_value = sys_in32(PERIPH_ADDR_BASE_CREG_MST0);
	reg_value &= ~(ADC_CLOCK_GATE);
	sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0);
	sys_out32(ENABLE_ADC, adc_base + ADC_CTRL);

	info->state = ADC_STATE_IDLE;
}

static void adc_dw_disable(struct device *dev)
{
	u32_t saved;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;

	sys_out32(ADC_INT_DSB|ENABLE_ADC, adc_base + ADC_CTRL);
	adc_goto_deep_power_down();
	sys_out32(ADC_INT_DSB|ADC_SEQ_PTR_RST, adc_base + ADC_CTRL);

	saved = irq_lock();

	sys_out32(sys_in32(adc_base + ADC_SET)|ADC_FLUSH_RX,
		  adc_base + ADC_SET);
	irq_unlock(saved);

	info->state = ADC_STATE_DISABLED;
}

/* Implementation of the ADC driver API function: adc_channel_setup. */
static int adc_dw_channel_setup(struct device *dev,
				  const struct adc_channel_cfg *channel_cfg)
{
	u8_t channel_id = channel_cfg->channel_id;
	struct adc_info *info = dev->driver_data;

	if (channel_id >= DW_CHANNEL_COUNT) {
		return -EINVAL;
	}

	info->active_channels |= 1 << channel_id;

	return 0;
}

static int adc_dw_read_request(struct device *dev,
			       const struct adc_sequence *seq_tbl)
{
	int i, hit = 0, channel_id = 0;
	int error = 0;
	u32_t saved;
	struct adc_info *info = dev->driver_data;

	/*hardware requires minimum 10 us delay between consecutive samples*/
	if (seq_tbl->options->extra_samplings &&
	    seq_tbl->options->interval_us < 10) {
		return -EINVAL;
	}

	for (i = 0; i < DW_CHANNEL_COUNT; i++) {
		if ((seq_tbl->channels >> i) & 0x1) {
			hit++;
			channel_id = i;
		}
	}

	/* multiple channels not supported to read, if bitmask "channels"
	 * has more than 1 bit set than return error
	 */
	if ((hit > 1) || !(seq_tbl->channels & info->active_channels)) {
		return -EINVAL;
	}

	adc_info_dev.channel_id = channel_id;

	saved = irq_lock();
	info->entries = seq_tbl;
	info->buffer = (u16_t *)seq_tbl->buffer;
	info->seq_size = seq_tbl->options->extra_samplings + 1;

	info->state = ADC_STATE_SAMPLING;
	irq_unlock(saved);

	set_resolution(dev, seq_tbl);
	adc_context_start_read(&adc_info_dev.ctx, seq_tbl);
	error = adc_context_wait_for_completion(&adc_info_dev.ctx);
	adc_context_release(&adc_info_dev.ctx, error);

	if (info->state == ADC_STATE_ERROR) {
		info->state = ADC_STATE_IDLE;
		return -EIO;
	}

	return 0;
}

static int adc_dw_read(struct device *dev, const struct adc_sequence *seq_tbl)
{
	int ret;

	adc_context_lock(&adc_info_dev.ctx, false, NULL);

	ret = adc_dw_read_request(dev, seq_tbl);
	adc_dw_disable(dev);
	return ret;
}

#ifdef CONFIG_ADC_ASYNC
/* Implementation of the ADC driver API function: adc_read_async. */
static int adc_dw_read_async(struct device *dev,
			       const struct adc_sequence *sequence,
			       struct k_poll_signal *async)
{

	adc_context_lock(&adc_info_dev.ctx, true, async);
	return adc_dw_read_request(dev, sequence);
}
#endif

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_info *info = CONTAINER_OF(ctx, struct adc_info, ctx);
	const struct adc_config *config = info->dev->config->config_info;
	const struct adc_sequence *entry = ctx->sequence;
	u32_t adc_base = config->reg_base;
	u32_t interval_us = entry->options->interval_us;
	u32_t tmp_val;
	u32_t ctrl;

	adc_dw_enable(adc_info_dev.dev);
	ctrl = sys_in32(adc_base + ADC_CTRL);
	ctrl |= ADC_SEQ_PTR_RST;
	sys_out32(ctrl, adc_base + ADC_CTRL);

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ADC_SEQ_SIZE_SET_MASK;
	sys_out32(tmp_val, adc_base + ADC_SET);

	tmp_val = ((interval_us & ELEVEN_BITS_SET)
			<< SEQ_DELAY_EVEN_POS);
	tmp_val |= (adc_info_dev.channel_id & FIVE_BITS_SET);

	sys_out32(tmp_val, adc_base + ADC_SEQ);
	sys_out32(ctrl | ADC_SEQ_PTR_RST, adc_base + ADC_CTRL);

	sys_out32(START_ADC_SEQ, adc_base + ADC_CTRL);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
					      bool repeat)
{
	ARG_UNUSED(ctx);

	if (!repeat) {
		adc_info_dev.buffer += 1;
	}
}

int adc_dw_init(struct device *dev)
{
	u32_t tmp_val;
	u32_t val;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;
	struct adc_info *info = dev->driver_data;

	sys_out32(ADC_INT_DSB | ADC_CLK_ENABLE, adc_base + ADC_CTRL);

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ADC_CONFIG_SET_MASK;
	val = ((config->capture_mode & ONE_BIT_SET) << CAPTURE_MODE_POS);
	val |= ((config->out_mode & ONE_BIT_SET) << OUTPUT_MODE_POS);
	val |= ((config->serial_dly & FIVE_BITS_SET) << SERIAL_DELAY_POS);
	val |= ((config->seq_mode & ONE_BIT_SET) << SEQUENCE_MODE_POS);
	val &= ~(1 << INPUT_MODE_POS);
	sys_out32(tmp_val|val, adc_base + ADC_SET);

	sys_out32(config->clock_ratio & ADC_CLK_RATIO_MASK,
		adc_base + ADC_DIVSEQSTAT);

	sys_out32(ADC_INT_ENABLE & ~(ADC_CLK_ENABLE),
		adc_base + ADC_CTRL);

	config->config_func();

	int_unmask(config->reg_irq_mask);
	int_unmask(config->reg_err_mask);

	info->dev = dev;

	adc_context_unlock_unconditionally(&adc_info_dev.ctx);
	return 0;
}

static void adc_dw_rx_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	u32_t adc_base = config->reg_base;
	u32_t reg_val;
	u16_t *adc_buffer;

	reg_val = sys_in32(adc_base + ADC_SET);
	sys_out32(reg_val|ADC_POP_SAMPLE, adc_base + ADC_SET);
	adc_buffer = (u16_t *)info->buffer;
	*adc_buffer = sys_in32(adc_base + ADC_SAMPLE);

	/*Resume ADC state to continue new conversions*/
	sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
	reg_val = sys_in32(adc_base + ADC_SET);
	sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_SET);

	/*Clear data A register*/
	reg_val = sys_in32(adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_CLR_DATA_A, adc_base + ADC_CTRL);

	info->state = ADC_STATE_IDLE;

	adc_context_on_sampling_done(&info->ctx, dev);
}

static void adc_dw_err_isr(void *arg)
{
	struct device *dev = (struct device *) arg;
	const struct adc_config  *config = dev->config->config_info;
	struct adc_info    *info   = dev->driver_data;
	u32_t adc_base = config->reg_base;
	u32_t reg_val = sys_in32(adc_base + ADC_SET);

	sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_CTRL);
	sys_out32(FLUSH_ADC_ERRORS, adc_base + ADC_CTRL);

	info->state = ADC_STATE_ERROR;
	adc_context_on_sampling_done(&adc_info_dev.ctx, dev);
}

static const struct adc_driver_api api_funcs = {
	.channel_setup = adc_dw_channel_setup,
	.read          = adc_dw_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async    = adc_dw_read_async,
#endif
};

static struct adc_config adc_config_dev = {
		.reg_base = CONFIG_ADC_0_BASE_ADDRESS,
		.reg_irq_mask = SCSS_REGISTER_BASE + INT_SS_ADC_IRQ_MASK,
		.reg_err_mask = SCSS_REGISTER_BASE + INT_SS_ADC_ERR_MASK,
#ifdef CONFIG_ADC_DW_SERIAL
		.out_mode     = 0,
#elif CONFIG_ADC_DW_PARALLEL
		.out_mode     = 1,
#endif
		.seq_mode = 0,

#ifdef CONFIG_ADC_DW_RISING_EDGE
		.capture_mode = 0,
#elif CONFIG_ADC_DW_FALLING_EDGE
		.capture_mode = 1,
#endif
		.clock_ratio  = CONFIG_ADC_DW_CLOCK_RATIO,
		.serial_dly   = CONFIG_ADC_DW_SERIAL_DELAY,
		.config_func  = adc_config_irq,
	};

DEVICE_AND_API_INIT(adc_dw, CONFIG_ADC_0_NAME, &adc_dw_init,
		    &adc_info_dev, &adc_config_dev,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);

static void adc_config_irq(void)
{
	IRQ_CONNECT(CONFIG_ADC_0_IRQ, CONFIG_ADC_0_IRQ_PRI, adc_dw_rx_isr,
		    DEVICE_GET(adc_dw), 0);
	irq_enable(CONFIG_ADC_0_IRQ);

	IRQ_CONNECT(CONFIG_ADC_IRQ_ERR, CONFIG_ADC_0_IRQ_PRI,
		    adc_dw_err_isr, DEVICE_GET(adc_dw), 0);
	irq_enable(CONFIG_ADC_IRQ_ERR);
}
