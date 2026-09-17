/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/adc/ads1x2s14-adc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER
#include "adc_context.h"

LOG_MODULE_REGISTER(ads1x2s14, CONFIG_ADC_LOG_LEVEL);

/* Register addresses */
#define ADS1X2S14_REG_DEVICE_ID       0x00U
#define ADS1X2S14_REG_STATUS_MSB      0x02U
#define ADS1X2S14_REG_CONVERSION_CTRL 0x04U
#define ADS1X2S14_REG_DEVICE_CFG      0x05U
#define ADS1X2S14_REG_DATA_RATE_CFG   0x06U
#define ADS1X2S14_REG_MUX_CFG         0x07U
#define ADS1X2S14_REG_GAIN_CFG        0x08U
#define ADS1X2S14_REG_REFERENCE_CFG   0x09U
#define ADS1X2S14_REG_DIGITAL_CFG     0x0AU
#define ADS1X2S14_REG_IDAC_MAG_CFG    0x0DU
#define ADS1X2S14_REG_IDAC_MUX_CFG    0x0EU

/*
 * SPI commands (first command byte). The second command byte carries the
 * register data for a write and is ignored otherwise. Two zero bytes form the
 * no-operation command used to read conversion data.
 */
#define ADS1X2S14_CMD_NOP       0x00U
#define ADS1X2S14_CMD_RREG(reg) (0x40U | (reg))
#define ADS1X2S14_CMD_WREG(reg) (0x80U | (reg))

/* DEVICE_ID register, bits [3:0] identify the resolution */
#define ADS1X2S14_DEVICE_ID_MASK GENMASK(3, 0)
#define ADS112S14_DEVICE_ID      0x0AU
#define ADS122S14_DEVICE_ID      0x0BU

/* STATUS_MSB register */
#define ADS1X2S14_STATUS_MSB_RESETN             BIT(7)
#define ADS1X2S14_STATUS_MSB_AVDD_UVN           BIT(6)
#define ADS1X2S14_STATUS_MSB_REF_UVN            BIT(5)
#define ADS1X2S14_STATUS_MSB_REG_MAP_CRC_FAULTN BIT(3)
#define ADS1X2S14_STATUS_MSB_DRDY               BIT(0)
/* Sticky flags, cleared by writing 1 */
#define ADS1X2S14_STATUS_MSB_CLEAR_FLAGS                                                           \
	(ADS1X2S14_STATUS_MSB_RESETN | ADS1X2S14_STATUS_MSB_AVDD_UVN |                             \
	 ADS1X2S14_STATUS_MSB_REF_UVN | ADS1X2S14_STATUS_MSB_REG_MAP_CRC_FAULTN)

/* CONVERSION_CTRL register */
#define ADS1X2S14_CONVERSION_CTRL_RESET 0x58U /* RESET[5:0] = 010110b */
#define ADS1X2S14_CONVERSION_CTRL_START BIT(1)

/* DEVICE_CFG register */
#define ADS1X2S14_DEVICE_CFG_CONV_MODE_SINGLE_SHOT BIT(2)
#define ADS1X2S14_DEVICE_CFG_SPEED_MODE            GENMASK(1, 0)

/* DATA_RATE_CFG register */
#define ADS1X2S14_DATA_RATE_CFG_GC_EN    BIT(3)
#define ADS1X2S14_DATA_RATE_CFG_FLTR_OSR GENMASK(2, 0)

/* MUX_CFG register */
#define ADS1X2S14_MUX_CFG_AINP GENMASK(7, 4)
#define ADS1X2S14_MUX_CFG_AINN GENMASK(3, 0)
#define ADS1X2S14_MUX_GND      0x8U

/* GAIN_CFG register, PGA gain codes */
#define ADS1X2S14_GAIN_CFG_GAIN GENMASK(3, 0)
#define ADS1X2S14_GAIN_1_2      0x0U
#define ADS1X2S14_GAIN_1        0x1U
#define ADS1X2S14_GAIN_2        0x2U
#define ADS1X2S14_GAIN_4        0x3U
#define ADS1X2S14_GAIN_8        0x5U
#define ADS1X2S14_GAIN_16       0x7U
#define ADS1X2S14_GAIN_32       0x9U
#define ADS1X2S14_GAIN_64       0xBU
#define ADS1X2S14_GAIN_128      0xDU

/* REFERENCE_CFG register */
#define ADS1X2S14_REFERENCE_CFG_REF_VAL_2V5 BIT(2)
#define ADS1X2S14_REFERENCE_CFG_REF_SEL     GENMASK(1, 0)
#define ADS1X2S14_REF_SEL_INTERNAL          0x0U
#define ADS1X2S14_REF_SEL_EXTERNAL          0x1U
#define ADS1X2S14_REF_SEL_AVDD              0x2U

/* DIGITAL_CFG register */
#define ADS1X2S14_DIGITAL_CFG_STATUS_EN       BIT(4)
#define ADS1X2S14_DIGITAL_CFG_CODING_UNIPOLAR BIT(1)

/* IDAC_MAG_CFG and IDAC_MUX_CFG registers */
#define ADS1X2S14_IDAC_MAG_CFG_I2MAG      GENMASK(7, 4)
#define ADS1X2S14_IDAC_MAG_CFG_I1MAG      GENMASK(3, 0)
#define ADS1X2S14_IDAC_MUX_CFG_IUNIT_10UA BIT(7)
#define ADS1X2S14_IDAC_MUX_CFG_I2MUX      GENMASK(6, 4)
#define ADS1X2S14_IDAC_MUX_CFG_I1MUX      GENMASK(2, 0)

#define ADS1X2S14_NUM_INPUTS   8U
#define ADS1X2S14_NUM_CHANNELS 8U
#define ADS1X2S14_NO_CHANNEL   UINT8_MAX

#define ADS1X2S14_INTERNAL_REF_1V25_MV 1250U
#define ADS1X2S14_INTERNAL_REF_2V5_MV  2500U

/* STATUS header transmitted in front of every output frame once STATUS_EN is set */
#define ADS1X2S14_STATUS_HEADER_SIZE 2U
/* Longest output frame: STATUS header plus 24 bit of conversion data */
#define ADS1X2S14_MAX_FRAME_SIZE     (ADS1X2S14_STATUS_HEADER_SIZE + 3U)

/* td(RST): SPI communication start after software reset */
#define ADS1X2S14_RESET_DELAY_US 500U
/* Number of conversion data reads before a conversion is considered lost */
#define ADS1X2S14_POLL_RETRIES   50U
/* Lower bound for the delay between conversion data reads */
#define ADS1X2S14_POLL_MIN_US    100U

#define ADS1X2S14_DATA_RATE_SPEED_MODE(value) (((value) >> 3) & 0x3U)
#define ADS1X2S14_DATA_RATE_FLTR_OSR(value)   ((value) & 0x7U)

/* Highest valid acquisition time value */
#define ADS1X2S14_DATA_RATE_MAX ADS1X2S14_DATA_RATE(ADS1X2S14_SPEED_MODE_3, ADS1X2S14_DR_20SPS)

/* Register values of a configured channel */
struct ads1x2s14_channel_config {
	uint8_t device_cfg;
	uint8_t data_rate_cfg;
	uint8_t mux_cfg;
	uint8_t gain_cfg;
	uint8_t reference_cfg;
	uint8_t digital_cfg;
	uint8_t idac_mux_cfg;
	bool idac_enabled;
	bool differential;
	/* Time from conversion start to the first settled result */
	uint32_t conversion_time_us;
};

struct ads1x2s14_config {
	struct spi_dt_spec bus;
	uint16_t internal_ref_mv;
	uint16_t idac_current_ua[2];
	uint8_t resolution;
	uint8_t device_id;
	bool global_chop;
};

struct ads1x2s14_data {
	struct adc_context ctx;
	struct k_sem acquire_signal;
	struct ads1x2s14_channel_config channels[ADS1X2S14_NUM_CHANNELS];
	uint8_t configured_channels;
	/* Channel whose configuration is currently programmed into the device */
	uint8_t active_channel;
	/* IDAC_MAG_CFG value for channels with excitation current enabled */
	uint8_t idac_mag_cfg;
	/* IUNIT bit for IDAC_MUX_CFG */
	uint8_t idac_iunit;
	bool status_header;
	uint32_t pending_channels;
	void *buffer;
	void *repeat_buffer;
};

/*
 * Digital filter latency in modulator clock cycles, indexed by filter setting
 * and speed mode, for conversions started from idle mode (datasheet tables
 * 7-6 and 7-7).
 */
static const uint16_t ads1x2s14_latency_tmod[8][4] = {
	[ADS1X2S14_OSR_16] = {80, 88, 88, 104},
	[ADS1X2S14_OSR_32] = {144, 152, 152, 168},
	[ADS1X2S14_OSR_128] = {240, 248, 248, 264},
	[ADS1X2S14_OSR_256] = {368, 376, 376, 392},
	[ADS1X2S14_OSR_512] = {624, 632, 632, 648},
	[ADS1X2S14_OSR_1024] = {1136, 1144, 1144, 1160},
	[ADS1X2S14_DR_25SPS] = {1416, 10384, 20624, 41120},
	[ADS1X2S14_DR_20SPS] = {1736, 12944, 25744, 51360},
};

/* Modulator clock frequency per speed mode in Hz, nominal 4.096 MHz oscillator */
static const uint32_t ads1x2s14_fmod_hz[4] = {32000, 256000, 512000, 1024000};

/* IDAC output current as multiple of the unit current, indexed by IxMAG code */
static const uint8_t ads1x2s14_idac_multiplier[] = {0, 1, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};

static int ads1x2s14_reg_write(const struct device *dev, uint8_t reg, uint8_t value)
{
	const struct ads1x2s14_config *config = dev->config;
	uint8_t buffer_tx[2] = {ADS1X2S14_CMD_WREG(reg), value};
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = sizeof(buffer_tx),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	int ret;

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0) {
		LOG_ERR("%s: write of register 0x%02x failed: %d", dev->name, reg, ret);
	}

	return ret;
}

/*
 * Read one output frame. The frame is either conversion data or, in the frame
 * following a read register command, the register data. The optional STATUS
 * header precedes the data in both cases.
 */
static int ads1x2s14_read_frame(const struct device *dev, uint8_t *buffer_rx, size_t length)
{
	const struct ads1x2s14_config *config = dev->config;
	uint8_t buffer_tx[ADS1X2S14_MAX_FRAME_SIZE] = {ADS1X2S14_CMD_NOP};
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = length,
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	const struct spi_buf rx_buf = {
		.buf = buffer_rx,
		.len = length,
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count = 1,
	};
	int ret;

	ret = spi_transceive_dt(&config->bus, &tx, &rx);
	if (ret != 0) {
		LOG_ERR("%s: frame read failed: %d", dev->name, ret);
	}

	return ret;
}

/*
 * Register reads use a two frame protocol: the command frame is followed by a
 * response frame carrying the register data and the register address. The
 * response of the 24-bit device is padded to the size of a conversion result.
 */
static int ads1x2s14_reg_read(const struct device *dev, uint8_t reg, uint8_t *value)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;
	uint8_t buffer_tx[2] = {ADS1X2S14_CMD_RREG(reg), 0x00U};
	uint8_t buffer_rx[ADS1X2S14_MAX_FRAME_SIZE];
	const struct spi_buf tx_buf = {
		.buf = buffer_tx,
		.len = sizeof(buffer_tx),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count = 1,
	};
	size_t offset = data->status_header ? ADS1X2S14_STATUS_HEADER_SIZE : 0U;
	size_t length = offset + config->resolution / BITS_PER_BYTE;
	int ret;

	ret = spi_write_dt(&config->bus, &tx);
	if (ret != 0) {
		LOG_ERR("%s: read command for register 0x%02x failed: %d", dev->name, reg, ret);
		return ret;
	}

	ret = ads1x2s14_read_frame(dev, buffer_rx, length);
	if (ret != 0) {
		return ret;
	}

	if (buffer_rx[offset + 1U] != reg) {
		LOG_ERR("%s: register 0x%02x read returned address 0x%02x", dev->name, reg,
			buffer_rx[offset + 1U]);
		return -EIO;
	}

	*value = buffer_rx[offset];

	return 0;
}

static int ads1x2s14_read_conversion(const struct device *dev, uint8_t *status, uint32_t *raw)
{
	const struct ads1x2s14_config *config = dev->config;
	uint8_t buffer_rx[ADS1X2S14_MAX_FRAME_SIZE];
	size_t length = ADS1X2S14_STATUS_HEADER_SIZE + config->resolution / BITS_PER_BYTE;
	int ret;

	ret = ads1x2s14_read_frame(dev, buffer_rx, length);
	if (ret != 0) {
		return ret;
	}

	*status = buffer_rx[0];

	if (config->resolution > 16U) {
		*raw = sys_get_be24(&buffer_rx[ADS1X2S14_STATUS_HEADER_SIZE]);
	} else {
		*raw = sys_get_be16(&buffer_rx[ADS1X2S14_STATUS_HEADER_SIZE]);
	}

	return 0;
}

static int ads1x2s14_gain_cfg(enum adc_gain gain, uint8_t *gain_cfg)
{
	switch (gain) {
	case ADC_GAIN_1_2:
		*gain_cfg = ADS1X2S14_GAIN_1_2;
		break;
	case ADC_GAIN_1:
		*gain_cfg = ADS1X2S14_GAIN_1;
		break;
	case ADC_GAIN_2:
		*gain_cfg = ADS1X2S14_GAIN_2;
		break;
	case ADC_GAIN_4:
		*gain_cfg = ADS1X2S14_GAIN_4;
		break;
	case ADC_GAIN_8:
		*gain_cfg = ADS1X2S14_GAIN_8;
		break;
	case ADC_GAIN_16:
		*gain_cfg = ADS1X2S14_GAIN_16;
		break;
	case ADC_GAIN_32:
		*gain_cfg = ADS1X2S14_GAIN_32;
		break;
	case ADC_GAIN_64:
		*gain_cfg = ADS1X2S14_GAIN_64;
		break;
	case ADC_GAIN_128:
		*gain_cfg = ADS1X2S14_GAIN_128;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ads1x2s14_reference_cfg(const struct device *dev, enum adc_reference reference,
				   uint8_t *reference_cfg)
{
	const struct ads1x2s14_config *config = dev->config;

	switch (reference) {
	case ADC_REF_INTERNAL:
		*reference_cfg =
			FIELD_PREP(ADS1X2S14_REFERENCE_CFG_REF_SEL, ADS1X2S14_REF_SEL_INTERNAL);
		if (config->internal_ref_mv == ADS1X2S14_INTERNAL_REF_2V5_MV) {
			*reference_cfg |= ADS1X2S14_REFERENCE_CFG_REF_VAL_2V5;
		}
		break;
	case ADC_REF_EXTERNAL0:
		*reference_cfg =
			FIELD_PREP(ADS1X2S14_REFERENCE_CFG_REF_SEL, ADS1X2S14_REF_SEL_EXTERNAL);
		break;
	case ADC_REF_VDD_1:
		*reference_cfg =
			FIELD_PREP(ADS1X2S14_REFERENCE_CFG_REF_SEL, ADS1X2S14_REF_SEL_AVDD);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 * The acquisition time carries the speed mode and the digital filter setting,
 * see ADS1X2S14_DATA_RATE() in the devicetree bindings.
 */
static int ads1x2s14_data_rate(uint16_t acquisition_time, uint8_t *speed_mode, uint8_t *fltr_osr)
{
	uint16_t value;

	if (acquisition_time == ADC_ACQ_TIME_DEFAULT) {
		*speed_mode = ADS1X2S14_SPEED_MODE_0;
		*fltr_osr = ADS1X2S14_OSR_16;
		return 0;
	}

	if (acquisition_time == ADC_ACQ_TIME_MAX) {
		*speed_mode = ADS1X2S14_SPEED_MODE_0;
		*fltr_osr = ADS1X2S14_DR_20SPS;
		return 0;
	}

	if (ADC_ACQ_TIME_UNIT(acquisition_time) != ADC_ACQ_TIME_TICKS) {
		return -EINVAL;
	}

	value = ADC_ACQ_TIME_VALUE(acquisition_time);
	if (value > ADS1X2S14_DATA_RATE_MAX) {
		return -EINVAL;
	}

	*speed_mode = ADS1X2S14_DATA_RATE_SPEED_MODE(value);
	*fltr_osr = ADS1X2S14_DATA_RATE_FLTR_OSR(value);

	return 0;
}

static uint32_t ads1x2s14_conversion_time_us(const struct device *dev, uint8_t speed_mode,
					     uint8_t fltr_osr)
{
	const struct ads1x2s14_config *config = dev->config;
	uint32_t latency_tmod = ads1x2s14_latency_tmod[fltr_osr][speed_mode];

	if (config->global_chop) {
		/* Two conversions with alternating polarity, datasheet equation 10 */
		latency_tmod = 2U * latency_tmod - 12U;
	}

	/* The latency has an uncertainty of one modulator clock cycle */
	latency_tmod += 1U;

	return (uint32_t)DIV_ROUND_UP((uint64_t)latency_tmod * USEC_PER_SEC,
				      ads1x2s14_fmod_hz[speed_mode]);
}

static int ads1x2s14_channel_setup(const struct device *dev,
				   const struct adc_channel_cfg *channel_cfg)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;
	struct ads1x2s14_channel_config channel = {0};
	uint8_t speed_mode;
	uint8_t fltr_osr;
	uint8_t input_negative;
	int ret;

	if (channel_cfg->channel_id >= ADS1X2S14_NUM_CHANNELS) {
		LOG_ERR("%s: invalid channel id %u", dev->name, channel_cfg->channel_id);
		return -EINVAL;
	}

	if (channel_cfg->input_positive >= ADS1X2S14_NUM_INPUTS) {
		LOG_ERR("%s: invalid positive input %u", dev->name, channel_cfg->input_positive);
		return -EINVAL;
	}

	if (channel_cfg->differential) {
		if (channel_cfg->input_negative >= ADS1X2S14_NUM_INPUTS) {
			LOG_ERR("%s: invalid negative input %u", dev->name,
				channel_cfg->input_negative);
			return -EINVAL;
		}
		input_negative = channel_cfg->input_negative;
	} else {
		input_negative = ADS1X2S14_MUX_GND;
	}

	ret = ads1x2s14_gain_cfg(channel_cfg->gain, &channel.gain_cfg);
	if (ret != 0) {
		LOG_ERR("%s: unsupported gain %d", dev->name, channel_cfg->gain);
		return ret;
	}

	ret = ads1x2s14_reference_cfg(dev, channel_cfg->reference, &channel.reference_cfg);
	if (ret != 0) {
		LOG_ERR("%s: unsupported reference %d", dev->name, channel_cfg->reference);
		return ret;
	}

	ret = ads1x2s14_data_rate(channel_cfg->acquisition_time, &speed_mode, &fltr_osr);
	if (ret != 0) {
		LOG_ERR("%s: unsupported acquisition time 0x%04x", dev->name,
			channel_cfg->acquisition_time);
		return ret;
	}

	if (channel_cfg->current_source_pin_set) {
		if (channel_cfg->current_source_pin[0] >= ADS1X2S14_NUM_INPUTS ||
		    channel_cfg->current_source_pin[1] >= ADS1X2S14_NUM_INPUTS) {
			LOG_ERR("%s: invalid current source pins %u/%u", dev->name,
				channel_cfg->current_source_pin[0],
				channel_cfg->current_source_pin[1]);
			return -EINVAL;
		}
		channel.idac_enabled = true;
		channel.idac_mux_cfg = data->idac_iunit |
				       FIELD_PREP(ADS1X2S14_IDAC_MUX_CFG_I1MUX,
						  channel_cfg->current_source_pin[0]) |
				       FIELD_PREP(ADS1X2S14_IDAC_MUX_CFG_I2MUX,
						  channel_cfg->current_source_pin[1]);
	}

	channel.device_cfg = ADS1X2S14_DEVICE_CFG_CONV_MODE_SINGLE_SHOT |
			     FIELD_PREP(ADS1X2S14_DEVICE_CFG_SPEED_MODE, speed_mode);
	channel.data_rate_cfg = FIELD_PREP(ADS1X2S14_DATA_RATE_CFG_FLTR_OSR, fltr_osr);
	if (config->global_chop) {
		channel.data_rate_cfg |= ADS1X2S14_DATA_RATE_CFG_GC_EN;
	}
	channel.mux_cfg = FIELD_PREP(ADS1X2S14_MUX_CFG_AINP, channel_cfg->input_positive) |
			  FIELD_PREP(ADS1X2S14_MUX_CFG_AINN, input_negative);
	/*
	 * Differential channels use two's complement coding, single-ended
	 * channels unipolar straight binary coding to cover the full code
	 * range with positive inputs.
	 */
	channel.digital_cfg = ADS1X2S14_DIGITAL_CFG_STATUS_EN;
	if (!channel_cfg->differential) {
		channel.digital_cfg |= ADS1X2S14_DIGITAL_CFG_CODING_UNIPOLAR;
	}
	channel.differential = channel_cfg->differential;
	channel.conversion_time_us = ads1x2s14_conversion_time_us(dev, speed_mode, fltr_osr);

	data->channels[channel_cfg->channel_id] = channel;
	data->configured_channels |= BIT(channel_cfg->channel_id);
	if (data->active_channel == channel_cfg->channel_id) {
		data->active_channel = ADS1X2S14_NO_CHANNEL;
	}

	return 0;
}

static int ads1x2s14_apply_channel(const struct device *dev, uint8_t channel_id)
{
	struct ads1x2s14_data *data = dev->data;
	const struct ads1x2s14_channel_config *channel = &data->channels[channel_id];
	int ret;

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_IDAC_MAG_CFG,
				  channel->idac_enabled ? data->idac_mag_cfg : 0U);
	if (ret != 0) {
		return ret;
	}

	if (channel->idac_enabled) {
		ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_IDAC_MUX_CFG, channel->idac_mux_cfg);
		if (ret != 0) {
			return ret;
		}
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_DEVICE_CFG, channel->device_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_DATA_RATE_CFG, channel->data_rate_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_MUX_CFG, channel->mux_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_GAIN_CFG, channel->gain_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_REFERENCE_CFG, channel->reference_cfg);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_DIGITAL_CFG, channel->digital_cfg);
	if (ret != 0) {
		return ret;
	}

	data->active_channel = channel_id;

	return 0;
}

static void ads1x2s14_store_sample(const struct device *dev, bool differential, uint32_t raw)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;

	if (config->resolution > 16U) {
		int32_t *buffer = data->buffer;

		if (differential && (raw & BIT(23)) != 0U) {
			raw |= GENMASK(31, 24);
		}
		*buffer = (int32_t)raw;
		data->buffer = buffer + 1;
	} else {
		uint16_t *buffer = data->buffer;

		*buffer = (uint16_t)raw;
		data->buffer = buffer + 1;
	}
}

/*
 * Start a single-shot conversion and read the result. The device only
 * outputs settled data, so the conversion time is waited before the first
 * read. The DRDY bit in the STATUS header distinguishes new from stale data.
 */
static int ads1x2s14_read_channel(const struct device *dev, uint8_t channel_id)
{
	struct ads1x2s14_data *data = dev->data;
	const struct ads1x2s14_channel_config *channel = &data->channels[channel_id];
	uint32_t poll_us = MAX(channel->conversion_time_us / 8U, ADS1X2S14_POLL_MIN_US);
	uint8_t status;
	uint32_t raw;
	int ret;

	if (data->active_channel != channel_id) {
		ret = ads1x2s14_apply_channel(dev, channel_id);
		if (ret != 0) {
			return ret;
		}
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_CONVERSION_CTRL,
				  ADS1X2S14_CONVERSION_CTRL_START);
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_USEC(channel->conversion_time_us));

	for (uint32_t retry = 0U;; retry++) {
		ret = ads1x2s14_read_conversion(dev, &status, &raw);
		if (ret != 0) {
			return ret;
		}

		if ((status & ADS1X2S14_STATUS_MSB_DRDY) != 0U) {
			break;
		}

		if (retry >= ADS1X2S14_POLL_RETRIES) {
			LOG_ERR("%s: conversion on channel %u timed out", dev->name, channel_id);
			return -ETIMEDOUT;
		}

		k_sleep(K_USEC(poll_us));
	}

	LOG_DBG("%s: channel %u status 0x%02x raw 0x%06x", dev->name, channel_id, status, raw);

	ads1x2s14_store_sample(dev, channel->differential, raw);

	return 0;
}

static int ads1x2s14_validate_sequence(const struct device *dev,
				       const struct adc_sequence *sequence)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;
	size_t sample_size;
	size_t needed;

	if (sequence->resolution != config->resolution) {
		LOG_ERR("%s: invalid resolution %u", dev->name, sequence->resolution);
		return -EINVAL;
	}

	if (sequence->oversampling != 0U) {
		LOG_ERR("%s: oversampling is not supported", dev->name);
		return -ENOTSUP;
	}

	if (sequence->channels == 0U || sequence->channels >= BIT(ADS1X2S14_NUM_CHANNELS) ||
	    (sequence->channels & ~(uint32_t)data->configured_channels) != 0U) {
		LOG_ERR("%s: invalid channel selection 0x%08x", dev->name, sequence->channels);
		return -EINVAL;
	}

	sample_size = (config->resolution > 16U) ? sizeof(int32_t) : sizeof(uint16_t);
	needed = POPCOUNT(sequence->channels) * sample_size;

	if (sequence->options != NULL) {
		needed *= 1U + sequence->options->extra_samplings;
	}

	if (sequence->buffer_size < needed) {
		LOG_ERR("%s: buffer size %u too small, %u needed", dev->name, sequence->buffer_size,
			needed);
		return -ENOMEM;
	}

	return 0;
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct ads1x2s14_data *data = CONTAINER_OF(ctx, struct ads1x2s14_data, ctx);

	data->pending_channels = ctx->sequence.channels;
	data->repeat_buffer = data->buffer;
	k_sem_give(&data->acquire_signal);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct ads1x2s14_data *data = CONTAINER_OF(ctx, struct ads1x2s14_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static int ads1x2s14_perform_read(const struct device *dev)
{
	struct ads1x2s14_data *data = dev->data;
	int ret;

	k_sem_take(&data->acquire_signal, K_FOREVER);

	while (data->pending_channels != 0U) {
		uint8_t channel_id = find_lsb_set(data->pending_channels) - 1;

		ret = ads1x2s14_read_channel(dev, channel_id);
		if (ret != 0) {
			adc_context_complete(&data->ctx, ret);
			return ret;
		}

		data->pending_channels &= ~BIT(channel_id);
	}

	adc_context_on_sampling_done(&data->ctx, dev);

	return 0;
}

static int ads1x2s14_read(const struct device *dev, const struct adc_sequence *sequence)
{
	struct ads1x2s14_data *data = dev->data;
	int ret;

	adc_context_lock(&data->ctx, false, NULL);

	ret = ads1x2s14_validate_sequence(dev, sequence);
	if (ret == 0) {
		data->buffer = sequence->buffer;
		adc_context_start_read(&data->ctx, sequence);
	}

	while (ret == 0 && k_sem_take(&data->ctx.sync, K_NO_WAIT) != 0) {
		ret = ads1x2s14_perform_read(dev);
	}

	adc_context_release(&data->ctx, ret);

	return ret;
}

#ifdef CONFIG_ADC_ASYNC
static int ads1x2s14_read_async(const struct device *dev, const struct adc_sequence *sequence,
				struct k_poll_signal *async)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sequence);
	ARG_UNUSED(async);

	return -ENOTSUP;
}
#endif /* CONFIG_ADC_ASYNC */

static int ads1x2s14_ref_get(const struct device *dev, enum adc_reference ref, uint16_t *vref_mv)
{
	const struct ads1x2s14_config *config = dev->config;

	if (ref != ADC_REF_INTERNAL) {
		return -ENOTSUP;
	}

	*vref_mv = config->internal_ref_mv;

	return 0;
}

static int ads1x2s14_idac_mag(uint16_t current_ua, uint8_t unit_ua, uint8_t *mag)
{
	for (uint8_t i = 0U; i < ARRAY_SIZE(ads1x2s14_idac_multiplier); i++) {
		if ((uint16_t)ads1x2s14_idac_multiplier[i] * unit_ua == current_ua) {
			*mag = i;
			return 0;
		}
	}

	return -EINVAL;
}

/*
 * Both IDACs share one unit current. Use 1 uA when both currents can be
 * expressed with it, 10 uA otherwise.
 */
static int ads1x2s14_idac_init(const struct device *dev)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;
	static const uint8_t units_ua[] = {1U, 10U};

	for (size_t i = 0U; i < ARRAY_SIZE(units_ua); i++) {
		uint8_t i1mag;
		uint8_t i2mag;

		if (ads1x2s14_idac_mag(config->idac_current_ua[0], units_ua[i], &i1mag) != 0 ||
		    ads1x2s14_idac_mag(config->idac_current_ua[1], units_ua[i], &i2mag) != 0) {
			continue;
		}

		data->idac_mag_cfg = FIELD_PREP(ADS1X2S14_IDAC_MAG_CFG_I1MAG, i1mag) |
				     FIELD_PREP(ADS1X2S14_IDAC_MAG_CFG_I2MAG, i2mag);
		data->idac_iunit = (units_ua[i] == 10U) ? ADS1X2S14_IDAC_MUX_CFG_IUNIT_10UA : 0U;

		return 0;
	}

	LOG_ERR("%s: unsupported IDAC currents %u uA and %u uA", dev->name,
		config->idac_current_ua[0], config->idac_current_ua[1]);

	return -EINVAL;
}

static int ads1x2s14_init(const struct device *dev)
{
	const struct ads1x2s14_config *config = dev->config;
	struct ads1x2s14_data *data = dev->data;
	uint8_t device_id;
	int ret;

	if (!spi_is_ready_dt(&config->bus)) {
		LOG_ERR("%s: SPI bus is not ready", dev->name);
		return -ENODEV;
	}

	adc_context_init(&data->ctx);
	k_sem_init(&data->acquire_signal, 0, 1);
	data->active_channel = ADS1X2S14_NO_CHANNEL;

	ret = ads1x2s14_idac_init(dev);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_CONVERSION_CTRL,
				  ADS1X2S14_CONVERSION_CTRL_RESET);
	if (ret != 0) {
		return ret;
	}

	k_sleep(K_USEC(ADS1X2S14_RESET_DELAY_US));

	ret = ads1x2s14_reg_read(dev, ADS1X2S14_REG_DEVICE_ID, &device_id);
	if (ret != 0) {
		return ret;
	}

	if ((device_id & ADS1X2S14_DEVICE_ID_MASK) != config->device_id) {
		LOG_ERR("%s: unexpected device id 0x%02x", dev->name, device_id);
		return -ENODEV;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_STATUS_MSB, ADS1X2S14_STATUS_MSB_CLEAR_FLAGS);
	if (ret != 0) {
		return ret;
	}

	ret = ads1x2s14_reg_write(dev, ADS1X2S14_REG_DIGITAL_CFG, ADS1X2S14_DIGITAL_CFG_STATUS_EN);
	if (ret != 0) {
		return ret;
	}
	data->status_header = true;

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static DEVICE_API(adc, ads1x2s14_api) = {
	.channel_setup = ads1x2s14_channel_setup,
	.read = ads1x2s14_read,
	.ref_internal = ADS1X2S14_INTERNAL_REF_1V25_MV,
	.ref_get = ads1x2s14_ref_get,
#ifdef CONFIG_ADC_ASYNC
	.read_async = ads1x2s14_read_async,
#endif
};

#define ADS1X2S14_INIT(n, name, res, id)                                                           \
	static const struct ads1x2s14_config config_##name##_##n = {                               \
		.bus = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_CONTROLLER | SPI_MODE_CPHA |            \
						       SPI_WORD_SET(8)),                           \
		.internal_ref_mv = DT_INST_PROP(n, internal_reference_mv),                         \
		.idac_current_ua = {DT_INST_PROP(n, idac1_current_microamp),                       \
				    DT_INST_PROP(n, idac2_current_microamp)},                      \
		.resolution = res,                                                                 \
		.device_id = id,                                                                   \
		.global_chop = DT_INST_PROP(n, global_chop),                                       \
	};                                                                                         \
	static struct ads1x2s14_data data_##name##_##n;                                            \
	DEVICE_DT_INST_DEFINE(n, ads1x2s14_init, NULL, &data_##name##_##n, &config_##name##_##n,   \
			      POST_KERNEL, CONFIG_ADC_INIT_PRIORITY, &ads1x2s14_api);

/* ADS112S14: 16 bit */
#define DT_DRV_COMPAT     ti_ads112s14
#define ADS112S14_INIT(n) ADS1X2S14_INIT(n, ti_ads112s14, 16, ADS112S14_DEVICE_ID)
DT_INST_FOREACH_STATUS_OKAY(ADS112S14_INIT)

/* ADS122S14: 24 bit */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT     ti_ads122s14
#define ADS122S14_INIT(n) ADS1X2S14_INIT(n, ti_ads122s14, 24, ADS122S14_DEVICE_ID)
DT_INST_FOREACH_STATUS_OKAY(ADS122S14_INIT)
