/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <string.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/__assert.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor/adxl355.h>
#include "adxl355.h"

#define DT_DRV_COMPAT adi_adxl355

LOG_MODULE_REGISTER(ADXL355, CONFIG_SENSOR_LOG_LEVEL);

#ifdef ADXL355_BUS_I2C
/**
 * @brief Check if I2C bus is ready
 *
 * @param bus Pointer to bus union
 * @return int 0 if ready, negative error code otherwise
 */
static int adxl355_bus_is_ready_i2c(const union adxl355_bus *bus)
{
	if (!device_is_ready(bus->i2c.bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief I2C register access function
 *
 * @param dev Device pointer
 * @param read True for read, false for write
 * @param reg_addr
 * @param data
 * @param length
 * @return int
 */
static int adxl355_reg_access_i2c(const struct device *dev, bool read, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	const struct adxl355_dev_config *cfg = dev->config;

	if (read) {
		return i2c_burst_read_dt(&cfg->bus.i2c, reg_addr, data, length);
	} else {
		return i2c_burst_write_dt(&cfg->bus.i2c, reg_addr, data, length);
	}
}
#endif /* ADXL355_BUS_I2C */

#ifdef ADXL355_BUS_SPI
/**
 * @brief Check if SPI bus is ready
 *
 * @param bus Pointer to bus union
 * @return int 0 if ready, negative error code otherwise
 */
static int adxl355_bus_is_ready_spi(const union adxl355_bus *bus)
{
	if (!spi_is_ready_dt(&bus->spi)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	return 0;
}

/**
 * @brief SPI register access function
 *
 * @param dev Device pointer
 * @param read True for read, false for write
 * @param reg_addr Register address
 * @param data Data buffer
 * @param length Number of bytes to read/write
 * @return int
 */
static int adxl355_reg_access_spi(const struct device *dev, bool read, uint8_t reg_addr,
				  uint8_t *data, size_t length)
{
	int ret;
	const struct adxl355_dev_config *cfg = dev->config;
	uint8_t addr_buf = (reg_addr << 1) | (read ? 0x01 : 0x00);

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
#endif /* ADXL355_BUS_SPI */

/**
 * @brief Register Read function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Data buffer
 * @param length Number of bytes to read
 * @return int 0 on success, negative error code on failure
 */
int adxl355_reg_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length)
{
	const struct adxl355_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, true, reg_addr, data, length);
}

/**
 * @brief Register Write function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param data Data buffer
 * @param length Number of bytes to write
 * @return int 0 on success, negative error code on failure
 */
int adxl355_reg_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, size_t length)
{
	const struct adxl355_dev_config *cfg = dev->config;

	return cfg->reg_access(dev, false, reg_addr, data, length);
}

/**
 * @brief Register Update function
 *
 * @param dev Device pointer
 * @param reg_addr Register address
 * @param mask Bit mask
 * @param value Value to set
 * @return int 0 on success, negative error code on failure
 */
int adxl355_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t mask, uint8_t value)
{
	int ret;
	uint8_t reg_val;

	ret = adxl355_reg_read(dev, reg_addr, &reg_val, 1);
	if (ret != 0) {
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, value);

	return adxl355_reg_write(dev, reg_addr, &reg_val, 1);
}

/**
 * @brief Check if selected bus is ready
 *
 * @param dev Device pointer
 * @return int 0 if ready, negative error code otherwise
 */
static int adxl355_bus_is_ready(const struct device *dev)
{
	const struct adxl355_dev_config *cfg = dev->config;

	return cfg->bus_is_ready(&cfg->bus);
}

/**
 * @brief Set Output Data Rate
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_odr(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	enum adxl355_odr odr;

	switch (val->val1) {
	case 4000:
		odr = ADXL355_ODR_4000HZ;
		break;
	case 2000:
		odr = ADXL355_ODR_2000HZ;
		break;
	case 1000:
		odr = ADXL355_ODR_1000HZ;
		break;
	case 500:
		odr = ADXL355_ODR_500HZ;
		break;
	case 250:
		odr = ADXL355_ODR_250HZ;
		break;
	case 125:
		odr = ADXL355_ODR_125HZ;
		break;
	case 62:
		odr = ADXL355_ODR_62_5HZ;
		break;
	case 31:
		odr = ADXL355_ODR_31_25HZ;
		break;
	case 15:
		odr = ADXL355_ODR_15_625HZ;
		break;
	case 7:
		odr = ADXL355_ODR_7_813HZ;
		break;
	case 3:
		odr = ADXL355_ODR_3_906HZ;
		break;
	default:
		LOG_ERR("Invalid ODR %d Hz", val->val1);
		return -EINVAL;
	}
	data->odr = odr;
	return adxl355_reg_update(dev, ADXL355_FILTER, ADXL355_FILTER_ODR_MSK, odr);
}

/**
 * @brief Get Output Data Rate
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_odr(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	switch (data->odr) {
	case ADXL355_ODR_4000HZ:
		val->val1 = 4000;
		break;
	case ADXL355_ODR_2000HZ:
		val->val1 = 2000;
		break;
	case ADXL355_ODR_1000HZ:
		val->val1 = 1000;
		break;
	case ADXL355_ODR_500HZ:
		val->val1 = 500;
		break;
	case ADXL355_ODR_250HZ:
		val->val1 = 250;
		break;
	case ADXL355_ODR_125HZ:
		val->val1 = 125;
		break;
	case ADXL355_ODR_62_5HZ:
		val->val1 = 62;
		val->val2 = 500000;
		break;
	case ADXL355_ODR_31_25HZ:
		val->val1 = 31;
		val->val2 = 250000;
		break;
	case ADXL355_ODR_15_625HZ:
		val->val1 = 15;
		val->val2 = 625000;
		break;
	case ADXL355_ODR_7_813HZ:
		val->val1 = 7;
		val->val2 = 813000;
		break;
	case ADXL355_ODR_3_906HZ:
		val->val1 = 3;
		val->val2 = 906000;
		break;
	default:
		LOG_ERR("Invalid ODR setting");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Set Measurement Range
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_range(const struct device *dev, const struct sensor_value *val)
{
	enum adxl355_range range;
	struct adxl355_data *data = dev->data;

	switch (val->val1) {
	case 2:
		range = ADXL355_RANGE_2G;
		break;
	case 4:
		range = ADXL355_RANGE_4G;
		break;
	case 8:
		range = ADXL355_RANGE_8G;
		break;
	default:
		LOG_ERR("Invalid range %d g", val->val1);
		return -EINVAL;
	}
	data->range = range;
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_RANGE_MSK, range);
}

/**
 * @brief Get Measurement Range
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_range(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	switch (data->range) {
	case ADXL355_RANGE_2G:
		val->val1 = 2;
		break;
	case ADXL355_RANGE_4G:
		val->val1 = 4;
		break;
	case ADXL355_RANGE_8G:
		val->val1 = 8;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Set Data Ready Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_drdy_mode(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_ERR("Invalid DRDY mode value %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.drdy_mode = val->val1;
	return adxl355_reg_update(dev, ADXL355_POWER_CTL, ADXL355_POWER_CTL_DRDY_OFF_MSK,
				  val->val1 ? 1 : 0);
}

/**
 * @brief Get Data Ready Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_drdy_mode(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.drdy_mode;
	return 0;
}

/**
 * @brief Set Temperature Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_temp_mode(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_ERR("Invalid temp mode value %d", val->val1);
		return -EINVAL;
	}

	data->extra_attr.temp_mode = val->val1;
	return adxl355_reg_update(dev, ADXL355_POWER_CTL, ADXL355_POWER_CTL_TEMP_OFF_MSK,
				  val->val1 ? 1 : 0);
}

/**
 * @brief Get Temperature Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_temp_mode(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.temp_mode;
	return 0;
}

/**
 * @brief Set FIFO Watermark
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_fifo_watermark(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t fifo_watermark = val->val1;

	if (fifo_watermark > 96) {
		LOG_ERR("Invalid FIFO watermark %d", fifo_watermark);
		return -EINVAL;
	}

	if (fifo_watermark % 3 != 0) {
		LOG_ERR("FIFO watermark must be multiple of 3");
		return -EINVAL;
	}
	data->fifo_watermark = fifo_watermark;
	return adxl355_reg_write(dev, ADXL355_FIFO_SAMPLES, &fifo_watermark, 1);
}

/**
 * @brief Get FIFO Watermark
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int
 */
static int adxl355_get_fifo_watermark(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->fifo_watermark;
	return 0;
}

/**
 * @brief Set High-Pass Filter Corner
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_hpf_corner(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t corner = val->val1;

	if (corner < ADXL355_HPF_OFF || corner > ADXL355_HPF_0_0238e_4) {
		LOG_ERR("Invalid HPF corner %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.hpf_corner = corner;
	return adxl355_reg_update(dev, ADXL355_FILTER, ADXL355_FILTER_HPF_MASK, corner);
}

/**
 * @brief Get High-Pass Filter Corner
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_hpf_corner(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.hpf_corner;
	return 0;
}

/**
 * @brief Set External Clock
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_ext_clk(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t ext_clk = val->val1;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_ERR("Invalid ext clk value %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.ext_clk = ext_clk;
	return adxl355_reg_update(dev, ADXL355_SYNC, ADXL355_SYNC_EXT_CLK_MSK, ext_clk);
}

/**
 * @brief Get External Clock
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_ext_clk(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.ext_clk;
	return 0;
}

/**
 * @brief Set External Clock
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_ext_sync(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t ext_sync = val->val1;

	if (val->val1 < 0 || val->val1 > 2) {
		LOG_ERR("Invalid ext sync value %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.ext_sync = ext_sync;
	return adxl355_reg_update(dev, ADXL355_SYNC, ADXL355_SYNC_EXT_SYNC_MSK, ext_sync);
}

/**
 * @brief Get External Clock
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_ext_sync(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.ext_sync;
	return 0;
}

/**
 * @brief Set I2C High-Speed Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_i2c_hs(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t i2c_hs = val->val1;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_ERR("Invalid I2C HS value %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.i2c_hs = i2c_hs;
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_I2C_HS_MSK, i2c_hs);
}

/**
 * @brief Get I2C High-Speed Mode
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_i2c_hs(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.i2c_hs;
	return 0;
}

/**
 * @brief Set Interrupt Polarity
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_int_pol(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t int_pol = val->val1;

	if (val->val1 < 0 || val->val1 > 1) {
		LOG_ERR("Invalid INT_POL value %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.int_pol = int_pol;
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_INT_POL_MSK, int_pol);
}

/**
 * @brief Get Interrupt Polarity
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_int_pol(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.int_pol;
	return 0;
}

/**
 * @brief Enable or Disable Activity Detection
 *
 * @param dev Device pointer
 * @param mask Bit mask for activity axes
 * @param enable True to enable, false to disable
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_activity_enable(const struct device *dev, uint8_t mask, bool enable)
{
	return adxl355_reg_update(dev, ADXL355_ACT_EN, mask, enable ? 1 : 0);
}

/**
 * @brief Combine 3 bytes to form a signed 20-bit integer
 *
 * @param msb Most significant byte
 * @param mid Middle byte
 * @param lsb Least significant byte
 * @return int32_t Signed 20-bit integer
 */
static inline int32_t combine_bytes_to_int20(uint8_t msb, uint8_t mid, uint8_t lsb)
{
	return (int32_t)((((msb << 16) | (mid << 8) | lsb) << 8) >> 12);
}

/**
 * @brief Validate offset value based on measurement range
 *
 * @param offset_val Offset value in micro m/s^2
 * @param range Measurement range
 * @return int 0 if valid, negative error code otherwise
 */
static int is_valid_offset(int64_t offset_val, enum adxl355_range range)
{
	switch (range) {
	case ADXL355_RANGE_2G:
		/* Offset limits in micro m/s^2 for ±2g range; g = 9.81m/s^2 */
		if (offset_val < -19620000 || offset_val > 19620000) {
			LOG_ERR("Invalid offset %lld micro m/s^2 for ±2g range", offset_val);
			return -EINVAL;
		}
		break;
	case ADXL355_RANGE_4G:
		/* Offset limits in micro m/s^2 for ±4g range; g = 9.81m/s^2 */
		if (offset_val < -39240000 || offset_val > 39240000) {
			LOG_ERR("Invalid offset %lld micro m/s^2 for ±4g range", offset_val);
			return -EINVAL;
		}
		break;
	case ADXL355_RANGE_8G:
		/* Offset limits in micro m/s^2 for ±8g range; g = 9.81m/s^2 */
		if (offset_val < -78480000 || offset_val > 78480000) {
			LOG_ERR("Invalid offset %lld micro m/s^2 for ±8g range", offset_val);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Set Axis Offset
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @param chan Sensor channel
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_offset(const struct device *dev, const struct sensor_value *val,
			      enum sensor_channel chan)
{
	struct adxl355_data *data = dev->data;
	int ret;
	uint8_t buf[2];
	int64_t offset_val;
	int16_t reg_val;

	offset_val = sensor_value_to_micro(val);
	ret = is_valid_offset(offset_val, data->range);
	if (ret) {
		LOG_ERR("Invalid offset %lld micro m/s^2", offset_val);
		return ret;
	}
	switch (data->range) {
	case ADXL355_RANGE_2G:
		offset_val = (offset_val * ADXL355_SENSITIVITY_2G) / SENSOR_G;
		break;
	case ADXL355_RANGE_4G:
		offset_val = (offset_val * ADXL355_SENSITIVITY_4G) / SENSOR_G;
		break;
	case ADXL355_RANGE_8G:
		offset_val = (offset_val * ADXL355_SENSITIVITY_8G) / SENSOR_G;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	reg_val = (int16_t)(offset_val >> 4);
	buf[0] = (reg_val >> 8) & 0xFF;
	buf[1] = (uint8_t)(reg_val & 0xFF);

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		data->extra_attr.offset_x = offset_val & 0xFFFFFFFFFFFFFFF0u;
		return adxl355_reg_write(dev, ADXL355_OFFSET_X_H, buf, 2);
	case SENSOR_CHAN_ACCEL_Y:
		data->extra_attr.offset_y = offset_val & 0xFFFFFFFFFFFFFFF0u;
		return adxl355_reg_write(dev, ADXL355_OFFSET_Y_H, buf, 2);
	case SENSOR_CHAN_ACCEL_Z:
		data->extra_attr.offset_z = offset_val & 0xFFFFFFFFFFFFFFF0u;
		return adxl355_reg_write(dev, ADXL355_OFFSET_Z_H, buf, 2);
	default:
		LOG_ERR("Invalid channel for offset");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Get Axis Offset
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @param chan Sensor channel
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_offset(const struct device *dev, struct sensor_value *val,
			      enum sensor_channel chan)
{
	struct adxl355_data *data = dev->data;
	int64_t offset_val;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		offset_val = data->extra_attr.offset_x;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		offset_val = data->extra_attr.offset_y;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		offset_val = data->extra_attr.offset_z;
		break;
	default:
		LOG_ERR("Invalid channel for offset");
		return -EINVAL;
	}
	switch (data->range) {
	case ADXL355_RANGE_2G:
		offset_val = (offset_val * SENSOR_G) / ADXL355_SENSITIVITY_2G;
		break;
	case ADXL355_RANGE_4G:
		offset_val = (offset_val * SENSOR_G) / ADXL355_SENSITIVITY_4G;
		break;
	case ADXL355_RANGE_8G:
		offset_val = (offset_val * SENSOR_G) / ADXL355_SENSITIVITY_8G;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	sensor_value_from_micro(val, offset_val);
	return 0;
}

/**
 * @brief Read acceleration sample data
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_read_sample(const struct device *dev)
{
	int ret;
	uint8_t buf[9];
	struct adxl355_data *data = dev->data;
	struct adxl355_sample *sample = &data->samples;

	ret = adxl355_reg_read(dev, ADXL355_XDATA3, buf, 9);
	if (ret != 0) {
		LOG_ERR("Failed to read sample data");
		return ret;
	}

	sample->x = combine_bytes_to_int20(buf[0], buf[1], buf[2]);
	sample->y = combine_bytes_to_int20(buf[3], buf[4], buf[5]);
	sample->z = combine_bytes_to_int20(buf[6], buf[7], buf[8]);
	sample->range = data->range;
	return 0;
}

/**
 * @brief Convert raw acceleration sample to sensor_value
 *
 * @param val Sensor value pointer
 * @param sample Raw sample data
 * @param range Measurement range
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_accel_convert(struct sensor_value *val, int32_t sample, enum adxl355_range range)
{
	int32_t sensitivity;

	switch (range) {
	case ADXL355_RANGE_2G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_2G;
		break;
	case ADXL355_RANGE_4G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_4G;
		break;
	case ADXL355_RANGE_8G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_8G;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}

	val->val1 = sample * sensitivity / 1000000;
	val->val2 = sample * sensitivity % 1000000;

	return 0;
}

/**
 * @brief Validate activity threshold based on measurement range
 *
 * @param accel_val Activity threshold in micro m/s^2
 * @param range Measurement range
 * @return int 0 if valid, negative error code otherwise
 */
static int is_valid_act_threshold(int64_t accel_val, enum adxl355_range range)
{
	switch (range) {
	case ADXL355_RANGE_2G:
		if (accel_val < 0 || accel_val > 19620000) {
			LOG_ERR("Invalid activity threshold %lld micro m/s^2 for ±2g range",
				accel_val);
			return -EINVAL;
		}
		break;
	case ADXL355_RANGE_4G:
		if (accel_val < 0 || accel_val > 39240000) {
			LOG_ERR("Invalid activity threshold %lld micro m/s^2 for ±4g range",
				accel_val);
			return -EINVAL;
		}
		break;
	case ADXL355_RANGE_8G:
		if (accel_val < 0 || accel_val > 78480000) {
			LOG_ERR("Invalid activity threshold %lld micro m/s^2 for ±8g range",
				accel_val);
			return -EINVAL;
		}
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	return 0;
}

/**
 * @brief Set Activity Threshold
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_act_threshold(const struct device *dev, const struct sensor_value *val)
{
	int ret;
	struct adxl355_data *data = dev->data;
	int64_t accel_val = sensor_value_to_micro(val);
	uint8_t buf[2];

	ret = is_valid_act_threshold(accel_val, data->range);
	if (ret) {
		LOG_ERR("Invalid activity threshold %lld micro m/s^2", accel_val);
		return ret;
	}
	switch (data->range) {
	case ADXL355_RANGE_2G:
		accel_val = (accel_val * ADXL355_SENSITIVITY_2G) / SENSOR_G;
		break;
	case ADXL355_RANGE_4G:
		accel_val = (accel_val * ADXL355_SENSITIVITY_4G) / SENSOR_G;
		break;
	case ADXL355_RANGE_8G:
		accel_val = (accel_val * ADXL355_SENSITIVITY_8G) / SENSOR_G;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	data->extra_attr.act_threshold = accel_val & 0x7FFF8; /* Mask out Bits 3-18 */
	buf[0] = (accel_val >> 11) & 0xFF;                    /* Place Bits 11-18 in MSB */
	buf[1] = (accel_val >> 3) & 0xFF;                     /* Place Bits 3-10 in LSB */
	ret = adxl355_reg_write(dev, ADXL355_ACT_THRESH_H, buf, 2);
	if (ret != 0) {
		LOG_ERR("Failed to set activity threshold");
		return ret;
	}
	return 0;
}

/**
 * @brief Get Activity Threshold
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_act_threshold(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	int64_t act_threshold_buf;

	switch (data->range) {
	case ADXL355_RANGE_2G:
		act_threshold_buf =
			(data->extra_attr.act_threshold * SENSOR_G) / ADXL355_SENSITIVITY_2G;
		break;
	case ADXL355_RANGE_4G:
		act_threshold_buf =
			(data->extra_attr.act_threshold * SENSOR_G) / ADXL355_SENSITIVITY_4G;
		break;
	case ADXL355_RANGE_8G:
		act_threshold_buf =
			(data->extra_attr.act_threshold * SENSOR_G) / ADXL355_SENSITIVITY_8G;
		break;
	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}
	sensor_value_from_micro(val, act_threshold_buf);
	return 0;
}

/**
 * @brief Set Activity Count
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_set_act_count(const struct device *dev, const struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;
	uint8_t act_count = val->val1;

	if (val->val1 < 1 || val->val1 > 255) {
		LOG_ERR("Invalid activity count %d", val->val1);
		return -EINVAL;
	}
	data->extra_attr.act_count = act_count;
	return adxl355_reg_write(dev, ADXL355_ACT_COUNT, &act_count, 1);
}

/**
 * @brief Get Activity Count
 *
 * @param dev Device pointer
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_get_act_count(const struct device *dev, struct sensor_value *val)
{
	struct adxl355_data *data = dev->data;

	val->val1 = data->extra_attr.act_count;
	return 0;
}

/**
 * @brief Set Operating Mode
 *
 * @param dev Device pointer
 * @param measurement Operating mode
 * @return int 0 on success, negative error code otherwise
 */
int adxl355_set_op_mode(const struct device *dev, enum adxl355_op_mode measurement)
{
	struct adxl355_data *data = dev->data;

	data->op_mode = measurement;
	data->extra_attr.pwr_reg &= ~ADXL355_POWER_CTL_STANDBY_MSK;
	data->extra_attr.pwr_reg |= FIELD_PREP(ADXL355_POWER_CTL_STANDBY_MSK, measurement);
	return adxl355_reg_update(dev, ADXL355_POWER_CTL, ADXL355_POWER_CTL_STANDBY_MSK,
				  measurement);
}

/**
 * @brief Get data from specified channel
 *
 * @param dev Device pointer
 * @param chan Sensor channel
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	int ret;
	struct adxl355_data *data = dev->data;
	struct adxl355_sample *sample = &data->samples;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		return adxl355_accel_convert(val, sample->x, data->range);
	case SENSOR_CHAN_ACCEL_Y:
		return adxl355_accel_convert(val, sample->y, data->range);
	case SENSOR_CHAN_ACCEL_Z:
		return adxl355_accel_convert(val, sample->z, data->range);
	case SENSOR_CHAN_ACCEL_XYZ:
		ret = adxl355_accel_convert(val, sample->x, data->range);
		if (ret != 0) {
			LOG_ERR("Failed to convert X axis");
			return ret;
		}
		val++;
		ret = adxl355_accel_convert(val, sample->y, data->range);
		if (ret != 0) {
			LOG_ERR("Failed to convert Y axis");
			return ret;
		}
		val++;
		ret = adxl355_accel_convert(val, sample->z, data->range);
		if (ret != 0) {
			LOG_ERR("Failed to convert Z axis");
			return ret;
		}
		return 0;
	default:
		LOG_ERR("Channel %u not supported!", chan);
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Fetch sample data from the sensor
 *
 * @param dev Device pointer
 * @param chan Sensor channel
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	return adxl355_read_sample(dev);
}

/**
 * @brief Get attribute value
 *
 * @param dev Device pointer
 * @param chan Sensor channel
 * @param attr Sensor attribute
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return adxl355_get_odr(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		return adxl355_get_range(dev, val);
	case SENSOR_ATTR_CONFIGURATION:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			return adxl355_get_drdy_mode(dev, val);
		case SENSOR_CHAN_DIE_TEMP:
		case SENSOR_CHAN_AMBIENT_TEMP:
			return adxl355_get_temp_mode(dev, val);
		default:
			LOG_ERR("Channel %u not supported for configuration attribute", chan);
			return -ENOTSUP;
		}
	case SENSOR_ATTR_OFFSET:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
			return adxl355_get_offset(dev, val, chan);
		default:
			LOG_ERR("Channel %u not supported for offset attribute", chan);
			return -ENOTSUP;
		}
	case SENSOR_ATTR_ADXL355_FIFO_WATERMARK:
		return adxl355_get_fifo_watermark(dev, val);
	case SENSOR_ATTR_ADXL355_HPF_CORNER:
		return adxl355_get_hpf_corner(dev, val);
	case SENSOR_ATTR_ADXL355_EXT_CLK:
		return adxl355_get_ext_clk(dev, val);
	case SENSOR_ATTR_ADXL355_EXT_SYNC:
		return adxl355_get_ext_sync(dev, val);
	case SENSOR_ATTR_ADXL355_I2C_HS:
		return adxl355_get_i2c_hs(dev, val);
	case SENSOR_ATTR_ADXL355_INT_POL:
		return adxl355_get_int_pol(dev, val);
	case SENSOR_ATTR_ADXL355_ACTIVITY_THRESHOLD:
		return adxl355_get_act_threshold(dev, val);
	case SENSOR_ATTR_ADXL355_ACTIVITY_COUNT:
		return adxl355_get_act_count(dev, val);
	default:
		LOG_ERR("Attribute not supported");
		return -ENOTSUP;
	}
}

/**
 * @brief Set attribute value
 *
 * @param dev Device pointer
 * @param chan Sensor channel
 * @param attr Sensor attribute
 * @param val Sensor value pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	/* Changes to the configurations setting must be made in Standby Mode */
	int ret = 0;

	ret = adxl355_set_op_mode(dev, ADXL355_STANDBY);
	if (ret != 0) {
		LOG_ERR("Failed to set standby mode before attribute set");
		return ret;
	}
	switch ((int)attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		ret = adxl355_set_odr(dev, val);
		break;
	case SENSOR_ATTR_FULL_SCALE:
		ret = adxl355_set_range(dev, val);
		break;
	case SENSOR_ATTR_CONFIGURATION:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
		case SENSOR_CHAN_ACCEL_XYZ:
			ret = adxl355_set_drdy_mode(dev, val);
			break;
		case SENSOR_CHAN_DIE_TEMP:
		case SENSOR_CHAN_AMBIENT_TEMP:
			ret = adxl355_set_temp_mode(dev, val);
			break;
		default:
			LOG_ERR("Channel %u not supported for configuration attribute", chan);
			return -ENOTSUP;
		}
		break;
	case SENSOR_ATTR_OFFSET:
		switch (chan) {
		case SENSOR_CHAN_ACCEL_X:
		case SENSOR_CHAN_ACCEL_Y:
		case SENSOR_CHAN_ACCEL_Z:
			ret = adxl355_set_offset(dev, val, chan);
			break;
		default:
			LOG_ERR("Channel %u not supported for offset attribute", chan);
			return -ENOTSUP;
		}
		break;
	case SENSOR_ATTR_ADXL355_FIFO_WATERMARK:
		ret = adxl355_set_fifo_watermark(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_HPF_CORNER:
		ret = adxl355_set_hpf_corner(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_EXT_CLK:
		ret = adxl355_set_ext_clk(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_EXT_SYNC:
		ret = adxl355_set_ext_sync(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_I2C_HS:
		ret = adxl355_set_i2c_hs(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_INT_POL:
		ret = adxl355_set_int_pol(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_ACTIVITY_THRESHOLD:
		ret = adxl355_set_act_threshold(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_ACTIVITY_COUNT:
		ret = adxl355_set_act_count(dev, val);
		break;
	case SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_X:
		ret = adxl355_activity_enable(dev, ADXL355_ACT_EN_X_MSK, val->val1 ? true : false);
		break;
	case SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_Y:
		ret = adxl355_activity_enable(dev, ADXL355_ACT_EN_Y_MSK, val->val1 ? true : false);
		break;
	case SENSOR_ATTR_ADXL355_ACTIVITY_ENABLE_Z:
		ret = adxl355_activity_enable(dev, ADXL355_ACT_EN_Z_MSK, val->val1 ? true : false);
		break;
	default:
		LOG_ERR("Attribute not supported");
		return -ENOTSUP;
	}
	if (ret != 0) {
		LOG_ERR("Failed to set attribute");
		return ret;
	}
	ret = adxl355_set_op_mode(dev, ADXL355_MEASURE);
	if (ret != 0) {
		LOG_ERR("Failed to set measurement mode after attribute set");
		return ret;
	}
	return 0;
}

/**
 * @brief Validate device ID
 *
 * @param dev Device pointer
 * @return true if valid,
 * @return false otherwise
 */
static bool adxl355_validate_device_id(const struct device *dev)
{
	int ret;
	uint8_t device_id;
	uint8_t part_id;
	uint8_t mems_id;

	/* Read and verify device ID */
	ret = adxl355_reg_read(dev, ADXL355_DEVID_AD, &device_id, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read device ID");
		return false;
	}
	if (device_id != ADXL355_DEVID_AD_VAL) {
		LOG_ERR("Invalid device ID: 0x%X", device_id);
		return false;
	}

	/* Read and verify part ID */
	ret = adxl355_reg_read(dev, ADXL355_PARTID, &part_id, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read part ID");
		return false;
	}
	if (part_id != ADXL355_PARTID_VAL) {
		LOG_ERR("Invalid part ID: 0x%X", part_id);
		return false;
	}

	/* Read and verify MEMS ID */
	ret = adxl355_reg_read(dev, ADXL355_DEVID_MST, &mems_id, 1);
	if (ret != 0) {
		LOG_ERR("Failed to read MEMS ID");
		return false;
	}
	if (mems_id != ADXL355_DEVID_MST_VAL) {
		LOG_ERR("Invalid MEMS ID: 0x%X", mems_id);
		return false;
	}
	return true;
}

/**
 * @brief Reset device
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_reset_device(const struct device *dev)
{
	int ret;
	uint8_t reset_cmd = ADXL355_RESET_CMD;

	ret = adxl355_reg_write(dev, ADXL355_RESET, &reset_cmd, 1);
	if (ret != 0) {
		LOG_ERR("Failed to write reset command");
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, adxl355_api_funcs) = {
	.attr_set = adxl355_attr_set,
	.sample_fetch = adxl355_sample_fetch,
	.channel_get = adxl355_channel_get,
	.attr_get = adxl355_attr_get,
#ifdef CONFIG_ADXL355_TRIGGER
	.trigger_set = adxl355_trigger_set,
#endif /* CONFIG_ADXL355_TRIGGER */
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = adxl355_submit,
	.get_decoder = adxl355_get_decoder,
#endif /* CONFIG_SENSOR_ASYNC_API */
};

/**
 * @brief Set Output Data Rate during probe
 *
 * @param dev Device pointer
 * @param odr Output Data Rate
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_odr(const struct device *dev, enum adxl355_odr odr)
{
	return adxl355_reg_update(dev, ADXL355_FILTER, ADXL355_FILTER_ODR_MSK, odr);
}

/**
 * @brief Set Measurement Range during probe
 *
 * @param dev Device pointer
 * @param range Measurement Range
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_range(const struct device *dev, enum adxl355_range range)
{
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_RANGE_MSK, range);
}

/**
 * @brief Set FIFO Watermark during probe
 *
 * @param dev Device pointer
 * @param fifo_watermark FIFO Watermark level
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_fifo_watermark(const struct device *dev, uint8_t fifo_watermark)
{
	return adxl355_reg_write(dev, ADXL355_FIFO_SAMPLES, &fifo_watermark, 1);
}

/**
 * @brief Set High-Pass Filter Corner during probe
 *
 * @param dev Device pointer
 * @param hpf_corner High-Pass Filter Corner setting
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_hpf_corner(const struct device *dev, uint8_t hpf_corner)
{
	return adxl355_reg_update(dev, ADXL355_FILTER, ADXL355_FILTER_HPF_MASK, hpf_corner);
}

/**
 * @brief Set External Clock during probe
 *
 * @param dev Device pointer
 * @param ext_clk External Clock setting
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_ext_clk(const struct device *dev, uint8_t ext_clk)
{
	return adxl355_reg_update(dev, ADXL355_SYNC, ADXL355_SYNC_EXT_CLK_MSK, ext_clk);
}

/**
 * @brief Set External Sync during probe
 *
 * @param dev Device pointer
 * @param ext_sync External Sync setting
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_ext_sync(const struct device *dev, uint8_t ext_sync)
{
	return adxl355_reg_update(dev, ADXL355_SYNC, ADXL355_SYNC_EXT_SYNC_MSK, ext_sync);
}

/**
 * @brief Set I2C High-Speed Mode during probe
 *
 * @param dev Device pointer
 * @param i2c_hs I2C High-Speed setting
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_i2c_hs(const struct device *dev, uint8_t i2c_hs)
{
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_I2C_HS_MSK, i2c_hs);
}

/**
 * @brief Set Interrupt Polarity during probe
 *
 * @param dev Device pointer
 * @param int_pol Interrupt Polarity setting
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe_set_int_pol(const struct device *dev, uint8_t int_pol)
{
	return adxl355_reg_update(dev, ADXL355_RANGE, ADXL355_INT_POL_MSK, int_pol);
}

static int adxl355_self_test(const struct device *dev)
{
	struct adxl355_data *data = dev->data;
	int ret;
	int32_t x_baseline, y_baseline, z_baseline;
	int32_t x_self_test, y_self_test, z_self_test;
	int32_t x_diff, y_diff, z_diff;
	int32_t sensitivity;

	/* Enable self-test */
	ret = adxl355_reg_update(dev, ADXL355_SELF_TEST, ADXL355_SELF_TEST_ST1_MSK, 1);
	if (ret != 0) {
		LOG_ERR("Failed to enable self-test");
		return ret;
	}

	/* Wait for self-test to stabilize */
	k_sleep(K_MSEC(100));

	/* Read baseline sample */
	ret = adxl355_read_sample(dev);
	if (ret != 0) {
		LOG_ERR("Failed to read baseline sample");
		return ret;
	}

	/* Parse Baseline Readings */
	x_baseline = data->samples.x;
	y_baseline = data->samples.y;
	z_baseline = data->samples.z;

	/* Induce Electrostatic Force via ST2 */
	ret = adxl355_reg_update(dev, ADXL355_SELF_TEST, ADXL355_SELF_TEST_ST2_MSK, 1);
	if (ret != 0) {
		LOG_ERR("Failed to induce self-test force");
		return ret;
	}

	/* Wait for self-test to stabilize */
	k_sleep(K_MSEC(100));

	/* Read self-test sample */
	ret = adxl355_read_sample(dev);
	if (ret != 0) {
		LOG_ERR("Failed to read self-test sample");
		return ret;
	}
	/* Parse Self-Test Readings */
	x_self_test = data->samples.x;
	y_self_test = data->samples.y;
	z_self_test = data->samples.z;

	/* Disable self-test */
	ret = adxl355_reg_update(dev, ADXL355_SELF_TEST, ADXL355_SELF_TEST_ST2_MSK, 0);
	if (ret != 0) {
		LOG_ERR("Failed to disable self-test");
		return ret;
	}
	ret = adxl355_reg_update(dev, ADXL355_SELF_TEST, ADXL355_SELF_TEST_ST1_MSK, 0);
	if (ret != 0) {
		LOG_ERR("Failed to disable self-test");
		return ret;
	}
	/* Convert Samples */
	switch (data->range) {
	case ADXL355_RANGE_2G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_2G;
		break;

	case ADXL355_RANGE_4G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_4G;
		break;

	case ADXL355_RANGE_8G:
		sensitivity = SENSOR_G / ADXL355_SENSITIVITY_8G;
		break;

	default:
		LOG_ERR("Invalid range setting");
		return -EINVAL;
	}

	/* Calculate Differences */
	x_diff = abs((x_self_test - x_baseline) * sensitivity);
	y_diff = abs((y_self_test - y_baseline) * sensitivity);
	z_diff = abs((z_self_test - z_baseline) * sensitivity);

	/* Validate Differences against Self-Test Limits */
	if (x_diff < ADXL355_SELF_TEST_MIN_X || x_diff > ADXL355_SELF_TEST_MAX_X) {
		LOG_ERR("Self-test failed on X axis: %d g", x_diff);
		return -EINVAL;
	}

	if (y_diff < ADXL355_SELF_TEST_MIN_Y || y_diff > ADXL355_SELF_TEST_MAX_Y) {
		LOG_ERR("Self-test failed on Y axis: %d micro m/s^2", y_diff);
		return -EINVAL;
	}

	if (z_diff < ADXL355_SELF_TEST_MIN_Z || z_diff > ADXL355_SELF_TEST_MAX_Z) {
		LOG_ERR("Self-test failed on Z axis: %d micro m/s^2", z_diff);
		return -EINVAL;
	}
	LOG_INF("Self-test passed: X=%d, Y=%d, Z=%d micro m/s^2", x_diff, y_diff, z_diff);

	return 0;
}

/**
 * @brief Probe device and initialize settings
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_probe(const struct device *dev)
{
	int ret;
	const struct adxl355_dev_config *cfg = dev->config;
	struct adxl355_data *data = dev->data;

	ret = adxl355_probe_set_odr(dev, cfg->odr);
	if (ret != 0) {
		LOG_ERR("Failed to set ODR during probe");
		return ret;
	}
	data->odr = cfg->odr;

	ret = adxl355_probe_set_range(dev, cfg->range);
	if (ret != 0) {
		LOG_ERR("Failed to set range during probe");
		return ret;
	}
	data->range = cfg->range;

	ret = adxl355_probe_set_fifo_watermark(dev, cfg->fifo_watermark);
	if (ret != 0) {
		LOG_ERR("Failed to set FIFO watermark during probe");
		return ret;
	}
	data->fifo_watermark = cfg->fifo_watermark;

	ret = adxl355_probe_set_hpf_corner(dev, cfg->hpf_corner);
	if (ret != 0) {
		LOG_ERR("Failed to set HPF corner during probe");
		return ret;
	}
	data->extra_attr.hpf_corner = cfg->hpf_corner;

	ret = adxl355_probe_set_ext_clk(dev, cfg->ext_clk);
	if (ret != 0) {
		LOG_ERR("Failed to set external clock during probe");
		return ret;
	}
	data->extra_attr.ext_clk = cfg->ext_clk;

	ret = adxl355_probe_set_ext_sync(dev, cfg->ext_sync);
	if (ret != 0) {
		LOG_ERR("Failed to set external sync during probe");
		return ret;
	}
	data->extra_attr.ext_sync = cfg->ext_sync;

	ret = adxl355_probe_set_i2c_hs(dev, cfg->i2c_hs);
	if (ret != 0) {
		LOG_ERR("Failed to set I2C high speed during probe");
		return ret;
	}
	data->extra_attr.i2c_hs = cfg->i2c_hs;

	ret = adxl355_probe_set_int_pol(dev, cfg->int_pol);
	if (ret != 0) {
		LOG_ERR("Failed to set INT_POL during probe");
		return ret;
	}
	data->extra_attr.int_pol = cfg->int_pol;

	return 0;
}

/**
 * @brief Initialize ADXL355 sensor
 *
 * @param dev Device pointer
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_init(const struct device *dev)
{
	const struct adxl355_dev_config *cfg = dev->config;
	int ret;

	ret = adxl355_bus_is_ready(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize sensor bus\n");
		return ret;
	}

	if (!adxl355_validate_device_id(dev)) {
		LOG_ERR("Failed to read device ID\n");
		return -ENODEV;
	}

	ret = adxl355_reset_device(dev);
	if (ret != 0) {
		LOG_ERR("Failed to reset device\n");
		return ret;
	}

	ret = adxl355_probe(dev);
	if (ret != 0) {
		LOG_ERR("Failed to probe device\n");
		return ret;
	}

#ifdef CONFIG_ADXL355_TRIGGER
	ret = adxl355_init_interrupt(dev);
	if (ret != 0) {
		LOG_ERR("Failed to initialize interrupts\n");
		return ret;
	}
#endif /* CONFIG_ADXL355_TRIGGER */
	ret = adxl355_set_op_mode(dev, ADXL355_MEASURE);
	if (ret != 0) {
		LOG_ERR("Failed to set measurement mode\n");
		return ret;
	}
	/* Perform self-test if enabled */
	if (cfg->self_test) {
		ret = adxl355_self_test(dev);
		if (ret != 0) {
			LOG_ERR("Self-test failed during initialization\n");
			return ret;
		}
	}
	return 0;
}

#ifdef CONFIG_PM_DEVICE
/**
 * @brief Power Management action for ADXL355
 *
 * @param dev Device pointer
 * @param action Power management action
 * @return int 0 on success, negative error code otherwise
 */
static int adxl355_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	LOG_INF("PM action %d for ADXL355", action);
	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = adxl355_set_op_mode(dev, ADXL355_MEASURE);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = adxl355_set_op_mode(dev, ADXL355_STANDBY);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}
#endif

#ifdef CONFIG_PM_DEVICE
#define ADXL355_PM_DEFINE(inst) PM_DEVICE_DT_INST_DEFINE(inst, adxl355_pm_action);
#else
#define ADXL355_PM_DEFINE(inst)
#endif

#define ADXL355_DEVICE_INIT(inst)                                                                  \
	ADXL355_PM_DEFINE(inst)                                                                    \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, adxl355_init, PM_DEVICE_DT_INST_GET(inst),              \
				     &adxl355_data_##inst, &adxl355_config_##inst, POST_KERNEL,    \
				     CONFIG_SENSOR_INIT_PRIORITY, &adxl355_api_funcs);

#ifdef CONFIG_ADXL355_TRIGGER
#define ADXL355_CFG_IRQ(inst)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
		(	\
			.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, int1_gpios),	\
			.route_to_int2 = false,	\
		),	\
		(	\
			.interrupt_gpio = GPIO_DT_SPEC_INST_GET(inst, int2_gpios),	\
			.route_to_int2 = true,	\
		))
#else
#define ADXL355_CFG_IRQ(inst)
#endif /* CONFIG_ADXL355_TRIGGER */

#define ADXL355_SPI_CFG SPI_WORD_SET(8) | SPI_TRANSFER_MSB

#define ADXL355_RTIO_SPI_DEFINE(inst)                                                              \
	COND_CODE_1(CONFIG_SPI_RTIO,						\
				(SPI_DT_IODEV_DEFINE(adxl355_iodev_##inst,\
				DT_DRV_INST(inst),\
				ADXL355_SPI_CFG);),\
				())

#define ADXL355_RTIO_I2C_DEFINE(inst)                                                              \
	COND_CODE_1(CONFIG_I2C_RTIO,							\
				(I2C_DT_IODEV_DEFINE(adxl355_iodev_##inst,	\
				DT_DRV_INST(inst));),						\
				())

/** RTIO SQE/CQE pool size depends on the fifo-watermark because we
 * can't just burst-read all the fifo data at once. Datasheet specifies
 * we need to get one frame at a time (through the Data registers),
 * therefore, we set all the sequence at once to properly pull each
 * frame, and then end up calling the completion event so the
 * application receives it).
 */
#define ADXL355_RTIO_DEFINE(inst)                                                                  \
	/* Conditionally include SPI and/or I2C parts based on their presence */                   \
	COND_CODE_1(DT_INST_ON_BUS(inst, spi),					\
				(ADXL355_RTIO_SPI_DEFINE(inst)),			\
				())                                               \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c),					\
				(ADXL355_RTIO_I2C_DEFINE(inst)),			\
				())                                               \
	RTIO_DEFINE(adxl355_rtio_ctx_##inst, 2 * DT_INST_PROP(inst, fifo_watermark) + 2,           \
		    2 * DT_INST_PROP(inst, fifo_watermark) + 2);

#define ADXL355_CONFIG(inst)                                                                       \
	.odr = DT_INST_PROP(inst, odr), .range = DT_INST_PROP(inst, range),                        \
	.fifo_watermark = DT_INST_PROP(inst, fifo_watermark),                                      \
	.hpf_corner = DT_INST_PROP(inst, hpf_corner), .ext_clk = DT_INST_PROP(inst, ext_clk),      \
	.ext_sync = DT_INST_PROP(inst, ext_sync), .i2c_hs = DT_INST_PROP(inst, i2c_hs),            \
	.int_pol = DT_INST_PROP(inst, int_pol), .self_test = DT_INST_PROP(inst, self_test_enable),

#define ADXL355_CONFIG_SPI(inst)                                                                   \
	{.bus = {.spi = SPI_DT_SPEC_INST_GET(inst, ADXL355_SPI_CFG)},                              \
	 .bus_is_ready = adxl355_bus_is_ready_spi,                                                 \
	 .reg_access = adxl355_reg_access_spi,                                                     \
	 ADXL355_CONFIG(inst) COND_CODE_1(UTIL_OR(	\
					DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
					DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
					(ADXL355_CFG_IRQ(inst)), ()) }

#define ADXL355_CONFIG_I2C(inst)                                                                   \
	{.bus = {.i2c = I2C_DT_SPEC_INST_GET(inst)},                                               \
	 .bus_is_ready = adxl355_bus_is_ready_i2c,                                                 \
	 .reg_access = adxl355_reg_access_i2c,                                                     \
	 ADXL355_CONFIG(inst) COND_CODE_1(UTIL_OR(				\
						DT_INST_NODE_HAS_PROP(inst, int1_gpios),	\
						DT_INST_NODE_HAS_PROP(inst, int2_gpios)),	\
						(ADXL355_CFG_IRQ(inst)), ()) }

#define ADXL355_DEFINE(inst)                                                                       \
	BUILD_ASSERT(DT_INST_PROP(inst, fifo_watermark) <= 96, "FIFO watermark must be <= 96");    \
	BUILD_ASSERT(DT_INST_PROP(inst, fifo_watermark) % 3 == 0,                                  \
		     "FIFO watermark must be multiple of 3");                                      \
	IF_ENABLED(CONFIG_ADXL355_STREAM,						\
			(ADXL355_RTIO_DEFINE(inst)));                             \
	static struct adxl355_data adxl355_data_##inst = {                                         \
		IF_ENABLED(CONFIG_ADXL355_STREAM,					\
				(.rtio_ctx = &adxl355_rtio_ctx_##inst,		\
				.iodev = &adxl355_iodev_##inst,)) };               \
	static const struct adxl355_dev_config adxl355_config_##inst = COND_CODE_1(\
					DT_INST_ON_BUS(inst, spi),				\
					(ADXL355_CONFIG_SPI(inst)),				\
					(ADXL355_CONFIG_I2C(inst)));                  \
	ADXL355_DEVICE_INIT(inst)

DT_INST_FOREACH_STATUS_OKAY(ADXL355_DEFINE)
