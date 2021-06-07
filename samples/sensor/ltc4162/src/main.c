/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

static const char *get_charger_status(enum charger_status value)
{
	switch (value) {
	case CHARGER_STATUS_CHARGING:
		return "charging";
	case CHARGER_STATUS_NOT_CHARGING:
		return "not charging";
	case CHARGER_STATUS_UNKNOWN:
		return "unknown";
	default:
		return NULL;
	}
}

static const char *get_charger_type(enum charge_type type)
{
	switch (type) {
	case CHARGER_CHARGE_TYPE_NONE:
		return "not known";
	case CHARGER_CHARGE_TYPE_FAST:
		return "fast";
	case CHARGER_CHARGE_TYPE_TRICKLE:
		return "trickle";
	default:
		return NULL;
	}
}

static const char *get_charger_health(enum charger_health health)
{
	switch (health) {
	case CHARGER_HEALTH_UNSPEC_FAILURE:
		return "unspec failure";
	case CHARGER_HEALTH_DEAD:
		return "dead";
	case CHARGER_HEALTH_GOOD:
		return "good";
	default:
		return NULL;
	}
}

static void ltc4162_show_values(const char *type, struct sensor_value value)
{
	if ((value.val2 < 0) && (value.val1 >= 0)) {
		value.val2 = -(value.val2);
		printk("%s: -%d.%06d\n", type, value.val1, value.val2);
	} else if ((value.val2 > 0) && (value.val1 < 0)) {
		printk("%s: %d.%06d\n", type, value.val1, value.val2);
	} else if ((value.val2 < 0) && (value.val1 < 0)) {
		value.val2 = -(value.val2);
		printk("%s: %d.%06d\n", type, value.val1, value.val2);
	} else {
		printk("%s: %d.%06d\n", type, value.val1, value.val2);
	}
}

static void process_sample(const struct device *dev)
{
	const char *charger_status, *charger_type, *charger_health;
	struct sensor_value chrg_status, chrg_type,
	       input_current_limit, charger_temperature,
	       const_chrg_current, const_chrg_current_max,
	       chrg_health, input_voltage, input_current,
	       const_chrg_voltage, const_chrg_voltage_max;

	if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_CHARGER_STATUS) < 0) {
		printk("Failed to fetch charger status\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_STATUS,
			       &chrg_status) < 0) {
		printk("Can't read LTC4162 Charger Status channel\n");
		return;
	}

	charger_status = get_charger_status(chrg_status.val1);
	printk("Charger_status: %s\n", charger_status);

	if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_CHARGER_TYPE) < 0) {
		printk("Failed to fetch charger type\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_TYPE,
			       &chrg_type) < 0) {
		printk("Can't read LTC4162 Charger type channel\n");
		return;
	}

	charger_type = get_charger_type(chrg_type.val1);
	printk("Charger_type: %s\n", charger_type);

	if (sensor_sample_fetch_chan(dev, SENSOR_CHAN_CHARGER_HEALTH) < 0) {
		printk("Failed to fetch charger health\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_HEALTH,
			       &chrg_health) < 0) {
		printk("Can't read LTC4162 Charger health channel\n");
		return;
	}

	charger_health = get_charger_health(chrg_health.val1);
	printk("Charger_health: %s\n", charger_health);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_INPUT_VOLTAGE) < 0) {
		printk("Failed to fetch input voltage\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_INPUT_VOLTAGE,
			       &input_voltage) < 0) {
		printk("Can't read LTC4162 Charger Input Voltage\n");
		return;
	}

	ltc4162_show_values("Input Voltage in Volt", input_voltage);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_INPUT_CURRENT) < 0) {
		printk("Failed to fetch input current\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_INPUT_CURRENT,
			       &input_current) < 0) {
		printk("Can't read LTC4162 Charger Input current\n");
		return;
	}

	ltc4162_show_values("Input Current in Amps", input_current);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT) < 0) {
		printk("Failed to fetch constant charge current\n");
		return;
	}

	if (sensor_channel_get(dev,
			SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT, &const_chrg_current) < 0) {
		printk("Can't read LTC4162 constant charge current\n");
	}

	ltc4162_show_values("Constant current in Amps", const_chrg_current);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX) < 0) {
		printk("Failed to fetch constant charge current max\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX,
			       &const_chrg_current_max) < 0) {
		printk("Can't read LTC4162 constant charge current max\n");
		return;
	}

	ltc4162_show_values("Constant current max in Amps", const_chrg_current_max);


	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE) < 0) {
		printk("Failed to fetch constant charge voltage\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE,
			       &const_chrg_voltage) < 0) {
		printk("Can't read LTC4162 const charge voltage\n");
		return;
	}

	ltc4162_show_values("Constant voltage in Volt", const_chrg_voltage);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE_MAX) < 0) {
		printk("Failed to fetch constant charge voltage max\n");
		return;
	}

	if (sensor_channel_get(dev,
			       SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_VOLTAGE_MAX,
			       &const_chrg_voltage_max) < 0) {
		printk("Can't read LTC4162 const charge voltage max\n");
	}

	ltc4162_show_values("Constant voltage max in Volt", const_chrg_voltage_max);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT) < 0) {
		printk("Failed to fetch input current limit\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT,
			       &input_current_limit) < 0) {
		printk("Can't read LTC4162 input current limit\n");
	}

	ltc4162_show_values("Input current limit in Amps", input_current_limit);

	if (sensor_sample_fetch_chan(dev,
			SENSOR_CHAN_CHARGER_TEMPERATURE) < 0) {
		printk("Failed to fetch charger temperature\n");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_CHARGER_TEMPERATURE,
			       &charger_temperature) < 0) {
		printk("Can't read LTC4162 Charger temperature\n");
		return;
	}

	ltc4162_show_values("Charger die temperature in Celsius", charger_temperature);

	printk("\n");
}

void main(void)
{
	int ret;
	struct sensor_value chrg_prop;
	const struct device *dev = device_get_binding("LTC4162");

	if (dev == NULL) {
		printk("No device found...\n");
		return;
	}

	chrg_prop.val1 = 3;
	chrg_prop.val2 = 0;

	/** Set constant charge current max. range: 0 - 31 **/
	ret = sensor_attr_set(dev, SENSOR_CHAN_CHARGER_CONSTANT_CHARGE_CURRENT_MAX,
			      0, &chrg_prop);
	if (ret < 0) {
		printk("Failed to update charge current max\n");
		return;
	}

	chrg_prop.val1 = 60;
	chrg_prop.val2 = 0;

	/** set charger input current limit. range: 0 - 63 **/
	ret = sensor_attr_set(dev, SENSOR_CHAN_CHARGER_INPUT_CURRENT_LIMIT,
			      0, &chrg_prop);
	if (ret < 0) {
		printk("Failed to update the input current limit\n");
		return;
	}

	while (1) {
		process_sample(dev);
		k_sleep(K_MSEC(2000));
	}
}
