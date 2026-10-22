/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_ltc4286

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#include "ltc4286.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LTC4286, CONFIG_SENSOR_LOG_LEVEL);

int ltc4286_cmd_read(const struct device *dev, uint8_t *code, uint8_t code_size, uint8_t *val,
		     uint8_t val_size)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int ret;

	/* Sends a PMBus command and then reads from the device */
	if (!val) {
		return -EINVAL;
	}

	ret = i2c_write_read_dt(&cfg->i2c_dev, code, code_size, val, val_size);
	if (ret) {
		LOG_ERR("Failed to read from device (%d)", ret);
		return ret;
	}

	return 0;
}

int ltc4286_cmd_write(const struct device *dev, uint8_t code, uint8_t *val, uint8_t val_size)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t payload[4];
	uint8_t payload_size = val_size + 1;
	int ret;

	payload[0] = code;
	if (val) {
		if (payload_size > sizeof(payload)) {
			return -EINVAL;
		}

		memcpy(&payload[1], val, val_size);
	}

	ret = i2c_write_dt(&cfg->i2c_dev, payload, payload_size);
	if (ret) {
		LOG_ERR("Failed to write to device (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cmd_update(const struct device *dev, uint8_t cmd, uint16_t mask, uint16_t value,
			      uint8_t size)
{
	uint16_t reg_val = 0;
	uint8_t cmd_buf = cmd;
	uint8_t *reg_ptr = (uint8_t *)&reg_val;
	int ret;

	ret = ltc4286_cmd_read(dev, &cmd_buf, 1, reg_ptr, size);
	if (ret) {
		LOG_ERR("Failed to read register 0x%02X (%d)", cmd, ret);
		return ret;
	}

	reg_val &= ~mask;
	reg_val |= FIELD_PREP(mask, value);
	reg_val = sys_cpu_to_le16(reg_val);

	ret = ltc4286_cmd_write(dev, cmd, reg_ptr, size);
	if (ret) {
		LOG_ERR("Failed to write register 0x%02X (%d)", cmd, ret);
		return ret;
	}

	return 0;
}

static int ltc4286_read_reg16(const struct device *dev, uint8_t cmd, uint16_t *dest)
{
	uint16_t value = 0;
	int ret;

	ret = ltc4286_cmd_read(dev, &cmd, 1, (uint8_t *)&value, 2);
	if (ret) {
		LOG_ERR("Failed to read register 0x%02X (%d)", cmd, ret);
		return ret;
	}

	*dest = sys_le16_to_cpu(value);
	return 0;
}

static int ltc4286_read_reg8(const struct device *dev, uint8_t cmd, uint8_t *dest)
{
	uint8_t value = 0;
	int ret;

	ret = ltc4286_cmd_read(dev, &cmd, 1, &value, 1);
	if (ret) {
		LOG_ERR("Failed to read register 0x%02X (%d)", cmd, ret);
		return ret;
	}

	*dest = value;
	return 0;
}

static int ltc4286_fetch_telemetry(const struct device *dev)
{
	struct ltc4286_data *d = dev->data;
	int ret;

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_READ_VOUT, &d->vout_sample);
	if (ret) {
		LOG_ERR("Failed to fetch VOUT (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_READ_TEMP, &d->temp_sample);
	if (ret) {
		LOG_ERR("Failed to fetch temperature (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_READ_IOUT, &d->iout_sample);
	if (ret) {
		LOG_ERR("Failed to fetch IOUT (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_READ_PIN, &d->pin_sample);
	if (ret) {
		LOG_ERR("Failed to fetch PIN (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_READ_VIN, &d->vin_sample);
	if (ret) {
		LOG_ERR("Failed to fetch VIN (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_fetch_status(const struct device *dev)
{
	struct ltc4286_data *d = dev->data;
	int ret;

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_STATUS_WORD, &d->status_flags_sample);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_WORD (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_VOUT, &d->status_flags_vout);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_VOUT (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_IOUT, &d->status_flags_iout);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_IOUT (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_INPUT, &d->status_flags_input);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_INPUT (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_TEMPERATURE, &d->status_flags_temp);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_TEMPERATURE (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_CML, &d->status_flags_cml);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_CML (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_OTHER, &d->status_flags_other);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_OTHER (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_MFR_SPECIFIC, &d->status_flags_mfr_spec);
	if (ret) {
		LOG_ERR("Failed to fetch STATUS_MFR_SPECIFIC (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_SYSTEM_STATUS1,
				 &d->status_flags_mfr_sys_status1);
	if (ret) {
		LOG_ERR("Failed to fetch MFR_SYSTEM_STATUS1 (%d)", ret);
		return ret;
	}

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_SYSTEM_STATUS2,
				 &d->status_flags_mfr_sys_status2);
	if (ret) {
		LOG_ERR("Failed to fetch MFR_SYSTEM_STATUS2 (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ltc4286_data *drv_data = dev->data;
	int ret;

	switch ((int)chan) {
	case SENSOR_CHAN_ALL:
		ret = ltc4286_fetch_telemetry(dev);
		if (ret) {
			return ret;
		}
		return ltc4286_fetch_status(dev);
	case SENSOR_CHAN_VOLTAGE:
		return ltc4286_read_reg16(dev, LTC4286_CMD_READ_VOUT, &drv_data->vout_sample);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return ltc4286_read_reg16(dev, LTC4286_CMD_READ_TEMP, &drv_data->temp_sample);
	case SENSOR_CHAN_CURRENT:
		return ltc4286_read_reg16(dev, LTC4286_CMD_READ_IOUT, &drv_data->iout_sample);
	case SENSOR_CHAN_POWER:
		return ltc4286_read_reg16(dev, LTC4286_CMD_READ_PIN, &drv_data->pin_sample);
	case SENSOR_CHAN_LTC4286_VIN:
		return ltc4286_read_reg16(dev, LTC4286_CMD_READ_VIN, &drv_data->vin_sample);
	case SENSOR_CHAN_LTC4286_STATUS:
		return ltc4286_read_reg16(dev, LTC4286_CMD_STATUS_WORD,
					  &drv_data->status_flags_sample);
	case SENSOR_CHAN_LTC4286_STATUS_VOUT:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_VOUT,
					 &drv_data->status_flags_vout);
	case SENSOR_CHAN_LTC4286_STATUS_IOUT:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_IOUT,
					 &drv_data->status_flags_iout);
	case SENSOR_CHAN_LTC4286_STATUS_INPUT:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_INPUT,
					 &drv_data->status_flags_input);
	case SENSOR_CHAN_LTC4286_STATUS_TEMP:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_TEMPERATURE,
					 &drv_data->status_flags_temp);
	case SENSOR_CHAN_LTC4286_STATUS_CML:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_CML, &drv_data->status_flags_cml);
	case SENSOR_CHAN_LTC4286_STATUS_OTHER:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_OTHER,
					 &drv_data->status_flags_other);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SPEC:
		return ltc4286_read_reg8(dev, LTC4286_CMD_STATUS_MFR_SPECIFIC,
					 &drv_data->status_flags_mfr_spec);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS1:
		return ltc4286_read_reg16(dev, LTC4286_CMD_MFR_SYSTEM_STATUS1,
					  &drv_data->status_flags_mfr_sys_status1);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS2:
		return ltc4286_read_reg16(dev, LTC4286_CMD_MFR_SYSTEM_STATUS2,
					  &drv_data->status_flags_mfr_sys_status2);
	default:
		return -ENOTSUP;
	}
}

static void ltc4286_convert_voltage_value(uint16_t sample, bool vrange_set,
					  struct sensor_value *val)
{
	uint64_t data = (uint64_t)sample * LTC4286_ADC_VAL_TO_UVAL;

	if (vrange_set) {
		data = (data * LTC4286_ADC_VIN_VOUT_100V_FACTOR) / LTC4286_ADC_VIN_VOUT_100V_SCALE;
	} else {
		data = (data * LTC4286_ADC_VIN_VOUT_25V_FACTOR) / LTC4286_ADC_VIN_VOUT_25V_SCALE;
	}

	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

static void ltc4286_convert_power_value(uint16_t sample, bool vrange_set, uint32_t r_sense_uohms,
					struct sensor_value *val)
{
	uint64_t data = (uint64_t)sample * LTC4286_ADC_VAL_TO_UVAL;

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

static void ltc4286_convert_current_value(uint16_t sample, uint32_t r_sense_uohms,
					  struct sensor_value *val)
{
	uint64_t data = ((uint64_t)sample * LTC4286_ADC_VAL_TO_UVAL);

	data = (data * LTC4286_ADC_IOUT_FACTOR) / r_sense_uohms;
	data = data / LTC4286_ADC_IOUT_SCALE;

	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

static void ltc4286_convert_temperature_value(uint16_t sample, struct sensor_value *val)
{
	uint64_t data = ((uint64_t)sample * LTC4286_ADC_TEMP_SCALE) - LTC4286_ADC_TEMP_K_TO_C;

	data = data * LTC4286_ADC_VAL_TO_UVAL / LTC4286_ADC_TEMP_SCALE;

	val->val1 = data / LTC4286_ADC_VAL_TO_UVAL;
	val->val2 = data % LTC4286_ADC_VAL_TO_UVAL;
}

static int ltc4286_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	const struct ltc4286_data *drv_data = dev->data;
	int64_t data;

	if (!val) {
		return -EINVAL;
	}

	val->val2 = 0;

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		ltc4286_convert_voltage_value(drv_data->vout_sample, cfg->vrange_set, val);
		break;
	case SENSOR_CHAN_LTC4286_VIN:
		ltc4286_convert_voltage_value(drv_data->vin_sample, cfg->vrange_set, val);
		break;
	case SENSOR_CHAN_CURRENT:
		ltc4286_convert_current_value(drv_data->iout_sample, cfg->r_sense_uohms, val);
		break;
	case SENSOR_CHAN_POWER:
		ltc4286_convert_power_value(drv_data->pin_sample, cfg->vrange_set,
					    cfg->r_sense_uohms, val);
		break;
	case SENSOR_CHAN_AMBIENT_TEMP:
		ltc4286_convert_temperature_value(drv_data->temp_sample, val);
		break;
	case SENSOR_CHAN_LTC4286_STATUS:
		data = drv_data->status_flags_sample;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_VOUT:
		data = drv_data->status_flags_vout;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_IOUT:
		data = drv_data->status_flags_iout;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_INPUT:
		data = drv_data->status_flags_input;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_TEMP:
		data = drv_data->status_flags_temp;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_CML:
		data = drv_data->status_flags_cml;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_OTHER:
		data = drv_data->status_flags_other;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SPEC:
		data = drv_data->status_flags_mfr_spec;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS1:
		data = drv_data->status_flags_mfr_sys_status1;
		val->val1 = data;
		break;
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS2:
		data = drv_data->status_flags_mfr_sys_status2;
		val->val1 = data;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int ltc4286_config_attr_set(const struct device *dev, enum sensor_attribute attr,
				   const struct sensor_value *val)
{
	struct ltc4286_data *data = dev->data;
	uint8_t reg_val = val->val1;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_CONFIG_OPERATION:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_OPERATION, &reg_val, 1);
		if (ret) {
			LOG_ERR("Failed to set operating mode (%d)", ret);
			return ret;
		}

		return 0;
	case SENSOR_ATTR_LTC4286_CONFIG_CLEAR_FAULTS:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_CLEAR_FAULTS, NULL, 0);
		if (ret) {
			LOG_ERR("Failed to clear faults (%d)", ret);
			return ret;
		}

		return 0;
	case SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_WRITE_PROTECT, &reg_val, 1);
		if (ret) {
			LOG_ERR("Failed to set write protect (%d)", ret);
			return ret;
		}
		data->is_write_protect = !!(reg_val);

		return 0;
	default:
		return -ENOTSUP;
	}
}

static int ltc4286_config_attr_get(const struct device *dev, enum sensor_attribute attr,
				   struct sensor_value *val)
{
	struct ltc4286_data *data = dev->data;
	uint8_t reg_addr, reg_val;
	int ret;

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_CONFIG_OPERATION:
		reg_addr = LTC4286_CMD_OPERATION;
		ret = ltc4286_read_reg8(dev, reg_addr, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read operation register (%d)", ret);
			return ret;
		}
		val->val1 = reg_val;
		return 0;
	case SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT:
		val->val1 = data->is_write_protect;

		return 0;
	default:
		return -ENOTSUP;
	}
}

static uint16_t ltc4286_convert_voltage_code(int64_t scaled_value, bool vrange_set)
{
	uint16_t result;

	if (vrange_set) {
		result = (uint16_t)(scaled_value * LTC4286_ADC_VIN_VOUT_100V_SCALE /
				    (LTC4286_ADC_VIN_VOUT_100V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
		return sys_cpu_to_le16(result);
	}

	result = (uint16_t)(scaled_value * LTC4286_ADC_VIN_VOUT_25V_SCALE /
			((int64_t)LTC4286_ADC_VIN_VOUT_25V_FACTOR *
			LTC4286_ADC_VAL_TO_UVAL));
	return sys_cpu_to_le16(result);
}

static uint16_t ltc4286_convert_power_code(int64_t scaled_value, bool vrange_set,
					   uint32_t r_sense_uohms)
{
	int64_t code;
	uint16_t result;

	if (vrange_set) {
		code = scaled_value * LTC4286_ADC_PIN_100V_SCALE * r_sense_uohms;
		result = (uint16_t)(code / (LTC4286_ADC_PIN_100V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
		return sys_cpu_to_le16(result);
	}

	code = scaled_value * LTC4286_ADC_PIN_25V_SCALE * r_sense_uohms;
	result = (uint16_t)(code / (LTC4286_ADC_PIN_25V_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	return sys_cpu_to_le16(result);
}

static uint16_t ltc4286_convert_current_code(int64_t scaled_value, uint32_t r_sense_uohms)
{
	int64_t code = scaled_value * LTC4286_ADC_IOUT_SCALE * r_sense_uohms;
	uint16_t result;

	result = (uint16_t)(code / ((int64_t)LTC4286_ADC_IOUT_FACTOR * LTC4286_ADC_VAL_TO_UVAL));
	return sys_cpu_to_le16(result);
}

static uint16_t ltc4286_convert_temperature_code(int64_t scaled_value)
{
	int64_t data = scaled_value * LTC4286_ADC_TEMP_SCALE / LTC4286_ADC_VAL_TO_UVAL;
	uint16_t result;

	result = (uint16_t)((data + LTC4286_ADC_TEMP_K_TO_C) / LTC4286_ADC_TEMP_SCALE);
	return sys_cpu_to_le16(result);
}

static int ltc4286_vout_attr_get(const struct device *dev, enum sensor_attribute attr,
				 struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t code = 0;
	uint8_t cmd;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		cmd = LTC4286_CMD_VOUT_UPPER_THRES_WARN;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		cmd = LTC4286_CMD_VOUT_LOWER_THRES_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_read_reg16(dev, cmd, &code);
	if (ret) {
		LOG_ERR("Failed to read VOUT threshold (%d)", ret);
		return ret;
	}

	ltc4286_convert_voltage_value(code, cfg->vrange_set, val);
	return 0;
}

static int ltc4286_vout_attr_set(const struct device *dev, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int64_t scaled_value = ((int64_t)val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;
	uint8_t *code_ptr = (uint8_t *)&code;
	int ret;

	code = ltc4286_convert_voltage_code(scaled_value, cfg->vrange_set);

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_VOUT_UPPER_THRES_WARN, code_ptr, 2);
		if (ret) {
			LOG_ERR("Failed to set voltage upper threshold (%d)", ret);
			return ret;
		}

		return 0;
	case SENSOR_ATTR_LOWER_THRESH:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_VOUT_LOWER_THRES_WARN, code_ptr, 2);
		if (ret) {
			LOG_ERR("Failed to set voltage lower threshold (%d)", ret);
			return ret;
		}

		return 0;
	default:
		return -EINVAL;
	}
}

static int ltc4286_vin_attr_get(const struct device *dev, enum sensor_attribute attr,
				struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t code = 0;
	uint8_t reg_val;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_VIN_UPPER_THRES_WARN, &code);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		ltc4286_convert_voltage_value(code, cfg->vrange_set, val);
		return 0;
	case SENSOR_ATTR_LOWER_THRESH:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_VIN_LOWER_THRES_WARN, &code);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		ltc4286_convert_voltage_value(code, cfg->vrange_set, val);
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RESPONSE_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_OV_RETRY:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RETRY_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RESPONSE_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_UV_RETRY:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read VIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RETRY_MASK, reg_val);
		val->val2 = 0;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ltc4286_vin_attr_set(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int64_t scaled_value = ((int64_t)val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;
	uint8_t *code_ptr = (uint8_t *)&code;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		code = ltc4286_convert_voltage_code(scaled_value, cfg->vrange_set);
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_UPPER_THRES_WARN, code_ptr, 2);
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		code = ltc4286_convert_voltage_code(scaled_value, cfg->vrange_set);
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_LOWER_THRES_WARN, code_ptr, 2);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_OV_RESPONSE:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE,
					 LTC4286_FAULT_RESPONSE_MASK, val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_OV_RETRY:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE,
					 LTC4286_FAULT_RETRY_MASK, val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_UV_RESPONSE:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE,
					 LTC4286_FAULT_RESPONSE_MASK, val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_UV_RETRY:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE,
					 LTC4286_FAULT_RETRY_MASK, val->val1, 1);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		LOG_ERR("Failed to set VIN attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_iout_attr_get(const struct device *dev, enum sensor_attribute attr,
				 struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t code = 0;
	uint8_t reg_val;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_IOUT_UPPER_THRES_WARN, &code);
		if (ret) {
			LOG_ERR("Failed to read IOUT attribute (%d)", ret);
			return ret;
		}
		ltc4286_convert_current_value(code, cfg->r_sense_uohms, val);
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_RESPONSE:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read IOUT attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RESPONSE_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_RETRY:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read IOUT attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_RETRY_MASK, reg_val);
		val->val2 = 0;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ltc4286_iout_attr_set(const struct device *dev, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int64_t scaled_value = ((int64_t)val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;
	uint8_t *code_ptr = (uint8_t *)&code;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		code = ltc4286_convert_current_code(scaled_value, cfg->r_sense_uohms);
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_IOUT_UPPER_THRES_WARN, code_ptr, 2);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_RESPONSE:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE,
					 LTC4286_FAULT_RESPONSE_MASK, val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_RETRY:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE,
					 LTC4286_FAULT_RETRY_MASK, val->val1, 1);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		LOG_ERR("Failed to set IOUT attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_pin_attr_get(const struct device *dev, enum sensor_attribute attr,
				struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t code = 0;
	uint16_t reg_val;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_PIN_UPPER_THRES_WARN, &code);
		if (ret) {
			LOG_ERR("Failed to read PIN attribute (%d)", ret);
			return ret;
		}
		ltc4286_convert_power_value(code, cfg->vrange_set, cfg->r_sense_uohms, val);
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_RESPONSE:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read PIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_OP_RESPONSE_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_RETRY:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read PIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_OP_RETRY_MASK, reg_val);
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_OP_TIMER:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read PIN attribute (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_FAULT_OP_TIMER, reg_val);
		val->val2 = 0;
		return 0;
	default:
		return -EINVAL;
	}
}

static int ltc4286_pin_attr_set(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int64_t scaled_value = ((int64_t)val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;
	uint8_t *code_ptr = (uint8_t *)&code;
	uint16_t mask;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		code = ltc4286_convert_power_code(scaled_value, cfg->vrange_set,
						  cfg->r_sense_uohms);
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_PIN_UPPER_THRES_WARN, code_ptr, 2);
		if (ret) {
			LOG_ERR("Failed to set PIN attribute (%d)", ret);
			return ret;
		}

		return 0;
	case SENSOR_ATTR_LTC4286_FAULT_RESPONSE:
		mask = LTC4286_FAULT_OP_RESPONSE_MASK;
		break;
	case SENSOR_ATTR_LTC4286_FAULT_RETRY:
		mask = LTC4286_FAULT_OP_RETRY_MASK;
		break;
	case SENSOR_ATTR_LTC4286_FAULT_OP_TIMER:
		mask = LTC4286_FAULT_OP_TIMER;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, mask, val->val1, 2);
	if (ret) {
		LOG_ERR("Failed to set PIN attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_temp_attr_get(const struct device *dev, enum sensor_attribute attr,
				 struct sensor_value *val)
{
	uint16_t code = 0;
	uint8_t cmd;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		cmd = LTC4286_CMD_TEMP_UPPER_THRES_WARN;
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		cmd = LTC4286_CMD_TEMP_LOWER_THRES_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_read_reg16(dev, cmd, &code);
	if (ret) {
		LOG_ERR("Failed to read temperature threshold (%d)", ret);
		return ret;
	}

	ltc4286_convert_temperature_value(code, val);
	return 0;
}

static int ltc4286_temp_attr_set(const struct device *dev, enum sensor_attribute attr,
				 const struct sensor_value *val)
{
	int64_t scaled_value = ((int64_t)val->val1 * LTC4286_ADC_VAL_TO_UVAL) + val->val2;
	uint16_t code;
	uint8_t *code_ptr = (uint8_t *)&code;
	int ret;

	code = ltc4286_convert_temperature_code(scaled_value);

	switch ((int)attr) {
	case SENSOR_ATTR_UPPER_THRESH:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_TEMP_UPPER_THRES_WARN, code_ptr, 2);
		break;
	case SENSOR_ATTR_LOWER_THRESH:
		ret = ltc4286_cmd_write(dev, LTC4286_CMD_TEMP_LOWER_THRES_WARN, code_ptr, 2);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		LOG_ERR("Failed to set temperature attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_adc_attr_get(const struct device *dev, enum sensor_attribute attr,
				struct sensor_value *val)
{
	uint8_t reg_val;
	int ret;

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_ADC_VDS_SELECT:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_ADC_CONFIG, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read ADC config (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_CFG_VDS_SEL, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_ADC_CONFIG, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read ADC config (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_CFG_VIN_VOUT, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_ADC_DISP_AVG:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_AVG_SEL, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read ADC config (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_CFG_DISP_AVG, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_ADC_AVG_SELECT:
		ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_AVG_SEL, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read ADC config (%d)", ret);
			return ret;
		}
		val->val1 = FIELD_GET(LTC4286_CFG_ADC_AVG_SEL, reg_val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltc4286_adc_attr_set(const struct device *dev, enum sensor_attribute attr,
				const struct sensor_value *val)
{
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_ADC_VDS_SELECT:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_ADC_CONFIG, LTC4286_CFG_VDS_SEL,
					 val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_ADC_VIN_VOUT_SELECT:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_ADC_CONFIG, LTC4286_CFG_VIN_VOUT,
					 val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_ADC_DISP_AVG:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_AVG_SEL, LTC4286_CFG_DISP_AVG,
					 val->val1, 1);
		break;
	case SENSOR_ATTR_LTC4286_ADC_AVG_SELECT:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_AVG_SEL, LTC4286_CFG_ADC_AVG_SEL,
					 val->val1, 1);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		LOG_ERR("Failed to set ADC attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_mfr_config_attr_get(const struct device *dev, enum sensor_attribute attr,
				       struct sensor_value *val)
{
	uint16_t reg_val;
	int ret;

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_VPWR_SEL:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_CONFIG1, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read MFR config1 (%d)", ret);
			return ret;
		}

		if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM) {
			val->val1 = FIELD_GET(LTC4286_CFG_CURRENT_LIMIT, reg_val);
		} else if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL) {
			val->val1 = FIELD_GET(LTC4286_CFG_VRANGE_SEL, reg_val);
		} else {
			val->val1 = FIELD_GET(LTC4286_CFG_VPWR_SEL, reg_val);
		}

		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_RESET_FAULT:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_PWR_GOOD_RESET_CTRL:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_MASS_WRITE:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP:
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER:
		ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_CONFIG2, &reg_val);
		if (ret) {
			LOG_ERR("Failed to read MFR config2 (%d)", ret);
			return ret;
		}

		if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT) {
			val->val1 = FIELD_GET(LTC4286_CFG_PMBUS_1MBPS, reg_val);
		} else if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_RESET_FAULT) {
			val->val1 = FIELD_GET(LTC4286_CFG_RESET_FAULT_EN, reg_val);
		} else if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_PWR_GOOD_RESET_CTRL) {
			val->val1 = FIELD_GET(LTC4286_CFG_PWRGD_RESET, reg_val);
		} else if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_MASS_WRITE) {
			val->val1 = FIELD_GET(LTC4286_CFG_MASS_WR_EN, reg_val);
		} else if ((int)attr == SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP) {
			val->val1 = FIELD_GET(LTC4286_CFG_EXT_TEMP_EN, reg_val);
		} else {
			val->val1 = FIELD_GET(LTC4286_CFG_DEB_TMR_EN, reg_val);
		}

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltc4286_mfr_config_attr_set(const struct device *dev, enum sensor_attribute attr,
				       const struct sensor_value *val)
{
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_ILIM:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG1, LTC4286_CFG_CURRENT_LIMIT,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_VRANGE_SEL:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG1, LTC4286_CFG_VRANGE_SEL,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_VPWR_SEL:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG1, LTC4286_CFG_VPWR_SEL,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_PMBUS_1MBIT:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_PMBUS_1MBPS,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_RESET_FAULT:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_RESET_FAULT_EN,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_PWR_GOOD_RESET_CTRL:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_PWRGD_RESET,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_MASS_WRITE:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_MASS_WR_EN,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_EXT_TEMP:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_EXT_TEMP_EN,
					 val->val1, 2);
		break;
	case SENSOR_ATTR_LTC4286_MFR_CONFIG_DEBOUNCE_TIMER:
		ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_CONFIG2, LTC4286_CFG_DEB_TMR_EN,
					 val->val1, 2);
		break;
	default:
		return -EINVAL;
	}

	if (ret) {
		LOG_ERR("Failed to set MFR config attribute (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_fault_resp_fet_attr_get(const struct device *dev, enum sensor_attribute attr,
					   struct sensor_value *val)
{
	uint8_t reg_val;
	int ret;

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_FET_FAULT_RESPONSE, &reg_val);
	if (ret) {
		LOG_ERR("Failed to read FET fault response (%d)", ret);
		return ret;
	}

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_FAULT_RESPONSE:
		val->val1 = FIELD_GET(LTC4286_FAULT_RESPONSE_MASK, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_FAULT_RETRY:
		val->val1 = FIELD_GET(LTC4286_FAULT_RETRY_MASK, reg_val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltc4286_fault_resp_fet_attr_set(const struct device *dev, enum sensor_attribute attr,
					   const struct sensor_value *val)
{
	uint32_t ltc4286_attr = (uint32_t)attr;
	uint16_t mask = (ltc4286_attr == SENSOR_ATTR_LTC4286_FAULT_RESPONSE)
				? LTC4286_FAULT_RESPONSE_MASK
				: LTC4286_FAULT_RETRY_MASK;
	int ret;

	if (ltc4286_attr != SENSOR_ATTR_LTC4286_FAULT_RESPONSE &&
	    ltc4286_attr != SENSOR_ATTR_LTC4286_FAULT_RETRY) {
		return -EINVAL;
	}

	ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_FET_FAULT_RESPONSE, mask, val->val1, true);
	if (ret) {
		LOG_ERR("Failed to set FET fault response (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_mfr_flt_config_attr_get(const struct device *dev, enum sensor_attribute attr,
					   struct sensor_value *val)
{
	uint8_t reg_val;
	int ret;

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_FLT_CONFIG, &reg_val);
	if (ret) {
		LOG_ERR("Failed to read MFR fault config (%d)", ret);
		return ret;
	}

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_MFR_FLT_OP_TO_FAULT:
		val->val1 = FIELD_GET(LTC4286_FLT_CFG_OP_TO_FAULT, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_MFR_FLT_OT_TO_FAULT:
		val->val1 = FIELD_GET(LTC4286_FLT_CFG_OT_TO_FAULT, reg_val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltc4286_mfr_flt_config_attr_set(const struct device *dev, enum sensor_attribute attr,
					   const struct sensor_value *val)
{
	uint16_t mask;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_MFR_FLT_OP_TO_FAULT:
		mask = LTC4286_FLT_CFG_OP_TO_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_MFR_FLT_OT_TO_FAULT:
		mask = LTC4286_FLT_CFG_OT_TO_FAULT;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_FLT_CONFIG, mask, val->val1, 1);
	if (ret) {
		LOG_ERR("Failed to set MFR fault config (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_mfr_reboot_control_attr_get(const struct device *dev, enum sensor_attribute attr,
					       struct sensor_value *val)
{
	uint8_t reg_val;
	int ret;

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_REBOOT_CONTROL, &reg_val);
	if (ret) {
		LOG_ERR("Failed to read reboot control (%d)", ret);
		return ret;
	}

	val->val2 = 0;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_REBOOT_CONTROL_INIT:
		val->val1 = FIELD_GET(LTC4286_REBOOT_CONTROL_INIT, reg_val);
		break;
	case SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY:
		val->val1 = FIELD_GET(LTC4286_REBOOT_CONTROL_DELAY, reg_val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int ltc4286_mfr_reboot_control_attr_set(const struct device *dev, enum sensor_attribute attr,
					       const struct sensor_value *val)
{
	uint16_t mask;
	int ret;

	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_REBOOT_CONTROL_INIT:
		mask = LTC4286_REBOOT_CONTROL_INIT;
		break;
	case SENSOR_ATTR_LTC4286_REBOOT_CONTROL_DELAY:
		mask = LTC4286_REBOOT_CONTROL_DELAY;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_update(dev, LTC4286_CMD_MFR_REBOOT_CONTROL, mask, val->val1, 1);
	if (ret) {
		LOG_ERR("Failed to set reboot control (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint16_t status_word;
	uint8_t *status_ptr = (uint8_t *)&status_word;
	int ret;

	/* Clear status bits by writing to STATUS_WORD register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_VOUT:
		status_word = LTC4286_STATUS_WORD_VOUT_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_IOUT:
		status_word = LTC4286_STATUS_WORD_IOUT_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_INPUT:
		status_word = LTC4286_STATUS_WORD_INPUT_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFRSPEC:
		status_word = LTC4286_STATUS_WORD_MFRSPEC_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_OTHER:
		status_word = LTC4286_STATUS_WORD_OTHER_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_UNKNOWN:
		status_word = LTC4286_STATUS_WORD_UNKNOWN_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_BUSY:
		status_word = LTC4286_STATUS_WORD_BUSY_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_OFF:
		status_word = LTC4286_STATUS_WORD_OFF_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_IOUT_OC:
		status_word = LTC4286_STATUS_WORD_IOUT_OTHRES_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_VIN_UV:
		status_word = LTC4286_STATUS_WORD_VIN_UTHRES_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_TEMP:
		status_word = LTC4286_STATUS_WORD_TEMP_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_CML:
		status_word = LTC4286_STATUS_WORD_COMLINE_BIT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_NOT_CLEAR:
		status_word = LTC4286_STATUS_WORD_NOT_CLEAR_BIT;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_WORD, status_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to clear status word (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_vout_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_VOUT register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_VOUT_OV:
		status_val = LTC4286_STATUS_VOUT_OV_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_VOUT_UV:
		status_val = LTC4286_STATUS_VOUT_UV_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_VOUT, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_VOUT (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_iout_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_IOUT register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_IOUT_OC_FAULT:
		status_val = LTC4286_STATUS_IOUT_OC_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_IOUT_OC_WARN:
		status_val = LTC4286_STATUS_IOUT_OC_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_IOUT, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_IOUT (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_input_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_INPUT register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_VIN_OV_FAULT:
		status_val = LTC4286_STATUS_INPUT_VIN_OV_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_VIN_OV_WARN:
		status_val = LTC4286_STATUS_INPUT_VIN_OV_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_VIN_UV_FAULT:
		status_val = LTC4286_STATUS_INPUT_VIN_UV_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_VIN_UV_WARN:
		status_val = LTC4286_STATUS_INPUT_VIN_UV_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_PIN_OP_WARN:
		status_val = LTC4286_STATUS_INPUT_PIN_OP_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_INPUT, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_INPUT (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_temp_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_TEMPERATURE register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_TEMP_OT_FAULT:
		status_val = LTC4286_STATUS_TEMP_OT_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_TEMP_OT_WARN:
		status_val = LTC4286_STATUS_TEMP_OT_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_TEMP_UT_WARN:
		status_val = LTC4286_STATUS_TEMP_UT_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_TEMPERATURE, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_TEMPERATURE (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_cml_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_CML register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_CML_BAD_CMD:
		status_val = LTC4286_STATUS_CML_BAD_CMD;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_CML_BAD_DATA:
		status_val = LTC4286_STATUS_CML_BAD_DATA;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_CML_BAD_PEC_FAIL:
		status_val = LTC4286_STATUS_CML_PEC_FAIL;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_CML_MISC_FAULT:
		status_val = LTC4286_STATUS_CML_MISC_FAULT;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_CML, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_CML (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_other_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_OTHER register */
	if ((int)attr == SENSOR_ATTR_LTC4286_STATUS_OTHER_FIRST_ALERT) {
		status_val = LTC4286_STATUS_OTHER_FIRST_ALERT;
	} else {
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_OTHER, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_OTHER (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_mfr_spec_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint8_t status_val;
	int ret;

	/* Clear status bits by writing to STATUS_MFR_SPECIFIC register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_EN_CHANGED:
		status_val = LTC4286_STATUS_MFR_SPEC_EN_CHANGE;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_TSD_FAULT:
		status_val = LTC4286_STATUS_MFR_SPEC_TSD_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_VDD_UVLO:
		status_val = LTC4286_STATUS_MFR_SPEC_VDD_UVLO;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_PIN_OP2_FAULT:
		status_val = LTC4286_STATUS_MFR_SPEC_PIN_OP2_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_PIN_OP1_FAULT:
		status_val = LTC4286_STATUS_MFR_SPEC_PIN_OP1_FAULT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SPEC_FET_BAD_FAULT:
		status_val = LTC4286_STATUS_MFR_SPEC_FET_BAD_FAULT;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_STATUS_MFR_SPECIFIC, &status_val, 1);
	if (ret) {
		LOG_ERR("Failed to clear STATUS_MFR_SPECIFIC (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_mfr_sys1_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint16_t status_val;
	uint8_t *status_ptr = (uint8_t *)&status_val;
	int ret;

	/* Clear status bits by writing to MFR_SYSTEM_STATUS1 register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT1_ALERT:
		status_val = LTC4286_STATUS_MFR_SYS_STAT1_ALERT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT1_L_ALERT:
		status_val = LTC4286_STATUS_MFR_SYS_STAT1_L_ALERT;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_POWER_LOSS:
		status_val = LTC4286_STATUS_MFR_SYS_POWER_LOSS;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_RESET_DONE:
		status_val = LTC4286_STATUS_MFR_SYS_RESET_DONE;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_AVG_DONE:
		status_val = LTC4286_STATUS_MFR_SYS_AVG_DONE;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_ADC_CONV:
		status_val = LTC4286_STATUS_MFR_SYS_ADC_CONV;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_STAT2_SET:
		status_val = LTC4286_STATUS_MFR_SYS_STAT2_SET;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_SYSTEM_STATUS1, status_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to clear MFR_SYSTEM_STATUS1 (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_status_mfr_sys2_attr_set(const struct device *dev, enum sensor_attribute attr)
{
	uint16_t status_val;
	uint8_t *status_ptr = (uint8_t *)&status_val;
	int ret;

	/* Clear status bits by writing to MFR_SYSTEM_STATUS2 register */
	switch ((int)attr) {
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_POWER_FAILED_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_POWER_FAILED_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_FET_SHORT_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_FET_SHORT_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_VDS_UV_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_VDS_UV_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_VDS_OV_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_VDS_OV_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_IOUT_UC_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_IOUT_UC_WARN;
		break;
	case SENSOR_ATTR_LTC4286_STATUS_MFR_SYS_PIN_UP_WARN:
		status_val = LTC4286_STATUS_MFR_SYS_PIN_UP_WARN;
		break;
	default:
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_SYSTEM_STATUS2, status_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to clear MFR_SYSTEM_STATUS2 (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_mfr_pmb_status_attr_get(const struct device *dev, enum sensor_attribute attr,
					   struct sensor_value *val)
{
	uint8_t pmb_status;
	int ret;

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_PMB_STAT, &pmb_status);
	if (ret) {
		LOG_ERR("Failed to read PMB status (%d)", ret);
		return ret;
	}

	val->val1 = pmb_status;
	val->val2 = 0;
	return 0;
}

static int ltc4286_mfr_pmb_status_attr_set(const struct device *dev, enum sensor_attribute attr,
					   const struct sensor_value *val)
{
	uint8_t pmb_status = (uint8_t)val->val1;
	int ret;

	/* Validate against reserved ranges */
	if ((pmb_status >= 0x0D && pmb_status <= 0x12) || /* 0x0D-0x12 RESERVED */
	    (pmb_status == 0x14) ||                       /* 0x14 RESERVED */
	    (pmb_status == 0x18) ||                       /* 0x18 RESERVED */
	    (pmb_status >= 0x1A)) {                       /* 0x1A-0xFF RESERVED */
		return -EINVAL;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_PMB_STAT, &pmb_status, 1);
	if (ret) {
		LOG_ERR("Failed to set PMB status (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ltc4286_data *data = dev->data;

	if (!val) {
		return -EINVAL;
	}

	/* Block writes when write protection is enabled, except for disabling write protection */
	if (data->is_write_protect && !((int)chan == SENSOR_CHAN_LTC4286_CONFIG &&
					(int)attr == SENSOR_ATTR_LTC4286_CONFIG_WRITE_PROTECT)) {
		return -EACCES;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		return ltc4286_vout_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_VIN:
		return ltc4286_vin_attr_set(dev, attr, val);
	case SENSOR_CHAN_CURRENT:
		return ltc4286_iout_attr_set(dev, attr, val);
	case SENSOR_CHAN_POWER:
		return ltc4286_pin_attr_set(dev, attr, val);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return ltc4286_temp_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_CONFIG:
		return ltc4286_config_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_ADC:
		return ltc4286_adc_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_STATUS:
		return ltc4286_status_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_VOUT:
		return ltc4286_status_vout_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_IOUT:
		return ltc4286_status_iout_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_INPUT:
		return ltc4286_status_input_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_TEMP:
		return ltc4286_status_temp_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_CML:
		return ltc4286_status_cml_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_OTHER:
		return ltc4286_status_other_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SPEC:
		return ltc4286_status_mfr_spec_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS1:
		return ltc4286_status_mfr_sys1_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_STATUS_MFR_SYS_STATUS2:
		return ltc4286_status_mfr_sys2_attr_set(dev, attr);
	case SENSOR_CHAN_LTC4286_MFR_CONFIG:
		return ltc4286_mfr_config_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_FLT_CONFIG:
		return ltc4286_mfr_flt_config_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_REBOOT_CONTROL:
		return ltc4286_mfr_reboot_control_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_PMB_STATUS:
		return ltc4286_mfr_pmb_status_attr_set(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_COMMON:
	case SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS:
	case SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE:
		return -ENOTSUP;
	case SENSOR_CHAN_LTC4286_FAULT_RESP_FET:
		return ltc4286_fault_resp_fet_attr_set(dev, attr, val);
	default:
		return -EINVAL;
	}
}

static int ltc4286_mfr_pads_live_status_attr_get(const struct device *dev,
						 enum sensor_attribute attr,
						 struct sensor_value *val)
{
	uint8_t status_val;
	uint8_t cmd = LTC4286_CMD_MFR_PADS_LIVE_STATUS;
	int ret;

	ret = ltc4286_read_reg8(dev, cmd, &status_val);
	if (ret) {
		LOG_ERR("Failed to read pads live status (%d)", ret);
		return ret;
	}

	val->val1 = status_val;
	val->val2 = 0;
	return 0;
}

static int ltc4286_mfr_common_attr_get(const struct device *dev, enum sensor_attribute attr,
				       struct sensor_value *val)
{
	uint16_t common_val = 0;
	int ret;

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_COMMON, &common_val);
	if (ret) {
		LOG_ERR("Failed to read MFR common (%d)", ret);
		return ret;
	}

	val->val1 = common_val;
	val->val2 = 0;
	return 0;
}

static int ltc4286_mfr_shutdown_cause_attr_get(const struct device *dev, enum sensor_attribute attr,
					       struct sensor_value *val)
{
	uint8_t shutdown_val;
	int ret;

	ret = ltc4286_read_reg8(dev, LTC4286_CMD_MFR_SD_CAUSE, &shutdown_val);
	if (ret) {
		LOG_ERR("Failed to read shutdown cause (%d)", ret);
		return ret;
	}

	val->val1 = shutdown_val;
	val->val2 = 0;
	return 0;
}

static int ltc4286_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr, struct sensor_value *val)
{
	if (!val) {
		return -EINVAL;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_VOLTAGE:
		return ltc4286_vout_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_VIN:
		return ltc4286_vin_attr_get(dev, attr, val);
	case SENSOR_CHAN_CURRENT:
		return ltc4286_iout_attr_get(dev, attr, val);
	case SENSOR_CHAN_POWER:
		return ltc4286_pin_attr_get(dev, attr, val);
	case SENSOR_CHAN_AMBIENT_TEMP:
		return ltc4286_temp_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_ADC:
		return ltc4286_adc_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_CONFIG:
		return ltc4286_config_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_CONFIG:
		return ltc4286_mfr_config_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_FLT_CONFIG:
		return ltc4286_mfr_flt_config_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_PMB_STATUS:
		return ltc4286_mfr_pmb_status_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_PADS_LIVE_STATUS:
		return ltc4286_mfr_pads_live_status_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_COMMON:
		return ltc4286_mfr_common_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_REBOOT_CONTROL:
		return ltc4286_mfr_reboot_control_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_MFR_SHUTDOWN_CAUSE:
		return ltc4286_mfr_shutdown_cause_attr_get(dev, attr, val);
	case SENSOR_CHAN_LTC4286_FAULT_RESP_FET:
		return ltc4286_fault_resp_fet_attr_get(dev, attr, val);
	default:
		return -EINVAL;
	}
}

static int ltc4286_cfg_input_inv_gpios(const struct device *dev,
				       const struct ltc4286_gpio_config *gpios, uint8_t gpios_len)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;
	int ret;

	ret = ltc4286_read_reg16(dev, LTC4286_CMD_MFR_GPIO_INV, &val);
	if (ret) {
		LOG_ERR("Failed to read GPIO inversion (%d)", ret);
		return ret;
	}

	for (int i = 0; i < gpios_len; i++) {
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
		LOG_ERR("Failed to write GPIO inversion (%d)", ret);
		return ret;
	}

	/* GPIO input configurations */
	val = FIELD_PREP(LTC4286_CFG_GPI_RBT_EN, cfg->dev_gpio_reboot_en) |
	      FIELD_PREP(LTC4286_CFG_GPI_RBT_PIN, cfg->dev_gpio_reboot_pin) |
	      FIELD_PREP(LTC4286_CFG_GPI_CMP_SEL, cfg->dev_gpio_cmp_in_pin);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPI_SEL, val_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to write GPI select (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_adc_sel(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;
	int ret;

	val = FIELD_PREP(LTC4286_CFG_DISP_AVG, cfg->avg_read_en) |
	      FIELD_PREP(LTC4286_CFG_ADC_AVG_SEL, cfg->avg_samples_cfg);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_AVG_SEL, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write AVG select (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_adc_config(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;
	int ret;

	val = FIELD_PREP(LTC4286_CFG_VDS_SEL, cfg->vds_aux_en) |
	      FIELD_PREP(LTC4286_CFG_VIN_VOUT, cfg->vin_vout_en);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_ADC_CONFIG, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write ADC config (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_mfr1(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;
	int ret;

	val = FIELD_PREP(LTC4286_CFG_CURRENT_LIMIT, cfg->current_limit_cfg) |
	      FIELD_PREP(LTC4286_CFG_VRANGE_SEL, cfg->vrange_set) |
	      FIELD_PREP(LTC4286_CFG_VPWR_SEL, cfg->vpower_set) | LTC4286_CFG_CONFIG1_RSVD;

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_CONFIG1, val_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to write MFR config1 (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_mfr2(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint16_t val;
	uint8_t *val_ptr = (uint8_t *)&val;
	int ret;

	val = FIELD_PREP(LTC4286_CFG_PMBUS_1MBPS, cfg->pmbus_1mbps_en) |
	      FIELD_PREP(LTC4286_CFG_RESET_FAULT_EN, cfg->reset_fault_en) |
	      FIELD_PREP(LTC4286_CFG_PWRGD_RESET, cfg->pg_latch_rst_ctrl) |
	      FIELD_PREP(LTC4286_CFG_MASS_WR_EN, cfg->mass_write_en) |
	      FIELD_PREP(LTC4286_CFG_EXT_TEMP_EN, cfg->ext_temp_en) |
	      FIELD_PREP(LTC4286_CFG_DEB_TMR_EN, cfg->deb_tmr_en) | LTC4286_CFG_CONFIG2_RSVD;

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_CONFIG2, val_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to write MFR config2 (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_reboot(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t val;
	int ret;

	val = FIELD_PREP(LTC4286_CFG_REBOOT_INIT, cfg->reboot_init_fet_only) |
	      FIELD_PREP(LTC4286_CFG_REBOOT_DELAY, cfg->reboot_delay_cfg);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_REBOOT_CONTROL, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write reboot control (%d)", ret);
		return ret;
	}

	return 0;
}

static int ltc4286_cfg_fault_response(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	int ret;
	uint16_t op_val;
	uint8_t *op_val_ptr = (uint8_t *)&op_val;
	uint8_t val;

	/* Overcurrent */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overcurr_config.no_ignore
				  ? LTC4286_FAULT_RESP_TYP3
				  : LTC4286_FAULT_RESP_IGNORE)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overcurr_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_IOUT_OC_FAULT_RESPONSE, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write overcurrent fault response (%d)", ret);
		return ret;
	}

	/* Overtemperature */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overtemp_config.no_ignore
				  ? LTC4286_FAULT_RESP_TYP2
				  : LTC4286_FAULT_RESP_IGNORE)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overtemp_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_OT_FAULT_RESPONSE, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write overtemperature fault response (%d)", ret);
		return ret;
	}

	/* Vin Overvoltage */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.overvolt_config.no_ignore
				  ? LTC4286_FAULT_RESP_TYP2
				  : LTC4286_FAULT_RESP_IGNORE)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.overvolt_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_OV_FAULT_RESPONSE, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write VIN overvoltage fault response (%d)", ret);
		return ret;
	}

	/* Vin Undervoltage */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.undervolt_config.no_ignore ? LTC4286_FAULT_RESP_TYP2
									  : 0)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.undervolt_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_VIN_UV_FAULT_RESPONSE, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write VIN undervoltage fault response (%d)", ret);
		return ret;
	}

	/* FET Faults */
	val = FIELD_PREP(LTC4286_FAULT_RESPONSE_MASK,
			 (cfg->fault_responses.fet_config.no_ignore ? LTC4286_FAULT_RESP_TYP1
								    : LTC4286_FAULT_RESP_IGNORE)) |
	      FIELD_PREP(LTC4286_FAULT_RETRY_MASK, cfg->fault_responses.fet_config.retries) |
	      BIT(0);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_FET_FAULT_RESPONSE, &val, 1);
	if (ret) {
		LOG_ERR("Failed to write FET fault response (%d)", ret);
		return ret;
	}

	/* Overpower */
	op_val = FIELD_PREP(LTC4286_FAULT_OP_TIMER, cfg->op_tmr) |
		 FIELD_PREP(LTC4286_FAULT_OP_RESPONSE_MASK,
			    (cfg->fault_responses.overpower_config.no_ignore
				     ? LTC4286_FAULT_RESP_TYP2
				     : LTC4286_FAULT_RESP_IGNORE)) |
		 FIELD_PREP(LTC4286_FAULT_OP_RETRY_MASK,
			    cfg->fault_responses.overpower_config.retries);

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_OP_FAULT_RESPONSE, op_val_ptr, 2);
	if (ret) {
		LOG_ERR("Failed to write overpower fault response (%d)", ret);
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

static int ltc4286_alert_mask_update(const struct device *dev,
				     enum ltc4286_trig_alert_mask alert_reg, uint16_t mask,
				     bool set_bits)
{
	uint8_t read_cmd[] = {LTC4286_CMD_ALERT_MASK, alert_reg};
	uint8_t write_data[3];
	uint16_t reg_val = 0;
	uint8_t *reg_ptr = (uint8_t *)&reg_val;
	uint8_t size;
	int ret;

	/* Determine register size based on alert_reg */
	switch (alert_reg) {
	case LTC4286_ALERT_STATUS_WORD_MASK:
	case LTC4286_ALERT_STATUS_SYS1_MASK:
	case LTC4286_ALERT_STATUS_SYS2_MASK:
		size = 2;
		break;
	case LTC4286_ALERT_STATUS_VOUT_MASK:
	case LTC4286_ALERT_STATUS_IOUT_MASK:
	case LTC4286_ALERT_STATUS_INPUT_MASK:
	case LTC4286_ALERT_STATUS_TEMP_MASK:
	case LTC4286_ALERT_STATUS_COMMS_MASK:
	case LTC4286_ALERT_STATUS_SPEC_MASK:
		size = 1;
		break;
	default:
		LOG_ERR("Invalid Alert Mask: 0x%02X%02X", LTC4286_CMD_ALERT_MASK, alert_reg);
		return -EINVAL;
	}

	ret = ltc4286_cmd_read(dev, read_cmd, ARRAY_SIZE(read_cmd), reg_ptr, size);
	if (ret) {
		LOG_ERR("Failed to read Alert mask 0x%02X%02X", LTC4286_CMD_ALERT_MASK, alert_reg);
		return ret;
	}

	if (set_bits) {
		reg_val |= mask; /* Set bits to disable alerts */
	} else {
		reg_val &= ~mask; /* Clear bits to enable alerts */
	}

	write_data[0] = alert_reg;
	if (size == 2) {
		write_data[1] = FIELD_GET(GENMASK(7, 0), reg_val);
		write_data[2] = FIELD_GET(GENMASK(15, 8), reg_val);
	} else {
		write_data[1] = (uint8_t)reg_val;
	}

	ret = ltc4286_cmd_write(dev, LTC4286_CMD_ALERT_MASK, write_data, size + 1);
	if (ret) {
		LOG_ERR("Failed to write Alert mask 0x%02X%02X", LTC4286_CMD_ALERT_MASK, alert_reg);
		return ret;
	}

	return 0;
}

int ltc4286_enable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert_reg,
			      uint16_t alert_bit)
{
	return ltc4286_alert_mask_update(dev, alert_reg, alert_bit, false);
}

int ltc4286_disable_alert_mask(const struct device *dev, enum ltc4286_trig_alert_mask alert_reg,
			       uint16_t alert_bit)
{
	return ltc4286_alert_mask_update(dev, alert_reg, alert_bit, true);
}

static int ltc4286_probe(const struct device *dev)
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

	ret = ltc4286_cfg_input_inv_gpios(dev, gpio_configs, ARRAY_SIZE(gpio_configs));
	if (ret) {
		LOG_ERR("Could not configure input inv");
		return ret;
	}

#ifdef CONFIG_LTC4286_TRIGGER
	ret = ltc4286_init_interrupts(dev);
	if (ret) {
		LOG_ERR("Could not configure interrupts");
		return ret;
	}
#endif /* CONFIG_LTC4286_TRIGGER */

	ret = ltc4286_cfg_adc_config(dev);
	if (ret) {
		LOG_ERR("Could not configure adc");
		return ret;
	}

	ret = ltc4286_cfg_adc_sel(dev);
	if (ret) {
		LOG_ERR("Could not configure adc_sel");
		return ret;
	}

	ret = ltc4286_cfg_mfr1(dev);
	if (ret) {
		LOG_ERR("Could not configure mfr1");
		return ret;
	}

	ret = ltc4286_cfg_mfr2(dev);
	if (ret) {
		LOG_ERR("Could not configure mfr2");
		return ret;
	}

	ret = ltc4286_cfg_reboot(dev);
	if (ret) {
		LOG_ERR("Could not configure reboot");
		return ret;
	}

	ret = ltc4286_cfg_fault_response(dev);
	if (ret) {
		LOG_ERR("Could not configure fault responses");
		return ret;
	}

	/* configure GPIO pins */
	data_ptr[0] = FIELD_PREP(LTC4286_CFG_GPO_5_1, cfg->gpo_pin_output_select[0]) |
		      FIELD_PREP(LTC4286_CFG_GPO_6_2, cfg->gpo_pin_output_select[1]);
	data_ptr[1] = FIELD_PREP(LTC4286_CFG_GPO_7_3, cfg->gpo_pin_output_select[2]) |
		      FIELD_PREP(LTC4286_CFG_GPO_8_4, cfg->gpo_pin_output_select[3]);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPO_SEL41, data_ptr, 2);
	if (ret) {
		LOG_ERR("Could not configure GPO");
		return ret;
	}

	data_ptr[0] = FIELD_PREP(LTC4286_CFG_GPO_5_1, cfg->gpo_pin_output_select[4]) |
		      FIELD_PREP(LTC4286_CFG_GPO_6_2, cfg->gpo_pin_output_select[5]);
	data_ptr[1] = FIELD_PREP(LTC4286_CFG_GPO_7_3, cfg->gpo_pin_output_select[6]) |
		      FIELD_PREP(LTC4286_CFG_GPO_8_4, cfg->gpo_pin_output_select[7]);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_MFR_GPO_SEL85, data_ptr, 2);
	if (ret) {
		LOG_ERR("Could not configure GPO");
		return ret;
	}

	/* protect from future writes if set */
	val = FIELD_PREP(LTC4286_CFG_WRITE_PROTECT_1, cfg->write_protect_en);
	ret = ltc4286_cmd_write(dev, LTC4286_CMD_WRITE_PROTECT, &val, 1);
	if (ret) {
		LOG_ERR("Could not configure write protect");
		return ret;
	}

	return 0;
}

static int ltc4286_init(const struct device *dev)
{
	const struct ltc4286_dev_config *cfg = dev->config;
	uint8_t device_id[5] = {0}; /* expected: [read_length, "L", "T", "C", \0] */
	uint8_t read_reg = LTC4286_CMD_MFR_ID;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c_dev)) {
		LOG_ERR("Bus device is not ready");
		return -EINVAL;
	}

	ret = ltc4286_cmd_read(dev, &read_reg, 1, device_id, ARRAY_SIZE(device_id) - 1);
	if (ret) {
		LOG_ERR("Could not read Device ID");
		return ret;
	}

	/* first byte is read length */
	if (strcmp((device_id + 1), LTC4286_DEVICE_ID) != 0) {
		LOG_ERR("ID does not match: %s on target %s", device_id, LTC4286_DEVICE_ID);
		return -EINVAL;
	}

	return ltc4286_probe(dev);
}

static DEVICE_API(sensor, ltc4286_driver_api) = {
	.channel_get = ltc4286_channel_get,
	.attr_set = ltc4286_attr_set,
	.attr_get = ltc4286_attr_get,
	.sample_fetch = ltc4286_sample_fetch,
#ifdef CONFIG_LTC4286_TRIGGER
	.trigger_set = ltc4286_trigger_set,
#endif /* CONFIG_LTC4286_TRIGGER */
};

#define LTC4286_DEFINE(inst)                                                                       \
	static struct ltc4286_data ltc4286_data_##inst = {                                         \
		.is_write_protect = DT_INST_PROP(inst, write_protect_enable),                      \
	};                                                                                         \
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
				.reboot_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, reboot_gpios, {0}),  \
				.overpower_gpio =                                                  \
					GPIO_DT_SPEC_INST_GET_OR(inst, op_status_gpios, {0}),      \
				.overcurrent_gpio =                                                \
					GPIO_DT_SPEC_INST_GET_OR(inst, oc_status_gpios, {0}),      \
				.comp_out_gpio =                                                   \
					GPIO_DT_SPEC_INST_GET_OR(inst, comp_out_gpios, {0}),       \
				.fault_out_gpio =                                                  \
					GPIO_DT_SPEC_INST_GET_OR(inst, fault_out_gpios, {0}),      \
				.power_good_gpio =                                                 \
					GPIO_DT_SPEC_INST_GET_OR(inst, power_good_gpios, {0}),     \
				.alert_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, alert_gpios, {0}),    \
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
