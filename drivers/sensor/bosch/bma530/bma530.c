/* Bosch bma530 3-axis accelerometer driver
 *
 * Copyright (c) 2024 Arrow Electronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma530

#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include "bma530.h"

LOG_MODULE_REGISTER(bma530, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Helper function for converting offset values into register values.
 *	  Converts acceleration from [m/s^2] to [ug] (micro g) and calculates the value of
 *	  the offset register
 * @param val pointer to offset value in Zephyr sensor format (in [m/s^2])
 * @param reg_val pointer to the offset register value
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_offset_to_reg_val(const struct sensor_value *val, uint16_t *reg_val)
{
	const int32_t ug = sensor_ms2_to_ug(val);

	if ((ug < BMA530_OFFSET_MICROG_MIN) || (ug > BMA530_OFFSET_MICROG_MAX)) {
		return -ERANGE;
	}

	*reg_val = sys_cpu_to_le16(ug / BMA530_OFFSET_MICROG_PER_BIT);
	return 0;
}

/**
 * @brief Helper function for converting register values into offset values.
 * @param val pointer to offset value in Zephyr sensor format (in [m/s^2])
 * @param reg_val pointer to the offset register value
 */
static void bma530_reg_val_to_offset(struct sensor_value *val, const uint16_t *reg_val)
{
	const uint16_t micro_g = sys_le16_to_cpu(*reg_val) * BMA530_OFFSET_MICROG_PER_BIT;

	sensor_ug_to_ms2(micro_g, val);
}

/**
 * @brief Set the X, Y, or Z (or all at once) axis offsets.
 *	  The allowed value of offset is -0,25 to 0,25 [g] ~ -2,45 to 2,45 [m/s^2].
 *	  The resolution of offset is 0,98 [mg] ~ 0,0096 [m/s^2].
 * @param dev pointer to device struct
 * @param chan axis to change the offset
 * @param val pointer to offset value in Zephyr sensor format (in [m/s^2])
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_set_offset(const struct device *dev,
				  const enum sensor_channel chan,
				  const struct sensor_value *val)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t reg_addr;
	uint8_t size;
	uint16_t reg_val[3];
	int status;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		reg_addr = BMA530_REG_ACC_OFFSET_0 + (chan - SENSOR_CHAN_ACCEL_X)*2;
		size = sizeof(reg_val[0]);
		status = bma530_offset_to_reg_val(val, &reg_val[0]);
		if (status) {
			return status;
		}
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		/* Expect val to point to an array of three sensor_values */
		reg_addr = BMA530_REG_ACC_OFFSET_0;
		size = sizeof(reg_val);
		for (int i = 0; i < 3; i++) {
			status = bma530_offset_to_reg_val(&val[i], &reg_val[i]);
			if (status) {
				return status;
			}
		}
		break;
	default:
		return -ENOTSUP;
	}

	return bma530->hw_ops->write_data(dev, reg_addr, (uint8_t *)reg_val, size);
}

/**
 * @brief Get current X, Y, or Z (or all at once) axis offsets.
 * @param dev pointer to device struct
 * @param chan axis to get the offset
 * @param val pointer to offset value in Zephyr sensor format (in [m/s^2])
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_get_offset(const struct device *dev,
				  const enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t reg_addr;
	uint8_t reg_val[BMA530_PACKET_SIZE_ACC];
	int status;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
		reg_addr = BMA530_REG_ACC_OFFSET_0 + (chan - SENSOR_CHAN_ACCEL_X)*2;
		status = bma530->hw_ops->read_data(dev, reg_addr, &reg_val[0], sizeof(reg_val[0]));
		if (status) {
			return status;
		}
		bma530_reg_val_to_offset(val, (uint16_t *)&reg_val[0]);
		return 0;
	case SENSOR_CHAN_ACCEL_XYZ:
		reg_addr = BMA530_REG_ACC_OFFSET_0;
		status = bma530->hw_ops->read_data(dev, reg_addr, &reg_val[0], sizeof(reg_val));
		if (status) {
			return status;
		}
		/* Expect val to point to an array of three sensor_values */
		for (int i = 0; i < 3; i++) {
			bma530_reg_val_to_offset(&val[i], (uint16_t *)&reg_val[2*i]);
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const uint64_t odr_to_reg_map[] = {
	1562500,	/* 1,5625 Hz => 0x0 */
	3125000,	/* 3,125 Hz => 0x1 */
	6250000,	/* 6,25 Hz => 0x2 */
	12500000,	/* 12,5 Hz => 0x3 */
	25000000,	/* 25 Hz => 0x4 */
	50000000,	/* 50 Hz => 0x5 */
	100000000,	/* 100 Hz => 0x6 */
	200000000,	/* 200 Hz => 0x7 */
	400000000,	/* 400 Hz => 0x8 */
	800000000,	/* 800 Hz => 0x9 */
	1600000000,	/* 1600 Hz => 0xA */
	3200000000,	/* 3200 Hz => 0xB */
	6400000000,	/* 6400 Hz => 0xC */
};

/**
 * @brief Convert an ODR rate in micro Hz to a register value
 * @param microhertz output data rate in [uHz]
 * @param reg_val pointer to the register value variable
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_odr_to_reg(const uint64_t microhertz, uint8_t *reg_val)
{
	if (microhertz == 0) {
		/* Illegal ODR value. */
		return -ERANGE;
	}

	for (uint8_t i = 0; i < ARRAY_SIZE(odr_to_reg_map); i++) {
		if (microhertz <= odr_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested ODR is too high */
	return -ERANGE;
}
/**
 * @brief Check the output data rate register value for current power mode
 * @param high_power_mode accelerometer's high or low power mode
 * @param reg_val register value
 *
 * @return error code in case of incorrect ODR set given power mode, 0 otherwise
 */
static int bma530_check_min_max_odr(const bool high_power_mode, const uint8_t reg_val)
{
	/* Maximum and minimum ODR depend of perfromance mode. */
	if (high_power_mode) {
		if ((reg_val < BMA530_ODR_MIN_HPM) ||
		   (reg_val > BMA530_ODR_MAX_HPM)) {
			return -ERANGE;
		}
	} else {
		if (reg_val > BMA530_ODR_MAX_LPM) {
			return -ERANGE;
		}
	}
	return 0;
}

/**
 * @brief Set the sensors output data rate using register value and update value in sensors data
 *	  structure
 * @param dev pointer to device struct
 * @param reg_val register value
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_odr_set(const struct device *dev, const uint8_t reg_val)
{
	struct bma530_data *bma530 = dev->data;
	int status;

	status = bma530_check_min_max_odr(bma530->high_power_mode, reg_val);
	if (status < 0) {
		return status;
	}

	status = bma530->hw_ops->update_reg(dev, BMA530_REG_ACCEL_CONF_1, BMA530_MASK_ACC_CONF_ODR,
					    reg_val);
	if (status < 0) {
		return status;
	}

	bma530->accel_odr = reg_val;
	return 0;
}

/**
 * @brief Set ODR rate in Hz
 * @param dev pointer to device struct
 * @param val pointer to ODR value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_set_odr(const struct device *dev, const struct sensor_value *val)
{
	const uint64_t odr_microhertz = sensor_value_to_micro(val);
	uint8_t reg_val;
	int status;

	status = bma530_odr_to_reg(odr_microhertz, &reg_val);
	if (status < 0) {
		return status;
	}

	return bma530_odr_set(dev, reg_val);
}

/**
 * @brief Get ODR rate in Hz from register value
 * @param dev pointer to device struct
 * @param val pointer to ODR value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_get_odr(const struct device *dev, struct sensor_value *val)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t reg_val;
	int status;
	uint64_t odr;

	status = bma530->hw_ops->read_reg(dev, BMA530_REG_ACCEL_CONF_1, &reg_val);
	if (status < 0) {
		return status;
	}

	reg_val = reg_val & BMA530_MASK_ACC_CONF_ODR;
	if (reg_val > sizeof(odr_to_reg_map)) {
		return -EINVAL;
	}

	bma530->accel_odr = reg_val;
	odr = odr_to_reg_map[reg_val];
	return sensor_value_from_micro(val, odr);
}

static const uint32_t fs_to_reg_map[] = {
	2000000,  /* +-2G  => 0x0 */
	4000000,  /* +-4G  => 0x1 */
	8000000,  /* +-8G  => 0x2 */
	16000000, /* +-16G => 0x3 */
};

/**
 * @brief Convert a full scale range in micro g to a register value.
 *	  A minimum range that is bigger than or equal to selected is chosen.
 * @param range_ug range (in micro g [ug])
 * @param reg_val pointer to the register value variable
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_fs_to_reg(int32_t range_ug, uint8_t *reg_val)
{
	if (range_ug == 0) {
		/* Illegal value. */
		return -ERANGE;
	}

	range_ug = abs(range_ug);

	for (uint8_t i = 0; i < ARRAY_SIZE(fs_to_reg_map); i++) {
		if (range_ug <= fs_to_reg_map[i]) {
			*reg_val = i;
			return 0;
		}
	}

	/* Requested range is too high. */
	return -ERANGE;
}

/**
 * @brief Set the sensors full-scale range using register value and update value in sensors data
 *	  structure
 * @param dev pointer to device struct
 * @param val register value
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_fs_set(const struct device *dev, const uint8_t reg_val)
{
	struct bma530_data *bma530 = dev->data;
	int status;

	status = bma530->hw_ops->update_reg(dev, BMA530_REG_ACCEL_CONF_2, BMA530_MASK_ACC_RANGE,
					    reg_val);
	if (status < 0) {
		return status;
	}

	bma530->accel_fs_range = fs_to_reg_map[reg_val];
	return 0;
}

/**
 * @brief Set the sensors full-scale range
 * @param dev pointer to device struct
 * @param val pointer to range value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_set_range(const struct device *dev, const struct sensor_value *val)
{
	uint8_t reg_val;
	int status;

	/* Convert [m/s^2] to micro-G's and find closest register setting. */
	status = bma530_fs_to_reg(sensor_ms2_to_ug(val), &reg_val);
	if (status < 0) {
		return status;
	}

	return bma530_fs_set(dev, reg_val);
}

/**
 * @brief Get the sensors full-scale range
 * @param dev pointer to device struct
 * @param val pointer to range value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_get_range(const struct device *dev, struct sensor_value *val)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t reg_val;
	int status;

	status = bma530->hw_ops->read_reg(dev, BMA530_REG_ACCEL_CONF_2, &reg_val);
	if (status < 0) {
		return status;
	}

	/* Apply register mask. */
	reg_val = reg_val & BMA530_MASK_ACC_RANGE;

	sensor_ug_to_ms2(fs_to_reg_map[reg_val], val);
	return 0;
}

/**
 * @brief Set the sensors output data rate using register value and update value in sensors
 *	  data structure
 * @param dev pointer to device struct
 * @param power_mode selected power mode
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_power_mode_set(const struct device *dev, const uint8_t power_mode)
{
	struct bma530_data *bma530 = dev->data;
	int status;

	status = bma530->hw_ops->update_reg(dev, BMA530_REG_ACCEL_CONF_1, BMA530_BIT_ACC_PWR_MODE,
					    (power_mode << BMA530_SHIFT_ACC_PWR_MODE));
	if (status < 0) {
		return status;
	}

	bma530->high_power_mode = ((power_mode == BMA530_POWER_MODE_LPM) ? false : true);
	return 0;
}

/**
 * @brief Set the sensors bandwidth parameter
 * @param dev pointer to device struct
 * @param val pointer to power mode in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_set_power_mode(const struct device *dev, const struct sensor_value *val)
{
	const uint8_t power_mode = ((val->val1 == 0) ? BMA530_POWER_MODE_LPM :
						       BMA530_POWER_MODE_HPM);

	return bma530_power_mode_set(dev, power_mode);
}

/**
 * @brief Get the sensors power mode
 * @param dev pointer to device struct
 * @param val pointer to power mode in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_get_power_mode(const struct device *dev, struct sensor_value *val)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t reg_val;
	int status;

	status = bma530->hw_ops->read_reg(dev, BMA530_REG_ACCEL_CONF_1, &reg_val);
	if (status < 0) {
		return status;
	}

	/* Require that `val2` is unused, and that `val1` is the power mode content. */
	val->val2 = 0;
	val->val1 = (reg_val & BMA530_BIT_ACC_PWR_MODE) >> BMA530_SHIFT_ACC_PWR_MODE;

	bma530->high_power_mode = (val->val1 == 0) ? 0 : 1;

	return 0;
}

/**
 * @brief Implement the sensor API attribute set method.
 * @param dev pointer to device struct
 * @param chan channel (axis) to affect the change
 * @param attr attribute to set
 * @param val pointer to value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_set(const struct device *dev, const enum sensor_channel chan,
			   const enum sensor_attribute attr, const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bma530_attr_set_odr(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		return bma530_attr_set_range(dev, val);
	case SENSOR_ATTR_OFFSET:
		return bma530_attr_set_offset(dev, chan, val);
	case SENSOR_ATTR_CONFIGURATION:
		return bma530_attr_set_power_mode(dev, val);
	default:
		return -ENOTSUP;
	}
}

/**
 * @brief Implement the sensor API attribute get method.
 * @param dev pointer to device struct
 * @param chan channel (axis) to read the value from
 * @param attr attribute to get
 * @param val pointer to value in Zephyr sensor format
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_attr_get(const struct device *dev, const enum sensor_channel chan,
			   const enum sensor_attribute attr, struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_SAMPLING_FREQUENCY:
		return bma530_attr_get_odr(dev, val);
	case SENSOR_ATTR_FULL_SCALE:
		return bma530_attr_get_range(dev, val);
	case SENSOR_ATTR_OFFSET:
		return bma530_attr_get_offset(dev, chan, val);
	case SENSOR_ATTR_CONFIGURATION:
		return bma530_attr_get_power_mode(dev, val);
	default:
		return -ENOTSUP;
	}
}

/*
 * Sample fetch and conversion
 */

/**
 * @brief Read acceleration (and optional die temperature) data from BMA530
 * @param dev pointer to device struct
 * @param chan channel to read from
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_sample_fetch(const struct device *dev, const enum sensor_channel chan)
{
	struct bma530_data *bma530 = dev->data;
	uint8_t read_data[BMA530_PACKET_SIZE_MAX];
	uint8_t address;
	uint8_t len;
	int status;

	switch (chan) {
#ifdef CONFIG_BMA530_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		address = BMA530_REG_TEMP_DATA;
		len = BMA530_PACKET_SIZE_TEMP;
		break;
#endif /* CONFIG_BMA530_TEMPERATURE */
	case SENSOR_CHAN_ACCEL_X:
		address = BMA530_REG_ACC_DATA_0;
		len = BMA530_ACC_CHANNEL_SIZE_BYTES;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		address = BMA530_REG_ACC_DATA_2;
		len = BMA530_ACC_CHANNEL_SIZE_BYTES;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		address = BMA530_REG_ACC_DATA_4;
		len = BMA530_ACC_CHANNEL_SIZE_BYTES;
		break;
	case SENSOR_CHAN_ALL:
		address = BMA530_REG_ACC_DATA_0;
		len = BMA530_PACKET_SIZE_MAX;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		address = BMA530_REG_ACC_DATA_0;
		len = BMA530_PACKET_SIZE_ACC;
		break;
	default:
		return -ENOTSUP;
	}

	status = bma530->hw_ops->read_data(dev, address, read_data, len);
	if (status < 0) {
		LOG_ERR("Cannot read data: %d", status);
		return status;
	}

	/* Data needs to be converted from accelerometer's little endian. */
	switch (chan) {
#ifdef CONFIG_BMA530_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		bma530->temp = read_data[0];
		LOG_DBG("Register temp val %d", bma530->temp);
		break;
#endif /* CONFIG_BMA530_TEMPERATURE */
	case SENSOR_CHAN_ACCEL_X:
		bma530->x = sys_get_le16(&read_data[0]);
		LOG_DBG("Raw [%#02X, %#02X]", read_data[0], read_data[1]);
		LOG_DBG("Register X val %d", bma530->x);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		bma530->y = sys_get_le16(&read_data[0]);
		LOG_DBG("Raw [%#02X, %#02X]", read_data[0], read_data[1]);
		LOG_DBG("Register Y val %d", bma530->y);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		bma530->z = sys_get_le16(&read_data[0]);
		LOG_DBG("Raw [%#02X, %#02X]", read_data[0], read_data[1]);
		LOG_DBG("Register Z val %d", bma530->z);
		break;
	case SENSOR_CHAN_ALL:
#ifdef CONFIG_BMA530_TEMPERATURE
		bma530->temp = read_data[6];
		LOG_DBG("Register temp val %d", bma530->temp);
#endif /* CONFIG_BMA530_TEMPERATURE */
		/* fallthrough */
	case SENSOR_CHAN_ACCEL_XYZ:
		bma530->x = sys_get_le16(&read_data[0]);
		bma530->y = sys_get_le16(&read_data[2]);
		bma530->z = sys_get_le16(&read_data[4]);
		LOG_DBG("Raw [%#02X, %#02X, %#02X, %#02X, %#02X, %#02X]", read_data[0],
			read_data[1], read_data[2], read_data[3], read_data[4], read_data[5]);
		LOG_DBG("Register XYZ val %d, %d, %d", bma530->x, bma530->y, bma530->z);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Get and decode data from drivers internal buffer
 * @param dev pointer to device struct
 * @param chan channel to read from
 * @param val pointer to value in Zephyr sensor format to write the data
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_channel_get(const struct device *dev, const enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct bma530_data *bma530 = dev->data;
	/* Scale of [ug] (micro g's) in LSB. The acceleration is stored in 16 bits, so the full
	 * scale needs to be shifted by 16 bits right. The accel_fs_range contains selected range
	 * of the accelerometer, but note that this range is signed, so the actual range is 2x
	 * bigger (so LSB represents also 2x more [ug]). To calculate the correct [ug] in LSB, it
	 * is shifted right by one bit less.
	 */
	const int32_t ug_in_lsb = bma530->accel_fs_range >> (BMA530_ACC_CHANNEL_SIZE_BITS - 1);

	switch (chan) {
#ifdef CONFIG_BMA530_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		val->val1 = bma530->temp + BMA530_TEMP_OFFSET;
		val->val2 = 0;
		break;
#endif /* CONFIG_BMA530_TEMPERATURE */
	case SENSOR_CHAN_ACCEL_X:
		sensor_ug_to_ms2(bma530->x * ug_in_lsb, val);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		sensor_ug_to_ms2(bma530->y * ug_in_lsb, val);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		sensor_ug_to_ms2(bma530->z * ug_in_lsb, val);
		break;
	case SENSOR_CHAN_ALL:
#ifdef CONFIG_BMA530_TEMPERATURE
		val[3].val1 = (int32_t)bma530->temp + BMA530_TEMP_OFFSET;
		val[3].val2 = 0;
#endif /* CONFIG_BMA530_TEMPERATURE */
		/* fallthrough */
	case SENSOR_CHAN_ACCEL_XYZ:
		/* Expect val to point to an array of three sensor_values. */
		sensor_ug_to_ms2(bma530->x * ug_in_lsb, &val[0]);
		sensor_ug_to_ms2(bma530->y * ug_in_lsb, &val[1]);
		sensor_ug_to_ms2(bma530->z * ug_in_lsb, &val[2]);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief Internal device initialization function
 * @param dev pointer to device struct
 *
 * @return error code in case of error, 0 otherwise
 */
static int bma530_chip_init(const struct device *dev)
{
	const struct bma530_config *cfg = dev->config;
	struct bma530_data *bma530 = dev->data;
	int status;
	int retry = 0;
	uint8_t chip_id;
	uint8_t health_reg;

	/* Sensor bus-specific initialization. */
	status = cfg->bus_init(dev);
	if (status) {
		LOG_ERR("bus_init failed: %d", status);
		return status;
	}

#ifdef CONFIG_BMA530_DELAY_COMMUNICATION_AFTER_POWER_ON
	/* It is recommended to wait at least 3 ms after power on before any communication with
	 * the accelerometer. This time should already pass until Zephyr initializes the driver.
	 * Enable this wait time in case sensor initialization occurs earlier.
	 */
	k_msleep(3);
#endif /* CONFIG_BMA530_DELAY_COMMUNICATION_AFTER_POWER_ON */

	/* First read from accelerometer selects the interface and the result is invalid. */
	bma530->hw_ops->read_reg(dev, BMA530_REG_CHIP_ID, &chip_id);

	/* Read Chip ID */
	status = bma530->hw_ops->read_reg(dev, BMA530_REG_CHIP_ID, &chip_id);
	if (status) {
		LOG_ERR("Could not read chip_id: %d", status);
		return status;
	}
	LOG_DBG("chip_id is 0x%02x", chip_id);

	if (chip_id != BMA530_CHIP_ID) {
		LOG_WRN("Driver tested for BMA530. Check for unintended operation.");
	}

	status = bma530->hw_ops->read_reg(dev, BMA530_REG_HEALTH, &health_reg);
	if (status) {
		LOG_ERR("Could not read health register: %d", status);
		return status;
	}

	while ((health_reg & BMA530_REG_HEALTH_MASK) != BMA530_HEALTH_OK) {
		retry++;
		status = bma530->hw_ops->read_reg(dev, BMA530_REG_HEALTH, &health_reg);
		if (status) {
			LOG_ERR("Could not read health register, tried %d times: %d",
				retry, status);
			return status;
		}

		if (retry >= BMA530_HEALTH_CHECK_RETRIES) {
			LOG_ERR("Read health register %d times, but device still is not in a good "
				"health.",
				retry);
			return status;
		}
		k_msleep(3);
	}

	if (retry > 1) {
		LOG_DBG("Read health register %d times until device in a good health.", retry);
	}

	/* Set power mode to the value set in config. Value in cfg is already the value to write to
	 * the register (with a proper offset).
	 */
	status = bma530_power_mode_set(dev, cfg->power_mode);
	if (status < 0) {
		LOG_ERR("Could not set power mode, status %d.", status);
		return status;
	}

	/* Set full scale range to the value set in config. Value in cfg is already the value to
	 * write to the register (with a proper offset).
	 */
	status = bma530_fs_set(dev, cfg->full_scale_range);
	if (status < 0) {
		LOG_ERR("Could not set full scale range, status %d.", status);
		return status;
	}

	/* Set output data rate to the value set in config. Value in cfg is already the value to
	 * write to the register (with a proper offset).
	 */
	status = bma530_odr_set(dev, cfg->accel_odr);
	if (status < 0) {
		LOG_ERR("Could not set data rate, status %d.", status);
		return status;
	}

	return 0;
}

/*
 * Sensor driver API
 */

static const struct sensor_driver_api bma530_driver_api = {
	.attr_set = bma530_attr_set,
	.attr_get = bma530_attr_get,
	.sample_fetch = bma530_sample_fetch,
	.channel_get = bma530_channel_get,
};

/*
 * Device instantiation macros
 */

/* Initializes a struct bma530_config for an instance on a SPI bus.
 * SPI operation is not currently supported.
 */
#define BMA530_CONFIG_SPI(inst)									\
		.bus_cfg.spi = SPI_DT_SPEC_INST_GET(inst, 0, 0), .bus_init = &bma530_spi_init,

/* Initializes a struct bma530_config for an instance on an I2C bus. */
#define BMA530_CONFIG_I2C(inst)									\
		.bus_cfg.i2c = I2C_DT_SPEC_INST_GET(inst), .bus_init = &bma530_i2c_init,

/*
 * Main instantiation macro, which selects the correct bus-specific
 * instantiation macros for the instance.
 */
#define BMA530_DEFINE(inst)									\
	static struct bma530_data bma530_data_##inst;						\
	static const struct bma530_config bma530_config_##inst = {				\
		COND_CODE_1(DT_INST_ON_BUS(inst, spi),						\
			(BMA530_CONFIG_SPI(inst)), (BMA530_CONFIG_I2C(inst)))			\
		.full_scale_range = DT_INST_ENUM_IDX(inst, full_scale_range_g),			\
		.accel_odr = DT_INST_ENUM_IDX(inst, sampling_frequency_hz),			\
		.power_mode = DT_INST_ENUM_IDX(inst, power_mode),				\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bma530_chip_init, NULL, &bma530_data_##inst,		\
					 &bma530_config_##inst, POST_KERNEL,			\
					 CONFIG_SENSOR_INIT_PRIORITY, &bma530_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BMA530_DEFINE)
