/*
 * Copyright (c) 2025 Jonas Berg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_ina228

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/dt-bindings/sensor/ina228.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include "ina228_priv.h"

#define INA228_MANUFACTURER_ID       0x5449
#define INA228_DEVICE_ID_DIEID       0x228
#define INA228_DEVICE_ID_DIEID_SHIFT 4
#define INA228_DEVICE_ID_REV_ID      0x1

/* Volt per LSB, from data sheet */
#define INA228_VBUS_SCALING_FACTOR              1.953125e-4
/* Compensate for nA in conversion */
#define INA228_CURRENT_SCALING_FACTOR           1.0e-9
/* From data sheet, compensated for nA in conversion */
#define INA228_POWER_SCALING_FACTOR             3.2e-9
/* Compensate for nA in conversion */
#define INA228_CHARGE_SCALING_FACTOR            1.0e-9
/* From data sheet (16*3.2), compensated for nA in conversion */
#define INA228_ENERGY_SCALING_FACTOR            5.12e-8
/* deg Centigrades per LSB, from data sheet */
#define INA228_DIETEMP_SCALING_FACTOR           7.8125e-3
/* Volt per LSB. From data sheet, for use with high ADC range */
#define INA228_VSHUNT_SCALING_FACTOR_RANGE_HIGH 3.125e-7
/* Volt per LSB. From data sheet, for use with low ADC range */
#define INA228_VSHUNT_SCALING_FACTOR_RANGE_LOW  7.8125e-8
/* From data sheet, compensated for nA and uOhm */
#define INA228_SHUNT_CALIBRATION_FACTOR         1.31072e-5

#define INA228_RST_MASK            0x01
#define INA228_RST_SHIFT           15
#define INA228_RSTACC_MASK         0x01
#define INA228_RSTACC_SHIFT        14
#define INA228_CONVDLY_MASK        0xFF
#define INA228_CONVDLY_SHIFT       6
#define INA228_CONVDLY_MS_MAX      510
#define INA228_CONVDLY_RATIO       2
#define INA228_TEMPCOMP_FLAG_MASK  0x01
#define INA228_TEMPCOMP_FLAG_SHIFT 5
#define INA228_ADCRANGE_MASK       0x01
#define INA228_ADCRANGE_SHIFT      4
#define INA228_ADCRANGE_RANGE_HIGH 0
#define INA228_ADCRANGE_RANGE_LOW  1
#define INA228_ADCRANGE_RATIO      4
#define INA228_MODE_MASK           0x0F
#define INA228_MODE_SHIFT          12
#define INA228_VBUSCT_MASK         0x07
#define INA228_VBUSCT_SHIFT        9
#define INA228_VSHCT_MASK          0x07
#define INA228_VSHCT_SHIFT         6
#define INA228_VTCT_MASK           0x07
#define INA228_VTCT_SHIFT          3
#define INA228_AVG_MASK            0x07
#define INA228_AVG_SHIFT           0
#define INA228_SHUNT_CAL_MASK      0x7FFF
#define INA228_SHUNT_TEMPCO_MASK   0x3FFF

#define INA228_SIZEOF_UINT24 3
#define INA228_SIZEOF_UINT40 5

#define INA228_GET_BIT_4_TO_23(x) ((x) >> 4) & 0x000FFFFF

struct ina228_data {
	const struct device *dev;

	/** Current (Ampere). 20 bits, two's complement value in sensor.
	 *  Resolution depends on LSB configuration setting.
	 */
	int32_t current;

	/** Bus voltage (Volt). 20 bits, two's complement value in sensor, but always positive.
	 *  Resolution 195.3125 uV per bit.
	 */
	int32_t bus_voltage;

	/** Power (Watt).  24 bits unsigned value value in sensor.
	 *  Resolution depends on LSB configuration setting and a constant.
	 */
	uint32_t power;

#ifdef CONFIG_INA228_CUMULATIVE
	/** Accumulated charge (Coulomb). 40 bits, two's complement value in sensor.
	 *  Resolution depends on LSB configuration setting.
	 */
	int64_t charge;

	/** Accumulated energy (Joule). 40 bits unsigned value in sensor.
	 *  Resolution depends on LSB configuration setting and a constant.
	 */
	uint64_t energy;
#endif

#ifdef CONFIG_INA228_VSHUNT
	/** Shunt voltage (Volt). 20 bits, two's complement value in sensor.
	 *  Resolution 312.5 nV or 78.125 nV per bit, depending on ADCRANGE.
	 */
	int32_t shunt_voltage;
#endif

#ifdef CONFIG_INA228_TEMPERATURE
	/** Temperature (deg Centigrades). 16 bits, two's complement value in sensor.
	 *  Resolution 7.8125 mdegC per bit.
	 */
	int16_t die_temperature;
#endif
};

struct ina228_config {
	const struct i2c_dt_spec bus;

	/** Shunt resistance, in microohms */
	uint32_t rshunt;

	/** LSB value for current conversions, in nA */
	uint32_t lsb_na;

	/** Initial conversion delay in steps of 2 ms.
	 *  Valid values are 0, 2, 4, 6, ..., 508, 510.
	 *  This value will be divided by 2 before writing to the CONFIG register.
	 */
	uint16_t conversion_delay;

	/** Shunt resistor temperature compensation, in ppm/degC. 0x0000 to 0x3FFF */
	uint16_t tempcomp;

	/** ADC range. false = +-163.84 mV, true = +-40.96 mV */
	bool adc_low_range;

	/** ADC operation mode, 0x00 to 0x0F */
	uint8_t mode;

	/** Conversion time for Vbus, 0x00 to 0x07 */
	uint8_t vbusct;

	/** Conversion time for Vshunt, 0x00 to 0x07 */
	uint8_t vshct;

	/** Conversion time for temperature, 0x00 to 0x07 */
	uint8_t vtct;

	/** Averaging, 0x00 to 0x07 */
	uint8_t avg;
};

LOG_MODULE_REGISTER(INA228, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Convert the 20 lowest bits to a signed value (from two's complement)
 *
 * @param input      Input value
 *
 * @retval converted value
 */
int32_t ina228_convert_20bits_to_signed(uint32_t input)
{
	if (input <= INA228_INT20_MAX) {
		return (int32_t)input;
	}

	uint32_t intermediate = (~input & INA228_UINT20_MAX) + 1;

	return -((int32_t)intermediate);
}

#ifdef CONFIG_INA228_CUMULATIVE

/**
 * @brief Convert the 40 lowest bits to a signed value (from two's complement)
 *
 * @param input      Input value
 *
 * @retval converted value
 */
int64_t ina228_convert_40bits_to_signed(uint64_t input)
{
	if (input <= INA228_INT40_MAX) {
		return (int64_t)input;
	}

	uint64_t intermediate = (~input & INA228_UINT40_MAX) + 1;

	return -((int64_t)intermediate);
}

#endif /* CONFIG_INA228_CUMULATIVE */

/**
 * @brief Write to an I2C register of size 16 bits
 *
 * @param dev           Device pointer
 * @param reg_addr      Address of the I2C register
 * @param reg_value     Value to be written
 *
 * @retval 0 on success
 * @retval negative on error
 */
static int ina228_register_write_16(const struct device *dev, uint8_t reg_addr, uint16_t reg_value)
{
	const struct ina228_config *cfg = dev->config;
	uint8_t send_buffer[sizeof(uint8_t) + sizeof(uint16_t)];

	send_buffer[0] = reg_addr;
	sys_put_be16(reg_value, &send_buffer[1]);

	return i2c_write_dt(&cfg->bus, send_buffer, sizeof(send_buffer));
}

/**
 * @brief Read an I2C register of size 16 bits
 *
 * @param dev           Device pointer
 * @param reg_addr      Address of the I2C register
 * @param reg_value     Resulting value
 *
 * The resulting value is unsigned. If the register value represents a signed value, the resulting
 * value must be converted afterwards.
 *
 * @retval 0 on success
 * @retval negative on error
 */
static int ina228_register_read_16(const struct device *dev, uint8_t reg_addr, uint16_t *reg_value)
{
	const struct ina228_config *cfg = dev->config;
	uint8_t receive_buffer[sizeof(uint16_t)];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), receive_buffer,
			       sizeof(receive_buffer));
	if (rc == 0) {
		*reg_value = sys_get_be16(receive_buffer);
	}

	return rc;
}

/**
 * @brief Read an I2C register of size 24 bits
 *
 * @param dev           Device pointer
 * @param reg_addr      Address of the I2C register
 * @param reg_value     Resulting value
 *
 * The resulting value is unsigned. If the register value represents a signed value, the resulting
 * value must be converted afterwards.
 *
 * @retval 0 on success
 * @retval negative on error
 */
static int ina228_register_read_24(const struct device *dev, uint8_t reg_addr, uint32_t *reg_value)
{
	const struct ina228_config *cfg = dev->config;
	uint8_t receive_buffer[INA228_SIZEOF_UINT24];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), receive_buffer,
			       sizeof(receive_buffer));
	if (rc == 0) {
		*reg_value = sys_get_be24(receive_buffer);
	}

	return rc;
}

#ifdef CONFIG_INA228_CUMULATIVE
/**
 * @brief Read an I2C register of size 40 bits
 *
 * @param dev           Device pointer
 * @param reg_addr      Address of the I2C register
 * @param reg_value     Resulting value
 *
 * The resulting value is unsigned. If the register value represents a signed value, the resulting
 * value must be converted afterwards.
 *
 * @retval 0 on success
 * @retval negative on error
 */
static int ina228_register_read_40(const struct device *dev, uint8_t reg_addr, uint64_t *reg_value)
{
	const struct ina228_config *cfg = dev->config;
	uint8_t receive_buffer[INA228_SIZEOF_UINT40];
	int rc;

	rc = i2c_write_read_dt(&cfg->bus, &reg_addr, sizeof(reg_addr), receive_buffer,
			       sizeof(receive_buffer));
	if (rc == 0) {
		*reg_value = sys_get_be40(receive_buffer);
	}

	return rc;
}
#endif /* CONFIG_INA228_CUMULATIVE */

/**
 * @brief Check if the channel is supported by this sensor
 *
 * @param chan	Channel
 *
 * @retval true if the channel is supported, false otherwise
 */
static bool ina228_is_channel_valid(enum sensor_channel chan)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE && chan != SENSOR_CHAN_CURRENT &&
	    chan != SENSOR_CHAN_POWER
#ifdef CONFIG_INA228_VSHUNT
	    && chan != SENSOR_CHAN_VSHUNT
#endif /* CONFIG_INA228_VSHUNT */
#ifdef CONFIG_INA228_TEMPERATURE
	    && chan != SENSOR_CHAN_DIE_TEMP
#endif /* CONFIG_INA228_TEMPERATURE */
#ifdef CONFIG_INA228_CUMULATIVE
	    && chan != SENSOR_CHAN_CHARGE && chan != SENSOR_CHAN_ENERGY
#endif /* CONFIG_INA228_CUMULATIVE */
	) {
		return false;
	}

	return true;
}

/**
 * @brief Fetch the bus voltage reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_bus_voltage(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	uint32_t registervalue_32bit = 0;
	int ret = 0;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_VOLTAGE)) {
		return 0;
	}

	ret = ina228_register_read_24(dev, INA228_REG_VBUS, &registervalue_32bit);
	if (ret) {
		return ret;
	}

	data->bus_voltage =
		ina228_convert_20bits_to_signed(INA228_GET_BIT_4_TO_23(registervalue_32bit));

	return 0;
}

/**
 * @brief Fetch the current reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_current(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	uint32_t registervalue_32bit = 0;
	int ret = 0;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_CURRENT)) {
		return 0;
	}

	ret = ina228_register_read_24(dev, INA228_REG_CURRENT, &registervalue_32bit);
	if (ret) {
		return ret;
	}

	data->current =
		ina228_convert_20bits_to_signed(INA228_GET_BIT_4_TO_23(registervalue_32bit));

	return 0;
}

/**
 * @brief Fetch the power reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_power(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_POWER)) {
		return 0;
	}

	return ina228_register_read_24(dev, INA228_REG_POWER, &data->power); /* Unsigned */
}

#ifdef CONFIG_INA228_CUMULATIVE
/**
 * @brief Fetch the charge reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_charge(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	uint64_t registervalue_64bit = 0;
	int ret = 0;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_CHARGE)) {
		return 0;
	}

	ret = ina228_register_read_40(dev, INA228_REG_CHARGE, &registervalue_64bit);
	if (ret) {
		return ret;
	}

	data->charge = ina228_convert_40bits_to_signed(registervalue_64bit);

	return 0;
}

/**
 * @brief Fetch the energy reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_energy(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_ENERGY)) {
		return 0;
	}

	return ina228_register_read_40(dev, INA228_REG_ENERGY, &data->energy); /* Unsigned */
}
#endif /* CONFIG_INA228_CUMULATIVE */

#ifdef CONFIG_INA228_TEMPERATURE
/**
 * @brief Fetch the die temperature reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_temperature(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_DIE_TEMP)) {
		return 0;
	}

	return ina228_register_read_16(dev, INA228_REG_DIETEMP,
				       &data->die_temperature); /* Converts to int16_t */
}
#endif /* CONFIG_INA228_TEMPERATURE */

#ifdef CONFIG_INA228_VSHUNT
/**
 * @brief Fetch the shunt voltage reading from the sensor
 *
 * This is a no-op if not among the requested channels.
 *
 * @param dev	Sensor device
 * @param chan	Channel
 *
 * @retval 0 on success (or channel not requested), negative value on failure
 */
static int ina228_fetch_shunt_voltage(const struct device *dev, enum sensor_channel chan)
{
	struct ina228_data *data = dev->data;
	uint32_t registervalue_32bit = 0;
	int ret = 0;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_VSHUNT)) {
		return 0;
	}

	ret = ina228_register_read_24(dev, INA228_REG_VSHUNT, &registervalue_32bit);
	if (ret) {
		return ret;
	}

	data->shunt_voltage =
		ina228_convert_20bits_to_signed(INA228_GET_BIT_4_TO_23(registervalue_32bit));

	return 0;
}
#endif /* CONFIG_INA228_VSHUNT */

static int ina228_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	int ret = 0;

	if (!ina228_is_channel_valid(chan)) {
		return -ENOTSUP;
	}

	ret = ina228_fetch_bus_voltage(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 bus voltage");
		return ret;
	}

	ret = ina228_fetch_current(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 current");
		return ret;
	}

	ret = ina228_fetch_power(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 power");
		return ret;
	}

#ifdef CONFIG_INA228_CUMULATIVE
	ret = ina228_fetch_charge(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 charge");
		return ret;
	}

	ret = ina228_fetch_energy(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 energy");
		return ret;
	}
#endif /* CONFIG_INA228_CUMULATIVE */

#ifdef CONFIG_INA228_TEMPERATURE
	ret = ina228_fetch_temperature(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 temperature");
		return ret;
	}
#endif /* CONFIG_INA228_TEMPERATURE */

#ifdef CONFIG_INA228_VSHUNT
	ret = ina228_fetch_shunt_voltage(dev, chan);
	if (ret) {
		LOG_ERR("Failed to read INA228 shunt voltage");
		return ret;
	}
#endif /* CONFIG_INA228_VSHUNT */

	return 0;
}

static int ina228_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	const struct ina228_config *cfg = dev->config;
	struct ina228_data *data = dev->data;
	double tmp;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		tmp = data->bus_voltage * INA228_VBUS_SCALING_FACTOR;
		break;
	case SENSOR_CHAN_CURRENT:
		tmp = data->current * (cfg->lsb_na * INA228_CURRENT_SCALING_FACTOR);
		break;
	case SENSOR_CHAN_POWER:
		tmp = data->power * (cfg->lsb_na * INA228_POWER_SCALING_FACTOR);
		break;
#ifdef CONFIG_INA228_CUMULATIVE
	case SENSOR_CHAN_CHARGE:
		tmp = data->charge * (cfg->lsb_na * INA228_CHARGE_SCALING_FACTOR);
		break;
	case SENSOR_CHAN_ENERGY:
		tmp = data->energy * (cfg->lsb_na * INA228_ENERGY_SCALING_FACTOR);
		break;
#endif /* CONFIG_INA228_CUMULATIVE */
#ifdef CONFIG_INA228_TEMPERATURE
	case SENSOR_CHAN_DIE_TEMP:
		tmp = data->die_temperature * INA228_DIETEMP_SCALING_FACTOR;
		break;
#endif /* CONFIG_INA228_TEMPERATURE */
#ifdef CONFIG_INA228_VSHUNT
	case SENSOR_CHAN_VSHUNT:
		if (cfg->adc_low_range) {
			tmp = data->shunt_voltage * INA228_VSHUNT_SCALING_FACTOR_RANGE_LOW;
		} else {
			tmp = data->shunt_voltage * INA228_VSHUNT_SCALING_FACTOR_RANGE_HIGH;
		}
		break;
#endif /* CONFIG_INA228_VSHUNT */
	default:
		return -ENOTSUP;
	}

	return sensor_value_from_double(val, tmp);
}

static int ina228_init(const struct device *dev)
{
	const struct ina228_config *cfg = dev->config;
	uint16_t register_reading;
	uint8_t convdly;
	uint16_t config;
	uint16_t adc_config;
	uint16_t shunt_cal;
	uint16_t shunt_tempco;
	int ret;

	__ASSERT_NO_MSG(cfg->mode <= INA228_MODE_MASK);
	__ASSERT_NO_MSG(cfg->vbusct <= INA228_VBUSCT_MASK);
	__ASSERT_NO_MSG(cfg->vshct <= INA228_VSHCT_MASK);
	__ASSERT_NO_MSG(cfg->vtct <= INA228_VTCT_MASK);
	__ASSERT_NO_MSG(cfg->avg <= INA228_AVG_MASK);

	if (cfg->conversion_delay > INA228_CONVDLY_MS_MAX) {
		LOG_ERR("Too large conversion delay: %d ms. Max allowed value is %d ms",
			cfg->conversion_delay, INA228_CONVDLY_MS_MAX);
		return -EFAULT;
	}

	if (cfg->tempcomp > INA228_SHUNT_TEMPCO_MASK) {
		LOG_ERR("Too large temperature compensation: %u ms. Max allowed value is %u",
			cfg->tempcomp, INA228_SHUNT_TEMPCO_MASK);
		return -EFAULT;
	}

	if (!i2c_is_ready_dt(&cfg->bus)) {
		LOG_ERR("I2C bus is not ready.");
		return -ENODEV;
	}

	ret = ina228_register_read_16(dev, INA228_REG_MANUFACTURER_ID, &register_reading);
	if (ret) {
		LOG_ERR("No communication with INA228 sensor.");
		return ret;
	}
	if (register_reading != INA228_MANUFACTURER_ID) {
		LOG_ERR("INA228: Wrong manufacturer ID: 0x%04x", register_reading);
		return -EFAULT;
	}

	ret = ina228_register_read_16(dev, INA228_REG_DEVICE_ID, &register_reading);
	if (ret) {
		LOG_ERR("Failed to read DEVICE_ID from INA228 sensor.");
		return ret;
	}
	if (register_reading !=
	    (INA228_DEVICE_ID_REV_ID | (INA228_DEVICE_ID_DIEID << INA228_DEVICE_ID_DIEID_SHIFT))) {
		LOG_ERR("Unexpected chip ID and version: 0x%04x", register_reading);
		return -EFAULT;
	}

	/* Reset the INA228 chip */
	config = INA228_RST_MASK << INA228_RST_SHIFT;
	ret = ina228_register_write_16(dev, INA228_REG_CONFIG, config);
	if (ret) {
		LOG_ERR("Failed to write CONFIG register to INA228 sensor to reset the chip.");
		return ret;
	}

	/* Write configuration settings */
	convdly = (uint8_t)(cfg->conversion_delay / INA228_CONVDLY_RATIO);
	config = (convdly & INA228_CONVDLY_MASK) << INA228_CONVDLY_SHIFT;
	if (cfg->adc_low_range) {
		config |= (INA228_ADCRANGE_RANGE_LOW << INA228_ADCRANGE_SHIFT);
	}
	if (cfg->tempcomp > 0) {
		config |= (INA228_TEMPCOMP_FLAG_MASK << INA228_TEMPCOMP_FLAG_SHIFT);
	}
	ret = ina228_register_write_16(dev, INA228_REG_CONFIG, config);
	if (ret) {
		LOG_ERR("Failed to write CONFIG register to INA228 sensor.");
		return ret;
	}

	adc_config = (cfg->mode & INA228_MODE_MASK) << INA228_MODE_SHIFT |
		     (cfg->vbusct & INA228_VBUSCT_MASK) << INA228_VBUSCT_SHIFT |
		     (cfg->vshct & INA228_VSHCT_MASK) << INA228_VSHCT_SHIFT |
		     (cfg->vtct & INA228_VTCT_MASK) << INA228_VTCT_SHIFT |
		     (cfg->avg & INA228_AVG_MASK) << INA228_AVG_SHIFT;
	ret = ina228_register_write_16(dev, INA228_REG_ADC_CONFIG, adc_config);
	if (ret) {
		LOG_ERR("Failed to write ADC_CONFIG register to INA228 sensor.");
		return ret;
	}

	shunt_cal = (uint16_t)((INA228_SHUNT_CALIBRATION_FACTOR * cfg->lsb_na) * cfg->rshunt);
	if (cfg->adc_low_range) {
		shunt_cal *= INA228_ADCRANGE_RATIO;
	}
	if (shunt_cal > INA228_SHUNT_CAL_MASK) {
		LOG_ERR("Too large calculated SHUNT_CAL register value for the INA228 sensor, as "
			"the product of the shunt resistor value and the current LSB is too "
			"large. LSB %u nA, Rshunt %u uOhm, Low ADC range: %u, Shunt cal: 0x%04X",
			cfg->lsb_na, cfg->rshunt, cfg->adc_low_range, shunt_cal);
		return -EFAULT;
	}
	ret = ina228_register_write_16(dev, INA228_REG_SHUNT_CAL, shunt_cal);
	if (ret) {
		LOG_ERR("Failed to write SHUNT_CAL register to INA228 sensor.");
		return ret;
	}

	shunt_tempco = cfg->tempcomp & INA228_SHUNT_TEMPCO_MASK;
	ret = ina228_register_write_16(dev, INA228_REG_SHUNT_TEMPCO, shunt_tempco);
	if (ret) {
		LOG_ERR("Failed to write SHUNT_TEMPCO register to INA228 sensor.");
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ina228_device_api) = {
	.sample_fetch = ina228_sample_fetch,
	.channel_get = ina228_channel_get,
};

#define INSTANTIATE_INA228(index)                                                                  \
	static const struct ina228_config ina228_config_##index = {                                \
		.bus = I2C_DT_SPEC_INST_GET(index),                                                \
		.rshunt = DT_INST_PROP(index, rshunt_micro_ohms),                                  \
		.lsb_na = DT_INST_PROP(index, lsb_nanoamp),                                        \
		.conversion_delay = DT_INST_PROP(index, initial_delay_ms),                         \
		.tempcomp = DT_INST_PROP(index, temp_compensation_ppm),                            \
		.adc_low_range = DT_INST_PROP_OR(index, adc_low_range, 0),                         \
		.mode = DT_INST_ENUM_IDX(index, operating_mode),                                   \
		.vbusct = DT_INST_ENUM_IDX(index, vbus_conversion_time_us),                        \
		.vshct = DT_INST_ENUM_IDX(index, vshunt_conversion_time_us),                       \
		.vtct = DT_INST_ENUM_IDX(index, temp_conversion_time_us),                          \
		.avg = DT_INST_ENUM_IDX(index, avg_count),                                         \
	};                                                                                         \
	static struct ina228_data ina228_data_##index;                                             \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(index, ina228_init, NULL, &ina228_data_##index,               \
				     &ina228_config_##index, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ina228_device_api);

DT_INST_FOREACH_STATUS_OKAY(INSTANTIATE_INA228);
