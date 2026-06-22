/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(adc_ad4170, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* AD4170 registers */
#define AD4170_CONFIG_A_REG      0x00
#define AD4170_PRODUCT_ID_L      0x04
#define AD4170_PRODUCT_ID_H      0x05
#define AD4170_STATUS_REG        0x15
#define AD4170_DATA_24B_REG      0x1E
#define AD4170_CLOCK_CTRL_REG    0x6B
#define AD4170_ADC_CTRL_REG      0x71
#define AD4170_CHAN_EN_REG       0x79
#define AD4170_CHAN_SETUP_REG(x) (0x81 + 4 * (x))
#define AD4170_CHAN_MAP_REG(x)   (0x83 + 4 * (x))
#define AD4170_AFE_REG(x)        (0xC3 + 14 * (x))
#define AD4170_FILTER_REG(x)     (0xC5 + 14 * (x))
#define AD4170_FILTER_FS_REG(x)  (0xC7 + 14 * (x))
#define AD4170_REF_CTRL_REG      0x131

#define AD4170_REG_READ_MASK     BIT(6)
#define AD4170_REG_ADDR_LSB_MASK GENMASK(7, 0)

#define AD4170_PRODUCT_ID_H_MASK 0xFF00
#define AD4170_PRODUCT_ID_L_MASK 0x00FF

#define AD4170_CHIP_ID 0x40
#define AD4190_CHIP_ID 0x48
#define AD4195_CHIP_ID 0x48

/* AD4170_CONFIG_A_REG - INTERFACE_CONFIG_A REGISTER */
#define AD4170_SW_RESET_MSK (BIT(7) | BIT(0))

/* AD4170_STATUS_REG */
#define AD4170_CH_ACTIVE_MSK GENMASK(3, 0)
#define AD4170_RDYB_MSK      BIT(5)

/* AD4170_CLOCK_CTRL_REG */
#define AD4170_CLOCK_CTRL_CLOCKSEL_MSK GENMASK(1, 0)

/* AD4170_ADC_CTRL_REG */
#define AD4170_ADC_CTRL_MODE_MSK GENMASK(3, 0)

/* AD4170_CHAN_EN_REG */
#define AD4170_CHAN_EN(ch) BIT(ch)

/* AD4170_CHAN_SETUP_REG */
#define AD4170_CHAN_SETUP_SETUP_MSK GENMASK(2, 0)

/* AD4170_CHAN_MAP_REG */
#define AD4170_CHAN_MAP_AINP_MSK GENMASK(12, 8)
#define AD4170_CHAN_MAP_AINM_MSK GENMASK(4, 0)

/* AD4170_AFE_REG */
#define AD4170_AFE_REF_SELECT_MSK GENMASK(6, 5)
#define AD4170_AFE_BIPOLAR_MSK    BIT(4)
#define AD4170_AFE_PGA_GAIN_MSK   GENMASK(3, 0)

#define AD4170_REF_EN_MSK BIT(0)

/* AD4170_FILTER_REG */
#define AD4170_FILTER_TYPE_MSK GENMASK(3, 0)

/* Internal and external clock properties */
#define AD4170_INT_CLOCK_16MHZ   16000000UL
#define AD4170_EXT_CLOCK_MHZ_MIN 1000000UL
#define AD4170_EXT_CLOCK_MHZ_MAX 17000000UL

/* AD4170 register constants */

/* AD4170_FILTER_REG constants */
#define AD4170_FILTER_TYPE_SINC5_AVG 0x0
#define AD4170_FILTER_TYPE_SINC5     0x4
#define AD4170_FILTER_TYPE_SINC3     0x6

/* Device properties and auxiliary constants */
#define AD4170_MAX_ADC_CHANNELS 16
#define AD4170_MAX_SETUPS       8
#define AD4170_ADC_RESOLUTION   24
#define AD4170_FILTER_NUM       3

#define AD4170_INT_REF_2_5V 2500

/* AD4170 Error Values */
#define AD4170_INVALID_CHANNEL -1
#define AD4170_INVALID_SLOT    -1

static const unsigned int ad4170_sinc3_filt_fs_tbl[] = {
	4,     8,     12,  16,   20,   40,   48,    80,    /*  0 -  7 */
	100,   256,   500, 1000, 5000, 8332, 10000, 25000, /*  8 - 15 */
	50000, 65532,                                      /* 16 - 17 */
};

#define AD4170_MAX_FS_TBL_SIZE ARRAY_SIZE(ad4170_sinc3_filt_fs_tbl)

static const unsigned int ad4170_sinc5_filt_fs_tbl[] = {
	1, 2, 4, 8, 12, 16, 20, 40, 48, 80, 100, 256,
};

static const unsigned int ad4170_reg_size[] = {
	[AD4170_CONFIG_A_REG] = 1,       [AD4170_PRODUCT_ID_L] = 1,
	[AD4170_PRODUCT_ID_H] = 1,       [AD4170_STATUS_REG] = 2,
	[AD4170_DATA_24B_REG] = 3,       [AD4170_CLOCK_CTRL_REG] = 2,
	[AD4170_ADC_CTRL_REG] = 2,       [AD4170_CHAN_EN_REG] = 2,
	[AD4170_REF_CTRL_REG] = 2,       [AD4170_CHAN_SETUP_REG(0)] = 2,
	[AD4170_CHAN_SETUP_REG(1)] = 2,  [AD4170_CHAN_SETUP_REG(2)] = 2,
	[AD4170_CHAN_SETUP_REG(3)] = 2,  [AD4170_CHAN_SETUP_REG(4)] = 2,
	[AD4170_CHAN_SETUP_REG(5)] = 2,  [AD4170_CHAN_SETUP_REG(6)] = 2,
	[AD4170_CHAN_SETUP_REG(7)] = 2,  [AD4170_CHAN_SETUP_REG(8)] = 2,
	[AD4170_CHAN_SETUP_REG(9)] = 2,  [AD4170_CHAN_SETUP_REG(10)] = 2,
	[AD4170_CHAN_SETUP_REG(11)] = 2, [AD4170_CHAN_SETUP_REG(12)] = 2,
	[AD4170_CHAN_SETUP_REG(13)] = 2, [AD4170_CHAN_SETUP_REG(14)] = 2,
	[AD4170_CHAN_SETUP_REG(15)] = 2, [AD4170_CHAN_MAP_REG(0)] = 2,
	[AD4170_CHAN_MAP_REG(1)] = 2,    [AD4170_CHAN_MAP_REG(2)] = 2,
	[AD4170_CHAN_MAP_REG(3)] = 2,    [AD4170_CHAN_MAP_REG(4)] = 2,
	[AD4170_CHAN_MAP_REG(5)] = 2,    [AD4170_CHAN_MAP_REG(6)] = 2,
	[AD4170_CHAN_MAP_REG(7)] = 2,    [AD4170_CHAN_MAP_REG(8)] = 2,
	[AD4170_CHAN_MAP_REG(9)] = 2,    [AD4170_CHAN_MAP_REG(10)] = 2,
	[AD4170_CHAN_MAP_REG(11)] = 2,   [AD4170_CHAN_MAP_REG(12)] = 2,
	[AD4170_CHAN_MAP_REG(13)] = 2,   [AD4170_CHAN_MAP_REG(14)] = 2,
	[AD4170_CHAN_MAP_REG(15)] = 2,   [AD4170_AFE_REG(0)] = 2,
	[AD4170_FILTER_REG(0)] = 2,      [AD4170_FILTER_FS_REG(0)] = 2,
	[AD4170_AFE_REG(1)] = 2,         [AD4170_FILTER_REG(1)] = 2,
	[AD4170_FILTER_FS_REG(1)] = 2,   [AD4170_AFE_REG(2)] = 2,
	[AD4170_FILTER_REG(2)] = 2,      [AD4170_FILTER_FS_REG(2)] = 2,
	[AD4170_AFE_REG(3)] = 2,         [AD4170_FILTER_REG(3)] = 2,
	[AD4170_FILTER_FS_REG(3)] = 2,   [AD4170_AFE_REG(4)] = 2,
	[AD4170_FILTER_REG(4)] = 2,      [AD4170_FILTER_FS_REG(4)] = 2,
	[AD4170_AFE_REG(5)] = 2,         [AD4170_FILTER_REG(5)] = 2,
	[AD4170_FILTER_FS_REG(5)] = 2,   [AD4170_AFE_REG(6)] = 2,
	[AD4170_FILTER_REG(6)] = 2,      [AD4170_FILTER_FS_REG(6)] = 2,
	[AD4170_AFE_REG(7)] = 2,         [AD4170_FILTER_REG(7)] = 2,
	[AD4170_FILTER_FS_REG(7)] = 2,
};

enum ad4170_clk_sel {
	AD4170_CLKSEL_INT,
	AD4170_CLKSEL_INT_OUT,
	AD4170_CLKSEL_EXT,
	AD4170_CLKSEL_EXT_XTAL
};

enum ad4170_input {
	AD4170_AIN0,
	AD4170_AIN1,
	AD4170_AIN2,
	AD4170_AIN3,
	AD4170_AIN4,
	AD4170_AIN5,
	AD4170_AIN6,
	AD4170_AIN7,
	AD4170_TEMP = 0b10001,
	AD4170_AVDD_AVSS_5,
	AD4170_IOVDD_DGND_5,
	AD4170_ALDO = 0b10101,
	AD4170_DLDO,
	AD4170_AVSS,
	AD4170_DGND,
	AD4170_REFIN1_P,
	AD4170_REFIN1_N,
	AD4170_REFIN2_P,
	AD4170_REFIN2_N,
	AD4170_REFOUT
};

enum ad4170_setup {
	AD4170_SETUP_0,
	AD4170_SETUP_1,
	AD4170_SETUP_2,
	AD4170_SETUP_3,
	AD4170_SETUP_4,
	AD4170_SETUP_5,
	AD4170_SETUP_6,
	AD4170_SETUP_7
};

enum ad4170_ref_sel {
	AD4170_REF_REFIN1,
	AD4170_REF_REFIN2,
	AD4170_REF_REFOUT_AVSS,
	AD4170_REF_AVDD_AVSS,
	AD4170_REF_SEL_MAX
};

enum ad4170_adc_mode {
	AD4170_CONTINUOUS_MODE,
	AD4170_SINGLE_MODE,
	AD4170_STANDBY_MODE,
	AD4170_POWER_DOWN_MODE,
	AD4170_IDLE_MODE
};

enum ad4170_gain {
	AD4170_GAIN_1,
	AD4170_GAIN_2,
	AD4170_GAIN_4,
	AD4170_GAIN_8,
	AD4170_GAIN_16,
	AD4170_GAIN_32,
	AD4170_GAIN_64,
	AD4170_GAIN_128,
	AD4170_GAIN_1_2
};

enum ad4170_filter_type {
	AD4170_SINC5_AVG,
	AD4170_SINC5,
	AD4170_SINC3
};

struct ad4170_filter_props {
	enum ad4170_filter_type filter_type;
	uint16_t filter_fs;
};

struct ad4170_afe_props {
	enum ad4170_ref_sel ref_sel;
	enum ad4170_gain gain;
};

struct ad4170_channel_config {
	struct ad4170_afe_props afe;
	struct ad4170_filter_props filter;
	uint8_t cfg_slot;
	bool live_cfg;
};

struct ad4170_config {
	struct spi_dt_spec bus;
	uint32_t mclk_hz;
	uint16_t chip_id;
	uint8_t resolution;
	uint8_t clock_select;
	enum ad4170_adc_mode adc_mode;
	enum ad4170_filter_type filter_type;
	bool bipolar;
};

struct adc_ad4170_data {
	const struct device *dev;
	struct adc_context ctx;
	struct ad4170_channel_config channel_setup_cfg[AD4170_MAX_ADC_CHANNELS];
	int sps_tbl[AD4170_FILTER_NUM][AD4170_MAX_FS_TBL_SIZE];
	uint32_t *buffer;
	uint32_t *repeat_buffer;
	uint16_t channels;
	uint8_t setup_cfg_slots;
	struct k_sem acquire_signal;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADI_AD4170_ADC_ACQUISITION_THREAD_STACK_SIZE);
#endif /* CONFIG_ADC_ASYNC */
};

static size_t ad4170_get_reg_size(unsigned int reg_addr, size_t *reg_size)
{
	if (reg_addr >= ARRAY_SIZE(ad4170_reg_size)) {
		return -EINVAL;
	}

	*reg_size = ad4170_reg_size[reg_addr];
	return 0;
}

static int ad4170_reg_write(const struct device *dev, unsigned int reg_addr, unsigned int val)
{
	const struct ad4170_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;

	uint8_t reg_write_tx_buf[5] = {0};
	size_t reg_size = 0;
	int ret;

	ret = ad4170_get_reg_size(reg_addr, &reg_size);
	if (ret) {
		return ret;
	}

	reg_write_tx_buf[1] = FIELD_PREP(AD4170_REG_ADDR_LSB_MASK, reg_addr);
	reg_write_tx_buf[0] = reg_addr >> 8;

	switch (reg_size) {
	case 1:
		reg_write_tx_buf[2] = val;
		break;
	case 2:
		sys_put_be16(val, &reg_write_tx_buf[2]);
		break;
	case 3:
		sys_put_be24(val, &reg_write_tx_buf[2]);
		break;
	default:
		return -EINVAL;
	}

	const struct spi_buf tx_buf = {.buf = reg_write_tx_buf, .len = reg_size + 2};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(spec, &tx);
}

static int ad4170_reg_read(const struct device *dev, unsigned int reg_addr, unsigned int *val)
{
	const struct ad4170_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;
	int ret;

	uint8_t reg_read_tx_buf[2] = {0};
	uint8_t reg_read_rx_buf[5] = {0};
	size_t reg_size = 0;

	ret = ad4170_get_reg_size(reg_addr, &reg_size);
	if (ret) {
		return ret;
	}

	reg_read_tx_buf[1] = FIELD_PREP(AD4170_REG_ADDR_LSB_MASK, reg_addr);
	reg_read_tx_buf[0] = AD4170_REG_READ_MASK | (reg_addr >> 8);

	const struct spi_buf tx_buf = {.buf = &reg_read_tx_buf, .len = 2};
	const struct spi_buf rx_buf = {.buf = reg_read_rx_buf, .len = reg_size + 2};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	ret = spi_transceive_dt(spec, &tx, &rx);
	if (ret) {
		return ret;
	}

	switch (reg_size) {
	case 1:
		*val = reg_read_rx_buf[2];
		return 0;
	case 2:
		*val = sys_get_be16(&reg_read_rx_buf[2]);
		return 0;
	case 3:
		*val = sys_get_be24(&reg_read_rx_buf[2]);
		return 0;
	default:
		return -EINVAL;
	}
}

static int ad4170_reg_write_msk(const struct device *dev, unsigned int reg_addr, unsigned int mask,
				unsigned int data)
{
	uint32_t reg_data;
	int ret;

	ret = ad4170_reg_read(dev, reg_addr, &reg_data);
	if (ret) {
		return ret;
	}

	reg_data &= ~mask;
	reg_data |= data;

	return ad4170_reg_write(dev, reg_addr, reg_data);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad4170_data *data = CONTAINER_OF(ctx, struct adc_ad4170_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ad4170_data *data = CONTAINER_OF(ctx, struct adc_ad4170_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int adc_ad4170_clock_select(const struct device *dev, enum ad4170_clk_sel clk_sel)
{
	const struct ad4170_config *config = dev->config;
	int ret;

	ret = ad4170_reg_write_msk(dev, AD4170_CLOCK_CTRL_REG, AD4170_CLOCK_CTRL_CLOCKSEL_MSK,
				   FIELD_PREP(AD4170_CLOCK_CTRL_CLOCKSEL_MSK, clk_sel));
	if (ret) {
		return ret;
	}

	if ((clk_sel == AD4170_CLKSEL_EXT || clk_sel == AD4170_CLKSEL_EXT_XTAL) &&
	    (config->mclk_hz < AD4170_EXT_CLOCK_MHZ_MIN ||
	     config->mclk_hz > AD4170_EXT_CLOCK_MHZ_MAX)) {
		LOG_ERR("Invalid external clock frequency %u or no external clock provided",
			config->mclk_hz);
		return -EINVAL;
	}

	return 0;
}

static void ad4170_fill_sps_tbl(const struct device *dev)
{
	unsigned int tmp;
	const struct ad4170_config *config = dev->config;
	struct adc_ad4170_data *data = dev->data;

	/*
	 * The ODR can be calculated the same way for sinc5+avg, sinc5, and
	 * sinc3 filter types with the exception that sinc5 filter has a
	 * narrowed range of allowed FILTER_FS values.
	 */
	for (int i = 0; i < ARRAY_SIZE(ad4170_sinc3_filt_fs_tbl); i++) {
		tmp = DIV_ROUND_CLOSEST(config->mclk_hz, 32 * ad4170_sinc3_filt_fs_tbl[i]);

		/* Fill sinc5+avg filter SPS table */
		data->sps_tbl[AD4170_SINC5_AVG][i] = tmp;

		/* Fill sinc3 filter SPS table */
		data->sps_tbl[AD4170_SINC3][i] = tmp;
	}
	/* Sinc5 filter ODR doesn't use all FILTER_FS bits */
	for (int i = 0; i < ARRAY_SIZE(ad4170_sinc5_filt_fs_tbl); i++) {
		tmp = DIV_ROUND_CLOSEST(config->mclk_hz, 32 * ad4170_sinc5_filt_fs_tbl[i]);

		/* Fill sinc5 filter SPS table */
		data->sps_tbl[AD4170_SINC5][i] = tmp;
	}
}

static int adc_ad4170_acq_time_to_odr(const struct device *dev, uint16_t acq_time, uint16_t *odr)
{
	const struct ad4170_config *config = dev->config;
	const struct adc_ad4170_data *data = dev->data;
	uint16_t acquisition_time_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint16_t acquisition_time_unit = ADC_ACQ_TIME_UNIT(acq_time);
	uint8_t sinc3_fs_tbl_size = ARRAY_SIZE(ad4170_sinc3_filt_fs_tbl);
	uint8_t sinc5_fs_tbl_size = ARRAY_SIZE(ad4170_sinc5_filt_fs_tbl);

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		if (config->filter_type == AD4170_SINC5_AVG ||
		    config->filter_type == AD4170_SINC3) {
			*odr = data->sps_tbl[AD4170_SINC5_AVG][sinc3_fs_tbl_size - 1];
		} else {
			*odr = data->sps_tbl[AD4170_SINC5][sinc5_fs_tbl_size - 1];
		}
		return 0;
	}

	if (acquisition_time_unit != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("Unsupported acquisition time unit %u", acquisition_time_unit);
		return -EINVAL;
	}

	switch (config->filter_type) {
	case AD4170_SINC5_AVG:
	case AD4170_SINC3:
		if (acquisition_time_value < data->sps_tbl[AD4170_SINC3][sinc3_fs_tbl_size - 1] ||
		    acquisition_time_value > data->sps_tbl[AD4170_SINC3][0]) {
			LOG_ERR("Unsupported acquisition time %u", acquisition_time_value);
			return -EINVAL;
		}

		break;
	case AD4170_SINC5:
		if (acquisition_time_value < data->sps_tbl[AD4170_SINC5][sinc5_fs_tbl_size - 1] ||
		    acquisition_time_value > data->sps_tbl[AD4170_SINC5][0]) {
			LOG_ERR("Unsupported acquisition time %u", acquisition_time_value);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Invalid filter type");
		return -EINVAL;
	}

	*odr = acquisition_time_value;
	return 0;
}

static size_t find_closest_idx(uint16_t fs, const unsigned int *fs_tbl, size_t tbl_size)
{
	size_t as1 = tbl_size - 1;
	size_t result = as1;
	long mid_x, left, right;

	for (size_t i = 0; i < as1; i++) {
		mid_x = (fs_tbl[i] + fs_tbl[i + 1]) / 2;
		if (fs <= mid_x) {
			left = fs - fs_tbl[i];
			right = fs_tbl[i + 1] - fs;
			result = (right < left) ? (i + 1) : i;
			break;
		}
	}
	return result;
}

static uint16_t adc_ad4170_odr_to_fs(const struct device *dev, int16_t odr)
{
	const struct ad4170_config *config = dev->config;
	uint16_t filter_fs;
	uint8_t fs_idx;

	filter_fs = DIV_ROUND_CLOSEST(config->mclk_hz, 32 * odr);

	switch (config->filter_type) {
	case AD4170_SINC5_AVG:
	case AD4170_SINC3:
		fs_idx = find_closest_idx(filter_fs, ad4170_sinc3_filt_fs_tbl,
					  ARRAY_SIZE(ad4170_sinc3_filt_fs_tbl));
		return ad4170_sinc3_filt_fs_tbl[fs_idx];
	case AD4170_SINC5:
		fs_idx = find_closest_idx(filter_fs, ad4170_sinc5_filt_fs_tbl,
					  ARRAY_SIZE(ad4170_sinc5_filt_fs_tbl));
		return ad4170_sinc5_filt_fs_tbl[fs_idx];
	default:
		return -EINVAL;
	}
}

static int adc_ad4170_set_filter_type(const struct device *dev, enum ad4170_filter_type filter,
				      enum ad4170_setup setup_id)
{
	switch (filter) {
	case AD4170_SINC5_AVG:
		return ad4170_reg_write_msk(
			dev, AD4170_FILTER_REG(setup_id), AD4170_FILTER_TYPE_MSK,
			FIELD_PREP(AD4170_FILTER_TYPE_MSK, AD4170_FILTER_TYPE_SINC5_AVG));
	case AD4170_SINC5:
		return ad4170_reg_write_msk(
			dev, AD4170_FILTER_REG(setup_id), AD4170_FILTER_TYPE_MSK,
			FIELD_PREP(AD4170_FILTER_TYPE_MSK, AD4170_FILTER_TYPE_SINC5));
	case AD4170_SINC3:
		return ad4170_reg_write_msk(
			dev, AD4170_FILTER_REG(setup_id), AD4170_FILTER_TYPE_MSK,
			FIELD_PREP(AD4170_FILTER_TYPE_MSK, AD4170_FILTER_TYPE_SINC3));
	default:
		return -EINVAL;
	}
}

static int adc_ad4170_setup_filter(const struct device *dev,
				   const struct ad4170_channel_config *cfg)
{
	int ret;

	ret = adc_ad4170_set_filter_type(dev, cfg->filter.filter_type, cfg->cfg_slot);
	if (ret) {
		return ret;
	}

	return ad4170_reg_write(dev, AD4170_FILTER_FS_REG(cfg->cfg_slot), cfg->filter.filter_fs);
}

static int adc_ad4170_set_ref(const struct device *dev, enum ad4170_ref_sel ref,
			      enum ad4170_setup setup_id)
{
	bool internal_reference;
	int ret;

	if (ref == AD4170_REF_REFOUT_AVSS) {
		internal_reference = true;
	} else {
		internal_reference = false;
	}

	ret = ad4170_reg_write_msk(dev, AD4170_REF_CTRL_REG, AD4170_REF_EN_MSK,
				   FIELD_PREP(AD4170_REF_EN_MSK, internal_reference));
	if (ret) {
		return ret;
	}

	return ad4170_reg_write_msk(dev, AD4170_AFE_REG(setup_id), AD4170_AFE_REF_SELECT_MSK,
				    FIELD_PREP(AD4170_AFE_REF_SELECT_MSK, ref));
}

static int adc_ad4170_set_gain(const struct device *dev, enum ad4170_gain gain,
			       enum ad4170_setup setup_id)
{
	return ad4170_reg_write_msk(dev, AD4170_AFE_REG(setup_id), AD4170_AFE_PGA_GAIN_MSK,
				    FIELD_PREP(AD4170_AFE_PGA_GAIN_MSK, gain));
}

static int adc_ad4170_setup_afe(const struct device *dev, const struct ad4170_channel_config *cfg)
{
	int ret;

	ret = adc_ad4170_set_ref(dev, cfg->afe.ref_sel, cfg->cfg_slot);
	if (ret) {
		return ret;
	}

	return adc_ad4170_set_gain(dev, cfg->afe.gain, cfg->cfg_slot);
}

static int adc_ad4170_find_similar_configuration(const struct device *dev,
						 const struct ad4170_channel_config *cfg,
						 uint8_t channel_id)
{
	struct adc_ad4170_data *data = dev->data;
	int similar_channel_index = AD4170_INVALID_CHANNEL;

	for (int i = 0; i < AD4170_MAX_SETUPS; i++) {
		if (!data->channel_setup_cfg[i].live_cfg && i == channel_id) {
			continue;
		}

		if (memcmp(&cfg->afe, &data->channel_setup_cfg[i].afe,
			   sizeof(struct ad4170_afe_props)) == 0) {
			similar_channel_index = i;
			break;
		}
	}

	return similar_channel_index;
}

static int adc_ad4170_find_new_slot(const struct device *dev)
{
	struct adc_ad4170_data *data = dev->data;
	uint8_t slot = data->setup_cfg_slots;

	for (int cnt = 0; cnt < AD4170_MAX_SETUPS; cnt++) {
		if (!(slot & (1 << cnt))) {
			return cnt;
		}
	}

	return AD4170_INVALID_SLOT;
}

static int adc_ad4170_create_new_cfg(const struct device *dev, const struct adc_channel_cfg *cfg,
				     struct ad4170_channel_config *new_cfg)
{
	const struct ad4170_config *config = dev->config;
	enum ad4170_ref_sel ref_source;
	enum ad4170_gain gain;
	uint16_t odr;
	int ret;

	/* Only support DEFAULT and TICKS units for acquisition time */
	if (ADC_ACQ_TIME_UNIT(cfg->acquisition_time) != ADC_ACQ_TIME_UNIT(ADC_ACQ_TIME_DEFAULT) &&
	    ADC_ACQ_TIME_UNIT(cfg->acquisition_time) != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("Unsupported acquisition time unit: %u",
			(unsigned int)ADC_ACQ_TIME_UNIT(cfg->acquisition_time));
		return -EINVAL;
	}

	switch (cfg->reference) {
	case ADC_REF_INTERNAL:
		ref_source = AD4170_REF_REFOUT_AVSS;
		break;
	case ADC_REF_EXTERNAL0:
		ref_source = AD4170_REF_REFIN1;
		break;
	case ADC_REF_EXTERNAL1:
		ref_source = AD4170_REF_REFIN2;
		break;
	case ADC_REF_VDD_1:
		ref_source = AD4170_REF_AVDD_AVSS;
		break;
	default:
		LOG_ERR("Invalid reference source (%u)", cfg->reference);
		return -EINVAL;
	}

	new_cfg->afe.ref_sel = ref_source;

	switch (cfg->gain) {
	case ADC_GAIN_1:
		gain = AD4170_GAIN_1;
		break;
	case ADC_GAIN_2:
		gain = AD4170_GAIN_2;
		break;
	case ADC_GAIN_4:
		gain = AD4170_GAIN_4;
		break;
	case ADC_GAIN_8:
		gain = AD4170_GAIN_8;
		break;
	case ADC_GAIN_16:
		gain = AD4170_GAIN_16;
		break;
	case ADC_GAIN_32:
		gain = AD4170_GAIN_32;
		break;
	case ADC_GAIN_64:
		gain = AD4170_GAIN_64;
		break;
	case ADC_GAIN_128:
		gain = AD4170_GAIN_128;
		break;
	case ADC_GAIN_1_2:
		gain = AD4170_GAIN_1_2;
		break;
	default:
		LOG_ERR("Invalid gain value (%u)", cfg->gain);
		return -EINVAL;
	}

	new_cfg->afe.gain = gain;

	ret = adc_ad4170_acq_time_to_odr(dev, cfg->acquisition_time, &odr);
	if (ret) {
		LOG_ERR("Invalid acquisition time (%u)", cfg->acquisition_time);
		return ret;
	}

	new_cfg->filter.filter_type = config->filter_type;
	new_cfg->filter.filter_fs = adc_ad4170_odr_to_fs(dev, odr);

	return 0;
}

static int adc_ad4170_set_channel_setup(const struct device *dev, uint8_t channel_id,
					enum ad4170_setup setup_id)
{
	return ad4170_reg_write_msk(dev, AD4170_CHAN_SETUP_REG(channel_id),
				    AD4170_CHAN_SETUP_SETUP_MSK,
				    FIELD_PREP(AD4170_CHAN_SETUP_SETUP_MSK, setup_id));
}

static int adc_ad4170_channel_en(const struct device *dev, uint8_t channel_id, bool enable)
{
	return ad4170_reg_write_msk(dev, AD4170_CHAN_EN_REG, AD4170_CHAN_EN(channel_id),
				    FIELD_PREP(AD4170_CHAN_EN(channel_id), enable));
}

static int adc_ad4170_connect_analog_input(const struct device *dev, uint8_t channel_id,
					   enum ad4170_input ainp, enum ad4170_input ainm)
{
	int ret;

	if (ainp < AD4170_AIN0 || ainp > AD4170_REFOUT || ainm < AD4170_AIN0 ||
	    ainm > AD4170_REFOUT) {
		return -EINVAL;
	}

	ret = ad4170_reg_write_msk(dev, AD4170_CHAN_MAP_REG(channel_id), AD4170_CHAN_MAP_AINP_MSK,
				   FIELD_PREP(AD4170_CHAN_MAP_AINP_MSK, ainp));
	if (ret) {
		return ret;
	}

	return ad4170_reg_write_msk(dev, AD4170_CHAN_MAP_REG(channel_id), AD4170_CHAN_MAP_AINM_MSK,
				    FIELD_PREP(AD4170_CHAN_MAP_AINM_MSK, ainm));
}

static int adc_ad4170_set_adc_mode(const struct device *dev, enum ad4170_adc_mode mode)
{
	return ad4170_reg_write_msk(dev, AD4170_ADC_CTRL_REG, AD4170_ADC_CTRL_MODE_MSK,
				    FIELD_PREP(AD4170_ADC_CTRL_MODE_MSK, mode));
}

static int adc_ad4170_set_polarity(const struct device *dev, bool enable)
{
	int ret;

	for (int i = 0; i < AD4170_MAX_SETUPS; i++) {
		ret = ad4170_reg_write_msk(dev, AD4170_AFE_REG(i), AD4170_AFE_BIPOLAR_MSK,
					   FIELD_PREP(AD4170_AFE_BIPOLAR_MSK, enable));
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int adc_ad4170_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	struct adc_ad4170_data *data = dev->data;
	struct ad4170_channel_config new_cfg;
	int similar_channel_index;
	int new_slot;
	int ret;

	if (cfg->channel_id >= AD4170_MAX_SETUPS) {
		LOG_ERR("Invalid channel (%u)", cfg->channel_id);
		return -EINVAL;
	}

	data->channel_setup_cfg[cfg->channel_id].live_cfg = false;

	ret = adc_ad4170_create_new_cfg(dev, cfg, &new_cfg);
	if (ret) {
		return ret;
	}

	new_slot = adc_ad4170_find_new_slot(dev);

	if (new_slot == AD4170_INVALID_SLOT) {
		similar_channel_index =
			adc_ad4170_find_similar_configuration(dev, &new_cfg, cfg->channel_id);
		if (similar_channel_index == AD4170_INVALID_CHANNEL) {
			return -EINVAL;
		}
		new_cfg.cfg_slot = data->channel_setup_cfg[similar_channel_index].cfg_slot;
	} else {
		new_cfg.cfg_slot = new_slot;
		WRITE_BIT(data->setup_cfg_slots, new_slot, true);
	}

	new_cfg.live_cfg = true;

	memcpy(&data->channel_setup_cfg[cfg->channel_id], &new_cfg,
	       sizeof(struct ad4170_channel_config));

	ret = adc_ad4170_setup_afe(dev, &data->channel_setup_cfg[cfg->channel_id]);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	ret = adc_ad4170_connect_analog_input(dev, cfg->channel_id, cfg->input_positive,
					      cfg->input_negative);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	ret = adc_ad4170_setup_filter(dev, &data->channel_setup_cfg[cfg->channel_id]);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	ret = adc_ad4170_set_channel_setup(dev, cfg->channel_id, new_cfg.cfg_slot);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	ret = adc_ad4170_channel_en(dev, cfg->channel_id, true);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	WRITE_BIT(data->channels, cfg->channel_id, true);

	return 0;
}

static bool get_next_ch_idx(uint16_t ch_mask, uint16_t last_idx, uint16_t *new_idx)
{
	last_idx++;

	if (last_idx >= AD4170_MAX_SETUPS) {
		return 0;
	}

	ch_mask >>= last_idx;
	if (!ch_mask) {
		*new_idx = AD4170_INVALID_CHANNEL;
		return 0;
	}

	while (!(ch_mask & 1)) {
		last_idx++;
		ch_mask >>= 1;
	}

	*new_idx = last_idx;

	return 1;
}

static int adc_ad4170_get_read_channel_id(const struct device *dev, uint16_t *channel_id)
{
	int ret;
	uint32_t reg_temp;

	ret = ad4170_reg_read(dev, AD4170_STATUS_REG, &reg_temp);
	if (ret) {
		return ret;
	}

	*channel_id = FIELD_GET(AD4170_CH_ACTIVE_MSK, reg_temp);

	return 0;
}

static int adc_ad4170_wait_for_conv_ready(const struct device *dev)
{
	bool ready = false;
	uint32_t reg_val;
	int ret;

	while (!ready) {
		ret = ad4170_reg_read(dev, AD4170_STATUS_REG, &reg_val);
		if (ret) {
			return ret;
		}

		ready = FIELD_GET(AD4170_RDYB_MSK, reg_val);
	}

	return 0;
}

static int adc_ad4170_perform_read(const struct device *dev)
{
	struct adc_ad4170_data *data = dev->data;
	int ret;
	uint16_t ch_idx = AD4170_INVALID_CHANNEL;
	uint16_t prev_ch_idx = AD4170_INVALID_CHANNEL;
	uint16_t adc_ch_id = 0;
	bool status;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	do {
		prev_ch_idx = ch_idx;

		status = get_next_ch_idx(data->ctx.sequence.channels, ch_idx, &ch_idx);
		if (!status) {
			break;
		}

		adc_ad4170_wait_for_conv_ready(dev);

		ret = ad4170_reg_read(dev, AD4170_DATA_24B_REG, data->buffer);
		if (ret) {
			LOG_ERR("Reading sample failed");
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		ret = adc_ad4170_get_read_channel_id(dev, &adc_ch_id);
		if (ret) {
			LOG_ERR("Reading channel ID failed");
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		if (ch_idx == adc_ch_id) {
			data->buffer++;
		} else {
			ch_idx = prev_ch_idx;
		}

	} while (true);

	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
}

static int adc_ad4170_validate_sequence(const struct device *dev,
					const struct adc_sequence *sequence)
{
	const struct ad4170_config *config = dev->config;
	struct adc_ad4170_data *data = dev->data;
	const size_t channel_maximum = AD4170_MAX_SETUPS * sizeof(sequence->channels);
	uint32_t num_requested_channels;
	size_t necessary;

	if (sequence->resolution != config->resolution) {
		LOG_ERR("Unsupported resolution %u", sequence->resolution);
		return -EINVAL;
	}

	if (!sequence->channels) {
		LOG_ERR("no channel selected");
		return -EINVAL;
	}

	if (sequence->oversampling) {
		LOG_ERR("oversampling is not supported");
		return -EINVAL;
	}

	num_requested_channels = POPCOUNT(sequence->channels);
	necessary = num_requested_channels * sizeof(int32_t);

	if (sequence->options) {
		necessary *= (1 + sequence->options->extra_samplings);
	}

	if (sequence->buffer_size < necessary) {
		LOG_ERR("buffer size %u is too small, need %u", sequence->buffer_size, necessary);
		return -ENOMEM;
	}

	for (size_t i = 0; i < channel_maximum; ++i) {
		if ((BIT(i) & sequence->channels) == 0) {
			continue;
		}

		if ((BIT(i) & sequence->channels) && !(BIT(i) & data->channels)) {
			LOG_ERR("Channel-%d not enabled", i);
			return -EINVAL;
		}

		if (i >= AD4170_MAX_SETUPS) {
			LOG_ERR("invalid channel selection");
			return -EINVAL;
		}
	}

	return 0;
}

static int adc_ad4170_start_read(const struct device *dev, const struct adc_sequence *sequence,
				 bool wait)
{
	int result;
	struct adc_ad4170_data *data = dev->data;

	result = adc_ad4170_validate_sequence(dev, sequence);
	if (result != 0) {
		LOG_ERR("Failed to validate sequence: %d", result);
		return result;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	if (wait) {
		result = adc_context_wait_for_completion(&data->ctx);
	}

	return result;
}

#if CONFIG_ADC_ASYNC
static int adc_ad4170_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	int status;
	struct adc_ad4170_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_ad4170_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

static int adc_ad4170_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_ad4170_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_ad4170_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

#else
static int adc_ad4170_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct adc_ad4170_data *data = dev->data;
	int status;

	adc_context_lock(&data->ctx, false, NULL);

	status = adc_ad4170_start_read(dev, sequence, false);

	while (status == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		status = adc_ad4170_perform_read(dev);
	}

	adc_context_release(&data->ctx, status);

	return 0;
}
#endif

#if CONFIG_ADC_ASYNC
static void adc_ad4170_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		adc_ad4170_perform_read(dev);
	}
}
#endif /* CONFIG_ADC_ASYNC */

static int ad4170_check_chip_id(const struct device *dev)
{
	const struct ad4170_config *config = dev->config;
	int ret;
	unsigned int val;
	uint16_t id;

	ret = ad4170_reg_read(dev, AD4170_PRODUCT_ID_H, &val);
	if (ret) {
		LOG_ERR("Failed to read chip ID: %d", ret);
		return ret;
	}

	id = (val << 8) & AD4170_PRODUCT_ID_H_MASK;

	ret = ad4170_reg_read(dev, AD4170_PRODUCT_ID_L, &val);
	if (ret) {
		LOG_ERR("Failed to read chip ID: %d", ret);
		return ret;
	}

	id |= (val & AD4170_PRODUCT_ID_L_MASK);

	if (id != config->chip_id) {
		LOG_ERR("Invalid chip ID (0x%04X != 0x%04X)", id, config->chip_id);
		return -EINVAL;
	}

	return 0;
}

static int ad4170_soft_reset(const struct device *dev)
{
	int ret;

	ret = ad4170_reg_write(dev, AD4170_CONFIG_A_REG, AD4170_SW_RESET_MSK);
	if (ret) {
		LOG_ERR("Failed to reset ad4170: %d", ret);
		return ret;
	}

	/* AD4170-4 requires 1 ms between reset and any register access. */
	k_msleep(1);

	return 0;
}

static int adc_ad4170_setup(const struct device *dev)
{
	const struct ad4170_config *config = dev->config;
	int ret;

	ret = ad4170_soft_reset(dev);
	if (ret) {
		return ret;
	}

	ret = ad4170_check_chip_id(dev);
	if (ret) {
		return ret;
	}

	ret = adc_ad4170_clock_select(dev, config->clock_select);
	if (ret) {
		return ret;
	}

	ad4170_fill_sps_tbl(dev);

	/* Disable Channel 0 */
	ret = adc_ad4170_channel_en(dev, 0, false);
	if (ret) {
		return ret;
	}

	ret = adc_ad4170_set_polarity(dev, config->bipolar);
	if (ret) {
		return ret;
	}

	return adc_ad4170_set_adc_mode(dev, config->adc_mode);
}

static int ad4170_init(const struct device *dev)
{
	const struct ad4170_config *config = dev->config;
	struct adc_ad4170_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_sem_init(&data->acquire_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("spi bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = adc_ad4170_setup(dev);
	if (ret) {
		return ret;
	}

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(&data->thread, data->stack,
				      CONFIG_ADI_AD4170_ADC_ACQUISITION_THREAD_STACK_SIZE,
				      adc_ad4170_acquisition_thread, (void *)dev, NULL, NULL,
				      CONFIG_ADI_AD4170_ADC_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ad4170");
#endif /* CONFIG_ADC_ASYNC */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_ad4170_driver_api) = {
	.channel_setup = adc_ad4170_channel_setup,
	.read = adc_ad4170_read,
	.ref_internal = AD4170_INT_REF_2_5V,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ad4170_read_async,
#endif
};

#define DT_INST_AD4170(inst, compat) DT_INST(inst, compat)

#define AD4170_ADC_INIT(compat, inst, id)                                                          \
	static const struct ad4170_config ad4170_config_##compat##_##inst = {                      \
		.bus = SPI_DT_SPEC_GET(DT_INST_AD4170(inst, compat),                               \
				       SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA |        \
					       SPI_WORD_SET(8) | SPI_TRANSFER_MSB),                \
		.resolution = AD4170_ADC_RESOLUTION,                                               \
		.bipolar = DT_INST_PROP_OR(inst, bipolar, 1),                                      \
		.adc_mode = DT_INST_PROP_OR(inst, adc_mode, 0),                                    \
		.filter_type = DT_INST_PROP_OR(inst, filter_type, AD4170_SINC5_AVG),               \
		.clock_select = DT_INST_PROP_OR(inst, clock_select, AD4170_CLKSEL_INT),            \
		.mclk_hz = DT_INST_PROP_OR(inst, clock_frequency, AD4170_INT_CLOCK_16MHZ),         \
		.chip_id = id,                                                                     \
	};                                                                                         \
	static struct adc_ad4170_data ad4170_data_##compat##_##inst = {                            \
		ADC_CONTEXT_INIT_LOCK(ad4170_data_##compat##_##inst, ctx),                         \
		ADC_CONTEXT_INIT_TIMER(ad4170_data_##compat##_##inst, ctx),                        \
		ADC_CONTEXT_INIT_SYNC(ad4170_data_##compat##_##inst, ctx),                         \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_AD4170(inst, compat), ad4170_init, NULL,                          \
			 &ad4170_data_##compat##_##inst, &ad4170_config_##compat##_##inst,         \
			 POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &adc_ad4170_driver_api);

/* AD4170-4 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT     adi_ad4170_adc
#define AD4170_INIT(inst) AD4170_ADC_INIT(adi_ad4170_adc, inst, AD4170_CHIP_ID)
DT_INST_FOREACH_STATUS_OKAY(AD4170_INIT)

/* AD4190-4 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT     adi_ad4190_adc
#define AD4190_INIT(inst) AD4170_ADC_INIT(adi_ad4190_adc, inst, AD4190_CHIP_ID)
DT_INST_FOREACH_STATUS_OKAY(AD4190_INIT)

/* AD4195-4 */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT     adi_ad4195_adc
#define AD4195_INIT(inst) AD4170_ADC_INIT(adi_ad4195_adc, inst, AD4195_CHIP_ID)
DT_INST_FOREACH_STATUS_OKAY(AD4195_INIT)
