/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(PRESS_INT_SAMPLE, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Get a device structure from a devicetree node from alias
 * "pressure_sensor".
 */
static const struct device *get_pressure_sensor_device(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_ALIAS(pressure_sensor));

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting sensor data\n", dev->name);
	return dev;
}

#ifdef CONFIG_PRESSURE_DRDY
static void data_ready_handler(const struct device *dev, const struct sensor_trigger *trig)
{

	if (trig->type == SENSOR_TRIG_DATA_READY) {
		int rc = sensor_sample_fetch_chan(dev, trig->chan);

		if (rc < 0) {
			printf("sample fetch failed: %d\n", rc);
			printf("cancelling trigger\n");
			(void)sensor_trigger_set(dev, trig, NULL);
			return;
		} else if (rc == 0) {

			struct sensor_value pressure;
			struct sensor_value temperature;
			struct sensor_value altitude;

			sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);
			sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature);
			sensor_channel_get(dev, SENSOR_CHAN_ALTITUDE, &altitude);

			LOG_INF("temp %.2f Cel, pressure %f kPa, altitude %f m",
				sensor_value_to_double(&temperature),
				sensor_value_to_double(&pressure),
				sensor_value_to_double(&altitude));
		}
	}
}
#endif

#ifdef CONFIG_PRESSURE_THRESHOLD
static void threshold_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	if (trig->type == SENSOR_TRIG_THRESHOLD) {
		LOG_INF("PRESSURE THRESHOLD INTERRUPT");
	}
}
#endif

#ifdef CONFIG_PRESSURE_DELTA
static void delta_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	if (trig->type == SENSOR_TRIG_DELTA) {
		LOG_INF("PRESSURE CHANGE INTERRUPT\n");
	}
}
#endif

int main(void)
{
	static struct sensor_trigger data_ready_trigger;
	const struct device *dev = get_pressure_sensor_device();
#ifdef CONFIG_PRESSURE_THRESHOLD
	static struct sensor_trigger threshold_trigger;
	struct sensor_value pressure, pressure_threshold;
#endif
#ifdef CONFIG_PRESSURE_DELTA
	static struct sensor_trigger delta_trigger;
	struct sensor_value pressure_delta;
#endif

	if (dev == NULL) {
		return 0;
	}
#ifdef CONFIG_PRESSURE_DRDY
	/* Configure data ready trigger */
	data_ready_trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ALL,
	};
	if (sensor_trigger_set(dev, &data_ready_trigger, data_ready_handler) < 0) {
		printf("Cannot configure data trigger!!!\n");
		return 0;
	}
#endif

#ifdef CONFIG_PRESSURE_THRESHOLD
	/* Configure pressure threshold trigger */
	/* Interrupt is triggered if sensor is lifted of about 50cm */
	/* Read current pressure */
	k_sleep(K_MSEC(50));
	sensor_sample_fetch_chan(dev, SENSOR_CHAN_PRESS);
	sensor_channel_get(dev, SENSOR_CHAN_PRESS, &pressure);

	/* Retrieve 5 Pa, around 50cm altitude increase */
	pressure_threshold.val1 = pressure.val1;
	pressure_threshold.val2 = pressure.val2 - 5000;
	sensor_attr_set(dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_LOWER_THRESH, &pressure_threshold);

	LOG_INF("Pressure at reset %.3f kPa, interrupt sets at %.3f kPa.\n",
		sensor_value_to_double(&pressure), sensor_value_to_double(&pressure_threshold));

	threshold_trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_PRESS,
	};
	if (sensor_trigger_set(dev, &threshold_trigger, threshold_handler) < 0) {
		printf("Cannot configure threshold trigger!!!\n");
		return 0;
	}
#endif

#ifdef CONFIG_PRESSURE_DELTA
	/* Configure pressure delta trigger */
	/* Set pressure delta to 0.01 kPa to detect a press on the sensor */
	pressure_delta.val1 = 0;
	pressure_delta.val2 = 10000;
	sensor_attr_set(dev, SENSOR_CHAN_PRESS, SENSOR_ATTR_SLOPE_TH, &pressure_delta);
	delta_trigger = (struct sensor_trigger){
		.type = SENSOR_TRIG_DELTA,
		.chan = SENSOR_CHAN_PRESS,
	};
	if (sensor_trigger_set(dev, &delta_trigger, delta_handler) < 0) {
		printf("Cannot configure threshold trigger!!!\n");
		return 0;
	}
#endif

	LOG_INF("Starting pressure event sample.\n");

	return 0;
}
