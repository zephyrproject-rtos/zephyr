/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <errno.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/adxl38x.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include "adxl38x.h"

LOG_MODULE_REGISTER(ADXL38X, CONFIG_SENSOR_LOG_LEVEL);

#ifdef ADXL38X_BUS_SPI
static int adxl38x_bus_is_ready_spi(const union adxl38x_bus *bus)
{
	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

static int adxl38x_reg_access_spi(const struct device *dev, bool read, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	int ret;
	const struct adxl38x_dev_config *cfg = dev->config;
	uint8_t addr_buf = (reg_addr << 1) | (read ? 0x01u : 0x00u);

	const struct spi_buf tx_bufs[] = {
		{
			.buf = &addr_buf,
			.len = 1,
		},
		{
			.buf = read ? NULL : data,
			.len = read ? 0 : length,
		},
	};
	const struct spi_buf_set tx = {
		.buffers = tx_bufs,
		.count = ARRAY_SIZE(tx_bufs),
	};
	const struct spi_buf rx_bufs[] = {
		{
			.buf = NULL,
			.len = 1,
		},
		{
			.buf = read ? data : NULL,
			.len = read ? length : 0,
		},
	};
	const struct spi_buf_set rx = {
		.buffers = rx_bufs,
		.count = ARRAY_SIZE(rx_bufs),
	};

	ret = spi_transceive_dt(&cfg->bus.spi, &tx, &rx);
	if (ret) {
		LOG_ERR("SPI %s failed: %d", read ? "read" : "write", ret);
		return ret;
	}

	return 0;
}
#endif /* ADXL38X_BUS_SPI */

int adxl38x_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length)
{
	const struct adxl38x_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, true, reg_addr, data, length);
}

int adxl38x_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length)
{
	const struct adxl38x_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, false, reg_addr, data, length);
}

int adxl38x_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
	int ret;
	uint8_t reg_val;

	ret = adxl38x_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret != 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, value);

	return adxl38x_reg_write(dev, reg_addr, &reg_val, 1);
}

static int adxl38x_bus_is_ready(const struct device *dev)
{
	const struct adxl38x_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

static bool adxl38x_is_dsm_mode(enum adxl38x_op_mode op_mode)
{
	switch (op_mode) {
	case ADXL38X_LP_MODE:
	case ADXL38X_LP_ULP_MODE:
	case ADXL38X_LP_VLP_MODE:
	case ADXL38X_RBW_MODE:
	case ADXL38X_RBW_ULP_MODE:
	case ADXL38X_RBW_VLP_MODE:
	case ADXL38X_HP_MODE:
	case ADXL38X_HP_ULP_MODE:
	case ADXL38X_HP_VLP_MODE:
		return true;
	default:
		return false;
	}
}

static uint32_t adxl38x_base_odr(enum adxl38x_chip_type chip_type, enum adxl38x_op_mode op_mode)
{
	bool adxl382 = (chip_type == ADXL38X_CHIP_ADXL382);

	switch (op_mode) {
	case ADXL38X_HP_MODE:
	case ADXL38X_HP_ULP_MODE:
	case ADXL38X_HP_VLP_MODE:
		return adxl382 ? ADXL38X_ODR_16000_HZ : ADXL38X_ODR_8000_HZ;
	case ADXL38X_RBW_MODE:
	case ADXL38X_RBW_ULP_MODE:
	case ADXL38X_RBW_VLP_MODE:
		return adxl382 ? ADXL38X_ODR_8000_HZ : ADXL38X_ODR_4000_HZ;
	case ADXL38X_LP_MODE:
	case ADXL38X_LP_ULP_MODE:
	case ADXL38X_LP_VLP_MODE:
		return adxl382 ? ADXL38X_ODR_4000_HZ : ADXL38X_ODR_2000_HZ;
	case ADXL38X_HEART_SOUND_MODE:
		return ADXL38X_ODR_400_HZ;
	case ADXL38X_ULTRA_LP_MODE:
		return ADXL38X_ODR_50_HZ;
	case ADXL38X_VERY_LP_MODE:
		return ADXL38X_ODR_500_HZ;
	default:
		return 0;
	}
}

static uint32_t adxl38x_effective_odr(const struct device *dev)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	const struct adxl38x_data *data = dev->data;
	uint32_t odr = adxl38x_base_odr(cfg->chip_type, data->op_mode);

	if (odr == 0) {
		return 0;
	}

	if (!adxl38x_is_dsm_mode(data->op_mode)) {
		return odr;
	}

	if (data->sinc_rate) {
		odr *= ADXL38X_ODR_MULT_SINC_RATE;
	} else if (data->dec2x_bypass) {
		odr *= ADXL38X_ODR_MULT_DEC2X_BYPASS;
	}

	if (cfg->chip_type == ADXL38X_CHIP_ADXL380 && data->double_speed) {
		odr *= ADXL38X_ODR_MULT_DOUBLE_SPEED;
	}

	return odr;
}

int adxl38x_set_op_mode(const struct device *dev, enum adxl38x_op_mode op_mode)
{
	struct adxl38x_data *data = dev->data;
	int ret;

	ret = adxl38x_reg_update(dev, ADXL38X_OP_MODE, ADXL38X_OP_MODE_MSK, op_mode);
	if (ret != 0) {
		return ret;
	}

	data->op_mode = op_mode;
	data->accel_odr = adxl38x_effective_odr(dev);
	return 0;
}

static int adxl38x_set_odr(const struct device *dev, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	const struct adxl38x_dev_config *cfg = dev->config;
	uint32_t requested = (uint32_t)val->val1;
	uint32_t unit;
	uint8_t sinc_rate;
	uint8_t dec2x_bypass;
	int ret;

	if (!adxl38x_is_dsm_mode(data->op_mode)) {
		LOG_ERR("ODR is fixed in the current (SAR) operating mode");
		return -ENOTSUP;
	}

	unit = adxl38x_base_odr(cfg->chip_type, data->op_mode);
	if (cfg->chip_type == ADXL38X_CHIP_ADXL380 && data->double_speed) {
		unit *= ADXL38X_ODR_MULT_DOUBLE_SPEED;
	}
	if (unit == 0) {
		return -EINVAL;
	}

	if (requested == unit) {
		sinc_rate = 0;
		dec2x_bypass = 0;
	} else if (requested == unit * ADXL38X_ODR_MULT_DEC2X_BYPASS) {
		sinc_rate = 0;
		dec2x_bypass = 1;
	} else if (requested == unit * ADXL38X_ODR_MULT_SINC_RATE) {
		sinc_rate = 1;
		dec2x_bypass = 0;
	} else {
		LOG_ERR("Unsupported ODR %u Hz for the current mode", requested);
		return -EINVAL;
	}

	ret = adxl38x_reg_update(dev, ADXL38X_TRIG_CFG, ADXL38X_TRIG_CFG_SINC_RATE_MSK, sinc_rate);
	if (ret != 0) {
		return ret;
	}
	ret = adxl38x_reg_update(dev, ADXL38X_TRIG_CFG, ADXL38X_TRIG_CFG_DEC_2X_BYPASS_MSK,
				 dec2x_bypass);
	if (ret != 0) {
		return ret;
	}

	data->sinc_rate = sinc_rate;
	data->dec2x_bypass = dec2x_bypass;
	data->accel_odr = adxl38x_effective_odr(dev);
	return 0;
}

static int adxl38x_get_odr(const struct device *dev, struct sensor_value *val)
{
	val->val1 = (int32_t)adxl38x_effective_odr(dev);
	val->val2 = 0;
	return 0;
}

static int adxl38x_get_sensitivity(enum adxl38x_chip_type chip_type, uint8_t range,
				   int32_t *sensitivity)
{
	if (sensitivity == NULL) {
		return -EINVAL;
	}

	switch (chip_type) {
	case ADXL38X_CHIP_ADXL380:
		switch (range) {
		case ADXL380_DT_RANGE_4G:
			*sensitivity = ADXL380_SENSITIVITY_4G;
			break;
		case ADXL380_DT_RANGE_8G:
			*sensitivity = ADXL380_SENSITIVITY_8G;
			break;
		case ADXL380_DT_RANGE_16G:
			*sensitivity = ADXL380_SENSITIVITY_16G;
			break;
		default:
			LOG_ERR("Invalid ADXL380 range setting: %d", range);
			return -EINVAL;
		}
		break;
	case ADXL38X_CHIP_ADXL382:
		switch (range) {
		case ADXL382_DT_RANGE_15G:
			*sensitivity = ADXL382_SENSITIVITY_15G;
			break;
		case ADXL382_DT_RANGE_30G:
			*sensitivity = ADXL382_SENSITIVITY_30G;
			break;
		case ADXL382_DT_RANGE_60G:
			*sensitivity = ADXL382_SENSITIVITY_60G;
			break;
		default:
			LOG_ERR("Invalid ADXL382 range setting: %d", range);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Invalid chip type: %d", chip_type);
		return -EINVAL;
	}

	return 0;
}

static int adxl38x_accel_convert(struct sensor_value *val, int16_t sample, uint8_t range,
				 enum adxl38x_chip_type chip_type)
{
	int ret;
	int32_t sensitivity;
	int64_t micro_ms2;

	ret = adxl38x_get_sensitivity(chip_type, range, &sensitivity);
	if (ret != 0) {
		return ret;
	}

	micro_ms2 = ((int64_t)sample * SENSOR_G) / sensitivity;
	val->val1 = (int32_t)(micro_ms2 / ADXL38X_MICRO_PER_UNIT);
	val->val2 = (int32_t)(micro_ms2 % ADXL38X_MICRO_PER_UNIT);

	return 0;
}

int adxl38x_read_accel_data(const struct device *dev, struct adxl38x_sample *sample)
{
	int ret;
	uint8_t data_raw[6];
	struct adxl38x_data *data = dev->data;
	const struct adxl38x_dev_config *cfg = dev->config;

	ret = adxl38x_reg_read(dev, ADXL38X_XDATA_H, data_raw, sizeof(data_raw));
	if (ret != 0) {
		LOG_ERR("Failed to read accel data");
		return ret;
	}

	sample->x = (int16_t)(data_raw[0] << 8 | data_raw[1]);
	sample->y = (int16_t)(data_raw[2] << 8 | data_raw[3]);
	sample->z = (int16_t)(data_raw[4] << 8 | data_raw[5]);
	sample->range = data->range;
	sample->chip_type = cfg->chip_type;

	return 0;
}

int adxl38x_read_temp_raw(const struct device *dev, int16_t *raw)
{
	int ret;
	uint8_t data_raw[2];

	ret = adxl38x_reg_read(dev, ADXL38X_TDATA_H, data_raw, sizeof(data_raw));
	if (ret != 0) {
		LOG_ERR("Failed to read temperature data");
		return ret;
	}

	*raw = sign_extend((data_raw[0] << ADXL38X_TEMP_LSB_BITS) |
				   FIELD_GET(ADXL38X_TEMP_OUTPUT_TDATA_L_MSK, data_raw[1]),
			   ADXL38X_TEMP_SIGN_BIT);

	return 0;
}

static int adxl38x_read_temp_data(const struct device *dev)
{
	struct adxl38x_data *data = dev->data;

	return adxl38x_read_temp_raw(dev, &data->temp_val);
}

static void adxl38x_temp_convert(struct sensor_value *val, int16_t raw)
{
	int64_t micro_c = (int64_t)ADXL38X_TEMP_REF_DEG_C * ADXL38X_MICRO_PER_UNIT +
			  ((int64_t)(raw - ADXL38X_TEMP_25C_INTERCEPT) * ADXL38X_MICRO_PER_UNIT *
			   ADXL38X_TEMP_SENS_SCALE) /
				  ADXL38X_TEMP_SENS_X10;

	val->val1 = (int32_t)(micro_c / ADXL38X_MICRO_PER_UNIT);
	val->val2 = (int32_t)(micro_c % ADXL38X_MICRO_PER_UNIT);
}

static int adxl38x_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct adxl38x_data *data = dev->data;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ALL:
		ret = adxl38x_read_accel_data(dev, &data->samples);
		if (ret != 0) {
			return ret;
		}
		return adxl38x_read_temp_data(dev);
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		return adxl38x_read_accel_data(dev, &data->samples);
	case SENSOR_CHAN_DIE_TEMP:
		return adxl38x_read_temp_data(dev);
	default:
		LOG_ERR("Channel %u not supported", chan);
		return -ENOTSUP;
	}
}

static int adxl38x_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	struct adxl38x_sample *sample = &data->samples;
	int ret;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		return adxl38x_accel_convert(val, sample->x, data->range, cfg->chip_type);
	case SENSOR_CHAN_ACCEL_Y:
		return adxl38x_accel_convert(val, sample->y, data->range, cfg->chip_type);
	case SENSOR_CHAN_ACCEL_Z:
		return adxl38x_accel_convert(val, sample->z, data->range, cfg->chip_type);
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = adxl38x_accel_convert(val, sample->x, data->range, cfg->chip_type);
		if (ret != 0) {
			return ret;
		}
		ret = adxl38x_accel_convert(val + 1, sample->y, data->range, cfg->chip_type);
		if (ret != 0) {
			return ret;
		}
		return adxl38x_accel_convert(val + 2, sample->z, data->range, cfg->chip_type);
	case SENSOR_CHAN_DIE_TEMP:
		adxl38x_temp_convert(val, data->temp_val);
		return 0;
	default:
		LOG_ERR("Channel %u not supported", chan);
		return -ENOTSUP;
	}
}

static int adxl38x_set_range(const struct device *dev, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	const struct adxl38x_dev_config *cfg = dev->config;
	uint8_t range;
	int ret;

	if (cfg->chip_type == ADXL38X_CHIP_ADXL382) {
		switch (val->val1) {
		case ADXL382_RANGE_15G_FS:
			range = ADXL382_DT_RANGE_15G;
			break;
		case ADXL382_RANGE_30G_FS:
			range = ADXL382_DT_RANGE_30G;
			break;
		case ADXL382_RANGE_60G_FS:
			range = ADXL382_DT_RANGE_60G;
			break;
		default:
			LOG_ERR("Invalid ADXL382 range %d g", val->val1);
			return -EINVAL;
		}
	} else {
		switch (val->val1) {
		case ADXL380_RANGE_4G_FS:
			range = ADXL380_DT_RANGE_4G;
			break;
		case ADXL380_RANGE_8G_FS:
			range = ADXL380_DT_RANGE_8G;
			break;
		case ADXL380_RANGE_16G_FS:
			range = ADXL380_DT_RANGE_16G;
			break;
		default:
			LOG_ERR("Invalid ADXL380 range %d g", val->val1);
			return -EINVAL;
		}
	}

	ret = adxl38x_reg_update(dev, ADXL38X_OP_MODE, ADXL38X_OP_MODE_RANGE_MSK, range);
	if (ret != 0) {
		return ret;
	}

	data->range = range;
	return 0;
}

static int adxl38x_get_range(const struct device *dev, struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	const struct adxl38x_dev_config *cfg = dev->config;

	val->val2 = 0;
	if (cfg->chip_type == ADXL38X_CHIP_ADXL382) {
		switch (data->range) {
		case ADXL382_DT_RANGE_15G:
			val->val1 = ADXL382_RANGE_15G_FS;
			break;
		case ADXL382_DT_RANGE_30G:
			val->val1 = ADXL382_RANGE_30G_FS;
			break;
		case ADXL382_DT_RANGE_60G:
			val->val1 = ADXL382_RANGE_60G_FS;
			break;
		default:
			return -EINVAL;
		}
	} else {
		switch (data->range) {
		case ADXL380_DT_RANGE_4G:
			val->val1 = ADXL380_RANGE_4G_FS;
			break;
		case ADXL380_DT_RANGE_8G:
			val->val1 = ADXL380_RANGE_8G_FS;
			break;
		case ADXL380_DT_RANGE_16G:
			val->val1 = ADXL380_RANGE_16G_FS;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static int adxl38x_set_op_mode_attr(const struct device *dev, const struct sensor_value *val)
{
	switch (val->val1) {
	case ADXL38X_STANDBY_MODE:
	case ADXL38X_HEART_SOUND_MODE:
	case ADXL38X_ULTRA_LP_MODE:
	case ADXL38X_VERY_LP_MODE:
	case ADXL38X_LP_MODE:
	case ADXL38X_LP_ULP_MODE:
	case ADXL38X_LP_VLP_MODE:
	case ADXL38X_RBW_MODE:
	case ADXL38X_RBW_ULP_MODE:
	case ADXL38X_RBW_VLP_MODE:
	case ADXL38X_HP_MODE:
	case ADXL38X_HP_ULP_MODE:
	case ADXL38X_HP_VLP_MODE:
		break;
	default:
		LOG_ERR("Invalid/Nonvalid operating mode %d", val->val1);
		return -EINVAL;
	}

	return adxl38x_set_op_mode(dev, (enum adxl38x_op_mode)val->val1);
}

static int adxl38x_get_op_mode_attr(const struct device *dev, struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;

	val->val1 = data->op_mode;
	val->val2 = 0;
	return 0;
}

static int adxl38x_set_hpf_corner(const struct device *dev, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;

	if (val->val1 < ADXL38X_DT_HPF_OFF || val->val1 > ADXL38X_DT_HPF_0_0238e_4) {
		LOG_ERR("Invalid HPF corner %d", val->val1);
		return -EINVAL;
	}

	int ret = adxl38x_reg_update(dev, ADXL38X_FILTER, ADXL38X_FILTER_HPF_CORNER_MSK,
				     (uint8_t)val->val1);
	if (ret != 0) {
		return ret;
	}

	data->hpf_corner = val->val1;
	return 0;
}

static int adxl38x_get_hpf_corner(const struct device *dev, struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;

	val->val1 = data->hpf_corner;
	val->val2 = 0;
	return 0;
}

static int adxl38x_set_int_pol(const struct device *dev, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	int ret;

	if (val->val1 < ADXL38X_DT_INT_POL_ACTIVE_HIGH ||
	    val->val1 > ADXL38X_DT_INT_POL_ACTIVE_LOW) {
		LOG_ERR("Invalid interrupt polarity %d", val->val1);
		return -EINVAL;
	}

	ret = adxl38x_reg_update(dev, ADXL38X_INT0, ADXL38X_INT_POL_MSK, (uint8_t)val->val1);
	if (ret != 0) {
		return ret;
	}
	ret = adxl38x_reg_update(dev, ADXL38X_INT1, ADXL38X_INT_POL_MSK, (uint8_t)val->val1);
	if (ret != 0) {
		return ret;
	}

	data->int_pol = val->val1;
	return 0;
}

static int adxl38x_get_int_pol(const struct device *dev, struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;

	val->val1 = data->int_pol;
	val->val2 = 0;
	return 0;
}

/* Write the 9-bit FIFO watermark: bit [8] in FIFO_CFG0, bits [7:0] in FIFO_CFG1. */
static int adxl38x_write_fifo_watermark(const struct device *dev, uint16_t watermark)
{
	int ret;

	ret = adxl38x_reg_update(dev, ADXL38X_FIFO_CFG0, ADXL38X_FIFO_CFG0_SAMPLES_8_MSK,
				 watermark >> ADXL38X_FIFO_SAMPLES_LOW_BITS);
	if (ret != 0) {
		return ret;
	}

	uint8_t low = watermark & ADXL38X_FIFO_CFG1_SAMPLES_LOW_MSK;

	return adxl38x_reg_write(dev, ADXL38X_FIFO_CFG1, &low, 1);
}

static int adxl38x_set_fifo_watermark(const struct device *dev, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	uint16_t watermark = (uint16_t)val->val1;
	int ret;

	if (watermark == 0 || watermark > ADXL38X_FIFO_MAX_SAMPLES) {
		LOG_ERR("Invalid FIFO watermark %u", watermark);
		return -EINVAL;
	}
	if (watermark % ADXL38X_FIFO_CHANNELS != 0) {
		LOG_ERR("FIFO watermark must be a multiple of %u", ADXL38X_FIFO_CHANNELS);
		return -EINVAL;
	}

	ret = adxl38x_write_fifo_watermark(dev, watermark);
	if (ret != 0) {
		return ret;
	}

	data->fifo_watermark = watermark;
	return 0;
}

static int adxl38x_get_fifo_watermark(const struct device *dev, struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;

	val->val1 = data->fifo_watermark;
	val->val2 = 0;
	return 0;
}

static int adxl38x_accel_to_sar_code(const struct device *dev, int64_t micro_ms2, uint16_t *code)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	int fsr_g;
	int64_t codes;

	if (micro_ms2 < 0) {
		LOG_ERR("Threshold must be non-negative");
		return -EINVAL;
	}

	if (cfg->chip_type == ADXL38X_CHIP_ADXL382) {
		fsr_g = (data->range == ADXL382_DT_RANGE_15G)   ? ADXL382_RANGE_15G_FS
			: (data->range == ADXL382_DT_RANGE_30G) ? ADXL382_RANGE_30G_FS
								: ADXL382_RANGE_60G_FS;
	} else {
		fsr_g = (data->range == ADXL380_DT_RANGE_4G)   ? ADXL380_RANGE_4G_FS
			: (data->range == ADXL380_DT_RANGE_8G) ? ADXL380_RANGE_8G_FS
							       : ADXL380_RANGE_16G_FS;
	}

	/* g = micro_ms2 / SENSOR_G; code = g * ADXL38X_SAR_FULL_SCALE_CODES / fsr_g */
	codes = (micro_ms2 * ADXL38X_SAR_FULL_SCALE_CODES) / ((int64_t)SENSOR_G * fsr_g);
	if (codes > ADXL38X_THRESH_MAX) {
		LOG_ERR("Threshold out of range");
		return -EINVAL;
	}

	*code = (uint16_t)codes;
	return 0;
}

static int adxl38x_set_activity_threshold(const struct device *dev, const struct sensor_value *val,
					  uint8_t reg_h, uint8_t reg_l)
{
	uint16_t code;
	uint8_t buf[2];
	int ret;

	ret = adxl38x_accel_to_sar_code(dev, sensor_value_to_micro(val), &code);
	if (ret != 0) {
		return ret;
	}

	sys_put_be16(code, buf);
	buf[0] &= ADXL38X_THRESH_H_MSK;
	ret = adxl38x_reg_write(dev, reg_h, &buf[0], 1);
	if (ret != 0) {
		return ret;
	}
	return adxl38x_reg_write(dev, reg_l, &buf[1], 1);
}

static int adxl38x_set_activity_time(const struct device *dev, const struct sensor_value *val,
				     uint8_t reg_h, uint8_t reg_m, uint8_t reg_l)
{
	int64_t us = sensor_value_to_micro(val);
	uint32_t ticks;
	uint8_t buf[3];
	int ret;

	if (us < 0) {
		return -EINVAL;
	}

	ticks = (uint32_t)(us / ADXL38X_TIME_ACT_LSB_US);
	if (ticks > ADXL38X_TIME_ACT_MAX) {
		LOG_ERR("Activity time out of range");
		return -EINVAL;
	}

	sys_put_be24(ticks, buf);

	ret = adxl38x_reg_write(dev, reg_h, &buf[0], 1);
	if (ret != 0) {
		return ret;
	}
	ret = adxl38x_reg_write(dev, reg_m, &buf[1], 1);
	if (ret != 0) {
		return ret;
	}
	return adxl38x_reg_write(dev, reg_l, &buf[2], 1);
}

static int adxl38x_activity_axis_enable(const struct device *dev, uint8_t axis_bit, bool enable)
{
	return adxl38x_reg_update(dev, ADXL38X_SNSR_AXIS_EN, axis_bit, enable ? 1 : 0);
}

static int adxl38x_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	if (val == NULL) {
		return -EINVAL;
	}

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl38x_get_odr(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		return adxl38x_get_range(dev, val);
	case SENSOR_ATTR_ADXL38X_OP_MODE:
		return adxl38x_get_op_mode_attr(dev, val);
	case SENSOR_ATTR_ADXL38X_FIFO_WATERMARK:
		return adxl38x_get_fifo_watermark(dev, val);
	case SENSOR_ATTR_ADXL38X_HPF_CORNER:
		return adxl38x_get_hpf_corner(dev, val);
	case SENSOR_ATTR_ADXL38X_INT_POL:
		return adxl38x_get_int_pol(dev, val);
	default:
		LOG_ERR("attr_get: attribute %d not supported", attr);
		return -ENOTSUP;
	}
}

static int adxl38x_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct adxl38x_data *data = dev->data;
	enum adxl38x_op_mode saved_mode;
	int ret;
	int restore_ret;

	if (val == NULL) {
		return -EINVAL;
	}

	/* Configuration changes must be made in standby; restore mode afterwards. */
	saved_mode = data->op_mode;
	ret = adxl38x_set_op_mode(dev, ADXL38X_STANDBY_MODE);
	if (ret != 0) {
		LOG_ERR("Failed to enter standby before attribute set");
		return ret;
	}
	/* set_op_mode overwrote data->op_mode; keep the target mode for ODR math. */
	data->op_mode = saved_mode;

	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = adxl38x_set_odr(dev, val);
		break;
	case SENSOR_ATTR_FULL_SCALE:
		ret = adxl38x_set_range(dev, val);
		break;
	case SENSOR_ATTR_ADXL38X_OP_MODE:
		ret = adxl38x_set_op_mode_attr(dev, val);
		saved_mode = data->op_mode;
		break;
	case SENSOR_ATTR_ADXL38X_FIFO_WATERMARK:
		ret = adxl38x_set_fifo_watermark(dev, val);
		break;
	case SENSOR_ATTR_ADXL38X_HPF_CORNER:
		ret = adxl38x_set_hpf_corner(dev, val);
		break;
	case SENSOR_ATTR_ADXL38X_INT_POL:
		ret = adxl38x_set_int_pol(dev, val);
		break;
	case SENSOR_ATTR_ADXL38X_ACTIVITY_THRESHOLD:
		ret = adxl38x_set_activity_threshold(dev, val, ADXL38X_THRESH_ACT_H,
						     ADXL38X_THRESH_ACT_L);
		break;
	case SENSOR_ATTR_ADXL38X_ACTIVITY_TIME:
		ret = adxl38x_set_activity_time(dev, val, ADXL38X_TIME_ACT_H, ADXL38X_TIME_ACT_M,
						ADXL38X_TIME_ACT_L);
		break;
	case SENSOR_ATTR_ADXL38X_INACTIVITY_THRESHOLD:
		ret = adxl38x_set_activity_threshold(dev, val, ADXL38X_THRESH_INACT_H,
						     ADXL38X_THRESH_INACT_L);
		break;
	case SENSOR_ATTR_ADXL38X_INACTIVITY_TIME:
		ret = adxl38x_set_activity_time(dev, val, ADXL38X_TIME_INACT_H,
						ADXL38X_TIME_INACT_M, ADXL38X_TIME_INACT_L);
		break;
	case SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_X:
		ret = adxl38x_activity_axis_enable(dev, BIT(0), val->val1 != 0);
		break;
	case SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_Y:
		ret = adxl38x_activity_axis_enable(dev, BIT(1), val->val1 != 0);
		break;
	case SENSOR_ATTR_ADXL38X_ACTIVITY_ENABLE_Z:
		ret = adxl38x_activity_axis_enable(dev, BIT(2), val->val1 != 0);
		break;
	default:
		LOG_ERR("attr_set: attribute %d not supported", attr);
		restore_ret = adxl38x_set_op_mode(dev, saved_mode);
		if (restore_ret != 0) {
			LOG_ERR("Failed to restore op mode after attr_set: %d", restore_ret);
		}
		return -ENOTSUP;
	}

	if (ret != 0) {
		LOG_ERR("Failed to set attribute %d", attr);
		restore_ret = adxl38x_set_op_mode(dev, saved_mode);
		if (restore_ret != 0) {
			LOG_ERR("Failed to restore op mode after attr_set: %d", restore_ret);
		}
		return ret;
	}

	return adxl38x_set_op_mode(dev, saved_mode);
}

static int adxl38x_read_device_id(const struct device *dev)
{
	int ret;
	uint8_t device_id;
	uint8_t part_id;
	uint8_t mems_id;

	/* Read and verify device ID */
	ret = adxl38x_reg_read(dev, ADXL38X_DEVID_AD, &device_id, 1);
	if (ret != 0) {
		return ret;
	}
	if (device_id != ADXL38X_DEVID_AD_VAL) {
		LOG_DBG("Unexpected DEVID_AD: 0x%02x", device_id);
		return -ENODEV;
	}

	/* Read and verify part ID */
	ret = adxl38x_reg_read(dev, ADXL38X_PART_ID, &part_id, 1);
	if (ret != 0) {
		return ret;
	}
	if (part_id != ADXL38X_PART_ID_VAL) {
		LOG_DBG("Unexpected PART_ID: 0x%02x", part_id);
		return -ENODEV;
	}

	/* Read and verify MEMS ID */
	ret = adxl38x_reg_read(dev, ADXL38X_DEVID_MST, &mems_id, 1);
	if (ret != 0) {
		return ret;
	}
	if (mems_id != ADXL38X_DEVID_MST_VAL) {
		LOG_DBG("Unexpected DEVID_MST: 0x%02x", mems_id);
		return -ENODEV;
	}

	return 0;
}

static int adxl38x_check_device_id(const struct device *dev)
{
	unsigned int attempts = ADXL38X_ID_READ_RETRIES;
	int ret;

	do {
		ret = adxl38x_read_device_id(dev);
		if (ret == 0) {
			return 0;
		}
		if (--attempts > 0U) {
			k_sleep(K_MSEC(ADXL38X_ID_READ_RETRY_MS));
		}
	} while (attempts > 0U);

	LOG_ERR("Device ID check failed after %u attempts (last error %d)",
		ADXL38X_ID_READ_RETRIES, ret);
	return ret;
}

static int adxl38x_soft_reset(const struct device *dev)
{
	int ret;
	uint8_t reset_code = ADXL38X_RESET_CODE;

	ret = adxl38x_reg_write(dev, ADXL38X_REG_RESET, &reset_code, 1);
	if (ret != 0) {
		LOG_ERR("Failed to issue soft reset");
		return ret;
	}

	/* Wait for the EFUSE refresh to complete before touching any register. */
	k_sleep(K_MSEC(ADXL38X_RESET_SETTLE_MS));

	return 0;
}

static int adxl38x_configure(const struct device *dev)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	struct adxl38x_data *data = dev->data;
	int ret;

	/* Set default range from devicetree overlay */
	ret = adxl38x_reg_update(dev, ADXL38X_OP_MODE, ADXL38X_OP_MODE_RANGE_MSK, cfg->range);
	if (ret != 0) {
		LOG_ERR("Failed to set range");
		return ret;
	}
	data->range = cfg->range;

	/* Set default HPF corner from devicetree overlay */
	ret = adxl38x_reg_update(dev, ADXL38X_FILTER, ADXL38X_FILTER_HPF_CORNER_MSK,
				 cfg->hpf_corner);
	if (ret != 0) {
		LOG_ERR("Failed to set HPF corner");
		return ret;
	}
	data->hpf_corner = cfg->hpf_corner;

	/* Set default interrupt polarity from devicetree overlay */
	ret = adxl38x_reg_update(dev, ADXL38X_INT0, ADXL38X_INT_POL_MSK, cfg->int_pol);
	if (ret != 0) {
		return ret;
	}
	ret = adxl38x_reg_update(dev, ADXL38X_INT1, ADXL38X_INT_POL_MSK, cfg->int_pol);
	if (ret != 0) {
		return ret;
	}
	data->int_pol = cfg->int_pol;

	/* Set default FIFO watermark from devicetree overlay */
	ret = adxl38x_write_fifo_watermark(dev, cfg->fifo_watermark);
	if (ret != 0) {
		return ret;
	}
	data->fifo_watermark = cfg->fifo_watermark;

	/* Enable X, Y, Z and temperature channels */
	ret = adxl38x_reg_update(dev, ADXL38X_DIG_EN,
				 ADXL38X_DIG_EN_CHANNEL_XYZ_MSK | ADXL38X_DIG_EN_CHANNEL_TEMP_MSK,
				 0xF);
	if (ret != 0) {
		LOG_ERR("Failed to enable channels");
		return ret;
	}

	if (cfg->chip_type == ADXL38X_CHIP_ADXL380) {
		ret = adxl38x_reg_update(dev, ADXL38X_DIG_EN, ADXL38X_DIG_EN_DOUBLE_SPEED_MSK,
					 cfg->double_speed);
		if (ret != 0) {
			LOG_ERR("Failed to set double-speed mode");
			return ret;
		}
		data->double_speed = cfg->double_speed;
	}

#ifdef CONFIG_ADXL38X_STREAM
	/* Enable the FIFO and use normal mode: it operates in every op mode and,
	 * read on the watermark interrupt, provides continuous streaming as long
	 * as the host keeps up (true "stream" mode is ULP/HS only per datasheet).
	 */
	ret = adxl38x_reg_update(dev, ADXL38X_FIFO_CFG0, ADXL38X_FIFO_CFG0_MODE_MSK,
				 ADXL38X_FIFO_NORMAL);
	if (ret != 0) {
		LOG_ERR("Failed to set FIFO mode");
		return ret;
	}
	ret = adxl38x_reg_update(dev, ADXL38X_DIG_EN, ADXL38X_DIG_EN_FIFO_EN_MSK, 1);
	if (ret != 0) {
		LOG_ERR("Failed to enable FIFO");
		return ret;
	}
#endif

	/* Decimation default values after soft reset */
	data->sinc_rate = 0;
	data->dec2x_bypass = 0;

	return 0;
}

static int adxl38x_init(const struct device *dev)
{
	const struct adxl38x_dev_config *cfg = dev->config;
	int ret;

	ret = adxl38x_bus_is_ready(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize sensor bus\n");
		return ret;
	}

	ret = adxl38x_check_device_id(dev);
	if (ret != 0) {
		return ret;
	}

	ret = adxl38x_soft_reset(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset device\n");
		return ret;
	}

	ret = adxl38x_configure(dev);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_ADXL38X_TRIGGER
	ret = adxl38x_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupt");
		return ret;
	}
#endif

	ret = adxl38x_set_op_mode(dev, cfg->op_mode);
	if (ret != 0) {
		LOG_ERR("Failed to set operating mode");
		return ret;
	}

	return 0;
}

#ifdef CONFIG_PM_DEVICE
static int adxl38x_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct adxl38x_dev_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return adxl38x_set_op_mode(dev, cfg->op_mode);
	case PM_DEVICE_ACTION_SUSPEND:
		return adxl38x_set_op_mode(dev, ADXL38X_STANDBY_MODE);
	default:
		return -ENOTSUP;
	}
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(sensor, adxl38x_api_funcs) = {
	.attr_set = adxl38x_attr_set,
	.attr_get = adxl38x_attr_get,
	.sample_fetch = adxl38x_sample_fetch,
	.channel_get = adxl38x_channel_get,
#ifdef CONFIG_ADXL38X_TRIGGER
	.trigger_set = adxl38x_trigger_set,
#endif
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = adxl38x_submit,
	.get_decoder = adxl38x_get_decoder,
#endif
};

#define ADXL38X_SPI_CFG (SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#ifdef CONFIG_PM_DEVICE
#define ADXL38X_PM_DEFINE(name, inst) PM_DEVICE_DT_INST_DEFINE(inst, adxl38x_pm_action);
#define ADXL38X_PM_GET(inst)          PM_DEVICE_DT_INST_GET(inst)
#else
#define ADXL38X_PM_DEFINE(name, inst)
#define ADXL38X_PM_GET(inst) NULL
#endif

#ifdef CONFIG_ADXL38X_TRIGGER
#define ADXL38X_CFG_IRQ(inst)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int0_gpios),				\
		(.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, int0_gpios),		\
		 .use_int1 = false,),							\
		(.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, int1_gpios),		\
		 .use_int1 = true,))
#else
#define ADXL38X_CFG_IRQ(inst)
#endif

#define ADXL38X_HAS_IRQ(inst)                                                                      \
	UTIL_OR(DT_INST_NODE_HAS_PROP(inst, int0_gpios), DT_INST_NODE_HAS_PROP(inst, int1_gpios))

#define ADXL38X_RTIO_DEFINE(name, inst)                                                            \
	SPI_DT_IODEV_DEFINE(adxl38x_iodev_##name##_##inst, DT_DRV_INST(inst), ADXL38X_SPI_CFG);    \
	RTIO_DEFINE(adxl38x_rtio_ctx_##name##_##inst, 16, 16);

#define ADXL38X_CONFIG(inst, chip)                                                                 \
	.chip_type = (chip), .op_mode = DT_INST_PROP(inst, op_mode),                               \
	.range = DT_INST_PROP(inst, range), .fifo_watermark = DT_INST_PROP(inst, fifo_watermark),  \
	.hpf_corner = DT_INST_PROP(inst, hpf_corner), .int_pol = DT_INST_PROP(inst, int_pol),      \
	.double_speed = DT_INST_PROP_OR(inst, double_speed, 0)

#define ADXL38X_CONFIG_SPI(inst, chip)                                                             \
	{.bus = {.spi = SPI_DT_SPEC_INST_GET(inst, ADXL38X_SPI_CFG)},                              \
	 .bus_is_ready = adxl38x_bus_is_ready_spi,                                                 \
	 .reg_access = adxl38x_reg_access_spi,                                                     \
	 ADXL38X_CONFIG(inst, chip),                                                               \
	 COND_CODE_1(ADXL38X_HAS_IRQ(inst), (ADXL38X_CFG_IRQ(inst)), ()) }

#define ADXL38X_DEVICE_INIT(name, inst, chip_val)                                                  \
	BUILD_ASSERT(DT_INST_PROP(inst, fifo_watermark) <= ADXL38X_FIFO_MAX_SAMPLES,               \
		     "fifo-watermark must be <= 320");                                             \
	BUILD_ASSERT(DT_INST_PROP(inst, fifo_watermark) % ADXL38X_FIFO_CHANNELS == 0,              \
		     "fifo-watermark must be a multiple of 4 (X, Y, Z, T words)");                 \
	ADXL38X_PM_DEFINE(name, inst)                                                              \
	IF_ENABLED(CONFIG_ADXL38X_STREAM, (ADXL38X_RTIO_DEFINE(name, inst)))                       \
	static struct adxl38x_data adxl38x_data_##name##_##inst = {                                \
		IF_ENABLED(CONFIG_ADXL38X_STREAM,						\
			   (.rtio_ctx = &adxl38x_rtio_ctx_##name##_##inst,			\
			    .iodev = &adxl38x_iodev_##name##_##inst,)) };      \
	static const struct adxl38x_dev_config adxl38x_config_##name##_##inst =                    \
		ADXL38X_CONFIG_SPI(inst, chip_val);                                                \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl38x_init, ADXL38X_PM_GET(inst),                     \
				     &adxl38x_data_##name##_##inst,                                \
				     &adxl38x_config_##name##_##inst, POST_KERNEL,                 \
				     CONFIG_ADXL38X_INIT_PRIORITY, &adxl38x_api_funcs);

/* ADXL380 instances */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT        adi_adxl380
#define ADXL380_DEFINE(inst) ADXL38X_DEVICE_INIT(adxl380, inst, ADXL38X_CHIP_ADXL380)
DT_INST_FOREACH_STATUS_OKAY(ADXL380_DEFINE)

/* ADXL382 instances */
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT        adi_adxl382
#define ADXL382_DEFINE(inst) ADXL38X_DEVICE_INIT(adxl382, inst, ADXL38X_CHIP_ADXL382)
DT_INST_FOREACH_STATUS_OKAY(ADXL382_DEFINE)
