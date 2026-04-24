/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ltc4286

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "ltc4286.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LTC4286, CONFIG_SENSOR_LOG_LEVEL);

/**
 * @brief Reads from a sent PMBus command to the LTC4286.
 *
 * @param dev Pointer to the device structure of the I2C controller configured
 * as the host
 * @param code Pointer to the PMBus command code to write to the LTC4286
 * @param code_size Number of bytes of the PMBus code (for MFR commands)
 * @param val Pointer to the storage of the read data
 * @param val_size Number of bytes to be read
 */
int ltc4286_cmd_read(const struct device *dev, uint8_t *code, uint8_t code_size, uint8_t *val,
		     uint8_t val_size)
{
	const struct ltc4286_dev_config *cfg = dev->config;

	/* Sends a PMBus command and then reads from the device */
	if (!val) {
		return -EINVAL;
	}

	return i2c_write_read_dt(&cfg->i2c_dev, code, code_size, val, val_size);
}

/**
 * @brief Writes a value to the device.
 *
 * @param dev Pointer to the device structure of the I2C controller configured
 * as the host
 * @param code Pointer to the PMBus command code to write to the LTC4286
 * @param val Pointer to the storage of the read data
 * @param val_size Number of bytes to be read
 */
int ltc4286_cmd_write(const struct device *dev, uint8_t code, uint8_t *val, uint8_t val_size)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t payload[4];

	if (!val) {
		return -EINVAL;
	}

	if ((val_size + 1) >= sizeof(payload)) {
		return -ENOMEM;
	}

	payload[0] = code;
	memcpy(&payload[1], val, val_size);

	return i2c_write_dt(&cfg->i2c_dev, payload, val_size + 1);
}

/**
 * @brief Fetches telemetry information from the LTC4286 and stores them in the
 * respective sensor channels.
 *
 * @param dev Pointer to the device structure to store data in
 * @param chan Sensor channels to fetch data from
 */
static int ltc4286_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ltc4286_data *drv_data = dev->data;
	int ret;
	uint16_t command;
	uint16_t value = 0;
	uint8_t *cmd_ptr = (uint8_t *)&command;
	int8_t *val_ptr = (int8_t *)&value;

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		command = LTC4286_CMD_READ_VOUT;
		ret = ltc4286_cmd_read(dev, cmd_ptr, 1, val_ptr, 2);
		if (ret) {
			return ret;
		}

		drv_data->vout_sample = value;
		break;
	case SENSOR_CHAN_CURRENT:
		command = LTC4286_CMD_READ_IOUT;
		ret = ltc4286_cmd_read(dev, cmd_ptr, 1, val_ptr, 2);
		if (ret) {
			return ret;
		}

		drv_data->iout_sample = value;
		break;
	case SENSOR_CHAN_POWER:
		command = LTC4286_CMD_READ_PIN;
		ret = ltc4286_cmd_read(dev, cmd_ptr, 1, val_ptr, 2);
		if (ret) {
			return ret;
		}

		drv_data->pin_sample = value;
		break;
	case SENSOR_CHAN_LTC4286_VIN:
		command = LTC4286_CMD_READ_VIN;
		ret = ltc4286_cmd_read(dev, cmd_ptr, 1, val_ptr, 2);
		if (ret) {
			return ret;
		}

		drv_data->vin_sample = value;
		break;
	case SENSOR_CHAN_LTC4286_STATUS:
		command = LTC4286_CMD_STATUS_WORD;
		ret = ltc4286_cmd_read(dev, cmd_ptr, 1, val_ptr, 2);
		if (ret) {
			return ret;
		}

		drv_data->status_flags_sample = value;
		break;
	default:
		return -ENOTSUP;
	}
	return 0;
}

/**
 * @brief Helper function to convert voltage ADC values to sensor_value
 *
 * @param sample Raw ADC sample value
 * @param vrange_set Voltage range setting (true = 100V, false = 25V)
 * @param val Pointer to the return location of the converted sensor data
 */
static void ltc4286_convert_voltage(uint32_t sample, bool vrange_set, struct sensor_value *val)
{
	int64_t data = sample * LTC4286_ADC_VAL_TO_UVAL;

	if (vrange_set) {
		data = (data * LTC4286_ADC_VIN_VOUT_100V_FACTOR) / LTC4286_ADC_VIN_VOUT_100V_SCALE;
	} else {
		data = data * LTC4286_ADC_VIN_VOUT_25V_FACTOR / LTC4286_ADC_VIN_VOUT_25V_SCALE;
	}
	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

/**
 * @brief Helper function to convert power ADC values to sensor_value
 *
 * @param sample Raw ADC sample value
 * @param vrange_set Voltage range setting (true = 100V, false = 25V)
 * @param r_sense_uohms Sense resistor value in micro-ohms
 * @param val Pointer to the return location of the converted sensor data
 */
static void ltc4286_convert_power(uint32_t sample, bool vrange_set, uint32_t r_sense_uohms,
				  struct sensor_value *val)
{
	int64_t data = sample * LTC4286_ADC_VAL_TO_UVAL;

	if (vrange_set) {
		data = (data * LTC4286_ADC_PIN_100V_FACTOR) / r_sense_uohms;
		data = data / LTC4286_ADC_PIN_100V_SCALE;
	} else {
		data = (data * LTC4286_ADC_PIN_25V_FACTOR) / r_sense_uohms;
		data = data / LTC4286_ADC_PIN_25V_SCALE;
	}
	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

/**
 * @brief Helper function to convert current ADC values to sensor_value
 *
 * @param sample Raw ADC sample value
 * @param r_sense_uohms Sense resistor value in micro-ohms
 * @param val Pointer to the return location of the converted sensor data
 */
static void ltc4286_convert_current(uint32_t sample, uint32_t r_sense_uohms,
				    struct sensor_value *val)
{
	int64_t data = ((int64_t)sample * LTC4286_ADC_VAL_TO_UVAL);

	data = (data * LTC4286_ADC_IOUT_FACTOR) / r_sense_uohms;
	data = data / LTC4286_ADC_IOUT_SCALE;
	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

/**
 * @brief Helper function to convert temperature ADC values to sensor_value
 *
 * @param sample Raw ADC sample value
 * @param val Pointer to the return location of the converted sensor data
 */
static void ltc4286_convert_temperature(uint32_t sample, struct sensor_value *val)
{
	int64_t data = sample * LTC4286_ADC_TEMP_SCALE - LTC4286_ADC_TEMP_K_TO_C;

	data = data * LTC4286_ADC_VAL_TO_UVAL;
	data = data / LTC4286_ADC_TEMP_K_TO_C;
	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

/**
 * @brief Retrieves telemetry information stored from the last fetch.
 *
 * @param dev Pointer to the device structure to retrieve data from
 * @param chan Sensor channel to get data from
 * @param val Pointer to the return location of the retrieved sensor data
 */
static int ltc4286_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	const struct ltc4286_data *drv_data = dev->data;
	int64_t data;

	if (!val) {
		return -EINVAL;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		ltc4286_convert_voltage(drv_data->vout_sample, cfg->vrange_set, val);
		break;
	case SENSOR_CHAN_LTC4286_VIN:
		ltc4286_convert_voltage(drv_data->vin_sample, cfg->vrange_set, val);
		break;
	case SENSOR_CHAN_CURRENT:
		ltc4286_convert_current(drv_data->iout_sample, cfg->r_sense_uohms, val);
		break;
	case SENSOR_CHAN_POWER:
		ltc4286_convert_power(drv_data->pin_sample, cfg->vrange_set, cfg->r_sense_uohms,
				      val);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		ltc4286_convert_temperature(drv_data->temp_sample, val);
		break;

	case SENSOR_CHAN_LTC4286_STATUS:
		data = drv_data->status_flags_sample;
		val->val1 = data;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Helper function to calculate voltage threshold code
 *
 * @param scaled_value Scaled sensor value in microvolts
 * @param vrange_set Voltage range setting (true = 100V, false = 25V)
 * @return uint16_t Register code value
 */
static uint16_t ltc4286_calc_voltage_threshold(int16_t scaled_value, bool vrange_set)
{
	if (vrange_set) {
		scaled_value = scaled_value * LTC4286_ADC_VIN_VOUT_100V_SCALE;
		return (uint16_t)(scaled_value /
				  (LTC4286_ADC_VIN_VOUT_100V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	} else {
		scaled_value = scaled_value * LTC4286_ADC_VIN_VOUT_25V_SCALE;
		return (uint16_t)(scaled_value /
				  (LTC4286_ADC_VIN_VOUT_25V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	}
}

/**
 * @brief Helper function to calculate power threshold code
 *
 * @param scaled_value Scaled sensor value in microwatts
 * @param vrange_set Voltage range setting (true = 100V, false = 25V)
 * @return uint16_t Register code value
 */
static uint16_t ltc4286_calc_power_threshold(int16_t scaled_value, bool vrange_set)
{
	if (vrange_set) {
		scaled_value = scaled_value * LTC4286_ADC_PIN_100V_SCALE;
		return (uint16_t)(scaled_value /
				  (LTC4286_ADC_PIN_100V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	} else {
		scaled_value = scaled_value * LTC4286_ADC_PIN_25V_SCALE;
		return (uint16_t)(scaled_value /
				  (LTC4286_ADC_PIN_25V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	}
}

/**
 * @brief Helper function to calculate current threshold code
 *
 * @param scaled_value Scaled sensor value in microamps
 * @return uint16_t Register code value
 */
static uint16_t ltc4286_calc_current_threshold(int16_t scaled_value)
{
	scaled_value = scaled_value * LTC4286_ADC_IOUT_SCALE;
	return (uint16_t)(scaled_value / (LTC4286_ADC_IOUT_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
}

/**
 * @brief Helper function to calculate temperature threshold code
 *
 * @param scaled_value Scaled sensor value in micro-degrees Celsius
 * @return uint16_t Register code value
 */
static uint16_t ltc4286_calc_temperature_threshold(int16_t scaled_value)
{
	scaled_value = scaled_value * LTC4286_ADC_TEMP_SCALE;
	return (uint16_t)(scaled_value - LTC4286_ADC_TEMP_K_TO_C) / LTC4286_ADC_VAL_TO_UVAL;
}

/**
 * @brief Sets the lower threshold for a channel
 *
 * @param dev Pointer to the device structure to set data to
 * @param chan Sensor channel to set lower threshold to
 * @param val Pointer to the sensor value to set as the channel's threshold
 */
static int ltc4286_set_lower_threshold(const struct device *dev, enum sensor_channel chan,
				       const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t code;
	int16_t scaled_value = (val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		code = ltc4286_calc_voltage_threshold(scaled_value, cfg->vrange_set);
		return ltc4286_cmd_write(dev, LTC4286_CMD_VOUT_LOWER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_LTC4286_VIN:
		code = ltc4286_calc_voltage_threshold(scaled_value, cfg->vrange_set);
		return ltc4286_cmd_write(dev, LTC4286_CMD_VIN_LOWER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_AMBIENT_TEMP:
		code = ltc4286_calc_temperature_threshold(scaled_value);
		return ltc4286_cmd_write(dev, LTC4286_CMD_TEMP_LOWER_THRES_WARN, (uint8_t *)&code,
					 2);
	default:
		return -EINVAL;
	}
}

/**
 * @brief Sets the lower threshold for a channel
 *
 * @param dev Pointer to the device structure to set data to
 * @param chan Sensor channel to set upper threshold to
 * @param val Pointer to the sensor value to set as the channel's threshold
 */
static int ltc4286_set_upper_threshold(const struct device *dev, enum sensor_channel chan,
				       const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int16_t scaled_value = (val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;

	switch (chan) {
	case SENSOR_CHAN_VOLTAGE:
		code = ltc4286_calc_voltage_threshold(scaled_value, cfg->vrange_set);
		return ltc4286_cmd_write(dev, LTC4286_CMD_VOUT_UPPER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_LTC4286_VIN:
		code = ltc4286_calc_voltage_threshold(scaled_value, cfg->vrange_set);
		return ltc4286_cmd_write(dev, LTC4286_CMD_VIN_UPPER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_CURRENT:
		code = ltc4286_calc_current_threshold(scaled_value);
		return ltc4286_cmd_write(dev, LTC4286_CMD_IOUT_UPPER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_POWER:
		code = ltc4286_calc_power_threshold(scaled_value, cfg->vrange_set);
		return ltc4286_cmd_write(dev, LTC4286_CMD_PIN_UPPER_THRES_WARN, (uint8_t *)&code,
					 2);
	case SENSOR_CHAN_AMBIENT_TEMP:
		code = ltc4286_calc_temperature_threshold(scaled_value);
		return ltc4286_cmd_write(dev, LTC4286_CMD_TEMP_UPPER_THRES_WARN, (uint8_t *)&code,
					 2);
	default:
		return -EINVAL;
	}
}

/**
 * @brief Sets an attribute on the device instance's channel
 *
 * @param dev Pointer to the device structure to retrieve data from
 * @param chan Sensor channel to set
 * @param attr Sensor attribute selected to set
 * @param val Pointer to the sensor value to set on the device
 */
static int ltc4286_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	if (!val) {
		return -EINVAL;
	}

	switch (attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		return ltc4286_set_upper_threshold(dev, chan, val);
	case SENSOR_ATTR_LOWER_THRESH:
		return ltc4286_set_lower_threshold(dev, chan, val);
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * @brief Configures the device GPIO pin usage and settings on the LTC4286
 *
 * @param dev Device to get configuration parameters from
 * @param gpios
 */
static int ltc4286_cfg_input_inv_gpios(const struct device *dev,
				       const struct ltc4286_gpio_config *gpios, uint8_t gpios_len)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;
	uint8_t cmd, i;
	int ret;

	cmd = LTC4286_CMD_MFR_GPIO_INV;
	ret = ltc4286_cmd_read(dev, &cmd, 1, val_ptr, 2);
	if (ret) {
		return ret;
	}

	for (i = 0; i < gpios_len; i++) {
		if (!gpio_is_ready_dt(gpios[i].gpio_spec)) {
			continue;
		}

		/* configure polarity of gpio pins on device based on gpio's polarity */
		val &= ~BIT(gpios[i].dev_gpio_pin);
		val |= FIELD_PREP(BIT(gpios[i].dev_gpio_pin),
				  (gpios[i].gpio_spec->dt_flags & GPIO_ACTIVE_LOW));

		ret = gpio_pin_configure_dt(gpios[i].gpio_spec, gpios[i].flags);
		if (ret) {
			LOG_ERR("Failed to configure GPIO pin\n");
			return ret;
		}
	}

	val |= FIELD_PREP(LTC4286_CFG_REBOOT_INV, cfg->dev_gpio_reboot_inv);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPIO_INV, val_ptr, 2);
	if (ret) {
		return ret;
	}

	/* GPIO input configurations */
	val = FIELD_PREP(LTC4286_CFG_GPI_RBT_EN, cfg->dev_gpio_reboot_en) |
	      FIELD_PREP(LTC4286_CFG_GPI_RBT_PIN, cfg->dev_gpio_reboot_pin) |
	      FIELD_PREP(LTC4286_CFG_GPI_CMP_SEL, cfg->dev_gpio_cmp_in_pin);

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPI_SEL, val_ptr, 2);
}

/**
 * @brief Configures the LTC4286 ADC settings
 *
 * @param dev Device to get configuration parameters from
 */
static int ltc4286_cfg_adc_sel(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;

	val = FIELD_PREP(LTC4286_CFG_DISP_AVG, cfg->avg_read_en) |
	      FIELD_PREP(LTC4286_CFG_ADC_AVG_SEL, cfg->avg_samples_cfg);

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_AVG_SEL, &val, 1);
}

static int ltc4286_cfg_adc_config(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;

	val = FIELD_PREP(LTC4286_CFG_VDS_SEL, cfg->vds_aux_en) |
	      FIELD_PREP(LTC4286_CFG_VIN_VOUT, cfg->vin_vout_en);

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_ADC_CONFIG, &val, 1);
}

/**
 * @brief Configures the LTC4286 device MFR1 settings
 *
 * @param dev Device to get configuration parameters from
 */
static int ltc4286_cfg_mfr1(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;

	val = FIELD_PREP(LTC4286_CFG_CURRENT_LIMIT, cfg->current_limit_cfg) |
	      FIELD_PREP(LTC4286_CFG_VRANGE_SEL, cfg->vrange_set) |
	      FIELD_PREP(LTC4286_CFG_VPWR_SEL, cfg->vpower_set) | LTC4286_CFG_CONFIG1_RSVD;

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_CONFIG1, val_ptr, 2);
}

/**
 * @brief Configures the LTC4286 device MFR2 settings
 *
 * @param dev Device to get configuration parameters from
 */
static int ltc4286_cfg_mfr2(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;

	val = FIELD_PREP(LTC4286_CFG_PMBUS_1MBPS, cfg->pmbus_1mbps_en) |
	      FIELD_PREP(LTC4286_CFG_RESET_FAULT_EN, cfg->reset_fault_en) |
	      FIELD_PREP(LTC4286_CFG_PWRGD_RESET, cfg->pg_latch_rst_ctrl) |
	      FIELD_PREP(LTC4286_CFG_MASS_WR_EN, cfg->mass_write_en) |
	      FIELD_PREP(LTC4286_CFG_EXT_TEMP_EN, cfg->ext_temp_en) |
	      FIELD_PREP(LTC4286_CFG_DEB_TMR_EN, cfg->deb_tmr_en) | LTC4286_CFG_CONFIG2_RSVD;

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_CONFIG2, val_ptr, 2);
}

/**
 * @brief Configures the LTC4286 device reboot settings
 *
 * @param dev Device to get configuration parameters from
 */
static int ltc4286_cfg_reboot(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;

	val = FIELD_PREP(LTC4286_CFG_REBOOT_INIT, cfg->reboot_init_fet_only) |
	      FIELD_PREP(LTC4286_CFG_REBOOT_DELAY, cfg->reboot_delay_cfg);

	return ltc4286_cmd_write(dev, LTC4286_CMD_MFR_REBOOT_CONTROL, &val, 1);
}

/**
 * @brief Configures the LTC4286 device MFR2 setting
 *
 * @param dev Device to get configuration parameters from
 */
static int ltc4286_cfg_fault_response(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int ret;
	uint16_t op_val;
	uint8_t *op_val_ptr = (uint8_t *)&op_val;
	uint8_t val;

	/* Overcurrent */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overcurr_config.no_ignore ? LTC4286_FAULT_RESP_TYP3
									 : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overcurr_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE, &val, 1);
	if (ret) {
		return ret;
	}

	/* Overtemperature */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overtemp_config.no_ignore ? LTC4286_FAULT_RESP_TYP2
									 : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overtemp_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_OT_FAULT_RESPONSE, &val, 1);
	if (ret) {
		return ret;
	}

	/* Vin Overvoltage */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overvolt_config.no_ignore ? LTC4286_FAULT_RESP_TYP2
									 : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overvolt_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE, &val, 1);
	if (ret) {
		return ret;
	}
	/* Vin Undervoltage */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.undervolt_config.no_ignore ? LTC4286_FAULT_RESP_TYP2
									  : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.undervolt_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE, &val, 1);
	if (ret) {
		return ret;
	}

	/* FET Faults */
	val = FIELD_PREP(
		      LTC4286_FAULT_RESPONSE_MASK,
		      (cfg->fault_responses.fet_config.no_ignore ? LTC4286_FAULT_RESP_TYP1 : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.fet_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_FET_FAULT_RESPONSE, &val, 1);
	if (ret) {
		return ret;
	}

	/* Overpower */
	op_val = FIELD_PREP(GENMASK(15, 5), cfg->op_tmr) |
		 FIELD_PREP(GENMASK(4, 3), (cfg->fault_responses.overpower_config.no_ignore
						    ? LTC4286_FAULT_RESP_TYP1
						    : 0)) |
		 FIELD_PREP(GENMASK(2, 0), cfg->fault_responses.overpower_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, op_val_ptr, 2);
	if (ret) {
		return ret;
	}

	return 0;
}

/* Helper function to lookup GPIO pin corresponding to a function */
static int ltc4286_get_gpio_pin_output(const struct device *dev,
				       const enum ltc4286_gpo_mode target_mode)
{
	const struct ltc4286_dev_config *cfg = dev->config;

	for (int i = 0; i < LTC4286_NUM_GPIOS; i++) {
		if (cfg->gpo_pin_output_select[i] == target_mode) {
			return i;
		}
	}

	return -EINVAL;
}

/**
 * @brief Initializes the device configuration of GPIO and operation
 *
 * @param dev Device instance to get configuration data from
 */
static int ltc4286_init(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	const struct ltc4286_gpio_config gpio_configs[] = {
		{.gpio_spec = &cfg->gpios.overpower_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_P_IN_OP),
		 .flags = GPIO_INPUT},
		{.gpio_spec = &cfg->gpios.overcurrent_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_I_OUT_OC),
		 .flags = GPIO_INPUT},
		{.gpio_spec = &cfg->gpios.fault_out_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_FAULT),
		 .flags = GPIO_INPUT},
		{.gpio_spec = &cfg->gpios.power_good_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_PWR_GOOD),
		 .flags = GPIO_INPUT},
		{.gpio_spec = &cfg->gpios.comp_out_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_CMPOUT),
		 .flags = GPIO_INPUT},
		{.gpio_spec = &cfg->gpios.alert_gpio,
		 .dev_gpio_pin = ltc4286_get_gpio_pin_output(dev, LTC4286_GPO_ALERT),
		 .flags = GPIO_INPUT}};
	int ret;
	uint16_t data;
	uint8_t *data_ptr = (uint8_t *)&data;
	uint8_t val;

	if (!i2c_is_ready_dt(&cfg->i2c_dev)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	ret = ltc4286_cfg_input_inv_gpios(dev, gpio_configs, ARRAY_SIZE(gpio_configs));
	if (ret) {
		return ret;
	}

#ifdef CONFIG_LTC4286_TRIGGER
	ret = ltc4286_init_interrupts(dev);
	if (ret) {
		return ret;
	}
#endif /* CONFIG_LTC4286_TRIGGER */

	ret = ltc4286_cfg_adc_config(dev);
	if (ret) {
		return ret;
	}

	ret = ltc4286_cfg_adc_sel(dev);
	if (ret) {
		return ret;
	}

	ret = ltc4286_cfg_mfr1(dev);
	if (ret) {
		return ret;
	}

	ret = ltc4286_cfg_mfr2(dev);
	if (ret) {
		return ret;
	}

	ret = ltc4286_cfg_reboot(dev);
	if (ret) {
		return ret;
	}

	ret = ltc4286_cfg_fault_response(dev);
	if (ret) {
		return ret;
	}

	/* configure GPIO pins */
	data_ptr[0] = FIELD_PREP(LTC4286_CFG_GPO_5_1, cfg->gpo_pin_output_select[0]) |
		      FIELD_PREP(LTC4286_CFG_GPO_6_2, cfg->gpo_pin_output_select[1]);
	data_ptr[1] = FIELD_PREP(LTC4286_CFG_GPO_7_3, cfg->gpo_pin_output_select[2]) |
		      FIELD_PREP(LTC4286_CFG_GPO_8_4, cfg->gpo_pin_output_select[3]);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPO_SEL41, data_ptr, 2);
	if (ret) {
		return ret;
	}

	data_ptr[0] = FIELD_PREP(LTC4286_CFG_GPO_5_1, cfg->gpo_pin_output_select[4]) |
		      FIELD_PREP(LTC4286_CFG_GPO_6_2, cfg->gpo_pin_output_select[5]);
	data_ptr[1] = FIELD_PREP(LTC4286_CFG_GPO_7_3, cfg->gpo_pin_output_select[6]) |
		      FIELD_PREP(LTC4286_CFG_GPO_8_4, cfg->gpo_pin_output_select[7]);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPO_SEL85, data_ptr, 2);
	if (ret) {
		return ret;
	}

	/* activate masks for alerts */
	static const uint8_t alert_masks[] = {
		LTC4286_CMD_ALERT_MASK_VOUT, LTC4286_CMD_ALERT_MASK_IOUT,
		LTC4286_CMD_ALERT_MASK_INPUT, LTC4286_CMD_ALERT_MASK_TEMP};

	data_ptr[1] = 0;
	for (size_t i = 0; i < ARRAY_SIZE(alert_masks); i++) {
		data_ptr[0] = alert_masks[i];
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_ALERT_MASK, data_ptr, 2);
		if (ret) {
			return ret;
		}
	}

	/* protect from future writes if set */
	val = FIELD_PREP(LTC4286_CFG_WRITE_PROTECT_1, cfg->write_protect_en);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_WRITE_PROTECT, &val, 1);
	if (ret) {
		return ret;
	}

	return 0;
}

static DEVICE_API(sensor, ltc4286_driver_api) = {
	.channel_get = ltc4286_channel_get,
	.attr_set = ltc4286_attr_set,
	.sample_fetch = ltc4286_sample_fetch,
#ifdef CONFIG_LTC4286_TRIGGER
	.trigger_set = ltc4286_trigger_set,
#endif /* CONFIG_LTC4286_TRIGGER */
};

#define LTC4286_DEFINE(inst)                                                                       \
	static struct ltc4286_data ltc4286_data_##inst;                                            \
                                                                                                   \
	static const struct ltc4286_dev_config ltc4286_config_##inst = {                           \
		.i2c_dev = I2C_DT_SPEC_INST_GET(inst),                                             \
		.r_sense_uohms = DT_INST_PROP(inst, rsense_uohms),                                 \
		.avg_samples_cfg = DT_INST_ENUM_IDX(inst, average_samples_set),                    \
		.op_tmr = DT_INST_PROP(inst, overpower_fault_timer),                               \
		.current_limit_cfg = DT_INST_PROP(inst, current_limit_set),                        \
		.reboot_delay_cfg = DT_INST_PROP(inst, reboot_delay_config),                       \
		.write_protect_en = DT_INST_PROP(inst, write_protect_enable),                      \
		.vds_aux_en = DT_INST_PROP(inst, vds_enable),                                      \
		.vin_vout_en = DT_INST_PROP(inst, vin_vout_enable),                                \
		.avg_read_en = DT_INST_PROP(inst, display_averaged_read),                          \
		.vrange_set = DT_INST_PROP(inst, vrange_select),                                   \
		.vpower_set = DT_INST_PROP(inst, vpower_select),                                   \
		.pmbus_1mbps_en = DT_INST_PROP(inst, pmbus_1mbit_enable),                          \
		.reset_fault_en = DT_INST_PROP(inst, reset_fault_enable),                          \
		.pg_latch_rst_ctrl = DT_INST_PROP(inst, power_grid_reset_control_set),             \
		.mass_write_en = DT_INST_PROP(inst, mass_write_enable),                            \
		.ext_temp_en = DT_INST_PROP(inst, external_temp_sensor_enable),                    \
		.deb_tmr_en = DT_INST_PROP(inst, debounce_timer_enable),                           \
		.reboot_init_fet_only = DT_INST_PROP(inst, reboot_init_fet_only),                  \
		.dev_gpio_reboot_en = DT_INST_PROP(inst, dev_gpio_reboot_enable),                  \
		.dev_gpio_reboot_pin = DT_INST_PROP(inst, dev_gpio_reboot_pin),                    \
		.dev_gpio_reboot_inv = DT_INST_PROP(inst, dev_gpio_reboot_inv),                    \
		.dev_gpio_cmp_in_pin = DT_INST_PROP(inst, dev_gpio_cmp_input_pin),                 \
		.fault_responses =                                                                 \
			{                                                                          \
				.overcurr_config =                                                 \
					{                                                          \
						.retries = DT_INST_PROP(                           \
							inst, overcurrent_fault_retries),          \
						.no_ignore = DT_INST_PROP(                         \
							inst,                                      \
							overcurrent_fault_response_shutdown),      \
					},                                                         \
				.overvolt_config =                                                 \
					{                                                          \
						.retries = DT_INST_PROP(inst,                      \
									overvolt_fault_retries),   \
						.no_ignore = DT_INST_PROP(                         \
							inst, overvolt_fault_response_shutdown),   \
					},                                                         \
				.undervolt_config =                                                \
					{                                                          \
						.retries = DT_INST_PROP(inst,                      \
									undervolt_fault_retries),  \
						.no_ignore = DT_INST_PROP(                         \
							inst, undervolt_fault_response_shutdown),  \
					},                                                         \
				.overtemp_config =                                                 \
					{                                                          \
						.retries = DT_INST_PROP(inst,                      \
									overtemp_fault_retries),   \
						.no_ignore = DT_INST_PROP(                         \
							inst, overtemp_fault_response_shutdown),   \
					},                                                         \
				.overpower_config =                                                \
					{                                                          \
						.retries = DT_INST_PROP(inst,                      \
									overpower_fault_retries),  \
						.no_ignore = DT_INST_PROP(                         \
							inst, overpower_fault_response_shutdown),  \
					},                                                         \
				.fet_config =                                                      \
					{                                                          \
						.retries = DT_INST_PROP(inst, fet_fault_retries),  \
						.no_ignore = DT_INST_PROP(inst,                    \
									  fet_fault_response_bad), \
					},                                                         \
			},                                                                         \
		.gpios =                                                                           \
			{                                                                          \
				.reboot_gpio = GPIO_DT_SPEC_GET_OR(inst, reboot_gpios, {0}),       \
				.overpower_gpio = GPIO_DT_SPEC_GET_OR(inst, op_status_gpios, {0}), \
				.overcurrent_gpio =                                                \
					GPIO_DT_SPEC_GET_OR(inst, oc_status_gpios, {0}),           \
				.comp_out_gpio = GPIO_DT_SPEC_GET_OR(inst, comp_out_gpios, {0}),   \
				.fault_out_gpio = GPIO_DT_SPEC_GET_OR(inst, fault_out_gpios, {0}), \
				.power_good_gpio =                                                 \
					GPIO_DT_SPEC_GET_OR(inst, power_good_gpios, {0}),          \
				.alert_gpio = GPIO_DT_SPEC_GET_OR(inst, alert_gpios, {0}),         \
			},                                                                         \
		.gpo_pin_output_select[0] = DT_INST_PROP(inst, gpo1_output_select),                \
		.gpo_pin_output_select[1] = DT_INST_PROP(inst, gpo2_output_select),                \
		.gpo_pin_output_select[2] = DT_INST_PROP(inst, gpo3_output_select),                \
		.gpo_pin_output_select[3] = DT_INST_PROP(inst, gpo4_output_select),                \
		.gpo_pin_output_select[4] = DT_INST_PROP(inst, gpo5_output_select),                \
		.gpo_pin_output_select[5] = DT_INST_PROP(inst, gpo6_output_select),                \
		.gpo_pin_output_select[6] = DT_INST_PROP(inst, gpo7_output_select),                \
		.gpo_pin_output_select[7] = DT_INST_PROP(inst, gpo8_output_select),                \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ltc4286_init, NULL, &ltc4286_data_##inst,               \
				     &ltc4286_config_##inst, POST_KERNEL,                          \
				     CONFIG_SENSOR_INIT_PRIORITY, &ltc4286_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LTC4286_DEFINE)
