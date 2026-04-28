/*
 * Copyright (c) 2026 Analog Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/fuel_gauge.h>
#include <zephyr/drivers/regulator.h>
#include <sys/errno.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/__assert.h>

#define DEVICE_NAME_CHARGER              "charger"
#define DEVICE_NAME_FUEL_GAUGE           "fuel gauge"
#define DEVICE_NAME_BUCK_REGULATOR       "buck regulator"
#define DEVICE_NAME_BUCK_BOOST_REGULATOR "buck boost regulator"

static const char *now_str(void)
{
	static char buf[16]; /* ...HH:MM:SS.MMM */
	uint32_t now = k_uptime_get_32();
	unsigned int ms = now % MSEC_PER_SEC;
	unsigned int s;
	unsigned int min;
	unsigned int h;

	now /= MSEC_PER_SEC;
	s = now % 60U;
	now /= 60U;
	min = now % 60U;
	now /= 60U;
	h = now;

	snprintf(buf, sizeof(buf), "%u:%02u:%02u.%03u", h, min, s, ms);
	return buf;
}

static int check_device_ready(const struct device *dev, const char *dev_name)
{
	if (dev == NULL) {
		printk("Error: no %s device found.\n", dev_name);
		return -1;
	}

	if (!device_is_ready(dev)) {
		printk("Error: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return -1;
	}

	return 0;
}

static int read_charger_properties(const struct device *chgdev)
{
	union charger_propval charger_val;
	int ret;

	printk("[%s]: Found device \"%s\", getting charger data\n", now_str(), chgdev->name);

	ret = charger_get_prop(chgdev, CHARGER_PROP_CONSTANT_CHARGE_CURRENT_UA, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Constant charge current on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}
	printk("[%s]: Current (Fast Charge): %d uA\n", now_str(),
	       charger_val.const_charge_current_ua);

	ret = charger_get_prop(chgdev, CHARGER_PROP_INPUT_REGULATION_VOLTAGE_UV, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Adaptive Voltage on device! ret %d\n", now_str(), ret);
		return -1;
	}

	printk("[%s]: Adaptive Voltage: %d uV\n", now_str(),
	       charger_val.input_voltage_regulation_voltage_uv);

	ret = charger_get_prop(chgdev, CHARGER_PROP_CHARGE_TERM_CURRENT_UA, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Termination Current on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}

	printk("[%s]: Current (Termination): %d uA\n", now_str(),
	       charger_val.charge_term_current_ua);

	return 0;
}

static int enable_charger(const struct device *chgdev)
{
	int ret;

	ret = charger_charge_enable(chgdev, true);
	if (ret) {
		printk("[%s]: Failed to start charger! ret %d\n", now_str(), ret);
		return -1;
	}

	return 0;
}

static int read_fuel_gauge_properties(const struct device *flgdev)
{
	union fuel_gauge_prop_val fuel_gauge_val;
	int ret;

	ret = fuel_gauge_get_prop(flgdev, FUEL_GAUGE_DESIGN_CAPACITY, &fuel_gauge_val);
	if (ret) {
		printk("[%s]: Error getting Fuel Gauge Battery Capacity on device! ret %d\n",
		       now_str(), ret);
		return -1;
	}

	return 0;
}

static int read_regulator_voltages(const struct device *bckdev, const struct device *bstdev)
{
	uint32_t volt_out_uv;
	int ret;

	ret = regulator_get_voltage(bckdev, &volt_out_uv);
	if (ret) {
		printk("[%s]: Error getting buck voltage on device! ret %d\n", now_str(), ret);
		return -1;
	}

	printk("[%s]: Buck Output Voltage: %d\n", now_str(), volt_out_uv);

	ret = regulator_get_voltage(bstdev, &volt_out_uv);
	if (ret) {
		printk("[%s]: Error getting buck boost voltage on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}

	printk("[%s]: Buck Boost Output Voltage: %d\n", now_str(), volt_out_uv);

	return 0;
}

static int monitor_charger_status(const struct device *chgdev)
{
	union charger_propval charger_val;
	int ret;

	ret = charger_get_prop(chgdev, CHARGER_PROP_ONLINE, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Online status on device! ret %d\n", now_str(), ret);
		return -1;
	}

	if (charger_val.online == CHARGER_ONLINE_FIXED) {
		printk("[%s]: Online: Charger is active.\n", now_str());
	} else {
		printk("[%s]: Online: Charger is offline.\n", now_str());
	}

	ret = charger_get_prop(chgdev, CHARGER_PROP_PRESENT, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Battery Present status on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}

	if (charger_val.present) {
		printk("[%s]: Present: Battery is Present.\n", now_str());
	} else {
		printk("[%s]: Present: Battery is Not Present.\n", now_str());
	}

	ret = charger_get_prop(chgdev, CHARGER_PROP_STATUS, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Charging Status on device! ret %d\n", now_str(), ret);
		return -1;
	}

	switch (charger_val.status) {
	case CHARGER_STATUS_NOT_CHARGING:
		printk("[%s]: Status: Charger is Not Charging.\n", now_str());
		break;
	case CHARGER_STATUS_CHARGING:
		printk("[%s]: Status: Charger is Charging.\n", now_str());
		break;
	case CHARGER_STATUS_FULL:
		printk("[%s]: Status: Charging is Complete.\n", now_str());
		break;
	case CHARGER_STATUS_DISCHARGING:
		printk("[%s]: Status: Charger is not charging and in an internal status "
		       "state.\n",
		       now_str());
		break;
	default:
		printk("[%s]: Status: Charger Status is unknown.\n", now_str());
	}

	ret = charger_get_prop(chgdev, CHARGER_PROP_CHARGE_TYPE, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Charging Mode on device! ret %d\n", now_str(), ret);
		return -1;
	}

	switch (charger_val.charge_type) {
	case CHARGER_CHARGE_TYPE_NONE:
		printk("[%s]: Mode: Charger is Not Charging.\n", now_str());
		break;
	case CHARGER_CHARGE_TYPE_TRICKLE:
		printk("[%s]: Mode: Charger is in Trickle Mode.\n", now_str());
		break;
	case CHARGER_CHARGE_TYPE_FAST:
		printk("[%s]: Mode: Charging is in Fast Mode (CC).\n", now_str());
		break;
	case CHARGER_CHARGE_TYPE_LONGLIFE:
		printk("[%s]: Mode: Charging is in Fast Mode (CV).\n", now_str());
		break;
	case CHARGER_CHARGE_TYPE_UNKNOWN:
		printk("[%s]: Mode: Charger is not charging and in an internal status "
		       "state.\n",
		       now_str());
	default:
		printk("[%s]: Mode: Charger Type is unknown.\n", now_str());
	}

	ret = charger_get_prop(chgdev, CHARGER_PROP_HEALTH, &charger_val);
	if (ret) {
		printk("[%s]: Error getting Battery Health Status on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}

	switch (charger_val.health) {
	case CHARGER_HEALTH_COLD:
		printk("[%s]: Health: Battery is Cold.\n", now_str());
		break;
	case CHARGER_HEALTH_COOL:
		printk("[%s]: Health: Battery is Cool.\n", now_str());
		break;
	case CHARGER_HEALTH_WARM:
		printk("[%s]: Health: Battery is Warm.\n", now_str());
		break;
	case CHARGER_HEALTH_HOT:
		printk("[%s]: Health: Battery is Hot.\n", now_str());
		break;
	case CHARGER_HEALTH_WATCHDOG_TIMER_EXPIRE:
		printk("[%s]: Health: Charger Watchdog has expired!\n", now_str());
		break;
	case CHARGER_HEALTH_OVERVOLTAGE:
		printk("[%s]: Health: Charger Overvoltage has occurred!\n", now_str());
		break;
	case CHARGER_HEALTH_OVERHEAT:
		printk("[%s]: Health: Charger is Overheating!\n", now_str());
		break;
	case CHARGER_HEALTH_SAFETY_TIMER_EXPIRE:
		printk("[%s]: Health: Charger safety timer has expired!\n", now_str());
		break;
	case CHARGER_HEALTH_NO_BATTERY:
		printk("[%s]: Health: No battery connected!\n", now_str());
		break;
	case CHARGER_HEALTH_GOOD:
		printk("[%s]: Health: Charger health is good.\n", now_str());
		break;
	default:
		printk("[%s]: Health: Charger health is unknown.\n", now_str());
	}

	return 0;
}

static int monitor_fuel_gauge_status(const struct device *flgdev)
{
	union fuel_gauge_prop_val fuel_gauge_val;
	int ret;

	ret = fuel_gauge_get_prop(flgdev, FUEL_GAUGE_VOLTAGE, &fuel_gauge_val);
	if (ret) {
		printk("[%s]: Error getting Fuel Gauge Voltage on device! ret %d\n", now_str(),
		       ret);
		return -1;
	}

	printk("[%s]: Fuel gauge Voltage: %d\n", now_str(), fuel_gauge_val.voltage);

	ret = fuel_gauge_get_prop(flgdev, FUEL_GAUGE_STATUS, &fuel_gauge_val);
	if (ret) {
		printk("[%s]: Error getting Fuel Gauge Status on device! ret %d\n", now_str(), ret);
		return -1;
	}

	printk("[%s]: Fuel gauge Status 0x%X\n", now_str(), fuel_gauge_val.fg_status);

	ret = fuel_gauge_get_prop(flgdev, FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE, &fuel_gauge_val);
	if (ret) {
		printk("[%s]: Error getting Fuel Gauge State of charge on device! ret %d\n",
		       now_str(), ret);
		return -1;
	}

	printk("[%s]: Fuel gauge SoC: %d\n", now_str(), fuel_gauge_val.absolute_state_of_charge);

	return 0;
}

static int monitor_loop(const struct device *chgdev, const struct device *flgdev)
{
	int ret;

	while (1) {
		ret = monitor_charger_status(chgdev);
		if (ret) {
			return ret;
		}

		printk("\n\n");

		ret = monitor_fuel_gauge_status(flgdev);
		if (ret) {
			return ret;
		}

		k_sleep(K_SECONDS(1));
	}

	return 0;
}

int main(void)
{
	const struct device *chgdev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(charger));
	const struct device *flgdev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(fuelgauge));
	const struct device *bckdev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(buck));
	const struct device *bstdev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(buckboost));
	int ret;

	ret = check_device_ready(chgdev, DEVICE_NAME_CHARGER);
	if (ret) {
		return ret;
	}

	ret = check_device_ready(flgdev, DEVICE_NAME_FUEL_GAUGE);
	if (ret) {
		return ret;
	}

	ret = check_device_ready(bckdev, DEVICE_NAME_BUCK_REGULATOR);
	if (ret) {
		return ret;
	}

	ret = check_device_ready(bstdev, DEVICE_NAME_BUCK_BOOST_REGULATOR);
	if (ret) {
		return ret;
	}

	ret = read_charger_properties(chgdev);
	if (ret) {
		return ret;
	}

	ret = enable_charger(chgdev);
	if (ret) {
		return ret;
	}

	ret = read_fuel_gauge_properties(flgdev);
	if (ret) {
		return ret;
	}

	ret = read_regulator_voltages(bckdev, bstdev);
	if (ret) {
		return ret;
	}

	ret = monitor_loop(chgdev, flgdev);
	if (ret) {
		return ret;
	}

	return 0;
}
