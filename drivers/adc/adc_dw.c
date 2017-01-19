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
#include "adc_dw.h"

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

#ifdef CONFIG_ADC_DW_CALIBRATION
static void calibration_command(uint8_t command)
{
	uint32_t state;
	uint32_t reg_value;

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
	uint8_t calibration_value;
	uint32_t reg_value;
	uint32_t state;

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
	uint32_t reg_value;
	uint32_t state;

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
	uint32_t reg_value;
	uint32_t state;

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
		} while ((reg_value & 0x1) == 0);
	}
}

static void adc_dw_enable(struct device *dev)
{
	uint32_t reg_value;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

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
	uint32_t saved;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;

	sys_out32(ADC_INT_DSB|ENABLE_ADC, adc_base + ADC_CTRL);
	adc_goto_deep_power_down();
	sys_out32(ADC_INT_DSB|ADC_SEQ_PTR_RST, adc_base + ADC_CTRL);

	saved = irq_lock();

	sys_out32(sys_in32(adc_base + ADC_SET)|ADC_FLUSH_RX, adc_base + ADC_SET);
	irq_unlock(saved);

	info->state = ADC_STATE_DISABLED;
}

static int adc_dw_read_request(struct device *dev, struct adc_seq_table *seq_tbl)
{
	uint32_t i;
	uint32_t ctrl;
	uint32_t tmp_val;
	uint32_t num_iters;
	uint32_t saved;
	struct adc_seq_entry *entry;
	struct adc_info *info = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;
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

	info->entries = seq_tbl->entries;
#ifdef CONFIG_ADC_DW_REPETITIVE
	memset(info->index, 0, seq_tbl->num_entries);
#endif
	info->state = ADC_STATE_SAMPLING;
	sys_out32(START_ADC_SEQ, adc_base + ADC_CTRL);

	k_sem_take(&info->device_sync_sem, K_FOREVER);

	if (info->state == ADC_STATE_ERROR) {
		info->state = ADC_STATE_IDLE;
		return -EIO;
	}

	return 0;
}

static int adc_dw_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	struct adc_info *info = dev->driver_data;

#ifdef CONFIG_ADC_DW_DUMMY_CONVERSION
	if (info->dummy_conversion == ADC_NONE_DUMMY) {
		adc_dw_read_request(dev, seq_tbl);
		info->dummy_conversion = ADC_DONE_DUMMY;
	}
#endif
	return adc_dw_read_request(dev, seq_tbl);
}

static struct adc_driver_api api_funcs = {
	.enable  = adc_dw_enable,
	.disable = adc_dw_disable,
	.read    = adc_dw_read,
};

int adc_dw_init(struct device *dev)
{
	uint32_t tmp_val;
	uint32_t val;
	const struct adc_config *config = dev->config->config_info;
	uint32_t adc_base = config->reg_base;
	struct adc_info *info = dev->driver_data;

	sys_out32(ADC_INT_DSB | ADC_CLK_ENABLE, adc_base + ADC_CTRL);

	tmp_val = sys_in32(adc_base + ADC_SET);
	tmp_val &= ADC_CONFIG_SET_MASK;
	val = (config->sample_width) & FIVE_BITS_SET;
	val &= ~(1 << INPUT_MODE_POS);
	val |= ((config->capture_mode & ONE_BIT_SET) << CAPTURE_MODE_POS);
	val |= ((config->out_mode & ONE_BIT_SET) << OUTPUT_MODE_POS);
	val |= ((config->serial_dly & FIVE_BITS_SET) << SERIAL_DELAY_POS);
	val |= ((config->seq_mode & ONE_BIT_SET) << SEQUENCE_MODE_POS);
	sys_out32(tmp_val|val, adc_base + ADC_SET);

	sys_out32(config->clock_ratio & ADC_CLK_RATIO_MASK,
		adc_base + ADC_DIVSEQSTAT);

	sys_out32(ADC_INT_ENABLE & ~(ADC_CLK_ENABLE),
		adc_base + ADC_CTRL);

	config->config_func();

	k_sem_init(&info->device_sync_sem, 0, UINT_MAX);

	int_unmask(config->reg_irq_mask);
	int_unmask(config->reg_err_mask);

	return 0;
}

#ifdef CONFIG_ADC_DW_SINGLESHOT
static void adc_dw_rx_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct device_config *dev_config = dev->config;
	const struct adc_config *config = dev_config->config_info;
	struct adc_info *info = dev->driver_data;
	uint32_t adc_base = config->reg_base;
	struct adc_seq_entry *entries = info->entries;
	uint32_t reg_val;
	uint32_t seq_index;

	for (seq_index = 0; seq_index < info->seq_size; seq_index++) {
		uint32_t *adc_buffer;

		reg_val = sys_in32(adc_base + ADC_SET);
		sys_out32(reg_val|ADC_POP_SAMPLE, adc_base + ADC_SET);
		adc_buffer = (uint32_t *)entries[seq_index].buffer;
		*adc_buffer = sys_in32(adc_base + ADC_SAMPLE);
	}

	/*Resume ADC state to continue new conversions*/
	sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
	reg_val = sys_in32(adc_base + ADC_SET);
	sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_SET);
	info->state = ADC_STATE_IDLE;

	/*Clear data A register*/
	reg_val = sys_in32(adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_CLR_DATA_A, adc_base + ADC_CTRL);

	k_sem_give(&info->device_sync_sem);
}
#else /*CONFIG_ADC_DW_REPETITIVE*/
static void adc_dw_rx_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct device_config *dev_config = dev->config;
	const struct adc_config *config = dev_config->config_info;
	struct adc_info *info = dev->driver_data;
	uint32_t adc_base = config->reg_base;
	struct adc_seq_entry *entries = info->entries;
	uint32_t reg_val;
	uint32_t sequence_index;
	uint8_t full_buffer_flag = 0;

	for (sequence_index = 0; sequence_index < info->seq_size; sequence_index++) {
		uint32_t *adc_buffer;
		uint32_t repetitive_index;

		repetitive_index = info->index[sequence_index];
		/*API array is 8 bits array but ADC reads blocks of 32 bits with every sample.*/
		if (repetitive_index >= (entries[sequence_index].buffer_length >> 2)) {
			full_buffer_flag = 1;
			continue;
		}

		reg_val = sys_in32(adc_base + ADC_SET);
		sys_out32(reg_val|ADC_POP_SAMPLE, adc_base + ADC_SET);
		adc_buffer = (uint32_t *)entries[sequence_index].buffer;
		adc_buffer[repetitive_index] = sys_in32(adc_base + ADC_SAMPLE);
		repetitive_index++;
		info->index[sequence_index] = repetitive_index;
	}

	if (full_buffer_flag == 1) {
		/*Resume ADC state to continue new conversions*/
		sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
		reg_val = sys_in32(adc_base + ADC_SET);
		sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_SET);
		info->state = ADC_STATE_IDLE;
		/*Clear data A register*/
		reg_val = sys_in32(adc_base + ADC_CTRL);
		sys_out32(reg_val | ADC_CLR_DATA_A, adc_base + ADC_CTRL);

		k_sem_give(&info->device_sync_sem);
		return;
	}

	/*Clear data A register*/
	reg_val = sys_in32(adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_CLR_DATA_A, adc_base + ADC_CTRL);
}
#endif


static void adc_dw_err_isr(void *arg)
{
	struct device *dev = (struct device *) arg;
	const struct adc_config  *config = dev->config->config_info;
	struct adc_info    *info   = dev->driver_data;
	uint32_t adc_base = config->reg_base;
	uint32_t reg_val = sys_in32(adc_base + ADC_SET);


	sys_out32(RESUME_ADC_CAPTURE, adc_base + ADC_CTRL);
	sys_out32(reg_val | ADC_FLUSH_RX, adc_base + ADC_CTRL);
	sys_out32(FLUSH_ADC_ERRORS, adc_base + ADC_CTRL);

	info->state = ADC_STATE_ERROR;

	k_sem_give(&info->device_sync_sem);
}

#ifdef CONFIG_ADC_DW

struct adc_info adc_info_dev = {
		.state = ADC_STATE_IDLE,
#ifdef CONFIG_ADC_DW_CALIBRATION
		.calibration_value = ADC_NONE_CALIBRATION,
#endif
#ifdef CONFIG_ADC_DW_DUMMY_CONVERSION
		.dummy_conversion = ADC_NONE_DUMMY,
#endif
	};

static struct adc_config adc_config_dev = {
		.reg_base = PERIPH_ADDR_BASE_ADC,
		.reg_irq_mask = SCSS_REGISTER_BASE + INT_SS_ADC_IRQ_MASK,
		.reg_err_mask = SCSS_REGISTER_BASE + INT_SS_ADC_ERR_MASK,
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
		.config_func  = adc_config_irq,
	};

DEVICE_AND_API_INIT(adc_dw, CONFIG_ADC_0_NAME, &adc_dw_init,
		    &adc_info_dev, &adc_config_dev,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);

static void adc_config_irq(void)
{
	IRQ_CONNECT(IRQ_ADC_IRQ, CONFIG_ADC_0_IRQ_PRI, adc_dw_rx_isr,
		    DEVICE_GET(adc_dw), 0);
	irq_enable(IRQ_ADC_IRQ);

	IRQ_CONNECT(IRQ_ADC_ERR, CONFIG_ADC_0_IRQ_PRI,
		    adc_dw_err_isr, DEVICE_GET(adc_dw), 0);
	irq_enable(IRQ_ADC_ERR);
}
#endif
