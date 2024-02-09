/*
 * Copyright 2023 Grinn
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT maxim_max20335_charger

#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/linear_range.h>

#include "zephyr/logging/log.h"
LOG_MODULE_REGISTER(max20335_charger);

#define MAX20335_REG_STATUSA 0x02
#define MAX20335_REG_STATUSB 0x03
#define MAX20335_REG_INTA 0x05
#define MAX20335_REG_INTB 0x06
#define MAX20335_REG_INTMASKA 0x07
#define MAX20335_REG_INTMASKB 0x08
#define MAX20335_REG_ILIMCNTL 0x09
#define MAX20335_REG_CHGCNTLA 0x0A

#define MAX20335_INTA_USBOK_MASK BIT(3)
#define MAX20335_INTA_CHGSTAT_MASK BIT(6)
#define MAX20335_ILIMCNTL_ILIMCNTL_MASK GENMASK(1, 0)
#define MAX20335_STATUSA_CHGSTAT_MASK GENMASK(2, 0)
#define MAX20335_STATUSB_USBOK_MASK BIT(3)
#define MAX20335_CHGCNTLA_BATREG_MASK GENMASK(4, 1)
#define MAX20335_CHGCNTLA_CHRGEN_MASK BIT(0)
#define MAX20335_CHGCNTLA_CHRGEN BIT(0)

#define MAX20335_REG_CVC_VREG_MIN_UV 4050000U
#define MAX20335_REG_CVC_VREG_STEP_UV 50000U
#define MAX20335_REG_CVC_VREG_MIN_IDX 0x0U
#define MAX20335_REG_CVC_VREG_MAX_IDX 0x0BU

#define INT_ENABLE_DELAY K_MSEC(500)

struct charger_max20335_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec int_gpio;
	uint32_t max_vreg_uv;
	uint32_t max_ichgin_to_sys_ua;
};

struct charger_max20335_data {
	const struct device *dev;
	struct gpio_callback gpio_cb;
	struct k_work int_routine_work;
	struct k_work_delayable int_enable_work;
	enum charger_status charger_status;
	enum charger_online charger_online;
	charger_status_notifier_t charger_status_notifier;
	charger_online_notifier_t charger_online_notifier;
	bool charger_enabled;
	uint32_t charge_voltage_uv;
};

static const struct linear_range charger_uv_range =
	LINEAR_RANGE_INIT(MAX20335_REG_CVC_VREG_MIN_UV,
			  MAX20335_REG_CVC_VREG_STEP_UV,
			  MAX20335_REG_CVC_VREG_MIN_IDX,
			  MAX20335_REG_CVC_VREG_MAX_IDX);

static int max20335_get_charger_status(const struct device *dev, enum charger_status *status)
{
	enum {
		MAX20335_CHARGER_OFF,
		MAX20335_CHARGING_SUSPENDED_DUE_TO_TEMPERATURE,
		MAX20335_PRE_CHARGE_IN_PROGRESS,
		MAX20335_FAST_CHARGE_IN_PROGRESS_1,
		MAX20335_FAST_CHARGE_IN_PROGRESS_2,
		MAX20335_MAINTAIN_CHARGE_IN_PROGRESS,
		MAX20335_MAIN_CHARGER_TIMER_DONE,
		MAX20335_CHARGER_FAULT_CONDITION,
	};
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_STATUSA, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(MAX20335_STATUSA_CHGSTAT_MASK, val);

	switch (val) {
	case MAX20335_CHARGER_OFF:
		__fallthrough;
	case MAX20335_CHARGING_SUSPENDED_DUE_TO_TEMPERATURE:
		__fallthrough;
	case MAX20335_CHARGER_FAULT_CONDITION:
		*status = CHARGER_STATUS_NOT_CHARGING;
		break;
	case MAX20335_PRE_CHARGE_IN_PROGRESS:
		__fallthrough;
	case MAX20335_FAST_CHARGE_IN_PROGRESS_1:
		__fallthrough;
	case MAX20335_FAST_CHARGE_IN_PROGRESS_2:
		__fallthrough;
	case MAX20335_MAINTAIN_CHARGE_IN_PROGRESS:
		*status = CHARGER_STATUS_CHARGING;
		break;
	case MAX20335_MAIN_CHARGER_TIMER_DONE:
		*status = CHARGER_STATUS_FULL;
		break;
	default:
		*status = CHARGER_STATUS_UNKNOWN;
		break;
	};

	return 0;
}

static int max20335_get_charger_online(const struct device *dev, enum charger_online *online)
{
	enum {
		MAX20335_CHGIN_IN_NOT_PRESENT_OR_INVALID,
		MAX20335_CHGIN_IN_PRESENT_AND_VALID,
	};
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;
	int ret;

	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_STATUSB, &val);
	if (ret) {
		return ret;
	}

	val = FIELD_GET(MAX20335_STATUSB_USBOK_MASK, val);

	switch (val) {
	case MAX20335_CHGIN_IN_PRESENT_AND_VALID:
		*online = CHARGER_ONLINE_FIXED;
		break;
	default:
		*online = CHARGER_ONLINE_OFFLINE;
		break;
	};

	return 0;
}

static int max20335_set_constant_charge_voltage(const struct device *dev,
						uint32_t voltage_uv)
{
	const struct charger_max20335_config *const config = dev->config;
	uint16_t idx;
	uint8_t val;
	int ret;

	ret = linear_range_get_index(&charger_uv_range, voltage_uv, &idx);
	if (ret < 0) {
		return ret;
	}

	val = FIELD_PREP(MAX20335_CHGCNTLA_BATREG_MASK, idx);

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_CHGCNTLA,
				      MAX20335_CHGCNTLA_BATREG_MASK,
				      val);
}

static int max20335_set_chgin_to_sys_current_limit(const struct device *dev, uint32_t current_ua)
{
	const struct charger_max20335_config *const config = dev->config;
	uint8_t val;

	switch (current_ua) {
	case 0:
		val = 0x00;
		break;
	case 100000:
		val = 0x01;
		break;
	case 500000:
		val = 0x02;
		break;
	case 1000000:
		val = 0x03;
		break;
	default:
		return -ENOTSUP;
	};

	val = FIELD_PREP(MAX20335_ILIMCNTL_ILIMCNTL_MASK, val);

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_ILIMCNTL,
				      MAX20335_ILIMCNTL_ILIMCNTL_MASK,
				      val);
}

static int max20335_set_enabled(const struct device *dev, bool enable)
{
	struct charger_max20335_data *data = dev->data;
	const struct charger_max20335_config *const config = dev->config;

	data->charger_enabled = enable;

	return i2c_reg_update_byte_dt(&config->bus,
				      MAX20335_REG_CHGCNTLA,
				      MAX20335_CHGCNTLA_CHRGEN_MASK,
				      enable ? MAX20335_CHGCNTLA_CHRGEN : 0);
}

static int max20335_get_interrupt_source(const struct device *dev, uint8_t *int_a, uint8_t *int_b)
{
	const struct charger_max20335_config *config = dev->config;
	uint8_t dummy;
	uint8_t *int_src;
	int ret;

	/* Both INT_A and INT_B registers need to be read to clear all int flags */

	int_src = (int_a != NULL) ? int_a : &dummy;
	ret = i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_INTA, int_src);
	if (ret < 0) {
		return ret;
	}

	int_src = (int_b != NULL) ? int_b : &dummy;

	return i2c_reg_read_byte_dt(&config->bus, MAX20335_REG_INTB, int_src);
}

static int max20335_enable_interrupts(const struct device *dev)
{
	enum {MASKA_VAL_ENABLE = 0xFF};
	const struct charger_max20335_config *config = dev->config;
	int ret;

	ret = max20335_get_interrupt_source(dev, NULL, NULL);
	if (ret < 0) {
		LOG_WRN("Failed to clear pending interrupts: %d", ret);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->bus, MAX20335_REG_INTMASKA, MASKA_VAL_ENABLE);
	if (ret < 0) {
		return ret;
	}

	return i2c_reg_write_byte_dt(&config->bus, MAX20335_REG_INTMASKB, 0);
}

static int max20335_init_properties(const struct device *dev)
{
	struct charger_max20335_data *data = dev->data;
	const struct charger_max20335_config *config = dev->config;
	int ret;

	data->charge_voltage_uv = config->max_vreg_uv;
	data->charger_enabled = true;

	ret = max20335_get_charger_status(dev, &data->charger_status);
	if (ret < 0) {
		LOG_ERR("Failed to read charger status: %d", ret);
		return ret;
	}

	ret = max20335_get_charger_online(dev, &data->charger_online);
	if (ret < 0) {
		LOG_ERR("Failed to read charger online: %d", ret);
		return ret;
	}

	return 0;
}

static int max20335_update_properties(const struct device *dev)
{
	struct charger_max20335_data *data = dev->data;
	const struct charger_max20335_config *config = dev->config;
	int ret;

	ret = max20335_set_chgin_to_sys_current_limit(dev, config->max_ichgin_to_sys_ua);
	if (ret < 0) {
		LOG_ERR("Failed to set chgin-to-sys current limit: %d", ret);
		return ret;
	}

	ret = max20335_set_constant_charge_voltage(dev, data->charge_voltage_uv);
	if (ret < 0) {
		LOG_ERR("Failed to set charge voltage: %d", ret);
		return ret;
	}

	ret = max20335_set_enabled(dev, data->charger_enabled);
	if (ret < 0) {
		LOG_ERR("Failed to set enabled: %d", ret);
		return ret;
	}

	return 0;
}

static int max20335_get_prop(const struct device *dev, charger_prop_t prop,
			     union charger_propval *val)
{
	struct charger_max20335_data *data = dev->data;

	switch (prop) {
	case CHARGER_PROP_ONLINE:
		val->online = data->charger_online;
		return 0;
	case CHARGER_PROP_STATUS:
		val->status = data->charger_status;
		return 0;
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		val->const_charge_voltage_uv = data->charge_voltage_uv;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max20335_set_prop(const struct device *dev, charger_prop_t prop,
			     const union charger_propval *val)
{
	struct charger_max20335_data *data = dev->data;
	int ret;

	switch (prop) {
	case CHARGER_PROP_CONSTANT_CHARGE_VOLTAGE_UV:
		ret =  max20335_set_constant_charge_voltage(dev, val->const_charge_voltage_uv);
		if (ret == 0) {
			data->charge_voltage_uv = val->const_charge_voltage_uv;
		}

		return ret;
	case CHARGER_PROP_STATUS_NOTIFICATION:
		data->charger_status_notifier = val->status_notification;
		return 0;
	case CHARGER_PROP_ONLINE_NOTIFICATION:
		data->charger_online_notifier = val->online_notification;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int max20335_enable_interrupt_pin(const struct device *dev, bool enabled)
{
	const struct charger_max20335_config *const config = dev->config;
	gpio_flags_t flags;
	int ret;

	flags = enabled ? GPIO_INT_LEVEL_ACTIVE : GPIO_INT_DISABLE;

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, flags);
	if (ret < 0) {
		LOG_ERR("Could not %s interrupt GPIO callback: %d", enabled ? "enable" : "disable",
			ret);
	}

	return ret;
}

static void max20335_gpio_callback(const struct device *dev, struct gpio_callback *cb,
				   uint32_t pins)
{
	struct charger_max20335_data *data = CONTAINER_OF(cb, struct charger_max20335_data,
							  gpio_cb);
	int ret;

	(void) max20335_enable_interrupt_pin(data->dev, false);

	ret = k_work_submit(&data->int_routine_work);
	if (ret < 0) {
		LOG_WRN("Could not submit int work: %d", ret);
	}
}

static void max20335_int_routine_work_handler(struct k_work *work)
{
	struct charger_max20335_data *data = CONTAINER_OF(work, struct charger_max20335_data,
							  int_routine_work);
	uint8_t int_src_a;
	int ret;

	ret = max20335_get_interrupt_source(data->dev, &int_src_a, NULL);
	if (ret < 0) {
		LOG_WRN("Failed to read interrupt source");
		return;
	}

	if ((int_src_a & MAX20335_INTA_CHGSTAT_MASK) != 0) {
		ret = max20335_get_charger_status(data->dev, &data->charger_status);
		if (ret < 0) {
			LOG_WRN("Failed to read charger status: %d", ret);
		} else {
			if (data->charger_status_notifier != NULL) {
				data->charger_status_notifier(data->charger_status);
			}
		}
	}

	if ((int_src_a & MAX20335_INTA_USBOK_MASK) != 0) {
		ret = max20335_get_charger_online(data->dev, &data->charger_online);
		if (ret < 0) {
			LOG_WRN("Failed to read charger online %d", ret);
		} else {
			if (data->charger_online_notifier != NULL) {
				data->charger_online_notifier(data->charger_online);
			}
		}

		if (data->charger_online != CHARGER_ONLINE_OFFLINE) {
			(void) max20335_update_properties(data->dev);
		}
	}

	ret = k_work_reschedule(&data->int_enable_work, INT_ENABLE_DELAY);
	if (ret < 0) {
		LOG_WRN("Could not reschedule int_enable_work: %d", ret);
	}
}

static void max20335_int_enable_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct charger_max20335_data *data = CONTAINER_OF(dwork, struct charger_max20335_data,
							  int_enable_work);

	(void) max20335_enable_interrupt_pin(data->dev, true);
}

static int max20335_configure_interrupt_pin(const struct device *dev)
{
	struct charger_max20335_data *data = dev->data;
	const struct charger_max20335_config *config = dev->config;
	int ret;

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("Interrupt GPIO device not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure interrupt GPIO");
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max20335_gpio_callback, BIT(config->int_gpio.pin));
	ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Could not add interrupt GPIO callback");
		return ret;
	}

	return 0;
}

static int max20335_init(const struct device *dev)
{
	struct charger_max20335_data *data = dev->data;
	const struct charger_max20335_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->bus)) {
		return -ENODEV;
	}

	data->dev = dev;

	ret = max20335_init_properties(dev);
	if (ret < 0) {
		return ret;
	}

	k_work_init(&data->int_routine_work, max20335_int_routine_work_handler);
	k_work_init_delayable(&data->int_enable_work, max20335_int_enable_work_handler);

	ret = max20335_configure_interrupt_pin(dev);
	if (ret < 0) {
		return ret;
	}

	ret = max20335_enable_interrupt_pin(dev, true);
	if (ret < 0) {
		return ret;
	}

	ret = max20335_enable_interrupts(dev);
	if (ret < 0) {
		LOG_ERR("Failed to enable interrupts");
		return ret;
	}

	return 0;
}

static const struct charger_driver_api max20335_driver_api = {
	.get_property = max20335_get_prop,
	.set_property = max20335_set_prop,
	.charge_enable = max20335_set_enabled,
};

#define MAX20335_DEFINE(inst)									\
	static struct charger_max20335_data charger_max20335_data_##inst;			\
	static const struct charger_max20335_config charger_max20335_config_##inst = {		\
		.bus = I2C_DT_SPEC_GET(DT_INST_PARENT(inst)),					\
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),				\
		.max_vreg_uv = DT_INST_PROP(inst, constant_charge_voltage_max_microvolt),	\
		.max_ichgin_to_sys_ua = DT_INST_PROP(inst, chgin_to_sys_current_limit_microamp),\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &max20335_init, NULL, &charger_max20335_data_##inst,	\
			      &charger_max20335_config_##inst,					\
			      POST_KERNEL, CONFIG_MFD_INIT_PRIORITY,				\
			      &max20335_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MAX20335_DEFINE)
