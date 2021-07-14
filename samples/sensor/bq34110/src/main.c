/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/sensor.h>
#include <sys/printk.h>

static void bq34110_show_values(const char *type, struct sensor_value value)
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

static void do_main(const struct device *dev)
{
	int ret = 0, observation = 0;
	struct sensor_value voltage, current, state_of_charge,
	       full_charge_capacity, remaining_charge_capacity, avg_power,
	       int_temp, state_of_health, time_to_full, time_to_empty;

	while (1) {
		memset(&voltage, 0, sizeof(voltage));
		memset(&current, 0, sizeof(current));
		memset(&state_of_charge, 0, sizeof(state_of_charge));
		memset(&full_charge_capacity, 0, sizeof(full_charge_capacity));
		memset(&remaining_charge_capacity, 0, sizeof(remaining_charge_capacity));
		memset(&avg_power, 0, sizeof(avg_power));
		memset(&int_temp, 0, sizeof(int_temp));
		memset(&state_of_health, 0, sizeof(state_of_health));
		memset(&time_to_full, 0, sizeof(time_to_full));
		memset(&time_to_empty, 0, sizeof(time_to_empty));

		printk("Observation: %d\n", observation);

		ret = sensor_sample_fetch_chan(dev, SENSOR_CHAN_ALL);
		if (ret < 0) {
			printk("Unable to fetch the sensor data\n");
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_VOLTAGE,
					 &voltage);
		if (ret < 0) {
			printk("Unable to get the Voltage value\n");
		}

		printk("Voltage: %d.%06dV\n", voltage.val1, voltage.val2);

		ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_CURRENT,
					 &current);
		if (ret < 0) {
			printk("Unable to get the Current Value\n");
		}

		bq34110_show_values("Avg Current in Amps", current);

		ret = sensor_channel_get(dev,
					 SENSOR_CHAN_GAUGE_STATE_OF_CHARGE,
					 &state_of_charge);
		if (ret < 0) {
			printk("Unable to get the State of Charge\n");
		}

		printk("State of Charge: %d%%\n", state_of_charge.val1);

		ret = sensor_channel_get(dev,
					 SENSOR_CHAN_GAUGE_STATE_OF_HEALTH,
					 &state_of_health);
		if (ret < 0) {
			printk("Unable to get State of Charge\n");
		}

		ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_AVG_POWER,
					 &avg_power);
		if (ret < 0) {
			printk("Unable to get Avg Power\n");
		}

		bq34110_show_values("Avg Power in Watt", avg_power);

		ret = sensor_channel_get(dev,
			SENSOR_CHAN_GAUGE_FULL_CHARGE_CAPACITY,
			&full_charge_capacity);
		if (ret < 0) {
			printk("Unable to get Full Charge Capacity\n");
		}

		printk("Full Charge Capacity: %d.%06dAh\n",
		       full_charge_capacity.val1, full_charge_capacity.val2);

		ret = sensor_channel_get(dev,
				SENSOR_CHAN_GAUGE_REMAINING_CHARGE_CAPACITY,
				&remaining_charge_capacity);
		if (ret < 0) {
			printk("Unable to get Remaining Charge Capacity\n");
		}

		printk("Remaining Charge Capacity: %d.%06dAh\n",
		       remaining_charge_capacity.val1,
		       remaining_charge_capacity.val2);

		ret = sensor_channel_get(dev, SENSOR_CHAN_GAUGE_TEMP,
					 &int_temp);
		if (ret < 0) {
			printk("Unable to read Internal Temperature\n");
		}

		printk("Gauge Temperature: %d.%06d C\n", int_temp.val1,
		       int_temp.val2);

		ret = sensor_channel_get(dev,
					SENSOR_CHAN_GAUGE_TIME_TO_FULL,
					&time_to_full);
		if (ret < 0) {
			printk("Unable to read Time to Full\n");
		}

		printk("Time to Full: %dminutes\n", time_to_full.val1);

		ret = sensor_channel_get(dev,
					SENSOR_CHAN_GAUGE_TIME_TO_EMPTY,
					&time_to_empty);
		if (ret < 0) {
			printk("Unable to read Time to Empty\n");
		}

		printk("Time to empty: %dminutes\n", time_to_empty.val1);

		k_sleep(K_MSEC(5000));

		observation++;
	}
}

void main(void)
{
	const struct device *dev;

	dev = DEVICE_DT_GET_ANY(ti_bq34110);
	if (!dev) {
		printk("Failed to get device binding\n");
		return;
	}

	printk("device is %p, name is %s\n", dev, dev->name);

	do_main(dev);
}
