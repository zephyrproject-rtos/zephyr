/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2021  Filip Primozic <filip.primozic@greyp.com>
 */

#define DT_DRV_COMPAT ti_bq24155

#include "bq24155.h"

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <kernel.h>
#include <logging/log.h>
#include <assert.h>

LOG_MODULE_REGISTER(bq24155, CONFIG_SENSOR_LOG_LEVEL);

static int bq24155_reg_read(const struct device *dev, uint8_t reg_addr)
{
	struct bq24155_data *bq2415x = dev->data;
	const struct bq24155_config *const config = dev->config;
	int status;
	uint8_t value;

	status = i2c_reg_read_byte(bq2415x->i2c, config->i2c_addr, reg_addr, &value);
	if (status < 0) {
		return status;
	}

	return value;
}

static int bq24155_reg_read_mask(const struct device *dev,
				 uint8_t reg_addr, uint8_t mask,
				 uint8_t shift)
{
	assert(shift <= BQ24155_REGISTER_SIZE);
	int status;

	status = bq24155_reg_read(dev, reg_addr);
	if (status < 0) {
		return status;
	}

	return (status & mask) >> shift;
}

static int bq24155_reg_read_bit(const struct device *dev,
				uint8_t reg_addr, uint8_t bit)
{
	assert(bit <= BQ24155_REGISTER_SIZE);

	return bq24155_reg_read_mask(dev, reg_addr, BIT(bit), bit);
}

static int bq24155_reg_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t value)
{
	struct bq24155_data *bq2415x = dev->data;
	const struct bq24155_config *const config = dev->config;

	return i2c_reg_write_byte(bq2415x->i2c, config->i2c_addr, reg_addr, value);
}

static int bq24155_reg_write_mask(const struct device *dev,
				  uint8_t reg_addr, uint8_t value, uint8_t mask,
				  uint8_t shift)
{
	assert(shift <= BQ24155_REGISTER_SIZE);
	int status;

	status = bq24155_reg_read(dev, reg_addr);
	if (status < 0) {
		return status;
	}

	status &= ~mask;
	status |= value << shift;

	return bq24155_reg_write(dev, reg_addr, status);
}

static int bq24155_reg_write_bit(const struct device *dev,
				 uint8_t reg_addr, bool set, uint8_t bit)
{
	assert(bit <= BQ24155_REGISTER_SIZE);

	return bq24155_reg_write_mask(dev, reg_addr, set, BIT(bit), bit);
}

static int bq24155_run_command(const struct device *dev,
			       enum bq24155_command command)
{
	int ret;

	switch (command) {
	case BQ24155_TIMER_RESET:
		return bq24155_reg_write_bit(dev, BQ24155_STATUS_REGISTER, 1,
					     BQ24155_BIT_TMR_RST);
	case BQ24155_ISEL_STATUS:
		return bq24155_reg_read_bit(dev, BQ24155_STATUS_REGISTER, BQ24155_BIT_ISEL);
	case BQ24155_STAT_PIN_STATUS:
		return bq24155_reg_read_bit(dev, BQ24155_STATUS_REGISTER,
					    BQ24155_BIT_EN_STAT);
	case BQ24155_STAT_PIN_ENABLE:
		return bq24155_reg_write_bit(dev, BQ24155_STATUS_REGISTER, 1,
					     BQ24155_BIT_EN_STAT);
	case BQ24155_STAT_PIN_DISABLE:
		return bq24155_reg_write_bit(dev, BQ24155_STATUS_REGISTER, 0,
					     BQ24155_BIT_EN_STAT);
	case BQ24155_CHARGE_STATUS:
		return bq24155_reg_read_mask(dev, BQ24155_STATUS_REGISTER,
					     BQ24155_MASK_STAT, BQ24155_SHIFT_STAT);
	case BQ24155_FAULT_STATUS:
		return bq24155_reg_read_mask(dev, BQ24155_STATUS_REGISTER,
					     BQ24155_MASK_FAULT, BQ24155_SHIFT_FAULT);

	case BQ24155_CHARGE_TERMINATION_STATUS:
		return bq24155_reg_read_bit(dev, BQ24155_CONTROL_REGISTER, BQ24155_BIT_TE);
	case BQ24155_CHARGE_TERMINATION_ENABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 1,
					     BQ24155_BIT_TE);
	case BQ24155_CHARGE_TERMINATION_DISABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 0,
					     BQ24155_BIT_TE);
	case BQ24155_CHARGER_STATUS:
		ret = bq24155_reg_read_bit(dev, BQ24155_CONTROL_REGISTER, BQ24155_BIT_CE);
		if (ret < 0) {
			return ret;
		}

		return ret > 0 ? 0 : 1;
	case BQ24155_CHARGER_ENABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 0,
					     BQ24155_BIT_CE);
	case BQ24155_CHARGER_DISABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 1,
					     BQ24155_BIT_CE);
	case BQ24155_HIGH_IMPEDANCE_STATUS:
		return bq24155_reg_read_bit(dev, BQ24155_CONTROL_REGISTER,
					    BQ24155_BIT_HZ_MODE);
	case BQ24155_HIGH_IMPEDANCE_ENABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 1,
					     BQ24155_BIT_HZ_MODE);
	case BQ24155_HIGH_IMPEDANCE_DISABLE:
		return bq24155_reg_write_bit(dev, BQ24155_CONTROL_REGISTER, 0,
					     BQ24155_BIT_HZ_MODE);

	case BQ24155_VENDER_CODE:
		return bq24155_reg_read_mask(dev, BQ24155_PART_REVISION_REGISTER,
					     BQ24155_MASK_VENDER, BQ24155_SHIFT_VENDER);
	case BQ24155_PART_NUMBER:
		return bq24155_reg_read_mask(dev, BQ24155_PART_REVISION_REGISTER, BQ24155_MASK_PN,
					     BQ24155_SHIFT_PN);
	case BQ24155_REVISION:
		return bq24155_reg_read_mask(dev, BQ24155_PART_REVISION_REGISTER,
					     BQ24155_MASK_REVISION,
					     BQ24155_SHIFT_REVISION);
	}
	return -EINVAL;
}

static int bq24155_read_vender_code(const struct device *dev)
{
	return bq24155_run_command(dev, BQ24155_VENDER_CODE);
}

static int bq24155_reset_timer_control(const struct device *dev, int enable)
{
	struct bq24155_data *bq2415x = dev->data;

	if (enable) {
		k_work_schedule(&bq2415x->dwork_timer_reset, K_SECONDS(BQ24155_TIMER_RESET_RATE));
		int status;

		status = bq24155_run_command(dev, BQ24155_TIMER_RESET);
		if (status < 0) {
			LOG_ERR("Failed to reset timer");
			return status;
		}
	} else {
		k_work_cancel_delayable(&bq2415x->dwork_timer_reset);
	}

	return 0;
}

static void bq24155_timer_dwork(struct k_work *work)
{
	struct bq24155_data *bq2415x =
		CONTAINER_OF(work, struct bq24155_data, dwork_timer_reset);
	int status;

	status = bq24155_run_command(bq2415x->dev, BQ24155_TIMER_RESET);
	if (status < 0) {
		LOG_ERR("Failed to reset timer.");
		bq24155_reset_timer_control(bq2415x->dev, 0);
		return;
	}

	k_work_schedule(&bq2415x->dwork_timer_reset, K_SECONDS(BQ24155_TIMER_RESET_RATE));
}


static int bq24155_set_power_up_values(const struct device *dev)
{
	int status;

	status = bq24155_reg_write(dev, BQ24155_CURRENT_REGISTER, BQ24155_RESET_CURRENT);
	status += bq24155_reg_write(dev, BQ24155_VOLTAGE_REGISTER, BQ24155_RESET_VOLTAGE);
	status += bq24155_reg_write(dev, BQ24155_CONTROL_REGISTER, BQ24155_RESET_CONTROL);
	status += bq24155_reg_write(dev, BQ24155_STATUS_REGISTER, BQ24155_RESET_STATUS);

	return status;
}

static int bq24155_set_input_current(const struct device *dev, int mA)
{
	int value;

	if (mA <= 100) {
		value = 0;
	} else if (mA <= 500) {
		value = 1;
	} else if (mA <= 800) {
		value = 2;
	} else {
		value = 3;
	}

	return bq24155_reg_write_mask(dev, BQ24155_CONTROL_REGISTER, value,
				      BQ24155_MASK_LIMIT, BQ24155_SHIFT_LIMIT);
}

static int bq24155_get_current_limit(const struct device *dev)
{
	int reg_value;

	reg_value = bq24155_reg_read_mask(dev, BQ24155_CONTROL_REGISTER, BQ24155_MASK_LIMIT,
					  BQ24155_SHIFT_LIMIT);
	if (reg_value < 0) {
		return reg_value;
	} else if (reg_value == 0) {
		return 100;
	} else if (reg_value == 1) {
		return 500;
	} else if (reg_value == 2) {
		return 800;
	} else if (reg_value == 3) {
		return 1800;
	}
	return -EINVAL;
}

static int bq24155_set_weak_voltage(const struct device *dev, int mV)
{
	int value;

	/* Formula origin: datasheet, pg.21, Table 4
	 * Weak voltage range = 3400mV to 3700mV with a step of 100mV
	 * Equation of a line: weak_voltage[mV] = step[mV]*value + minValue[mV] -> solve for value
	 */
	value = (mV - 3400) / 100;
	if (value < 0) {
		value = 0;
	} else if (value > 3) {
		value = 3;
	}

	return bq24155_reg_write_mask(dev, BQ24155_CONTROL_REGISTER, value,
				      BQ24155_MASK_VLOWV, BQ24155_SHIFT_VLOWV);
}

static int bq24155_get_weak_voltage(const struct device *dev)
{
	int reg_value;

	reg_value = bq24155_reg_read_mask(dev, BQ24155_CONTROL_REGISTER, BQ24155_MASK_VLOWV,
					  BQ24155_SHIFT_VLOWV);
	if (reg_value < 0) {
		return reg_value;
	}

	/* Formula origin: datasheet, pg.21, Table 4
	 * Weak voltage range = 3400mV to 3700mV with a step of 100mV
	 * Equation of a line: weak_voltage[mV] = step[mV]*reg_value + minValue[mV]
	 */
	return 100 * reg_value + 3400;
}

static int bq24155_set_regulation_voltage(const struct device *dev,
					  int mV)
{
	int value;

	/* Formula origin: datasheet, pg.21, Table 5
	 * Charge voltage range = 3500mV to 4440mV with a step of 20mV
	 * Equation of a line: voltage[mV] = step[mV]*value + minValue[mV] -> solve for value
	 */
	value = (mV - 3500) / 20;
	if (value < 0) {
		value = 0;
	} else if (value > 47) {
		value = 47;
	}

	return bq24155_reg_write_mask(dev, BQ24155_VOLTAGE_REGISTER, value, BQ24155_MASK_VO,
				      BQ24155_SHIFT_VO);
}

static int bq24155_get_regulation_voltage(const struct device *dev)
{
	int reg_value;

	reg_value = bq24155_reg_read_mask(dev, BQ24155_VOLTAGE_REGISTER, BQ24155_MASK_VO,
					  BQ24155_SHIFT_VO);
	if (reg_value < 0) {
		return reg_value;
	}

	/* Formula origin: datasheet, pg.21, Table 5
	 * Charge voltage range = 3500mV to 4440mV with a step of 20mV
	 * Equation of a line: regulation_voltage[mV] = step[mV]*reg_value + minValue[mV]
	 */
	return 20 * reg_value + 3500;
}

static int bq24155_set_charge_current(const struct device *dev, int mA)
{
	const struct bq24155_config *const config = dev->config;
	int value;

	/* Formula explanation:
	 * I_charge = I_charge_step * value + I_charge_default
	 * I_charge_step = Vi(CHRG0) / Rsns = 6.8mV / Rsns -> pg.22, Equation 3 + Table 7, bit 4
	 * I_charge_default = Vi(REG) / Rsns = 37.4mV / Rsns -> pg.22, Table 9
	 * Substitute I_charge_step and I_charge_default into above equation:
	 * I_charge[A] = (6.8mV * value + 37.4mV) / Rsns(mOhm) = (6.8 * value + 37.4) / Rsns
	 * I_charge[mA] = (6800 * value + 37400) / Rsns -> solve for value
	 */
	value = (mA * config->resistor_sense - 37400) / 6800;
	if (value < 0) {
		value = 0;
	} else if (value > 7) {
		value = 7;
	}

	return bq24155_reg_write_mask(dev, BQ24155_CURRENT_REGISTER, value,
				      BQ24155_MASK_VI_CHRG | BQ24155_MASK_RESET,
				      BQ24155_SHIFT_VI_CHRG);
}

static int bq24155_get_charge_current(const struct device *dev)
{
	const struct bq24155_config *const config = dev->config;
	int reg_value;

	/* prevent division by zero */
	if (config->resistor_sense == 0) {
		return -EINVAL;
	}

	reg_value = bq24155_reg_read_mask(dev, BQ24155_CURRENT_REGISTER, BQ24155_MASK_VI_CHRG,
					  BQ24155_SHIFT_VI_CHRG);
	if (reg_value < 0) {
		return reg_value;
	}

	/* Formula explanation:
	 * I_charge = I_charge_step * reg_value + I_charge_default
	 * I_charge_step = Vi(CHRG0) / Rsns = 6.8mV / Rsns -> pg.22, Equation 3 + Table 7, bit 4
	 * I_charge_default = Vi(REG) / Rsns = 37.4mV / Rsns -> pg.22, Table 9
	 * Substitute I_charge_step and I_charge_default into above equation:
	 * I_charge[A] = (6.8mV * reg_value + 37.4mV) / Rsns(mOhm) = (6.8 * reg_value + 37.4) / Rsns
	 * I_charge[mA] = (6800 * reg_value + 37400) / Rsns
	 */
	return (6800 * reg_value + 37400) / config->resistor_sense;
}

static int bq24155_set_termination_current(const struct device *dev, int mA)
{
	const struct bq24155_config *const config = dev->config;
	int value;

	/* Formula explanation:
	 * I_term = I_term_step * value + I_term_default
	 * I_term_step = Vi(TERM0) / Rsns = 3.4mV / Rsns -> pg.22, Equation 2 + Table 7, bit 0
	 * I_term_default = Vi(default) / Rsns = 3.4mV / Rsns -> pg.22, Table 8
	 * Substitute I_term_step and I_term_default into above equation:
	 * I_term[A] = (3.4mV * value + 3.4mV) / Rsns(mOhm) = (3.4 * value + 3.4) / Rsns
	 * I_term[mA] = (3400 * value + 3400) / Rsns -> solve for value
	 */
	value = (mA * config->resistor_sense - 3400) / 3400;
	if (value < 0) {
		value = 0;
	} else if (value > 7) {
		value = 7;
	}

	return bq24155_reg_write_mask(dev, BQ24155_CURRENT_REGISTER, value,
				      BQ24155_MASK_VI_TERM | BQ24155_MASK_RESET,
				      BQ24155_SHIFT_VI_TERM);
}

static int bq2415x_get_termination_current(const struct device *dev)
{
	const struct bq24155_config *const config = dev->config;
	int reg_value;

	/* prevent division by zero */
	if (config->resistor_sense == 0) {
		return -EINVAL;
	}

	reg_value = bq24155_reg_read_mask(dev, BQ24155_CURRENT_REGISTER, BQ24155_MASK_VI_TERM,
					  BQ24155_SHIFT_VI_TERM);
	if (reg_value < 0) {
		return reg_value;
	}

	/* Formula explanation:
	 * I_term = I_term_step * reg_value + I_term_default
	 * I_term_step = Vi(TERM0) / Rsns = 3.4mV / Rsns -> pg.22, Equation 2 + Table 7, bit 0
	 * I_term_default = Vi(default) / Rsns = 3.4mV / Rsns -> pg.22, Table 8
	 * Substitute I_term_step and I_term_default into above equation:
	 * I_term[A] = (3.4mV * reg_value + 3.4mV) / Rsns(mOhm) = (3.4 * reg_value + 3.4) / Rsns
	 * I_term[mA] = (3400 * reg_value + 3400) / Rsns
	 */
	return (3400 * reg_value + 3400) / config->resistor_sense;
}

static int bq24155_set_config_values(const struct device *dev)
{
	const struct bq24155_config *const config = dev->config;
	int status;

	status = bq24155_run_command(dev, BQ24155_CHARGER_DISABLE);
	status += bq24155_run_command(dev, BQ24155_CHARGE_TERMINATION_DISABLE);

	status += bq24155_set_input_current(dev, config->input_current);
	status += bq24155_set_weak_voltage(dev, config->weak_voltage);
	status += bq24155_set_regulation_voltage(dev, config->regulation_voltage);

	if (config->resistor_sense > 0) {
		status += bq24155_set_charge_current(dev, config->charge_current);
		status += bq24155_set_termination_current(dev, config->termination_current);
		status += bq24155_run_command(dev, BQ24155_CHARGE_TERMINATION_ENABLE);
	}

	return status;
}

static int bq24155_set_current_config(const struct device *dev,
				      enum sensor_attribute attr,
				      const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_CHARGE_CURRENT:
		return bq24155_set_charge_current(dev, val->val2 / 1000);
	case SENSOR_ATTR_TERMINATION_CURRENT:
		return bq24155_set_termination_current(dev, val->val2 / 1000);
	case SENSOR_ATTR_INPUT_CURRENT:
		return bq24155_set_input_current(dev, val->val2 / 1000);
	default:
		LOG_ERR("Current attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq24155_set_voltage_config(const struct device *dev,
				      enum sensor_attribute attr,
				      const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		return bq24155_set_weak_voltage(dev, val->val2 / 1000);
	case SENSOR_ATTR_UPPER_THRESH:
		return bq24155_set_regulation_voltage(dev, val->val2 / 1000);
	default:
		LOG_ERR("Voltage attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq24155_set_common_config(const struct device *dev,
				     enum sensor_attribute attr,
				     const struct sensor_value *val)
{
	int status;

	switch (attr) {
	case SENSOR_ATTR_CHARGE_CONTROL:
		if (val->val1 == 0) {
			status = bq24155_reset_timer_control(dev, 0);
			if (status < 0) {
				return status;
			}
			return bq24155_run_command(dev, BQ24155_CHARGER_DISABLE);
		} else if (val->val1 == 1) {
			status = bq24155_reset_timer_control(dev, 1);
			if (status < 0) {
				return status;
			}
			return bq24155_run_command(dev, BQ24155_CHARGER_ENABLE);
		}
		LOG_ERR("Value given %d not supported.", val->val1);
		return -ENOTSUP;

	case SENSOR_ATTR_OPERATION_MODE:
		if (val->val1 == 0) {
			return bq24155_run_command(dev, BQ24155_HIGH_IMPEDANCE_DISABLE);
		} else if (val->val1 == 1) {
			return bq24155_run_command(dev, BQ24155_HIGH_IMPEDANCE_ENABLE);
		}
		LOG_ERR("Value given %d not supported.", val->val1);
		return -ENOTSUP;

	default:
		LOG_ERR("Attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq24155_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_CURRENT:
		return bq24155_set_current_config(dev, attr, val);
	case SENSOR_CHAN_VOLTAGE:
		return bq24155_set_voltage_config(dev, attr, val);
	case SENSOR_CHAN_CHARGER_CONTROL:
		return bq24155_set_common_config(dev, attr, val);
	default:
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int bq24155_get_current_config(const struct device *dev,
				      enum sensor_attribute attr,
				      struct sensor_value *val)
{
	int status;

	switch (attr) {
	case SENSOR_ATTR_CHARGE_CURRENT:
		status = bq24155_get_charge_current(dev);
		break;
	case SENSOR_ATTR_TERMINATION_CURRENT:
		status = bq2415x_get_termination_current(dev);
		break;
	case SENSOR_ATTR_INPUT_CURRENT:
		status = bq24155_get_current_limit(dev);
		break;
	default:
		LOG_ERR("Current attribute not supported.");
		status = -ENOTSUP;
	}

	if (status < 0) {
		return status;
	}

	val->val1 = 0;
	val->val2 = status * 1000;

	return 0;
}

static int bq24155_get_voltage_config(const struct device *dev,
				      enum sensor_attribute attr,
				      struct sensor_value *val)
{
	int status;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		status = bq24155_get_weak_voltage(dev);
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		status = bq24155_get_regulation_voltage(dev);
		break;
	default:
		LOG_ERR("Voltage attribute not supported.");
		status = -ENOTSUP;
	}

	if (status < 0) {
		return status;
	}

	val->val1 = 0;
	val->val2 = status * 1000;

	return 0;
}

static int bq24155_get_common_config(const struct device *dev,
				     enum sensor_attribute attr,
				     struct sensor_value *val)
{
	int status;

	switch (attr) {
	case SENSOR_ATTR_CHARGE_CONTROL:
		status = bq24155_run_command(dev, BQ24155_CHARGER_STATUS);
		break;

	case SENSOR_ATTR_OPERATION_MODE:
		status = bq24155_run_command(dev, BQ24155_HIGH_IMPEDANCE_STATUS);
		break;

	default:
		LOG_ERR("Attribute not supported.");
		status = -ENOTSUP;
	}

	if (status < 0) {
		return status;
	}

	val->val1 = status;
	val->val2 = 0;

	return 0;
}

static int bq24155_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_CURRENT:
		return bq24155_get_current_config(dev, attr, val);
	case SENSOR_CHAN_VOLTAGE:
		return bq24155_get_voltage_config(dev, attr, val);
	case SENSOR_CHAN_CHARGER_CONTROL:
		return bq24155_get_common_config(dev, attr, val);
	default:
		LOG_ERR("attr_get() not supported on this channel.");
		return -ENOTSUP;
	}
}


static int bq24155_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct bq24155_data *bq2415x = dev->data;
	int status;

	switch (chan) {
	case SENSOR_CHAN_CHARGER_FAULT_STATUS:
		status = bq24155_run_command(dev, BQ24155_FAULT_STATUS);
		bq2415x->fault_status = status;
		break;

	case SENSOR_CHAN_CHARGER_CHARGING_STATUS:
		status = bq24155_run_command(dev, BQ24155_CHARGE_STATUS);
		bq2415x->charge_status = status;
		break;

	default:
		LOG_ERR("Channel not supported.");
		status = -ENOTSUP;
	}

	return status;
}

static int bq24155_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq24155_data *bq2415x = dev->data;

	switch (chan) {
	case SENSOR_CHAN_CHARGER_FAULT_STATUS:
		val->val1 = bq2415x->fault_status;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_CHARGER_CHARGING_STATUS:
		val->val1 = bq2415x->charge_status;
		val->val2 = 0;
		break;
	default:
		LOG_ERR("Channel not supported.");
		return -ENOTSUP;
	}

	return 0;
}

static int bq24155_init(const struct device *dev)
{
	struct bq24155_data *bq2415x = dev->data;
	const struct bq24155_config *const config = dev->config;
	int status = 0;

	bq2415x->i2c = device_get_binding(config->bus_name);
	if (bq2415x->i2c == NULL) {
		LOG_ERR("I2C master controller not found: %s.", config->bus_name);
		return -EINVAL;
	}

	bq2415x->dev = dev;

	status = bq24155_set_power_up_values(dev);
	if (status < 0) {
		LOG_ERR("Failed to set chip values to power up state");
		return -EIO;
	}

	status = bq24155_read_vender_code(dev);
	if (status < 0) {
		LOG_ERR("Failed to read vender ID");
		return -EIO;
	}

	if (status != BQ24155_DEFAULT_VENDER_CODE) {
		LOG_ERR("Unsupported chip detected (0x%x)!", status);
		return -ENODEV;
	}

	status = bq24155_set_config_values(dev);
	if (status < 0) {
		LOG_ERR("Failed to set config values");
		return -EIO;
	}

	k_work_init_delayable(&bq2415x->dwork_timer_reset, bq24155_timer_dwork);

	return 0;
}

static const struct sensor_driver_api bq24155_battery_driver_api = {
	.attr_set = bq24155_attr_set,
	.attr_get = bq24155_attr_get,
	.sample_fetch = bq24155_sample_fetch,
	.channel_get = bq24155_channel_get,
};

#define BQ24155_INIT(index)							      \
	static struct bq24155_data bq24155_driver_##index;			      \
										      \
	static const struct bq24155_config bq24155_config_##index = {		      \
		.bus_name = DT_INST_BUS_LABEL(index),				      \
		.i2c_addr =  DT_INST_REG_ADDR(index),				      \
		.input_current = DT_INST_PROP(index, input_current),		      \
		.weak_voltage = DT_INST_PROP(index, weak_voltage),		      \
		.regulation_voltage =						      \
			DT_INST_PROP(index, regulation_voltage),		      \
		.charge_current = DT_INST_PROP(index, charge_current),		      \
		.termination_current = DT_INST_PROP(index, termination_current),      \
		.resistor_sense = DT_INST_PROP(index, resistor_sense),		      \
	};									      \
										      \
	DEVICE_DT_INST_DEFINE(							      \
		index, &bq24155_init, device_pm_control_nop, &bq24155_driver_##index, \
		&bq24155_config_##index, POST_KERNEL,				      \
		CONFIG_SENSOR_INIT_PRIORITY, &bq24155_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ24155_INIT)
