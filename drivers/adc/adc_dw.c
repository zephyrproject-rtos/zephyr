/* adc_dw.c - Designware ADC driver */

/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <init.h>
#include <nanokernel.h>
#include <string.h>
#include <stdlib.h>
#include <board.h>
#include <adc.h>
#include <arch/cpu.h>
#include "adc_dw.h"

#define ADC_CLOCK_GATE      (1 << 31)
#define ADC_STANDBY          0x02
#define ADC_NORMAL_WO_CALIB  0x04
#define ADC_MODE_MASK        0x07

#define ONE_BIT_SET     0x1
#define FIVE_BITS_SET   0x1f
#define SIX_BITS_SET    0x3f
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

#ifdef CONFIG_SOC_QUARK_SE_SS
#define int_unmask(__mask)                                             \
	sys_write32(sys_read32((__mask)) & ENABLE_SSS_INTERRUPTS, (__mask))
#else
#define int_unmask(...) { ; }
#endif
static void adc_config_0_irq(struct device *dev);

static void adc_goto_normal_mode_wo_calibration(void)
{
	uint32_t reg_value;
	uint32_t state;

	reg_value = sys_in32(
		PERIPH_ADDR_BASE_CREG_SLV0 + SLV_OBSR);

	if ((reg_value & ADC_MODE_MASK) != ADC_NORMAL_WO_CALIB) {
		state = irq_lock();
		reg_value = sys_in32(
			PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);

		reg_value &= ~(ADC_MODE_MASK);
		reg_value |= ADC_STANDBY | ADC_CLOCK_GATE;

		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);
		irq_unlock(state);

		do {
			reg_value = sys_in32(
				PERIPH_ADDR_BASE_CREG_SLV0 + SLV_OBSR) & 0x8;
		} while (reg_value == 0);

		state = irq_lock();
		reg_value = sys_in32(
			PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);

		reg_value &= ~(ADC_MODE_MASK);
		reg_value |= ADC_NORMAL_WO_CALIB | ADC_CLOCK_GATE;

		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);
		irq_unlock(state);

		do {
			reg_value = sys_in32(
				PERIPH_ADDR_BASE_CREG_SLV0 + SLV_OBSR) & 0x8;
		} while (reg_value == 0);
	}
}

static void adc_goto_deep_power_down(void)
{
	uint32_t reg_value;
	uint32_t state;

	reg_value = sys_in32(
		PERIPH_ADDR_BASE_CREG_SLV0+SLV_OBSR);
	if ((reg_value & ADC_MODE_MASK) != 0) {
		state = irq_lock();

		reg_value = sys_in32(
			PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);

		reg_value &= ~(ADC_MODE_MASK);

		reg_value |= 0 | ADC_CLOCK_GATE;
		sys_out32(reg_value, PERIPH_ADDR_BASE_CREG_MST0 + MST_CTL);

		irq_unlock(state);
		do {
			reg_value = sys_in32(
				PERIPH_ADDR_BASE_CREG_SLV0 + SLV_OBSR) & 0x8;
		} while (reg_value == 0);
	}
}

static void adc_dw_enable(struct device *dev)
{
	struct adc_info *info = dev->driver_data;
	struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

	adc_goto_normal_mode_wo_calibration();
	sys_out32(ENABLE_ADC, adc_base + ADC_CTRL);
	info->state = ADC_STATE_IDLE;
}

static void adc_dw_disable(struct device *dev)
{
	uint32_t saved;
	struct adc_info *info = dev->driver_data;
	struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

	adc_goto_deep_power_down();
	sys_out32(ADC_INT_DSB|ADC_SEQ_PTR_RST, adc_base + ADC_CTRL);

	saved = irq_lock();

	sys_out32(sys_in32(adc_base + ADC_SET)|ADC_FLUSH_RX, adc_base + ADC_SET);
	irq_unlock(saved);

	info->state = ADC_STATE_DISABLED;
}

static int adc_dw_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	uint32_t i;
	uint32_t ctrl;
	uint32_t tmp_val;
	uint32_t num_iters;
	uint32_t saved;
	struct adc_seq_entry *entry;
	struct adc_info *info = dev->driver_data;
	struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

	if (info->state != ADC_STATE_IDLE) {
		return 1;
	}

	saved = irq_lock();
	info->seq_size = seq_tbl->num_entries;

	ctrl = sys_in32(adc_base + ADC_CTRL);
	ctrl |= ADC_SEQ_PTR_RST;
	sys_out32(ctrl, adc_base + ADC_CTRL);

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ADC_SEQ_SIZE_SET_MASK;
	tmp_val |= (((seq_tbl->num_entries - 1) & SIX_BITS_SET)
		<< SEQ_ENTRIES_POS);
	tmp_val |= ((seq_tbl->num_entries - 1) << THRESHOLD_POS);
	sys_out32(tmp_val, adc_base + ADC_SET);

	irq_unlock(saved);

	num_iters = seq_tbl->num_entries/2;

	for (i = 0, entry = seq_tbl->entries;
		i < num_iters; i++, entry += 2) {
		tmp_val = ((entry[1].sampling_delay & ELEVEN_BITS_SET)
			<< SEQ_DELAY_ODD_POS);
		tmp_val |= ((entry[1].channel_id & FIVE_BITS_SET)
			<< SEQ_MUX_ODD_POS);
		tmp_val |= ((entry[0].sampling_delay & ELEVEN_BITS_SET)
			<< SEQ_DELAY_EVEN_POS);
		tmp_val |= (entry[0].channel_id & FIVE_BITS_SET);
		sys_out32(tmp_val, adc_base + ADC_SEQ);
	}

	if ((seq_tbl->num_entries % 2) != 0) {
		tmp_val = ((entry[0].sampling_delay & ELEVEN_BITS_SET)
			<< SEQ_DELAY_EVEN_POS);
		tmp_val |= (entry[0].channel_id & FIVE_BITS_SET);
		sys_out32(tmp_val, adc_base + ADC_SEQ);
	}

	sys_out32(ctrl | ADC_SEQ_PTR_RST, adc_base + ADC_CTRL);

	int num_entries = seq_tbl->num_entries;

	info->entries = seq_tbl->entries;
	for (i = 0; i < num_entries; i++) {
		info->index[i] = 0;
	}

	info->state = ADC_STATE_SAMPLING;
	sys_out32(START_ADC_SEQ, adc_base + ADC_CTRL);
	return 0;
}

static void dw_set_user_callback(struct device *dev, adc_callback_t cb)
{
	struct adc_info *info = dev->driver_data;

	info->cb = cb;
}

static struct adc_driver_api api_funcs = {
	.enable  = adc_dw_enable,
	.disable = adc_dw_disable,
	.read    = adc_dw_read,
	.set_callback  = dw_set_user_callback
};

int adc_dw_init(struct device *dev)
{
	uint32_t tmp_val;
	uint32_t val;
	struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

	dev->driver_api = &api_funcs;

	sys_out32(ADC_INT_DSB | ADC_CLK_ENABLE, adc_base + ADC_CTRL);

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ADC_CONFIG_SET_MASK;
	val = (config->sample_width) & FIVE_BITS_SET;
	val |= ((config->in_mode & ONE_BIT_SET) << INPUT_MODE_POS);
	val |= ((config->capture_mode & ONE_BIT_SET) << CAPTURE_MODE_POS);
	val |= ((config->out_mode & ONE_BIT_SET) << OUTPUT_MODE_POS);
	val |= ((config->serial_dly & FIVE_BITS_SET) << SERIAL_DELAY_POS);
	val |= ((config->seq_mode & ONE_BIT_SET) << SEQUENCE_MODE_POS);
	sys_out32(tmp_val|val, adc_base + ADC_SET);

	sys_out32(config->clock_ratio & ADC_CLK_RATIO_MASK,
		adc_base + ADC_DIVSEQSTAT);

	sys_out32(ADC_INT_ENABLE & ~(ADC_CLK_ENABLE),
		adc_base + ADC_CTRL);

	config->config_func(dev);

	int_unmask(config->reg_irq_mask);
	int_unmask(config->reg_err_mask);

	return 0;
}

void adc_dw_rx_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct device_config *dev_config = dev->config;
	struct adc_config *config = dev_config->config_info;
	struct adc_info *info = dev->driver_data;
	uint32_t adc_base = config->reg_base;
	uint32_t reg_val;
	uint32_t sample_index;
	struct adc_seq_entry *entries = info->entries;

	for (sample_index = 0; sample_index < info->seq_size; sample_index++) {
		int rep_index;
		uint32_t *adc_buffer;

		reg_val = sys_in32(adc_base + ADC_SET);
		sys_out32(reg_val|ADC_POP_SAMPLE, adc_base + ADC_SET);
		rep_index = info->index[sample_index];
		adc_buffer = (uint32_t *)entries[sample_index].buffer;
		adc_buffer[rep_index] = sys_in32(adc_base + ADC_SAMPLE);
		info->index[sample_index]++;
		/*API array is 8 bits array but ADC reads blocks of 32 bits with every sample.*/
		if ((info->index[sample_index] * 4) >= entries[sample_index].buffer_length) {
			info->index[sample_index] = 0;
		}
	}

	if (likely(info->cb != NULL)) {
		info->cb(dev, ADC_CB_DONE);
	}

	if (config->seq_mode == IO_ADC_SEQ_MODE_SINGLESHOT) {
		sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
		reg_val = sys_in32(adc_base + ADC_SET);
		sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_SET);
		info->state = ADC_STATE_IDLE;
	}

	reg_val = sys_in32(adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_CLR_DATA_A, adc_base + ADC_CTRL);
}


void adc_dw_err_isr(void *arg)
{
	struct device *dev = (struct device *) arg;
	struct adc_config  *config = dev->config->config_info;
	struct adc_info    *info   = dev->driver_data;
	uint32_t adc_base = config->reg_base;
	uint32_t reg_val = sys_in32(adc_base + ADC_SET);


	sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_CTRL);
	sys_out32(FLUSH_ADC_ERRORS, adc_base + ADC_CTRL);

	info->state = ADC_STATE_IDLE;

	if (likely(info->cb != NULL)) {
		info->cb(dev, ADC_CB_ERROR);
	}
}

#ifdef CONFIG_ADC_DW_0

struct adc_info adc_info_dev_0 = {
		.state = ADC_STATE_IDLE
	};

struct adc_config adc_config_dev_0 = {
		.reg_base = PERIPH_ADDR_BASE_ADC,
		.reg_irq_mask = SCSS_REGISTER_BASE + INT_SS_ADC_IRQ_MASK,
		.reg_err_mask = SCSS_REGISTER_BASE + INT_SS_ADC_ERR_MASK,
		.rx_vector = CONFIG_ADC_DW_0_RX_IRQ,
		.err_vector = CONFIG_ADC_DW_0_ERR_IRQ,
#ifdef CONFIG_ADC_DW_SINGLE_ENDED
		.in_mode      = 0,
#elif CONFIG_ADC_DW_DIFFERENTIAL
		.in_mode      = 1,
#endif
#ifdef CONFIG_ADC_DW_SERIAL
		.out_mode     = 0,
#elif CONFIG_ADC_DW_PARALLEL
		.out_mode     = 1,
#endif
#ifdef CONFIG_ADC_DW_SINGLESHOT
		.seq_mode = 0,
#elif CONFIG_ADC_DW_REPETITIVE
		.seq_mode = 1,
#endif
#ifdef CONFIG_ADC_DW_RISING_EDGE
		.capture_mode = 0,
#elif CONFIG_ADC_DW_FALLING_EDGE
		.capture_mode = 1,
#endif
		.sample_width = CONFIG_ADC_DW_SAMPLE_WIDTH,
		.clock_ratio  = CONFIG_ADC_DW_CLOCK_RATIO,
		.serial_dly   = CONFIG_ADC_DW_SERIAL_DELAY,
		.config_func  = adc_config_0_irq,
	};

DECLARE_DEVICE_INIT_CONFIG(adc_dw_0,	/* config name*/
			CONFIG_ADC_DW_NAME_0,	/* driver name*/
			&adc_dw_init,	/* init function*/
			&adc_config_dev_0); /* config options*/

SYS_DEFINE_DEVICE(adc_dw_0, &adc_info_dev_0, SECONDARY,
					CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

IRQ_CONNECT_STATIC(adc_dw_0_rx,
		CONFIG_ADC_DW_0_RX_IRQ,
		CONFIG_ADC_DW_0_PRI,
		adc_dw_rx_isr,
		SYS_GET_DEVICE(adc_dw_0), 0);

IRQ_CONNECT_STATIC(adc_dw_0_err,
		CONFIG_ADC_DW_0_ERR_IRQ,
		CONFIG_ADC_DW_0_PRI,
		adc_dw_err_isr,
		SYS_GET_DEVICE(adc_dw_0), 0);

static void adc_config_0_irq(struct device *dev)
{
	struct adc_config *config = dev->config->config_info;

	IRQ_CONFIG(adc_dw_0_rx, CONFIG_ADC_DW_0_RX_IRQ);
	irq_enable(config->rx_vector);
	IRQ_CONFIG(adc_dw_0_err, CONFIG_ADC_DW_0_ERR_IRQ);
	irq_enable(config->err_vector);
}
#endif
