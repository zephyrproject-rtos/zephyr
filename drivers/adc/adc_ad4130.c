/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ad4130_adc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(adc_ad4130, CONFIG_ADC_LOG_LEVEL);

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

/* ---------------------------------------
 * AD4130 Register Addresses
 * ---------------------------------------
 */

#define AD4130_STATUS_REG       0x00
#define AD4130_ADC_CONTROL_REG  0x01
#define AD4130_DATA_REG         0x02
#define AD4130_IO_CONTROL_REG   0x03
#define AD4130_VBIAS_REG        0x04
#define AD4130_ID_REG           0x05
#define AD4130_ERROR_REG        0x06
#define AD4130_ERROR_EN_REG     0x07
#define AD4130_MCLK_COUNT_REG   0x08
#define AD4130_CHANNEL_X_REG(x) (0x09 + (x))
#define AD4130_CONFIG_X_REG(x)  (0x19 + (x))
#define AD4130_FILTER_X_REG(x)  (0x21 + (x))

/* ---------------------------------------
 * AD4130 Status Flags and Bit Masks
 * ---------------------------------------
 */

#define AD4130_STATUS_REG_DATA_READY BIT(7)
#define AD4130_COMMS_READ_MASK       BIT(6)

/* ---------------------------------------
 * AD4130 ADC Control Bit Masks
 * ---------------------------------------
 */

#define AD4130_ADC_CONTROL_BIPOLAR_MASK     BIT(14)
#define AD4130_ADC_CONTROL_INT_REF_VAL_MASK BIT(13)
#define AD4130_ADC_CONTROL_CSB_EN_MASK      BIT(9)
#define AD4130_ADC_CONTROL_INT_REF_EN_MASK  BIT(8)
#define AD4130_ADC_CONTROL_MODE_MASK        GENMASK(5, 2)
#define AD4130_ADC_CONTROL_MCLK_SEL_MASK    GENMASK(1, 0)

/* ---------------------------------------
 * AD4130 Channel Configuration Bit Masks
 * ---------------------------------------
 */

#define AD4130_CHANNEL_EN_MASK    BIT(23)
#define AD4130_CHANNEL_SETUP_MASK GENMASK(22, 20)
#define AD4130_CHANNEL_AINP_MASK  GENMASK(17, 13)
#define AD4130_CHANNEL_AINM_MASK  GENMASK(12, 8)

/* ---------------------------------------
 * AD4130 Configuration Register Bit Masks
 * ---------------------------------------
 */

#define AD4130_CONFIG_REF_SEL_MASK GENMASK(5, 4)
#define AD4130_CONFIG_PGA_MASK     GENMASK(3, 1)

/* ---------------------------------------
 * AD4130 Device-Specific Parameters
 * ---------------------------------------
 */

#define AD4130_MAX_CHANNELS 16
#define AD4130_MAX_SETUPS   8

/* ---------------------------------------
 * AD4130 Reset Parameters
 * ---------------------------------------
 */

#define AD4130_RESET_BUF_SIZE 8
#define AD4130_RESET_SLEEP_MS (160 * 1000 / AD4130_MCLK_FREQ_76_8KHZ)

/* ---------------------------------------
 * AD4130 Error Values
 * ---------------------------------------
 */

#define AD4130_INVALID_CHANNEL -1
#define AD4130_INVALID_SLOT    -1

/* ---------------------------------------
 * AD4130 Electrical Characteristics
 * ---------------------------------------
 */

#define AD4130_INT_REF_2_5V      2500U
#define AD4130_ADC_RESOLUTION    24
#define AD4130_MCLK_FREQ_76_8KHZ 76800

/* ---------------------------------------
 * AD4130-8 Identification
 * ---------------------------------------
 */

#define AD4130_8_ID 0x04 /* AD4130-8 Device ID */

static const unsigned int ad4130_reg_size[] = {
	[AD4130_STATUS_REG] = 1,
	[AD4130_ADC_CONTROL_REG] = 2,
	[AD4130_DATA_REG] = 3,
	[AD4130_VBIAS_REG] = 2,
	[AD4130_ID_REG] = 1,
	[AD4130_CHANNEL_X_REG(0)... AD4130_CHANNEL_X_REG(AD4130_MAX_CHANNELS - 1)] = 3,
	[AD4130_CONFIG_X_REG(0)... AD4130_CONFIG_X_REG(AD4130_MAX_SETUPS - 1)] = 2,
	[AD4130_FILTER_X_REG(0)... AD4130_FILTER_X_REG(AD4130_MAX_SETUPS - 1)] = 3,
};

enum ad4130_input {
	AD4130_AIN0,
	AD4130_AIN1,
	AD4130_AIN2,
	AD4130_AIN3,
	AD4130_AIN4,
	AD4130_AIN5,
	AD4130_AIN6,
	AD4130_AIN7,
	AD4130_AIN8,
	AD4130_AIN9,
	AD4130_AIN10,
	AD4130_AIN11,
	AD4130_AIN12,
	AD4130_AIN13,
	AD4130_AIN14,
	AD4130_AIN15,
	AD4130_TEMP,
	AD4130_AVSS,
	AD4130_INT_REF,
	AD4130_DGND,
	AD4130_AVDD_AVSS_6P,
	AD4130_AVDD_AVSS_6M,
	AD4130_IOVDD_DGND_6P,
	AD4130_IOVDD_DGND_6M,
	AD4130_ALDO_AVSS_6P,
	AD4130_ALDO_AVSS_6M,
	AD4130_DLDO_DGND_6P,
	AD4130_DLDO_DGND_6M,
	AD4130_V_MV_P,
	AD4130_V_MV_M,
};

enum ad4130_int_ref {
	AD4130_INT_REF_VAL_2_5V,
	AD4130_INT_REF_VAL_1_25V,
};

enum ad4130_setup {
	AD4130_SETUP_0,
	AD4130_SETUP_1,
	AD4130_SETUP_2,
	AD4130_SETUP_3,
	AD4130_SETUP_4,
	AD4130_SETUP_5,
	AD4130_SETUP_6,
	AD4130_SETUP_7
};

enum ad4130_mclk_sel {
	AD4130_MCLK_76_8KHZ,
	AD4130_MCLK_76_8KHZ_OUT,
	AD4130_MCLK_76_8KHZ_EXT,
	AD4130_MCLK_153_6KHZ_EXT,
};

enum ad4130_ref_sel {
	AD4130_REF_REFIN1,
	AD4130_REF_REFIN2,
	AD4130_REF_REFOUT_AVSS,
	AD4130_REF_AVDD_AVSS,
	AD4130_REF_SEL_MAX
};

enum ad4130_adc_mode {
	AD4130_MODE_CONTINUOUS = 0b0000,
	AD4130_STANDBY_MODE = 0b0010,
	AD4130_MODE_IDLE = 0b0100,
};

enum ad4130_gain {
	AD4130_GAIN_1,
	AD4130_GAIN_2,
	AD4130_GAIN_4,
	AD4130_GAIN_8,
	AD4130_GAIN_16,
	AD4130_GAIN_32,
	AD4130_GAIN_64,
	AD4130_GAIN_128,
};

struct ad4130_config_props {
	enum ad4130_ref_sel ref_sel;
	enum ad4130_gain gain;
};

struct ad4130_channel_config {
	struct ad4130_config_props props;
	uint8_t cfg_slot;
	bool live_cfg;
};

struct ad4130_config {
	struct spi_dt_spec bus;
	uint8_t resolution;
	bool bipolar;
	enum ad4130_int_ref int_ref;
	enum ad4130_adc_mode adc_mode;
	enum ad4130_mclk_sel mclk_sel;
};

struct adc_ad4130_data {
	const struct device *dev;
	struct adc_context ctx;
	struct ad4130_channel_config channel_setup_cfg[AD4130_MAX_SETUPS];
	uint8_t setup_cfg_slots;
	struct k_sem acquire_signal;
	uint16_t channels;
	uint32_t *buffer;
	uint32_t *repeat_buffer;
#if CONFIG_ADC_ASYNC
	struct k_thread thread;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADI_AD4130_ADC_ACQUISITION_THREAD_STACK_SIZE);
#endif /* CONFIG_ADC_ASYNC */
};

static size_t ad4130_get_reg_size(unsigned int reg_addr, size_t *reg_size)
{
	if (reg_addr >= ARRAY_SIZE(ad4130_reg_size)) {
		return -EINVAL;
	}

	*reg_size = ad4130_reg_size[reg_addr];
	return 0;
}

static int ad4130_reg_write(const struct device *dev, unsigned int reg_addr, unsigned int val)
{
	const struct ad4130_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;

	uint8_t reg_write_tx_buf[4] = {0};
	size_t reg_size = 0;
	int ret;

	ret = ad4130_get_reg_size(reg_addr, &reg_size);
	if (ret) {
		return ret;
	}

	reg_write_tx_buf[0] = reg_addr;

	switch (reg_size) {
	case 1:
		reg_write_tx_buf[1] = val;
		break;
	case 2:
		sys_put_be16(val, &reg_write_tx_buf[1]);
		break;
	case 3:
		sys_put_be24(val, &reg_write_tx_buf[1]);
		break;
	default:
		return -EINVAL;
	}

	const struct spi_buf tx_buf = {.buf = reg_write_tx_buf, .len = reg_size + 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	return spi_write_dt(spec, &tx);
}

static int ad4130_reg_read(const struct device *dev, unsigned int reg_addr, unsigned int *val)
{
	const struct ad4130_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;
	int ret;

	uint8_t reg_read_tx_buf[1] = {AD4130_COMMS_READ_MASK | reg_addr};
	uint8_t reg_read_rx_buf[4] = {0};
	size_t reg_size = 0;

	ret = ad4130_get_reg_size(reg_addr, &reg_size);
	if (ret) {
		return ret;
	}

	const struct spi_buf tx_buf = {.buf = reg_read_tx_buf, .len = 1};
	const struct spi_buf rx_buf = {.buf = reg_read_rx_buf, .len = reg_size + 1};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	ret = spi_transceive_dt(spec, &tx, &rx);
	if (ret) {
		return ret;
	}

	switch (reg_size) {
	case 1:
		*val = reg_read_rx_buf[1];
		break;
	case 2:
		*val = sys_get_be16(&reg_read_rx_buf[1]);
		break;
	case 3:
		*val = sys_get_be24(&reg_read_rx_buf[1]);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ad4130_reg_write_msk(const struct device *dev, unsigned int reg_addr, unsigned int data,
				unsigned int mask)
{
	uint32_t reg_data;
	int ret;

	ret = ad4130_reg_read(dev, reg_addr, &reg_data);
	if (ret) {
		return ret;
	}

	reg_data &= ~mask;
	reg_data |= data;

	return ad4130_reg_write(dev, reg_addr, reg_data);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct adc_ad4130_data *data = CONTAINER_OF(ctx, struct adc_ad4130_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct adc_ad4130_data *data = CONTAINER_OF(ctx, struct adc_ad4130_data, ctx);

	data->repeat_buffer = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static int adc_ad4130_set_ref(const struct device *dev, enum ad4130_ref_sel ref,
			      enum ad4130_setup setup_id)
{
	bool internal_reference;
	int ret;

	if (ref == AD4130_REF_REFOUT_AVSS) {
		internal_reference = true;
	} else {
		internal_reference = false;
	}

	ret = ad4130_reg_write_msk(
		dev, AD4130_ADC_CONTROL_REG,
		FIELD_PREP(AD4130_ADC_CONTROL_INT_REF_EN_MASK, internal_reference),
		AD4130_ADC_CONTROL_INT_REF_EN_MASK);
	if (ret) {
		return ret;
	}

	return ad4130_reg_write_msk(dev, AD4130_CONFIG_X_REG(setup_id),
				    FIELD_PREP(AD4130_CONFIG_REF_SEL_MASK, ref),
				    AD4130_CONFIG_REF_SEL_MASK);
}

static int adc_ad4130_set_gain(const struct device *dev, enum ad4130_gain gain,
			       enum ad4130_setup setup_id)
{
	return ad4130_reg_write_msk(dev, AD4130_CONFIG_X_REG(setup_id),
				    FIELD_PREP(AD4130_CONFIG_PGA_MASK, gain),
				    AD4130_CONFIG_PGA_MASK);
}

static int adc_ad4130_setup_cfg(const struct device *dev, const struct ad4130_channel_config *cfg)
{
	int ret;

	ret = adc_ad4130_set_ref(dev, cfg->props.ref_sel, cfg->cfg_slot);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_gain(dev, cfg->props.gain, cfg->cfg_slot);
	if (ret) {
		return ret;
	}

	return 0;
}

static int adc_ad4130_find_similar_configuration(const struct device *dev,
						 const struct ad4130_channel_config *cfg,
						 uint8_t channel_id)
{
	struct adc_ad4130_data *data = dev->data;
	int similar_channel_index = AD4130_INVALID_CHANNEL;

	for (int i = 0; i < AD4130_MAX_CHANNELS; i++) {
		if (!data->channel_setup_cfg[i].live_cfg && i == channel_id) {
			continue;
		}

		if (memcmp(&cfg->props, &data->channel_setup_cfg[i].props,
			   sizeof(struct ad4130_config_props)) == 0) {
			similar_channel_index = i;
			break;
		}
	}

	return similar_channel_index;
}

static int adc_ad4130_find_new_slot(const struct device *dev)
{
	struct adc_ad4130_data *data = dev->data;
	uint8_t slot = data->setup_cfg_slots;

	for (int i = 0; i < AD4130_MAX_SETUPS; i++) {
		if (!(slot & (1 << i))) {
			return i;
		}
	}

	return AD4130_INVALID_SLOT;
}

static int adc_ad4130_create_new_cfg(const struct device *dev, const struct adc_channel_cfg *cfg,
				     struct ad4130_channel_config *new_cfg)
{
	enum ad4130_ref_sel ref_source;
	enum ad4130_gain gain;

	if (cfg->channel_id >= AD4130_MAX_CHANNELS) {
		LOG_ERR("Invalid channel (%u)", cfg->channel_id);
		return -EINVAL;
	}

	if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
		LOG_ERR("invalid acquisition time %i", cfg->acquisition_time);
		return -EINVAL;
	}

	switch (cfg->reference) {
	case ADC_REF_INTERNAL:
		ref_source = AD4130_REF_REFOUT_AVSS;
		break;
	case ADC_REF_EXTERNAL0:
		ref_source = AD4130_REF_REFIN1;
		break;
	case ADC_REF_EXTERNAL1:
		ref_source = AD4130_REF_REFIN2;
		break;
	case ADC_REF_VDD_1:
		ref_source = AD4130_REF_AVDD_AVSS;
		break;
	default:
		LOG_ERR("Invalid reference source (%u)", cfg->reference);
		return -EINVAL;
	}

	new_cfg->props.ref_sel = ref_source;

	switch (cfg->gain) {
	case ADC_GAIN_1:
		gain = AD4130_GAIN_1;
		break;
	case ADC_GAIN_2:
		gain = AD4130_GAIN_2;
		break;
	case ADC_GAIN_4:
		gain = AD4130_GAIN_4;
		break;
	case ADC_GAIN_8:
		gain = AD4130_GAIN_8;
		break;
	case ADC_GAIN_16:
		gain = AD4130_GAIN_16;
		break;
	case ADC_GAIN_32:
		gain = AD4130_GAIN_32;
		break;
	case ADC_GAIN_64:
		gain = AD4130_GAIN_64;
		break;
	case ADC_GAIN_128:
		gain = AD4130_GAIN_128;
		break;
	default:
		LOG_ERR("Invalid gain value (%u)", cfg->gain);
		return -EINVAL;
	}

	new_cfg->props.gain = gain;

	return 0;
}

static int adc_ad4130_set_channel_setup(const struct device *dev, uint8_t channel_id,
					enum ad4130_setup setup_id)
{
	return ad4130_reg_write_msk(dev, AD4130_CHANNEL_X_REG(channel_id),
				    FIELD_PREP(AD4130_CHANNEL_SETUP_MASK, setup_id),
				    AD4130_CHANNEL_SETUP_MASK);
}

static int adc_ad4130_channel_en(const struct device *dev, uint8_t channel_id, bool enable)
{
	return ad4130_reg_write_msk(dev, AD4130_CHANNEL_X_REG(channel_id),
				    FIELD_PREP(AD4130_CHANNEL_EN_MASK, enable),
				    AD4130_CHANNEL_EN_MASK);
}

static int adc_ad4130_connect_analog_input(const struct device *dev, uint8_t channel_id,
					   enum ad4130_input ainp, enum ad4130_input ainm)
{
	if (ainp < AD4130_AIN0 || ainp > AD4130_V_MV_M || ainm < AD4130_AIN0 ||
	    ainm > AD4130_V_MV_M) {
		return -EINVAL;
	}

	int ret = ad4130_reg_write_msk(dev, AD4130_CHANNEL_X_REG(channel_id),
				       FIELD_PREP(AD4130_CHANNEL_AINP_MASK, ainp),
				       AD4130_CHANNEL_AINP_MASK);
	if (ret) {
		return ret;
	}

	return ad4130_reg_write_msk(dev, AD4130_CHANNEL_X_REG(channel_id),
				    FIELD_PREP(AD4130_CHANNEL_AINM_MASK, ainm),
				    AD4130_CHANNEL_AINM_MASK);
}

static int adc_ad4130_channel_setup(const struct device *dev, const struct adc_channel_cfg *cfg)
{
	struct adc_ad4130_data *data = dev->data;
	struct ad4130_channel_config new_cfg;
	int similar_channel_index;
	int new_slot;
	int ret;

	data->channel_setup_cfg[cfg->channel_id].live_cfg = false;

	ret = adc_ad4130_create_new_cfg(dev, cfg, &new_cfg);
	if (ret) {
		return ret;
	}

	/* AD4130 supports only 8 different configurations for 16 channels*/
	new_slot = adc_ad4130_find_new_slot(dev);

	if (new_slot == AD4130_INVALID_SLOT) {
		similar_channel_index =
			adc_ad4130_find_similar_configuration(dev, &new_cfg, cfg->channel_id);
		if (similar_channel_index == AD4130_INVALID_CHANNEL) {
			return -EINVAL;
		}
		new_cfg.cfg_slot = data->channel_setup_cfg[similar_channel_index].cfg_slot;
	} else {
		new_cfg.cfg_slot = new_slot;
		WRITE_BIT(data->setup_cfg_slots, new_slot, true);
	}

	new_cfg.live_cfg = true;

	memcpy(&data->channel_setup_cfg[cfg->channel_id], &new_cfg,
	       sizeof(struct ad4130_channel_config));

	/* Setup the channel configuration */
	ret = adc_ad4130_setup_cfg(dev, &data->channel_setup_cfg[cfg->channel_id]);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	/* Setup the channel */
	ret = adc_ad4130_connect_analog_input(dev, cfg->channel_id, cfg->input_positive,
					      cfg->input_negative);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_channel_setup(dev, cfg->channel_id, new_cfg.cfg_slot);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	ret = adc_ad4130_channel_en(dev, cfg->channel_id, true);
	if (ret) {
		LOG_ERR("Error setting up configuration");
		return ret;
	}

	WRITE_BIT(data->channels, cfg->channel_id, true);

	return 0;
}

static int adc_ad4130_reset(const struct device *dev)
{
	const struct ad4130_config *config = dev->config;
	const struct spi_dt_spec *spec = &config->bus;
	int ret;

	uint8_t buffer_write_tx[AD4130_RESET_BUF_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF,
							  0xFF, 0xFF, 0xFF, 0xFF};
	struct spi_buf tx_buf = {
		.buf = buffer_write_tx,
		.len = ARRAY_SIZE(buffer_write_tx),
	};

	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	/*
	 * Send 8 times 0xFF to reset the ADC
	 */
	ret = spi_write_dt(spec, &tx);
	if (ret) {
		return ret;
	}

	k_sleep(K_MSEC(AD4130_RESET_SLEEP_MS));

	return 0;
}

static int adc_ad4130_set_int_ref(const struct device *dev, enum ad4130_int_ref int_ref)
{
	int ret;

	switch (int_ref) {
	case AD4130_INT_REF_VAL_2_5V:
		ret = ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG, 0U,
					   AD4130_ADC_CONTROL_INT_REF_VAL_MASK);
		break;

	case AD4130_INT_REF_VAL_1_25V:
		ret = ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG,
					   FIELD_PREP(AD4130_ADC_CONTROL_INT_REF_VAL_MASK, 1),
					   AD4130_ADC_CONTROL_INT_REF_VAL_MASK);
		break;

	default:
		return -EINVAL;
	}

	return ret;
}

static int adc_ad4130_set_adc_mode(const struct device *dev, enum ad4130_adc_mode mode)
{
	return ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG,
				    FIELD_PREP(AD4130_ADC_CONTROL_MODE_MASK, mode),
				    AD4130_ADC_CONTROL_MODE_MASK);
}

static int adc_ad4130_check_chip_id(const struct device *dev)
{
	uint32_t reg_data;
	int ret;

	ret = ad4130_reg_read(dev, AD4130_ID_REG, &reg_data);
	if (ret) {
		return ret;
	}

	return reg_data == AD4130_8_ID ? 0 : -EINVAL;
}

static int adc_ad4130_set_polarity(const struct device *dev, bool enable)
{
	return ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG,
				    FIELD_PREP(AD4130_ADC_CONTROL_BIPOLAR_MASK, enable),
				    AD4130_ADC_CONTROL_BIPOLAR_MASK);
}

static int adc_ad4130_set_mclk(const struct device *dev, enum ad4130_mclk_sel clk)
{
	return ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG,
				    FIELD_PREP(AD4130_ADC_CONTROL_MCLK_SEL_MASK, clk),
				    AD4130_ADC_CONTROL_MCLK_SEL_MASK);
}

static int adc_ad4130_setup(const struct device *dev)
{
	const struct ad4130_config *config = dev->config;
	uint32_t reg_data;
	int ret;

	/* Reset the device interface */
	ret = adc_ad4130_reset(dev);
	if (ret) {
		return ret;
	}

	ret = ad4130_reg_read(dev, AD4130_STATUS_REG, &reg_data);
	if (ret) {
		return ret;
	}

	/* Change SPI to 4 wire*/
	ret = ad4130_reg_write_msk(dev, AD4130_ADC_CONTROL_REG, AD4130_ADC_CONTROL_CSB_EN_MASK,
				   AD4130_ADC_CONTROL_CSB_EN_MASK);
	if (ret) {
		return ret;
	}

	/* Check the device ID */
	ret = adc_ad4130_check_chip_id(dev);
	if (ret) {
		return ret;
	}

	/* Disable channel 0 */
	ret = adc_ad4130_channel_en(dev, 0, false);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_polarity(dev, config->bipolar);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_int_ref(dev, config->int_ref);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_adc_mode(dev, config->adc_mode);
	if (ret) {
		return ret;
	}

	ret = adc_ad4130_set_mclk(dev, config->mclk_sel);
	if (ret) {
		return ret;
	}

	return 0;
}

static bool get_next_ch_idx(uint16_t ch_mask, uint16_t last_idx, uint16_t *new_idx)
{
	last_idx++;
	if (last_idx >= AD4130_MAX_CHANNELS) {
		return 0;
	}
	ch_mask >>= last_idx;
	if (!ch_mask) {
		*new_idx = AD4130_INVALID_CHANNEL;
		return 0;
	}
	while (!(ch_mask & 1)) {
		last_idx++;
		ch_mask >>= 1;
	}
	*new_idx = last_idx;

	return 1;
}

static int adc_ad4130_get_read_channel_id(const struct device *dev, uint16_t *channel_id)
{
	int ret;
	uint32_t reg_temp;

	ret = ad4130_reg_read(dev, AD4130_STATUS_REG, &reg_temp);
	if (ret) {
		return ret;
	}

	*channel_id = reg_temp & 0xF;

	return 0;
}

static int adc_ad4130_wait_for_conv_ready(const struct device *dev)
{
	bool ready = false;
	uint32_t reg_val;
	int ret = 0;

	while (!ready) {

		ret = ad4130_reg_read(dev, AD4130_STATUS_REG, &reg_val);
		if (ret) {
			return ret;
		}

		ready = (reg_val & AD4130_STATUS_REG_DATA_READY) == 0;
	}

	return 0;
}

static int adc_ad4130_perform_read(const struct device *dev)
{
	int ret;
	struct adc_ad4130_data *data = dev->data;
	uint16_t ch_idx = AD4130_INVALID_CHANNEL;
	uint16_t prev_ch_idx = AD4130_INVALID_CHANNEL;
	uint16_t adc_ch_id = 0;
	bool status;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	do {
		prev_ch_idx = ch_idx;

		status = get_next_ch_idx(data->ctx.sequence.channels, ch_idx, &ch_idx);
		if (!status) {
			break;
		}

		adc_ad4130_wait_for_conv_ready(dev);

		ret = ad4130_reg_read(dev, AD4130_DATA_REG, data->buffer);
		if (ret) {
			LOG_ERR("reading sample failed");
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		ret = adc_ad4130_get_read_channel_id(dev, &adc_ch_id);
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

static int adc_ad4130_validate_sequence(const struct device *dev,
					const struct adc_sequence *sequence)
{
	const struct ad4130_config *config = dev->config;
	struct adc_ad4130_data *data = dev->data;
	const size_t channel_maximum = AD4130_MAX_SETUPS * sizeof(sequence->channels);
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

		if (i >= AD4130_MAX_CHANNELS) {
			LOG_ERR("invalid channel selection");
			return -EINVAL;
		}
	}

	return 0;
}

static int adc_ad4130_start_read(const struct device *dev, const struct adc_sequence *sequence,
				 bool wait)
{
	int result;
	struct adc_ad4130_data *data = dev->data;

	result = adc_ad4130_validate_sequence(dev, sequence);
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
static int adc_ad4130_read_async(const struct device *dev, const struct adc_sequence *sequence,
				 struct k_poll_signal *async)
{
	int status;
	struct adc_ad4130_data *data = dev->data;

	adc_context_lock(&data->ctx, true, async);
	status = adc_ad4130_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

static int adc_ad4130_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_ad4130_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);
	status = adc_ad4130_start_read(dev, sequence, true);
	adc_context_release(&data->ctx, status);

	return status;
}

#else
static int adc_ad4130_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int status;
	struct adc_ad4130_data *data = dev->data;

	adc_context_lock(&data->ctx, false, NULL);

	status = adc_ad4130_start_read(dev, sequence, false);

	while (status == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		status = adc_ad4130_perform_read(dev);
	}

	adc_context_release(&data->ctx, status);

	return status;
}
#endif

#if CONFIG_ADC_ASYNC
static void adc_ad4130_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;

	while (true) {
		adc_ad4130_perform_read(dev);
	}
}
#endif /* CONFIG_ADC_ASYNC */

static int ad4130_init(const struct device *dev)
{
	const struct ad4130_config *config = dev->config;
	struct adc_ad4130_data *data = dev->data;
	int ret;

	data->dev = dev;

	k_sem_init(&data->acquire_signal, 0, 1);

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("spi bus %s not ready", config->bus.bus->name);
		return -ENODEV;
	}

	ret = adc_ad4130_setup(dev);
	if (ret) {
		return ret;
	}

#if CONFIG_ADC_ASYNC
	k_tid_t tid = k_thread_create(
		&data->thread, data->stack, CONFIG_ADI_AD4130_ADC_ACQUISITION_THREAD_STACK_SIZE,
		adc_ad4130_acquisition_thread, (void *)dev, NULL, NULL,
		CONFIG_ADI_AD4130_ADC_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_ad4130");
#endif /* CONFIG_ADC_ASYNC */

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, adc_ad4130_driver_api) = {
	.channel_setup = adc_ad4130_channel_setup,
	.read = adc_ad4130_read,
	.ref_internal = AD4130_INT_REF_2_5V,
#ifdef CONFIG_ADC_ASYNC
	.read_async = adc_ad4130_read_async,
#endif
};

#define AD4130_ADC_INIT(inst)                                                                      \
	static const struct ad4130_config ad4130_config_##inst = {                                 \
		.bus = SPI_DT_SPEC_GET(DT_INST(inst, adi_ad4130_adc),                              \
				       SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,    \
				       1),                                                         \
		.resolution = AD4130_ADC_RESOLUTION,                                               \
		.bipolar = DT_INST_PROP_OR(inst, bipolar, 1),                                      \
		.int_ref = DT_INST_PROP_OR(inst, internal_reference_value, 0),                     \
		.adc_mode = DT_INST_PROP_OR(inst, adc_mode, 0),                                    \
		.mclk_sel = DT_INST_PROP_OR(inst, clock_type, 0),                                  \
	};                                                                                         \
	static struct adc_ad4130_data adc_ad4130_data_##inst = {                                   \
		ADC_CONTEXT_INIT_LOCK(adc_ad4130_data_##inst, ctx),                                \
		ADC_CONTEXT_INIT_TIMER(adc_ad4130_data_##inst, ctx),                               \
		ADC_CONTEXT_INIT_SYNC(adc_ad4130_data_##inst, ctx),                                \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, &ad4130_init, NULL, &adc_ad4130_data_##inst,                   \
			      &ad4130_config_##inst, POST_KERNEL, CONFIG_ADC_INIT_PRIORITY,        \
			      &adc_ad4130_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AD4130_ADC_INIT)
