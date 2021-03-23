/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2011-2013  Pali Rohár <pali@kernel.org>
 * Copyright (C) 2021  Filip Primozic <filip.primozic@greyp.com>
 *
 */

#define DT_DRV_COMPAT ti_bq2415x

#include "bq2415x.h"

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <kernel.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(bq2415x, CONFIG_SENSOR_LOG_LEVEL);

/* read value from register */
static int bq2415x_i2c_read(const struct device *dev, uint8_t reg_addr)
{
	struct bq2415x_device *bq2415x = dev->data;
	const struct bq2415x_config *const config = dev->config;
	int ret;
	uint8_t value;

	ret = i2c_reg_read_byte(bq2415x->i2c, config->i2c_addr, reg_addr, &value);
	if (ret < 0) {
		return ret;
	}

	return value;
}

/* read value from register, apply mask and right shift it */
static int bq2415x_i2c_read_mask(const struct device *dev,
				 uint8_t reg_addr, uint8_t mask,
				 uint8_t shift)
{
	int ret;

	if (shift > 8) {
		return -EINVAL;
	}

	ret = bq2415x_i2c_read(dev, reg_addr);
	if (ret < 0) {
		return ret;
	}

	return (ret & mask) >> shift;
}

/* read value from register and return one specified bit */
static int bq2415x_i2c_read_bit(const struct device *dev,
				uint8_t reg_addr, uint8_t bit)
{
	if (bit > 8) {
		return -EINVAL;
	}

	return bq2415x_i2c_read_mask(dev, reg_addr, BIT(bit), bit);
}

/* write value to register */
static int bq2415x_i2c_write(const struct device *dev, uint8_t reg_addr,
			     uint8_t val)
{
	struct bq2415x_device *bq2415x = dev->data;
	const struct bq2415x_config *const config = dev->config;
	int status;

	status = i2c_reg_write_byte(bq2415x->i2c, config->i2c_addr, reg_addr, val);
	if (status < 0) {
		return status;
	}

	return 0;
}

/* read value from register, change it with mask left shifted and write back */
static int bq2415x_i2c_write_mask(const struct device *dev,
				  uint8_t reg_addr, uint8_t val, uint8_t mask,
				  uint8_t shift)
{
	int ret;

	if (shift > 8) {
		return -EINVAL;
	}

	ret = bq2415x_i2c_read(dev, reg_addr);
	if (ret < 0) {
		return ret;
	}

	ret &= ~mask;
	ret |= val << shift;

	return bq2415x_i2c_write(dev, reg_addr, ret);
}

/* change only one bit in register */
static int bq2415x_i2c_write_bit(const struct device *dev,
				 uint8_t reg_addr, bool val, uint8_t bit)
{
	if (bit > 8) {
		return -EINVAL;
	}

	return bq2415x_i2c_write_mask(dev, reg_addr, val, BIT(bit), bit);
}

/* exec command function */
static int bq2415x_exec_command(const struct device *dev,
				enum bq2415x_command command)
{
	int ret;

	switch (command) {
	case BQ2415X_TIMER_RESET:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_STATUS, 1,
					     BQ2415X_BIT_TMR_RST);
	case BQ2415X_OTG_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_STATUS, BQ2415X_BIT_OTG);
	case BQ2415X_STAT_PIN_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_STATUS,
					    BQ2415X_BIT_EN_STAT);
	case BQ2415X_STAT_PIN_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_STATUS, 1,
					     BQ2415X_BIT_EN_STAT);
	case BQ2415X_STAT_PIN_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_STATUS, 0,
					     BQ2415X_BIT_EN_STAT);
	case BQ2415X_CHARGE_STATUS:
		return bq2415x_i2c_read_mask(dev, BQ2415X_REG_STATUS,
					     BQ2415X_MASK_STAT, BQ2415X_SHIFT_STAT);
	case BQ2415X_BOOST_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_STATUS,
					    BQ2415X_BIT_BOOST);
	case BQ2415X_FAULT_STATUS:
		return bq2415x_i2c_read_mask(dev, BQ2415X_REG_STATUS,
					     BQ2415X_MASK_FAULT, BQ2415X_SHIFT_FAULT);

	case BQ2415X_CHARGE_TERMINATION_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_CONTROL, BQ2415X_BIT_TE);
	case BQ2415X_CHARGE_TERMINATION_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 1,
					     BQ2415X_BIT_TE);
	case BQ2415X_CHARGE_TERMINATION_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 0,
					     BQ2415X_BIT_TE);
	case BQ2415X_CHARGER_STATUS:
		ret = bq2415x_i2c_read_bit(dev, BQ2415X_REG_CONTROL, BQ2415X_BIT_CE);
		if (ret < 0) {
			return ret;
		}

		return ret > 0 ? 0 : 1;
	case BQ2415X_CHARGER_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 0,
					     BQ2415X_BIT_CE);
	case BQ2415X_CHARGER_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 1,
					     BQ2415X_BIT_CE);
	case BQ2415X_HIGH_IMPEDANCE_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_CONTROL,
					    BQ2415X_BIT_HZ_MODE);
	case BQ2415X_HIGH_IMPEDANCE_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 1,
					     BQ2415X_BIT_HZ_MODE);
	case BQ2415X_HIGH_IMPEDANCE_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 0,
					     BQ2415X_BIT_HZ_MODE);
	case BQ2415X_BOOST_MODE_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_CONTROL,
					    BQ2415X_BIT_OPA_MODE);
	case BQ2415X_BOOST_MODE_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 1,
					     BQ2415X_BIT_OPA_MODE);
	case BQ2415X_BOOST_MODE_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_CONTROL, 0,
					     BQ2415X_BIT_OPA_MODE);

	case BQ2415X_OTG_LEVEL:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_VOLTAGE,
					    BQ2415X_BIT_OTG_PL);
	case BQ2415X_OTG_ACTIVATE_HIGH:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_VOLTAGE, 1,
					     BQ2415X_BIT_OTG_PL);
	case BQ2415X_OTG_ACTIVATE_LOW:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_VOLTAGE, 0,
					     BQ2415X_BIT_OTG_PL);
	case BQ2415X_OTG_PIN_STATUS:
		return bq2415x_i2c_read_bit(dev, BQ2415X_REG_VOLTAGE,
					    BQ2415X_BIT_OTG_EN);
	case BQ2415X_OTG_PIN_ENABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_VOLTAGE, 1,
					     BQ2415X_BIT_OTG_EN);
	case BQ2415X_OTG_PIN_DISABLE:
		return bq2415x_i2c_write_bit(dev, BQ2415X_REG_VOLTAGE, 0,
					     BQ2415X_BIT_OTG_EN);

	case BQ2415X_VENDER_CODE:
		return bq2415x_i2c_read_mask(dev, BQ2415X_REG_VENDER,
					     BQ2415X_MASK_VENDER, BQ2415X_SHIFT_VENDER);
	case BQ2415X_PART_NUMBER:
		return bq2415x_i2c_read_mask(dev, BQ2415X_REG_VENDER, BQ2415X_MASK_PN,
					     BQ2415X_SHIFT_PN);
	case BQ2415X_REVISION:
		return bq2415x_i2c_read_mask(dev, BQ2415X_REG_VENDER,
					     BQ2415X_MASK_REVISION,
					     BQ2415X_SHIFT_REVISION);
	}
	return -EINVAL;
}

/* detect chip type */
static enum bq2415x_chip bq2415x_detect_chip(const struct device *dev)
{
	struct bq2415x_device *bq2415x = dev->data;
	int ret;

	ret = bq2415x_exec_command(dev, BQ2415X_PART_NUMBER);
	if (ret < 0) {
		return ret;
	}

	switch (ret) {
	case 0:
		if (bq2415x->chip == BQ24151A) {
			return bq2415x->chip;
		}

		return BQ24151;
	case 1:
		if (bq2415x->chip == BQ24150A || bq2415x->chip == BQ24152 ||
		    bq2415x->chip == BQ24155) {
			return bq2415x->chip;
		}

		return BQ24150;
	case 2:
		if (bq2415x->chip == BQ24153A) {
			return bq2415x->chip;
		}

		return BQ24153;
	default:
		return BQUNKNOWN;
	}

	return BQUNKNOWN;
}

/* detect chip revision */
static int bq2415x_detect_revision(const struct device *dev)
{
	int ret = bq2415x_exec_command(dev, BQ2415X_REVISION);
	int chip = bq2415x_detect_chip(dev);

	if (ret < 0 || chip < 0) {
		return -1;
	}

	switch (chip) {
	case BQ24150:
	case BQ24150A:
	case BQ24151:
	case BQ24151A:
	case BQ24152:
		if (ret >= 0 && ret <= 3) {
			return ret;
		}

		return -1;
	case BQ24153:
	case BQ24153A:
	case BQ24156:
	case BQ24156A:
	case BQ24157S:
	case BQ24158:
		if (ret == 3) {
			return 0;
		} else if (ret == 1) {
			return 1;
		}
		return -1;
	case BQ24155:
		if (ret == 3) {
			return 3;
		}

		return -1;
	case BQUNKNOWN:
		return -1;
	}

	return -1;
}

/* return chip vender code */
static int bq2415x_get_vender_code(const struct device *dev)
{
	int ret;

	ret = bq2415x_exec_command(dev, BQ2415X_VENDER_CODE);
	if (ret < 0) {
		return 0;
	}

	/* convert to binary */
	return (ret & 0x1) + ((ret >> 1) & 0x1) * 10 + ((ret >> 2) & 0x1) * 100;
}

/* enable/disable auto resetting chip timer */
static int bq2415x_set_autotimer(const struct device *dev, int state)
{
	struct bq2415x_device *bq2415x = dev->data;

	if (state) {
		k_work_schedule(&bq2415x->dwork_timer_reset, K_SECONDS(BQ2415X_TIMER_TIMEOUT));
		int ret;

		ret = bq2415x_exec_command(dev, BQ2415X_TIMER_RESET);
		if (ret < 0) {
			LOG_ERR("Failed to reset timer");
			return ret;
		}
	} else {
		k_work_cancel_delayable(&bq2415x->dwork_timer_reset);
	}

	return 0;
}

/* delayed work function for auto resetting chip timer */
static void bq2415x_timer_work(struct k_work *work)
{
	struct bq2415x_device *bq2415x =
		CONTAINER_OF(work, struct bq2415x_device, dwork_timer_reset);
	int ret;

	ret = bq2415x_exec_command(bq2415x->dev, BQ2415X_TIMER_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to reset timer.");
		bq2415x_set_autotimer(bq2415x->dev, 0);
		return;
	}

	k_work_schedule(&bq2415x->dwork_timer_reset, K_SECONDS(BQ2415X_TIMER_TIMEOUT));
}


/* reset all chip registers to default state */
static int bq2415x_reset_chip(const struct device *dev)
{
	int ret;

	ret =   bq2415x_i2c_write(dev, BQ2415X_REG_CURRENT, BQ2415X_RESET_CURRENT);
	ret += bq2415x_i2c_write(dev, BQ2415X_REG_VOLTAGE, BQ2415X_RESET_VOLTAGE);
	ret += bq2415x_i2c_write(dev, BQ2415X_REG_CONTROL, BQ2415X_RESET_CONTROL);
	ret += bq2415x_i2c_write(dev, BQ2415X_REG_STATUS, BQ2415X_RESET_STATUS);
	return ret;
}

/* set current limit in mA */
static int bq2415x_set_current_limit(const struct device *dev, int mA)
{
	int val;

	if (mA <= 100) {
		val = 0;
	} else if (mA <= 500) {
		val = 1;
	} else if (mA <= 800) {
		val = 2;
	} else {
		val = 3;
	}

	return bq2415x_i2c_write_mask(dev, BQ2415X_REG_CONTROL, val,
				      BQ2415X_MASK_LIMIT, BQ2415X_SHIFT_LIMIT);
}

/* get current limit in mA */
static int bq2415x_get_current_limit(const struct device *dev)
{
	int ret;

	ret = bq2415x_i2c_read_mask(dev, BQ2415X_REG_CONTROL, BQ2415X_MASK_LIMIT,
				    BQ2415X_SHIFT_LIMIT);
	if (ret < 0) {
		return ret;
	} else if (ret == 0) {
		return 100;
	} else if (ret == 1) {
		return 500;
	} else if (ret == 2) {
		return 800;
	} else if (ret == 3) {
		return 1800;
	}
	return -EINVAL;
}

/* set weak battery voltage in mV */
static int bq2415x_set_weak_voltage(const struct device *dev, int mV)
{
	int val;

	/* round to 100mV */
	if (mV <= 3400 + 50) {
		val = 0;
	} else if (mV <= 3500 + 50) {
		val = 1;
	} else if (mV <= 3600 + 50) {
		val = 2;
	} else {
		val = 3;
	}

	return bq2415x_i2c_write_mask(dev, BQ2415X_REG_CONTROL, val,
				      BQ2415X_MASK_VLOWV, BQ2415X_SHIFT_VLOWV);
}

/* get weak battery voltage in mV */
static int bq2415x_get_weak_voltage(const struct device *dev)
{
	int ret;

	ret = bq2415x_i2c_read_mask(dev, BQ2415X_REG_CONTROL, BQ2415X_MASK_VLOWV,
				    BQ2415X_SHIFT_VLOWV);
	if (ret < 0) {
		return ret;
	}

	return 100 * (34 + ret);
}

/* set battery regulation voltage in mV */
static int bq2415x_set_regulation_voltage(const struct device *dev,
					  int mV)
{
	/* Formula origin; bq24155 datasheet, pg.21, Table 5
	 * Charge voltage range = 3500mV to 4440mV with an offset of 3500mV and step of 20mV
	 * val = ((mV - min_charge_voltage) - offset) / step
	 */
	int val = (mV / 10 - 350) / 2;

	/*
	 * According to datasheet, maximum battery regulation voltage is
	 * 4440mV which is b101111 = 47.
	 */
	if (val < 0) {
		val = 0;
	} else if (val > 47) {
		return -EINVAL;
	}

	return bq2415x_i2c_write_mask(dev, BQ2415X_REG_VOLTAGE, val, BQ2415X_MASK_VO,
				      BQ2415X_SHIFT_VO);
}

/* get battery regulation voltage in mV */
static int bq2415x_get_regulation_voltage(const struct device *dev)
{
	int ret;

	ret = bq2415x_i2c_read_mask(dev, BQ2415X_REG_VOLTAGE, BQ2415X_MASK_VO,
				    BQ2415X_SHIFT_VO);
	if (ret < 0) {
		return ret;
	}

	/* same as in bq2415x_set_regulation_voltage, but reversed */
	return 10 * (350 + 2 * ret);
}

/* set charge current in mA (platform data must provide resistor sense) */
static int bq2415x_set_charge_current(const struct device *dev, int mA)
{
	const struct bq2415x_config *const config = dev->config;
	int val;

	if (config->resistor_sense <= 0) {
		return -EINVAL;
	}

	/* I_charge = I_base + I_charge_step; bq2415 datasheet, Table 9, pg.22
	 * offset = 37.4mV; divide by 6.8 to normalize
	 */
	val = (mA * config->resistor_sense - 37400) / 6800;
	if (val < 0) {
		val = 0;
	} else if (val > 7) {
		val = 7;
	}

	return bq2415x_i2c_write_mask(dev, BQ2415X_REG_CURRENT, val,
				      BQ2415X_MASK_VI_CHRG | BQ2415X_MASK_RESET,
				      BQ2415X_SHIFT_VI_CHRG);
}

/* get charge current in mA (platform data must provide resistor sense) */
static int bq2415x_get_charge_current(const struct device *dev)
{
	const struct bq2415x_config *const config = dev->config;
	int ret;

	if (config->resistor_sense <= 0) {
		return -EINVAL;
	}

	ret = bq2415x_i2c_read_mask(dev, BQ2415X_REG_CURRENT, BQ2415X_MASK_VI_CHRG,
				    BQ2415X_SHIFT_VI_CHRG);
	if (ret < 0) {
		return ret;
	}

	/* same as for bq2415x_set_charge_current, but reversed */
	return (37400 + 6800 * ret) / config->resistor_sense;
}

/* set termination current in mA (platform data must provide resistor sense) */
static int bq2415x_set_termination_current(const struct device *dev, int mA)
{
	const struct bq2415x_config *const config = dev->config;
	int val;

	if (config->resistor_sense <= 0) {
		return -EINVAL;
	}

	/* I_termination = I_base + I_term_step; bq2415 datasheet, Table 8, pg.22
	 * offset = 3.4mV; divide by 3.4 to normalize
	 */
	val = (mA * config->resistor_sense - 3400) / 3400;
	if (val < 0) {
		val = 0;
	} else if (val > 7) {
		val = 7;
	}

	return bq2415x_i2c_write_mask(dev, BQ2415X_REG_CURRENT, val,
				      BQ2415X_MASK_VI_TERM | BQ2415X_MASK_RESET,
				      BQ2415X_SHIFT_VI_TERM);
}

/* get termination current in mA (platform data must provide resistor sense) */
static int bq2415x_get_termination_current(const struct device *dev)
{
	const struct bq2415x_config *const config = dev->config;
	int ret;

	if (config->resistor_sense <= 0) {
		return -EINVAL;
	}

	ret = bq2415x_i2c_read_mask(dev, BQ2415X_REG_CURRENT, BQ2415X_MASK_VI_TERM,
				    BQ2415X_SHIFT_VI_TERM);
	if (ret < 0) {
		return ret;
	}

	/* same as for bq2415x_set_termination_current, but reversed */
	return (3400 + 3400 * ret) / config->resistor_sense;
}

/* set default values of all properties */
static int bq2415x_set_defaults(const struct device *dev)
{
	const struct bq2415x_config *const config = dev->config;
	int ret;

	ret = bq2415x_exec_command(dev, BQ2415X_BOOST_MODE_DISABLE);
	ret += bq2415x_exec_command(dev, BQ2415X_CHARGER_DISABLE);
	ret += bq2415x_exec_command(dev, BQ2415X_CHARGE_TERMINATION_DISABLE);

	ret += bq2415x_set_current_limit(dev, config->current_limit);
	ret += bq2415x_set_weak_voltage(dev, config->weak_voltage);
	ret += bq2415x_set_regulation_voltage(dev, config->regulation_voltage);

	if (config->resistor_sense > 0) {
		ret += bq2415x_set_charge_current(dev, config->charge_current);
		ret += bq2415x_set_termination_current(dev, config->termination_current);
		ret += bq2415x_exec_command(dev, BQ2415X_CHARGE_TERMINATION_ENABLE);
	}

	return ret;
}

static int bq2415x_set_current_config(const struct device *dev,
				      enum sensor_attribute attr,
				      const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_CHARGE_CURRENT:
		return bq2415x_set_charge_current(dev, val->val2 / 1000);
	case SENSOR_ATTR_TERMINATION_CURRENT:
		return bq2415x_set_termination_current(dev, val->val2 / 1000);
	case SENSOR_ATTR_INPUT_CURRENT:
		return bq2415x_set_current_limit(dev, val->val2 / 1000);
	default:
		LOG_ERR("Current attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq2415x_set_voltage_config(const struct device *dev,
				      enum sensor_attribute attr,
				      const struct sensor_value *val)
{
	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		return bq2415x_set_weak_voltage(dev, val->val2 / 1000);
	case SENSOR_ATTR_UPPER_THRESH:
		return bq2415x_set_regulation_voltage(dev, val->val2 / 1000);
	default:
		LOG_ERR("Voltage attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq2415x_set_common_config(const struct device *dev,
				     enum sensor_attribute attr,
				     const struct sensor_value *val)
{
	int status;

	switch (attr) {
	case SENSOR_ATTR_CHARGE_CONTROL:
		if (val->val1 == 0) {
			status = bq2415x_set_autotimer(dev, 0);
			if (status < 0) {
				return status;
			}
			return bq2415x_exec_command(dev, BQ2415X_CHARGER_DISABLE);
		} else if (val->val1 == 1) {
			status = bq2415x_set_autotimer(dev, 1);
			if (status < 0) {
				return status;
			}
			return bq2415x_exec_command(dev, BQ2415X_CHARGER_ENABLE);
		}
		LOG_ERR("Value given %d not supported.", val->val1);
		return -ENOTSUP;

	default:
		LOG_ERR("Attribute not supported.");
		return -ENOTSUP;
	}
}

static int bq2415x_attr_set(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    const struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_CURRENT:
		return bq2415x_set_current_config(dev, attr, val);
	case SENSOR_CHAN_VOLTAGE:
		return bq2415x_set_voltage_config(dev, attr, val);
	case SENSOR_CHAN_CHARGER_CONTROL:
		return bq2415x_set_common_config(dev, attr, val);
	default:
		LOG_ERR("attr_set() not supported on this channel.");
		return -ENOTSUP;
	}

	return 0;
}

static int bq2415x_get_current_config(const struct device *dev,
				      enum sensor_attribute attr,
				      struct sensor_value *val)
{
	int status = 0;

	switch (attr) {
	case SENSOR_ATTR_CHARGE_CURRENT:
		status = bq2415x_get_charge_current(dev);
		break;
	case SENSOR_ATTR_TERMINATION_CURRENT:
		status = bq2415x_get_termination_current(dev);
		break;
	case SENSOR_ATTR_INPUT_CURRENT:
		status = bq2415x_get_current_limit(dev);
		break;
	default:
		LOG_ERR("Current attribute not supported.");
		return -ENOTSUP;
	}

	if (status < 0) {
		return status;
	}

	val->val1 = 0;
	val->val2 = status * 1000;

	return 0;
}

static int bq2415x_get_voltage_config(const struct device *dev,
				      enum sensor_attribute attr,
				      struct sensor_value *val)
{
	int status = 0;

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		status = bq2415x_get_weak_voltage(dev);
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		status = bq2415x_get_regulation_voltage(dev);
		break;
	default:
		LOG_ERR("Voltage attribute not supported.");
		return -ENOTSUP;
	}

	if (status < 0) {
		return status;
	}

	val->val1 = 0;
	val->val2 = status * 1000;

	return 0;
}

static int bq2415x_attr_get(const struct device *dev, enum sensor_channel chan,
			    enum sensor_attribute attr,
			    struct sensor_value *val)
{
	switch (chan) {
	case SENSOR_CHAN_CURRENT:
		return bq2415x_get_current_config(dev, attr, val);
	case SENSOR_CHAN_VOLTAGE:
		return bq2415x_get_voltage_config(dev, attr, val);
	default:
		LOG_ERR("attr_get() not supported on this channel.");
		return -ENOTSUP;
	}
}


static int bq2415x_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct bq2415x_device *bq2415x = dev->data;
	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_CHARGER_FAULT_STATUS:
		status = bq2415x_exec_command(dev, BQ2415X_FAULT_STATUS);
		bq2415x->fault_status = status;
		break;

	case SENSOR_CHAN_CHARGER_CHARGING_STATUS:
		status = bq2415x_exec_command(dev, BQ2415X_CHARGE_STATUS);
		bq2415x->charge_status = status;
		break;

	default:
		LOG_ERR("Channel not supported.");
		status = -ENOTSUP;
	}

	return status;
}

static int bq2415x_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct bq2415x_device *bq2415x = dev->data;

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

static int bq2415x_init(const struct device *dev)
{
	struct bq2415x_device *bq2415x = dev->data;
	const struct bq2415x_config *const config = dev->config;
	int status = 0;

	bq2415x->i2c = device_get_binding(config->bus_name);
	if (bq2415x->i2c == NULL) {
		LOG_ERR("I2C master controller not found: %s.", config->bus_name);
		return -EINVAL;
	}

	bq2415x->dev = dev;

	status = bq2415x_reset_chip(dev);
	if (status < 0) {
		LOG_ERR("Cannot reset chip");
		return -EIO;
	}

	status = bq2415x_detect_chip(dev);
	if (status < 0) {
		LOG_ERR("Cannot detect chip");
		return -EIO;
	}

	status = bq2415x_detect_revision(dev);
	if (status < 0) {
		LOG_ERR("Cannot detect chip revision");
		return -EIO;
	}

	status = bq2415x_get_vender_code(dev);
	if (status < 0) {
		LOG_ERR("Failed to read vender ID");
		return -EIO;
	}

	if (status != BQ2415X_DEFAULT_VENDER_CODE) {
		LOG_ERR("Unsupported chip detected (0x%x)!", status);
		return -ENODEV;
	}

	status = bq2415x_set_defaults(dev);
	if (status < 0) {
		LOG_ERR("Cannot set default values");
		return -EIO;
	}

	k_work_init_delayable(&bq2415x->dwork_timer_reset, bq2415x_timer_work);

	return 0;
}

static const struct sensor_driver_api bq2415x_battery_driver_api = {
	.attr_set = bq2415x_attr_set,
	.attr_get = bq2415x_attr_get,
	.sample_fetch = bq2415x_sample_fetch,
	.channel_get = bq2415x_channel_get,
};

#define BQ2415X_INIT(index)							      \
	static struct bq2415x_device bq2415x_driver_##index;			      \
										      \
	static const struct bq2415x_config bq2415x_config_##index = {		      \
		.bus_name = DT_INST_BUS_LABEL(index),				      \
		.i2c_addr =  DT_INST_REG_ADDR(index),				      \
		.current_limit = DT_INST_PROP(index, current_limit),		      \
		.weak_voltage = DT_INST_PROP(index, weak_voltage),		      \
		.regulation_voltage =						      \
			DT_INST_PROP(index, regulation_voltage),		      \
		.charge_current = DT_INST_PROP(index, charge_current),		      \
		.termination_current = DT_INST_PROP(index, termination_current),      \
		.resistor_sense = DT_INST_PROP(index, resistor_sense),		      \
	};									      \
										      \
	DEVICE_DT_INST_DEFINE(							      \
		index, &bq2415x_init, device_pm_control_nop, &bq2415x_driver_##index, \
		&bq2415x_config_##index, POST_KERNEL,				      \
		CONFIG_SENSOR_INIT_PRIORITY, &bq2415x_battery_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BQ2415X_INIT)
