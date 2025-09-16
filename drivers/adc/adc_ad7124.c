/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad7124_adc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/crc.h>

LOG_MODULE_REGISTER(adc_ad7124, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

#define AD7124_MAX_RETURNED_DATA_SIZE 6
#define AD7124_ADC_VREF_MV            2500U
#define AD7124_RESOLUTION             24
#define AD7124_SPI_RDY_POLL_CNT       10000

/* Maximum number of channels */
#define AD7124_MAX_CHANNELS 16
/* Total Number of Setups */
#define AD7124_MAX_SETUPS   8

/* AD7124-4 Standard Device ID */
#define AD7124_4_STD_ID     0x04
/* AD7124-4 B Grade Device ID */
#define AD7124_4_B_GRADE_ID 0x06
/* Device ID for the re-designed die in the AD7124-4 standard part and B-grade */
#define AD7124_4_NEW_ID     0x07

/* AD7124-8 Standard Device ID */
#define AD7124_8_STD_ID       0x14
/* AD7124-8 B and W Grade Device ID */
#define AD7124_8_B_W_GRADE_ID 0x16
/* Device ID for the re-designed die in the AD7124-8 standard part, B-grade and W-grade */
#define AD7124_8_NEW_ID       0x17

/* ODR */
#define ADC_ODR_DEFAULT_VALUE  0xA    /* 10SPS */
#define ADC_ODR_MIN_VALUE      0xA    /* 10SPS */
#define ADC_ODR_LOW_POWER_MAX  0x960  /* 2400SPS */
#define ADC_ODR_MID_POWER_MAX  0x12C0 /* 4800SPS */
#define ADC_ODR_HIGH_POWER_MAX 0x4B00 /* 19200SPS */

#define ADC_ODR_SEL_BITS_MAX 0x7FF
#define ADC_ODR_SEL_BITS_MIN 0x1

/* AD7124 registers */
#define AD7124_STATUS       0x00
#define AD7124_ADC_CONTROL  0x01
#define AD7124_DATA         0x02
#define AD7124_IO_CONTROL_1 0x03
#define AD7124_ID           0x05
#define AD7124_ERROR        0x06
#define AD7124_ERROR_EN     0x07
#define AD7124_CHANNEL(x)   (0x09 + (x))
#define AD7124_CONFIG(x)    (0x19 + (x))
#define AD7124_FILTER(x)    (0x21 + (x))

/* Configuration Registers 0-7 bits */
#define AD7124_CFG_REG_BIPOLAR   BIT(11)
#define AD7124_CFG_REG_REF_BUFP  BIT(8)
#define AD7124_CFG_REG_REF_BUFM  BIT(7)
#define AD7124_CFG_REG_AIN_BUFP  BIT(6)
#define AD7124_CFG_REG_AINN_BUFM BIT(5)

#define AD7124_REF_BUF_MSK                GENMASK(8, 7)
#define AD7124_AIN_BUF_MSK                GENMASK(6, 5)
#define AD7124_SETUP_CONF_REG_REF_SEL_MSK GENMASK(4, 3)
#define AD7124_SETUP_CONF_PGA_MSK         GENMASK(2, 0)
#define AD7124_ALL_BUF_MSK                GENMASK(8, 0)

#define AD7124_SETUP_CONFIGURATION_MASK (AD7124_CFG_REG_BIPOLAR | AD7124_ALL_BUF_MSK)

/* ADC_Control Register bits */
#define AD7124_ADC_CTRL_REG_DATA_STATUS BIT(10)
#define AD7124_ADC_CTRL_REG_REF_EN      BIT(8)

/* CRC */
#define AD7124_CRC8_POLYNOMIAL_REPRESENTATION 0x07 /* x8 + x2 + x + 1 */

/* Communication Register bits */
#define AD7124_COMM_REG_WEN   (0 << 7)
#define AD7124_COMM_REG_WR    (0 << 6)
#define AD7124_COMM_REG_RD    BIT(6)
#define AD7124_COMM_REG_RA(x) ((x) & 0x3F)

/* Filter register bits */
#define AD7124_FILTER_CONF_REG_FILTER_MSK GENMASK(23, 21)
#define AD7124_FILTER_FS_MSK              GENMASK(10, 0)

/* Channel register bits */
#define AD7124_CH_MAP_REG_CH_ENABLE    BIT(15)
#define AD7124_CHMAP_REG_SETUP_SEL_MSK GENMASK(14, 12)
#define AD7124_CHMAP_REG_AINPOS_MSK    GENMASK(9, 5)
#define AD7124_CHMAP_REG_AINNEG_MSK    GENMASK(4, 0)

/* Status register bits */
#define AD7124_STATUS_REG_RDY          BIT(7)
#define AD7124_STATUS_REG_POR_FLAG     BIT(4)
#define AD7124_STATUS_REG_CH_ACTIVE(x) ((x) & 0xF)

/* Error_En register bits */
#define AD7124_ERREN_REG_SPI_IGNORE_ERR_EN BIT(6)
#define AD7124_ERREN_REG_SPI_CRC_ERR_EN    BIT(2)

/* ADC control register bits */
#define AD7124_POWER_MODE_MSK        GENMASK(7, 6)
#define AD7124_ADC_CTRL_REG_MODE_MSK GENMASK(5, 2)

/* IO Control 1 register bits */
#define AD7124_IOUT1_CURRENT_MSK GENMASK(13, 11)
#define AD7124_IOUT0_CURRENT_MSK GENMASK(10, 8)
#define AD7124_IOUT1_CHANNEL_MSK GENMASK(7, 4)
#define AD7124_IOUT0_CHANNEL_MSK GENMASK(3, 0)
#define AD7124_IOUT_MSK                                                                            \
	(AD7124_IOUT1_CURRENT_MSK | AD7124_IOUT0_CURRENT_MSK | AD7124_IOUT1_CHANNEL_MSK |          \
	 AD7124_IOUT0_CHANNEL_MSK)

/* Current source configuration bits */
#define AD7124_CURRENT_SOURCE_IOUT_MSK    BIT(3)
#define AD7124_CURRENT_SOURCE_CURRENT_MSK GENMASK(2, 0)
#define AD7124_CURRENT_SOURCE_MASK                                                                 \
	(AD7124_CURRENT_SOURCE_IOUT_MSK | AD7124_CURRENT_SOURCE_CURRENT_MSK)

/* Error register bits */
#define AD7124_ERR_REG_SPI_IGNORE_ERR BIT(6)

enum ad7124_register_lengths {
	AD7124_STATUS_REG_LEN = 1,
	AD7124_ADC_CONTROL_REG_LEN = 2,
	AD7124_DATA_REG_LEN = 3,
	AD7124_IO_CONTROL_1_REG_LEN = 3,
	AD7124_ID_REG_LEN = 1,
	AD7124_ERROR_REG_LEN = 3,
	AD7124_ERROR_EN_REG_LEN = 3,
	AD7124_CHANNEL_REG_LEN = 2,
	AD7124_CONFIG_REG_LEN = 2,
	AD7124_FILTER_REG_LEN = 3,
};

enum ad7124_mode {
	AD7124_CONTINUOUS,
	AD7124_SINGLE,
	AD7124_STANDBY,
	AD7124_POWER_DOWN,
	AD7124_IDLE,
	AD7124_IN_ZERO_SCALE_OFF,
	AD7124_IN_FULL_SCALE_GAIN,
	AD7124_SYS_ZERO_SCALE_OFF,
	AD7124_SYS_ZERO_SCALE_GAIN,
};

enum ad7124_power_mode {
	AD7124_LOW_POWER_MODE,
	AD7124_MID_POWER_MODE,
	AD7124_HIGH_POWER_MODE
};

enum adc_ad7124_master_clk_freq_hz {
	AD7124_LOW_POWER_CLK = 76800,
	AD7124_MID_POWER_CLK = 153600,
	AD7124_HIGH_POWER_CLK = 614400,
};

enum ad7124_device_type {
	ID_AD7124_4,
	ID_AD7124_8
};

struct ad7124_control_status {
	uint16_t value;
	bool is_read;
};

enum ad7124_reference_source {
	/* External Reference REFIN1+/-*/
	EXTERNAL_REFIN1,
	/* External Reference REFIN2+/-*/
	EXTERNAL_REFIN2,
	/* Internal 2.5V Reference */
	INTERNAL_REF,
	/* AVDD - AVSS */
	AVDD_AVSS,
};

enum ad7124_gain {
	AD7124_GAIN_1,
	AD7124_GAIN_2,
	AD7124_GAIN_4,
	AD7124_GAIN_8,
	AD7124_GAIN_16,
	AD7124_GAIN_32,
	AD7124_GAIN_64,
	AD7124_GAIN_128
};

enum ad7124_filter_type {
	AD7124_FILTER_SINC4,
	AD7124_FILTER_SINC3 = 2U,
};

enum ad7124_iout_current {
	AD7124_IOUT_CURRENT_OFF,
	AD7124_IOUT_CURRENT_50_UA,
	AD7124_IOUT_CURRENT_100_UA,
	AD7124_IOUT_CURRENT_250_UA,
	AD7124_IOUT_CURRENT_500_UA,
	AD7124_IOUT_CURRENT_750_UA,
	AD7124_IOUT_CURRENT_1000_UA,
	AD7124_IOUT_CURRENT_0_1_UA,
};

enum ad7124_iout_channel {
	AD7124_IOUT_AIN0 = 0,
	AD7124_IOUT_AIN1 = 1,
	AD7124_IOUT_AIN2 = 4,
	AD7124_IOUT_AIN3 = 5,
	AD7124_IOUT_AIN4 = 10,
	AD7124_IOUT_AIN5 = 11,
	AD7124_IOUT_AIN6 = 14,
	AD7124_IOUT_AIN7 = 15,
};

struct ad7124_current_source_config {
	enum ad7124_iout_current current;
	enum ad7124_iout_channel channel;
};

struct ad7124_config_props {
	enum ad7124_reference_source refsel;
	enum ad7124_gain pga_bits;
	enum ad7124_filter_type filter_type;
	uint16_t odr_sel_bits;
	bool bipolar;
	bool inbuf_enable;
	bool refbuf_enable;
};

struct ad7124_channel_config {
	struct ad7124_config_props props;
	uint8_t cfg_slot;
	bool live_cfg;
};

struct adc_ad7124_config {
	struct spi_dt_spec bus;
	uint16_t filter_type_mask;
	uint16_t bipolar_mask;
	uint16_t inbuf_enable_mask;
	uint16_t refbuf_enable_mask;
	enum ad7124_mode adc_mode;
	enum ad7124_power_mode power_mode;
	enum ad7124_device_type active_device;
	uint8_t resolution;
	bool ref_en;
};

struct adc_ad7124_data {
	const struct device *dev;
	struct adc_context ctx;
	struct ad7124_control_status adc_control_status;
	struct ad7124_channel_config channel_setup_cfg[AD7124_MAX_CHANNELS];
	struct ad7124_current_source_config current_sources[2];
	uint8_t setup_cfg_slots;
	struct k_sem acquire_signal;
	uint16_t channels;
	uint32_t *buffer;
	uint32_t *repeat_buffer;
	bool crc_enable;
	bool spi_ready;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADI_AD7124_ADC_ACQUISITION_THREAD_STACK_SIZE);
#endif /* CONFIG_ADC_ASYNC */
};

static int adc_ad7124_read_reg(const struct device *dev, uint32_t reg,
			       enum ad7124_register_lengths len, uint32_t *val);

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad7124_data *data = CONTAINER_OF(ctx, struct adc_ad7124_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ad7124_data *data = CONTAINER_OF(ctx, struct adc_ad7124_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int adc_ad7124_acq_time_to_odr(const struct device *dev, uint16_t acq_time, uint16_t *odr)
{
	const struct adc_ad7124_config *config = dev->config;
	uint16_t acquisition_time_value = ADC_ACQ_TIME_VALUE(acq_time);
	uint16_t acquisition_time_unit = ADC_ACQ_TIME_UNIT(acq_time);

	/* The AD7124 uses samples per seconds units with the lowest being 10SPS
	 * regardless of the selected power mode and with acquisition_time only
	 * having 14b for time, this will not fit within here for microsecond units.
	 * Use Tick units and allow the user to specify the ODR directly.
	 */

	if (acq_time == ADC_ACQ_TIME_DEFAULT) {
		*odr = ADC_ODR_DEFAULT_VALUE;
		return 0;
	}

	if (acquisition_time_unit != ADC_ACQ_TIME_TICKS) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name, acquisition_time_value);
		return -EINVAL;
	}

	if (acquisition_time_value < ADC_ODR_MIN_VALUE) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name, acquisition_time_value);
		return -EINVAL;
	} else if (config->power_mode == AD7124_HIGH_POWER_MODE &&
		   acquisition_time_value > ADC_ODR_HIGH_POWER_MAX) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name, acquisition_time_value);
		return -EINVAL;
	} else if (config->power_mode == AD7124_MID_POWER_MODE &&
		   acquisition_time_value > ADC_ODR_MID_POWER_MAX) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name, acquisition_time_value);
		return -EINVAL;
	} else if (config->power_mode == AD7124_LOW_POWER_MODE &&
		   acquisition_time_value > ADC_ODR_LOW_POWER_MAX) {
		LOG_ERR("%s: invalid acquisition time %i", dev->name, acquisition_time_value);
		return -EINVAL;
	}

	*odr = acquisition_time_value;

	return 0;
}

static uint16_t adc_ad7124_odr_to_fs(const struct device *dev, int16_t odr)
{
	const struct adc_ad7124_config *config = dev->config;
	uint16_t odr_sel_bits;
	uint32_t master_clk_freq;

	switch (config->power_mode) {
	case AD7124_HIGH_POWER_MODE:
		master_clk_freq = AD7124_HIGH_POWER_CLK;
		break;
	case AD7124_MID_POWER_MODE:
		master_clk_freq = AD7124_MID_POWER_CLK;
		break;
	case AD7124_LOW_POWER_MODE:
		master_clk_freq = AD7124_LOW_POWER_CLK;
		break;
	default:
		LOG_ERR("Invalid power mode (%u)", config->power_mode);
		return -EINVAL;
	}

	if (odr <= 0) {
		LOG_ERR("Invalid ODR value: %d", odr);
		return -EINVAL;
	}

	odr_sel_bits = DIV_ROUND_CLOSEST(master_clk_freq, (uint32_t)odr * 32);

	if (odr_sel_bits < ADC_ODR_SEL_BITS_MIN) {
		odr_sel_bits = ADC_ODR_SEL_BITS_MIN;
	} else if (odr_sel_bits > ADC_ODR_SEL_BITS_MAX) {
		odr_sel_bits = ADC_ODR_SEL_BITS_MAX;
	}

	return odr_sel_bits;
}

static int adc_ad7124_create_new_cfg(const struct device *dev, const struct adc_channel_cfg *cfg,
				     struct ad7124_channel_config *new_cfg)
{
	const struct adc_ad7124_config *config = dev->config;
	uint16_t odr;
	enum ad7124_reference_source ref_source;
	enum ad7124_gain gain;
	int ret;

	if (cfg->channel_id >= AD7124_MAX_CHANNELS) {
		LOG_ERR("Invalid channel (%u)", cfg->channel_id);
		return -EINVAL;
	}

	switch (cfg->reference) {
	case ADC_REF_INTERNAL:
		ref_source = INTERNAL_REF;
		break;
	case ADC_REF_EXTERNAL0:
		ref_source = EXTERNAL_REFIN1;
		break;
	case ADC_REF_EXTERNAL1:
		ref_source = EXTERNAL_REFIN2;
		break;
	case ADC_REF_VDD_1:
		ref_source = AVDD_AVSS;
		break;
	default:
		LOG_ERR("Invalid reference source (%u)", cfg->reference);
		return -EINVAL;
	}

	new_cfg->props.refsel = ref_source;

	switch (cfg->gain) {
	case ADC_GAIN_1:
		gain = AD7124_GAIN_1;
		break;
	case ADC_GAIN_2:
		gain = AD7124_GAIN_2;
		break;
	case ADC_GAIN_4:
		gain = AD7124_GAIN_4;
		break;
	case ADC_GAIN_8:
		gain = AD7124_GAIN_8;
		break;
	case ADC_GAIN_16:
		gain = AD7124_GAIN_16;
		break;
	case ADC_GAIN_32:
		gain = AD7124_GAIN_32;
		break;
	case ADC_GAIN_64:
		gain = AD7124_GAIN_64;
		break;
	case ADC_GAIN_128:
		gain = AD7124_GAIN_128;
		break;
	default:
		LOG_ERR("Invalid gain value (%u)", cfg->gain);
		return -EINVAL;
	}

	new_cfg->props.pga_bits = gain;

	ret = adc_ad7124_acq_time_to_odr(dev, cfg->acquisition_time, &odr);
	if (ret) {
		return ret;
	}

	if (config->filter_type_mask & BIT(cfg->channel_id)) {
		new_cfg->props.filter_type = AD7124_FILTER_SINC3;
	} else {
		new_cfg->props.filter_type = AD7124_FILTER_SINC4;
	}

	new_cfg->props.odr_sel_bits = adc_ad7124_odr_to_fs(dev, odr);
	new_cfg->props.bipolar = config->bipolar_mask & BIT(cfg->channel_id);
	new_cfg->props.inbuf_enable = config->inbuf_enable_mask & BIT(cfg->channel_id);
	new_cfg->props.refbuf_enable = config->refbuf_enable_mask & BIT(cfg->channel_id);

	return 0;
}

static int adc_ad7124_find_new_slot(const struct device *dev)
{
	struct adc_ad7124_data *data = dev->data;
	uint8_t slot = data->setup_cfg_slots;

	int cnt = 0;

	while (slot) {
		if ((slot & 0x1) == 0) {
			return cnt;
		}
		slot >>= 1;
		cnt++;
	}

	if (cnt == AD7124_MAX_SETUPS) {
		return -1;
	}

	return cnt;
}

static int adc_ad7124_find_similar_configuration(const struct device *dev,
						 const struct ad7124_channel_config *cfg,
						 int channel_id)
{
	struct adc_ad7124_data *data = dev->data;
	int similar_channel_index = -1;

	for (int i = 0; i < AD7124_MAX_CHANNELS; i++) {
		if (!data->channel_setup_cfg[i].live_cfg && i == channel_id) {
			continue;
		}

		if (memcmp(&cfg->props, &data->channel_setup_cfg[i].props,
			   sizeof(struct ad7124_config_props)) == 0) {
			similar_channel_index = i;
			break;
		}
	}

	return similar_channel_index;
}

static int adc_ad7124_wait_for_spi_ready(const struct device *dev)
{
	int ret = 0;
	uint32_t read_val = 0;
	bool ready = false;
	uint16_t spi_ready_try_count = AD7124_SPI_RDY_POLL_CNT;

	while (!ready && --spi_ready_try_count) {
		ret = adc_ad7124_read_reg(dev, AD7124_ERROR, AD7124_ERROR_REG_LEN, &read_val);
		if (ret) {
			return ret;
		}

		ready = (read_val & AD7124_ERR_REG_SPI_IGNORE_ERR) == 0;
	}

	if (!spi_ready_try_count) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int adc_ad7124_read_reg(const struct device *dev, uint32_t reg,
			       enum ad7124_register_lengths len, uint32_t *val)
{
	const struct adc_ad7124_config *config = dev->config;
	struct adc_ad7124_data *data = dev->data;
	const struct spi_dt_spec *spec = &config->bus;

	int ret;
	uint32_t cntrl_value = 0;
	uint8_t add_status_length = 0;
	uint8_t buffer_tx[AD7124_MAX_RETURNED_DATA_SIZE] = {0};
	uint8_t buffer_rx[ARRAY_SIZE(buffer_tx)];
	uint8_t crc_check;

	if (reg != AD7124_ERROR && data->spi_ready) {
		ret = adc_ad7124_wait_for_spi_ready(dev);
		if (ret) {
			return ret;
		}
	}

	if (reg == AD7124_DATA) {

		if (data->adc_control_status.is_read) {
			cntrl_value = data->adc_control_status.value;
		} else {
			ret = adc_ad7124_read_reg(dev, AD7124_ADC_CONTROL,
						  AD7124_ADC_CONTROL_REG_LEN, &cntrl_value);
			if (ret) {
				return ret;
			}
		}

		if (cntrl_value & AD7124_ADC_CTRL_REG_DATA_STATUS) {
			add_status_length = 1;
		}
	}

	struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = 1,
	}};
	struct spi_buf rx_buf[] = {{
		.buf = buffer_rx,
		.len = ((data->crc_enable) ? len + 1 : len) + 1 + add_status_length,
	}};
	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};
	const struct spi_buf_set rx = {.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)};

	buffer_tx[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_RD | AD7124_COMM_REG_RA(reg);

	ret = spi_transceive_dt(spec, &tx, &rx);
	if (ret) {
		return ret;
	}

	if (data->crc_enable) {
		buffer_rx[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_RD | AD7124_COMM_REG_RA(reg);

		crc_check = crc8(buffer_rx, len + 2 + add_status_length,
				 AD7124_CRC8_POLYNOMIAL_REPRESENTATION, 0, false);
		if (crc_check) {
			return -EBADMSG;
		}
	}

	switch (len) {
	case 1:
		*val = buffer_rx[1];
		break;
	case 2:
		*val = sys_get_be16(&buffer_rx[1]);
		break;
	case 3:
		*val = sys_get_be24(&buffer_rx[1]);
		break;
	default:
		return -EINVAL;
	}

	if (reg == AD7124_ADC_CONTROL) {
		data->adc_control_status.value = *val;
		data->adc_control_status.is_read = true;
	}

	return 0;
}

static int adc_ad7124_write_reg(const struct device *dev, uint32_t reg,
				enum ad7124_register_lengths len, uint32_t val)
{
	const struct adc_ad7124_config *config = dev->config;
	struct adc_ad7124_data *data = dev->data;
	const struct spi_dt_spec *spec = &config->bus;

	int ret;
	uint8_t buffer_tx[AD7124_MAX_RETURNED_DATA_SIZE] = {0};
	uint8_t crc;

	if (data->spi_ready) {
		ret = adc_ad7124_wait_for_spi_ready(dev);
		if (ret) {
			return ret;
		}
	}

	buffer_tx[0] = AD7124_COMM_REG_WEN | AD7124_COMM_REG_WR | AD7124_COMM_REG_RA(reg);

	switch (len) {
	case 1:
		buffer_tx[1] = val;
		break;
	case 2:
		sys_put_be16(val, &buffer_tx[1]);
		break;
	case 3:
		sys_put_be24(val, &buffer_tx[1]);
		break;
	default:
		return -EINVAL;
	}

	if (data->crc_enable) {
		crc = crc8(buffer_tx, len + 1, AD7124_CRC8_POLYNOMIAL_REPRESENTATION, 0, false);
		buffer_tx[len + 1] = crc;
	}

	struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ((data->crc_enable) ? len + 1 : len) + 1,
	}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	ret = spi_transceive_dt(spec, &tx, NULL);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_reg_write_msk(const struct device *dev, uint32_t reg,
				    enum ad7124_register_lengths len, uint32_t data, uint32_t mask)
{
	int ret;
	uint32_t reg_data;

	ret = adc_ad7124_read_reg(dev, reg, len, &reg_data);
	if (ret) {
		return ret;
	}

	reg_data &= ~mask;
	reg_data |= data;

	ret = adc_ad7124_write_reg(dev, reg, len, reg_data);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_setup_cfg(const struct device *dev, const struct ad7124_channel_config *cfg)
{
	const struct adc_ad7124_config *config = dev->config;
	int ret;
	int configuration_setup = 0;
	int configuration_mask = 0;
	int ref_internal = 0;

	if (cfg->props.bipolar) {
		configuration_setup |= AD7124_CFG_REG_BIPOLAR;
	}

	if (cfg->props.inbuf_enable) {
		configuration_setup |= AD7124_CFG_REG_AIN_BUFP | AD7124_CFG_REG_AINN_BUFM;
	}

	if (cfg->props.refbuf_enable) {
		configuration_setup |= AD7124_CFG_REG_REF_BUFP | AD7124_CFG_REG_REF_BUFM;
	}

	configuration_setup |= FIELD_PREP(AD7124_SETUP_CONF_REG_REF_SEL_MSK, cfg->props.refsel);
	configuration_setup |= FIELD_PREP(AD7124_SETUP_CONF_PGA_MSK, cfg->props.pga_bits);
	configuration_mask |= AD7124_SETUP_CONFIGURATION_MASK;

	ret = adc_ad7124_reg_write_msk(dev, AD7124_CONFIG(cfg->cfg_slot), AD7124_CONFIG_REG_LEN,
				       configuration_setup, configuration_mask);
	if (ret) {
		return ret;
	}

	if (config->ref_en) {
		ref_internal = AD7124_ADC_CTRL_REG_REF_EN;
	}

	if (cfg->props.refsel == INTERNAL_REF) {
		ret = adc_ad7124_reg_write_msk(dev, AD7124_ADC_CONTROL, AD7124_ADC_CONTROL_REG_LEN,
					       ref_internal, AD7124_ADC_CTRL_REG_REF_EN);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int adc_ad7124_filter_cfg(const struct device *dev, const struct ad7124_channel_config *cfg)
{
	int filter_setup = 0;
	int filter_mask = 0;
	int ret;

	filter_setup = FIELD_PREP(AD7124_FILTER_CONF_REG_FILTER_MSK, cfg->props.filter_type) |
		       FIELD_PREP(AD7124_FILTER_FS_MSK, cfg->props.odr_sel_bits);
	filter_mask = AD7124_FILTER_CONF_REG_FILTER_MSK | AD7124_FILTER_FS_MSK;

	/* Set filter type and odr*/
	ret = adc_ad7124_reg_write_msk(dev, AD7124_FILTER(cfg->cfg_slot), AD7124_FILTER_REG_LEN,
				       filter_setup, filter_mask);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_connect_analog_input(const struct device *dev, uint8_t chn_num, uint8_t ainp,
					   uint8_t ainm)
{
	int ret;

	ret = adc_ad7124_reg_write_msk(dev, AD7124_CHANNEL(chn_num), AD7124_CHANNEL_REG_LEN,
				       FIELD_PREP(AD7124_CHMAP_REG_AINPOS_MSK, ainp),
				       AD7124_CHMAP_REG_AINPOS_MSK);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_reg_write_msk(dev, AD7124_CHANNEL(chn_num), AD7124_CHANNEL_REG_LEN,
				       FIELD_PREP(AD7124_CHMAP_REG_AINNEG_MSK, ainm),
				       AD7124_CHMAP_REG_AINNEG_MSK);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_set_channel_status(const struct device *dev, uint8_t chn_num,
					 bool channel_status)
{
	int ret;
	uint16_t status;

	if (channel_status) {
		status = AD7124_CH_MAP_REG_CH_ENABLE;
	} else {
		status = 0x0U;
	}

	ret = adc_ad7124_reg_write_msk(dev, AD7124_CHANNEL(chn_num), AD7124_CHANNEL_REG_LEN, status,
				       AD7124_CH_MAP_REG_CH_ENABLE);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_channel_cfg(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	struct adc_ad7124_data *data = dev->data;
	int ret;

	ret = adc_ad7124_connect_analog_input(dev, cfg->channel_id, cfg->input_positive,
					      cfg->input_negative);
	if (ret) {
		return ret;
	}

	/* Assign setup */
	ret = adc_ad7124_reg_write_msk(
		dev, AD7124_CHANNEL(cfg->channel_id), AD7124_CHANNEL_REG_LEN,
		FIELD_PREP(AD7124_CHMAP_REG_SETUP_SEL_MSK,
			   data->channel_setup_cfg[cfg->channel_id].cfg_slot),
		AD7124_CHMAP_REG_SETUP_SEL_MSK);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_set_channel_status(dev, cfg->channel_id, true);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_enable_current_sources(const struct device *dev,
					     const struct adc_channel_cfg *cfg)
{
	struct adc_ad7124_data *data = dev->data;
	uint8_t iout_idx;

	if (cfg->current_source_pin[0] > AD7124_CURRENT_SOURCE_MASK) {
		LOG_ERR("Invalid current source configuration %u", cfg->current_source_pin[0]);
		return -EINVAL;
	}

	iout_idx = FIELD_GET(AD7124_CURRENT_SOURCE_IOUT_MSK, cfg->current_source_pin[0]);
	data->current_sources[iout_idx].current = (enum ad7124_iout_current)FIELD_GET(
		AD7124_CURRENT_SOURCE_CURRENT_MSK, cfg->current_source_pin[0]);
	data->current_sources[iout_idx].channel = cfg->current_source_pin[1];

	uint32_t value = FIELD_PREP(AD7124_IOUT0_CURRENT_MSK, data->current_sources[0].current) |
			 FIELD_PREP(AD7124_IOUT0_CHANNEL_MSK, data->current_sources[0].channel) |
			 FIELD_PREP(AD7124_IOUT1_CURRENT_MSK, data->current_sources[1].current) |
			 FIELD_PREP(AD7124_IOUT1_CHANNEL_MSK, data->current_sources[1].channel);

	return adc_ad7124_reg_write_msk(dev, AD7124_IO_CONTROL_1, AD7124_IO_CONTROL_1_REG_LEN,
					FIELD_PREP(AD7124_IOUT_MSK, value), AD7124_IOUT_MSK);
}

static int adc_ad7124_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	struct adc_ad7124_data *data = dev->data;
	struct ad7124_channel_config new_cfg;
	int new_slot;
	int ret;
	int similar_channel_index;

	data->channel_setup_cfg[cfg->channel_id].live_cfg = false;

	ret = adc_ad7124_create_new_cfg(dev, cfg, &new_cfg);
	if (ret) {
		return ret;
	}

	/* AD7124 supports only 8 different configurations for 16 channels*/
	new_slot = adc_ad7124_find_new_slot(dev);

	if (new_slot == -1) {
		similar_channel_index =
			adc_ad7124_find_similar_configuration(dev, &new_cfg, cfg->channel_id);
		if (similar_channel_index == -1) {
			return -EINVAL;
		}
		new_cfg.cfg_slot = data->channel_setup_cfg[similar_channel_index].cfg_slot;
	} else {
		new_cfg.cfg_slot = new_slot;
		WRITE_BIT(data->setup_cfg_slots, new_slot, true);
	}

	new_cfg.live_cfg = true;

	memcpy(&data->channel_setup_cfg[cfg->channel_id], &new_cfg,
	       sizeof(struct ad7124_channel_config));

	/* Setup the channel configuration */
	ret = adc_ad7124_setup_cfg(dev, &data->channel_setup_cfg[cfg->channel_id]);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	/* Setup the filter configuration */
	ret = adc_ad7124_filter_cfg(dev, &data->channel_setup_cfg[cfg->channel_id]);
	if (ret) {
		LOG_ERR("Error setting up filter");
		return ret;
	}

	if (cfg->current_source_pin_set) {
		ret = adc_ad7124_enable_current_sources(dev, cfg);
		if (ret) {
			LOG_ERR("Error setting up current sources");
			return ret;
		}
	}

	/* Setup the channel */
	ret = adc_ad7124_channel_cfg(dev, cfg);
	if (ret) {
		LOG_ERR("Error setting up channel");
		return ret;
	}

	WRITE_BIT(data->channels, cfg->channel_id, true);

	return 0;
}

int adc_ad7124_wait_to_power_up(const struct device *dev)
{
	int ret = 0;
	uint32_t read_val = 0;
	bool powered_on = false;
	uint16_t spi_ready_try_count = AD7124_SPI_RDY_POLL_CNT;

	while (!powered_on && --spi_ready_try_count) {

		ret = adc_ad7124_read_reg(dev, AD7124_STATUS, AD7124_STATUS_REG_LEN, &read_val);
		if (ret) {
			return ret;
		}

		powered_on = (read_val & AD7124_STATUS_REG_POR_FLAG) == 0;
	}

	if (!spi_ready_try_count) {
		return -ETIMEDOUT;
	}

	return 0;
}

int adc_ad7124_reset(const struct device *dev)
{
	const struct adc_ad7124_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;

	int ret;
	uint8_t buffer_tx[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	struct spi_buf tx_buf[] = {{
		.buf = buffer_tx,
		.len = ARRAY_SIZE(buffer_tx),
	}};

	const struct spi_buf_set tx = {.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)};

	ret = spi_transceive_dt(spec, &tx, NULL);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_wait_to_power_up(dev);
	if (ret) {
		return ret;
	}

	return ret;
}

static int adc_ad7124_update_crc(const struct device *dev)
{
	struct adc_ad7124_data *data = dev->data;

	int ret = 0;
	uint32_t reg_temp = 0;

	ret = adc_ad7124_read_reg(dev, AD7124_ERROR_EN, AD7124_ERROR_EN_REG_LEN, &reg_temp);
	if (ret) {
		return ret;
	}

	if (reg_temp & AD7124_ERREN_REG_SPI_CRC_ERR_EN) {
		data->crc_enable = true;
	} else {
		data->crc_enable = false;
	}

	return 0;
}

static int adc_ad7124_update_spi_check_ready(const struct device *dev)
{
	struct adc_ad7124_data *data = dev->data;

	int ret = 0;
	uint32_t reg_temp = 0;

	ret = adc_ad7124_read_reg(dev, AD7124_ERROR_EN, AD7124_ERROR_EN_REG_LEN, &reg_temp);
	if (ret) {
		return ret;
	}

	if (reg_temp & AD7124_ERREN_REG_SPI_IGNORE_ERR_EN) {
		data->spi_ready = true;
	} else {
		data->spi_ready = false;
	}

	return 0;
}

static int adc_ad7124_check_chip_id(const struct device *dev)
{
	const struct adc_ad7124_config *config = dev->config;
	uint32_t reg_temp = 0;
	int ret;

	ret = adc_ad7124_read_reg(dev, AD7124_ID, AD7124_ID_REG_LEN, &reg_temp);
	if (ret) {
		return ret;
	}

	if (config->active_device == ID_AD7124_4) {
		switch (reg_temp) {
		case AD7124_4_STD_ID:
		case AD7124_4_B_GRADE_ID:
		case AD7124_4_NEW_ID:
			break;

		default:
			return -ENODEV;
		}
	} else if (config->active_device == ID_AD7124_8) {
		switch (reg_temp) {
		case AD7124_8_STD_ID:
		case AD7124_8_B_W_GRADE_ID:
		case AD7124_8_NEW_ID:
			break;

		default:
			return -ENODEV;
		}
	} else {
		return -ENODEV;
	}

	return 0;
}

static int adc_ad7124_set_adc_mode(const struct device *dev, enum ad7124_mode adc_mode)
{
	int ret;

	ret = adc_ad7124_reg_write_msk(dev, AD7124_ADC_CONTROL, AD7124_ADC_CONTROL_REG_LEN,
				       FIELD_PREP(AD7124_ADC_CTRL_REG_MODE_MSK, adc_mode),
				       AD7124_ADC_CTRL_REG_MODE_MSK);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_set_power_mode(const struct device *dev, enum ad7124_power_mode power_mode)
{
	int ret;

	ret = adc_ad7124_reg_write_msk(dev, AD7124_ADC_CONTROL, AD7124_ADC_CONTROL_REG_LEN,
				       FIELD_PREP(AD7124_POWER_MODE_MSK, power_mode),
				       AD7124_POWER_MODE_MSK);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_setup(const struct device *dev)
{
	const struct adc_ad7124_config *config = dev->config;
	int ret;

	/* Reset the device interface */
	ret = adc_ad7124_reset(dev);
	if (ret) {
		return ret;
	}

	/* Get CRC State */
	ret = adc_ad7124_update_crc(dev);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_update_spi_check_ready(dev);
	if (ret) {
		return ret;
	}

	/* Check the device ID */
	ret = adc_ad7124_check_chip_id(dev);
	if (ret) {
		return ret;
	}

	/* Disable channel 0 */
	ret = adc_ad7124_set_channel_status(dev, 0, false);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_set_adc_mode(dev, config->adc_mode);
	if (ret) {
		return ret;
	}

	ret = adc_ad7124_set_power_mode(dev, config->power_mode);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad7124_wait_for_conv_ready(const struct device *dev)
{
	int ret = 0;
	uint32_t read_val = 0;
	bool ready = false;
	uint16_t spi_ready_try_count = AD7124_SPI_RDY_POLL_CNT;

	while (!ready && --spi_ready_try_count) {

		ret = adc_ad7124_read_reg(dev, AD7124_STATUS, AD7124_STATUS_REG_LEN, &read_val);
		if (ret) {
			return ret;
		}

		ready = (read_val & AD7124_STATUS_REG_RDY) == 0;
	}

	if (!spi_ready_try_count) {
		return -ETIMEDOUT;
	}

	return 0;
}

static bool get_next_ch_idx(uint16_t ch_mask, uint16_t last_idx, uint16_t *new_idx)
{
	last_idx++;
	if (last_idx >= AD7124_MAX_CHANNELS) {
		return 0;
	}
	ch_mask >>= last_idx;
	if (!ch_mask) {
		*new_idx = -1;
		return 0;
	}
	while (!(ch_mask & 1)) {
		last_idx++;
		ch_mask >>= 1;
	}
	*new_idx = last_idx;

	return 1;
}

static int adc_ad7124_get_read_chan_id(const struct device *dev, uint16_t *chan_id)
{
	int ret;
	uint32_t reg_temp;

	ret = adc_ad7124_read_reg(dev, AD7124_STATUS, AD7124_STATUS_REG_LEN, &reg_temp);
	if (ret) {
		return ret;
	}

	*chan_id = reg_temp & AD7124_STATUS_REG_CH_ACTIVE(0xF);

	return 0;
}

static int adc_ad7124_perform_read(const struct device *dev)
{
	int ret = 0;
	struct adc_ad7124_data *data = dev->data;
	uint16_t ch_idx = -1;
	uint16_t prev_ch_idx = -1;
	uint16_t adc_ch_id = 0;
	bool status;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	do {
		prev_ch_idx = ch_idx;

		status = get_next_ch_idx(data->ctx.sequence.channels, ch_idx, &ch_idx);
		if (!status) {
			break;
		}

		ret = adc_ad7124_wait_for_conv_ready(dev);
		if (ret) {
			LOG_ERR("waiting for conversion ready failed");
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		ret = adc_ad7124_read_reg(dev, AD7124_DATA, AD7124_DATA_REG_LEN, data->buffer);
		if (ret) {
			LOG_ERR("reading sample failed");
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		ret = adc_ad7124_get_read_chan_id(dev, &adc_ch_id);
		if (ret) {
			LOG_ERR("reading channel id failed");
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

	return ret;
}

static int adc_ad7124_validate_sequence(const struct device *dev,
					const struct adc_sequence *sequence)
{
	const struct adc_ad7124_config *config = dev->config;
	struct adc_ad7124_data *data = dev->data;
	const size_t channel_maximum = 8 * sizeof(sequence->channels);
	uint32_t num_requested_channels;
	size_t necessary;

	if (sequence->resolution != config->resolution) {
		LOG_ERR("invalid resolution");
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

		if (i >= AD7124_MAX_CHANNELS) {
			LOG_ERR("invalid channel selection");
			return -EINVAL;
		}
	}

	return 0;
}

static int adc_ad7124_start_read(const struct device *dev, const struct adc_sequence *sequence,
				 bool wait)
{
	int result;
	struct adc_ad7124_data *data = dev->data;

	result = adc_ad7124_validate_sequence(dev, sequence);
	if (result != 0) {
		LOG_ERR("sequence validation failed");
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
static int adc_ad7124_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	int status;
	struct adc_ad7124_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_ad7124_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

static int adc_ad7124_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_ad7124_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_ad7124_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

#else
static int adc_ad7124_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_ad7124_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);

	status = adc_ad7124_start_read(dev, sequence, false);

	while (status == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		status = adc_ad7124_perform_read(dev);
	}

	adc_context_release(&data->ctx, status);

	return status;
}
#endif

#if CONFIG_ADC_ASYNC
static void adc_ad7124_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		adc_ad7124_perform_read(dev);
	}
}
#endif /* CONFIG_ADC_ASYNC */

static int adc_ad7124_init(const struct device *dev)
{
	const struct adc_ad7124_config *config = dev->config;
	struct adc_ad7124_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_sem_init(&data->acquire_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("spi bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = adc_ad7124_setup(dev);
	if (ret) {
		return ret;
	}

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(
		&data->thread, data->stack, CONFIG_ADI_AD7124_ADC_ACQUISITION_THREAD_STACK_SIZE,
		adc_ad7124_acquisition_thread, (void *)dev, NULL, NULL,
		CONFIG_ADI_AD7124_ADC_ACQUISITION_THREAD_INIT_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ad7124");
#endif /* CONFIG_ADC_ASYNC */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_ad7124_api) = {
	.channel_setup = adc_ad7124_channel_setup,
	.read = adc_ad7124_read,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ad7124_read_async,
#endif
	.ref_internal = AD7124_ADC_VREF_MV,
};

#define ADC_AD7124_INST_DEFINE(inst)                                                               \
	static const struct adc_ad7124_config adc_ad7124_config##inst = {                          \
		.bus = SPI_DT_SPEC_INST_GET(                                                       \
			inst,                                                                      \
			SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | SPI_WORD_SET(8), 0),  \
		.resolution = AD7124_RESOLUTION,                                                   \
		.filter_type_mask = DT_INST_PROP(inst, filter_type_mask),                          \
		.bipolar_mask = DT_INST_PROP(inst, bipolar_mask),                                  \
		.inbuf_enable_mask = DT_INST_PROP(inst, inbuf_enable_mask),                        \
		.refbuf_enable_mask = DT_INST_PROP(inst, refbuf_enable_mask),                      \
		.adc_mode = DT_INST_PROP(inst, adc_mode),                                          \
		.power_mode = DT_INST_PROP(inst, power_mode),                                      \
		.active_device = DT_INST_PROP(inst, active_device),                                \
		.ref_en = DT_INST_PROP(inst, reference_enable),                                    \
	};                                                                                         \
	static struct adc_ad7124_data adc_ad7124_data##inst = {                                    \
		ADC_CONTEXT_INIT_LOCK(adc_ad7124_data##inst, ctx),                                 \
		ADC_CONTEXT_INIT_TIMER(adc_ad7124_data##inst, ctx),                                \
		ADC_CONTEXT_INIT_SYNC(adc_ad7124_data##inst, ctx),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, adc_ad7124_init, NULL, &adc_ad7124_data##inst,                 \
			      &adc_ad7124_config##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,     \
			      &adc_ad7124_api);

DT_INST_FOREACH_STATUS_OKAY(ADC_AD7124_INST_DEFINE)
