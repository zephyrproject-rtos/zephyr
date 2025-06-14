/*
 * Copyright (c) 2025, Srishtik Bhandarkar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/pzem004t.h>

#define POWER_ALARM_THRESHOLD 3000
#define MODBUS_RTU_ADDRESS    0x21

void pzem004t_read_measurement_values(const struct device *dev)
{
	struct sensor_value voltage;
	struct sensor_value current;
	struct sensor_value power;
	struct sensor_value frequency;
	struct sensor_value energy;
	struct sensor_value power_factor;
	struct sensor_value alarm_status;

	int ret;
	/* Fetch the latest sample from the sensor */
	ret = sensor_sample_fetch(dev);
	if (ret) {
		printk("Failed to fetch sensor data: %d\n", ret);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_VOLTAGE, &voltage);
	if (ret) {
		printk("Failed to get voltage: %d\n", ret);
	} else {
		printk("Voltage: %d.%d V\n", voltage.val1, voltage.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_CURRENT, &current);
	if (ret) {
		printk("Failed to get current: %d\n", ret);
	} else {
		printk("Current: %d.%d A\n", current.val1, current.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_POWER, &power);
	if (ret) {
		printk("Failed to get power: %d\n", ret);
	} else {
		printk("Power: %d.%d W\n", power.val1, power.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_FREQUENCY, &frequency);
	if (ret) {
		printk("Failed to get frequency: %d\n", ret);
	} else {
		printk("Frequency: %d.%d Hz\n", frequency.val1, frequency.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PZEM004T_ENERGY, &energy);
	if (ret) {
		printk("Failed to get energy: %d\n", ret);
	} else {
		printk("Energy: %d.%d Wh\n", energy.val1, energy.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PZEM004T_POWER_FACTOR, &power_factor);
	if (ret) {
		printk("Failed to get power factor: %d\n", ret);
	} else {
		printk("Power Factor: %d.%d\n", power_factor.val1, power_factor.val2);
	}

	ret = sensor_channel_get(dev, SENSOR_CHAN_PZEM004T_ALARM_STATUS, &alarm_status);
	if (ret) {
		printk("Failed to get alarm status: %d\n", ret);
	} else {
		printk("Alarm Status: %d\n\n", alarm_status.val1);
	}
}

void pzem004t_read_sensor_parameters(const struct device *dev)
{
	int ret;
	struct sensor_value power_alarm_threshold;
	struct sensor_value modbus_rtu_address;

	ret = sensor_attr_get(dev, SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD,
			      SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD, &power_alarm_threshold);
	if (ret) {
		printk("Failed to get power alarm threshold: %d\n", ret);
	} else {
		printk("Power Alarm Threshold: %d W\n", power_alarm_threshold.val1);
	}

	ret = sensor_attr_get(dev, SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS,
			      SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS, &modbus_rtu_address);
	if (ret) {
		printk("Failed to get Modbus RTU address: %d\n", ret);
	} else {
		printk("Modbus RTU Address: 0x%02x\n", modbus_rtu_address.val1);
	}
}

void pzem004t_set_sensor_parameters(const struct device *dev)
{
	int ret;
	struct sensor_value power_alarm_threshold_set = {.val1 = POWER_ALARM_THRESHOLD, .val2 = 0};
	struct sensor_value modbus_rtu_address_set = {.val1 = MODBUS_RTU_ADDRESS, .val2 = 0};

	ret = sensor_attr_set(dev, SENSOR_CHAN_PZEM004T_POWER_ALARM_THRESHOLD,
			      SENSOR_ATTR_PZEM004T_POWER_ALARM_THRESHOLD,
			      &power_alarm_threshold_set);

	if (ret) {
		printk("Failed to set power alarm threshold: %d\n", ret);
	} else {
		printk("Power alarm threshold set to: %d W\n", power_alarm_threshold_set.val1);
	}

	ret = sensor_attr_set(dev, SENSOR_CHAN_PZEM004T_MODBUS_RTU_ADDRESS,
			      SENSOR_ATTR_PZEM004T_MODBUS_RTU_ADDRESS, &modbus_rtu_address_set);

	if (ret) {
		printk("Failed to set Modbus RTU address: %d\n", ret);
	} else {
		printk("Modbus RTU address set to: 0x%02x\n", modbus_rtu_address_set.val1);
	}
}

void pzem004t_reset_energy(const struct device *dev)
{
	int ret;

	ret = sensor_attr_set(dev, SENSOR_CHAN_PZEM004T_RESET_ENERGY,
			      SENSOR_ATTR_PZEM004T_RESET_ENERGY, NULL);
	if (ret) {
		printk("Failed to reset energy: %d\n", ret);
	} else {
		printk("Energy reset successfully\n");
	}
}

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(pzem004t));

	if (!device_is_ready(dev)) {
		printk("Device %s is not ready\n", dev->name);
		return -ENODEV;
	}

	/*
	 * If you have set your pzemoo4t device address other than the default address,
	 * and you want to set the device address, please use the below code as a reference.
	 *
	 * sensor_attr_set(dev, PZEM004T_SENSOR_CHAN_ADDRESS_INST_SET,
	 *				PZEM004T_SENSOR_ATTR_ADDRESS_INST_SET, value_struct);
	 *
	 * where value_struct is a struct sensor_value with the address you want to set.
	 *
	 * By default if you have a single pzem004t device directly connected on uart
	 * port, you dont have to do this as the default address is already set to 0xf8 in driver.
	 */

	while (1) {
#if CONFIG_READ_MEASUREMENT_VALUES
		pzem004t_read_measurement_values(dev);
		k_msleep(1000);
#endif /* READ_MEASUREMENT_VALUES */

#if CONFIG_READ_SENSOR_PARAMETERS
		pzem004t_read_sensor_parameters(dev);
		return 0;
#endif /* READ_SENSOR_PARAMETERS */

#if CONFIG_SET_SENSOR_PARAMETERS
		pzem004t_set_sensor_parameters(dev);
		return 0;
#endif /* CONFIG_SET_SENSOR_PARAMETERS */

#if CONFIG_RESET_ENERGY
		pzem004t_reset_energy(dev);
		return 0;
#endif /* RESET_ENERGY */
	}

	return 0;
}
