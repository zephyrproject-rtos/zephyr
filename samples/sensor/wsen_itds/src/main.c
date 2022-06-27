/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

LOG_MODULE_REGISTER(MAIN);

static void process_acceleration_sample(const struct device *dev)
{
	static unsigned int sample_count;
	struct sensor_value temperature;
	struct sensor_value accelerations[3];

	if (sensor_sample_fetch(dev) < 0) {
		LOG_ERR("Failed to fetch ITDS sensor sample.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, accelerations) < 0) {
		LOG_ERR("Failed to read ITDS acceleration (XYZ) channel.");
		return;
	}

	if (sensor_channel_get(dev, SENSOR_CHAN_AMBIENT_TEMP, &temperature) < 0) {
		LOG_ERR("Failed to read ITDS temperature channel.");
		return;
	}

	++sample_count;
	LOG_INF("Sample #%u", sample_count);

	/* Display accelerations */
	LOG_INF("Acceleration X: %.2f m/s2", sensor_value_to_double(&accelerations[0]));
	LOG_INF("Acceleration Y: %.2f m/s2", sensor_value_to_double(&accelerations[1]));
	LOG_INF("Acceleration Z: %.2f m/s2", sensor_value_to_double(&accelerations[2]));

	/* Display temperature */
	LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temperature));
}

static void itds_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	switch (trig->type) {
	case SENSOR_TRIG_DATA_READY:
		process_acceleration_sample(dev);
		break;

	case SENSOR_TRIG_TAP:
		LOG_INF("ITDS single tap event occurred");
		break;

	case SENSOR_TRIG_DOUBLE_TAP:
		LOG_INF("ITDS double tap event occurred");
		break;

	case SENSOR_TRIG_FREEFALL:
		LOG_INF("ITDS free-fall event occurred");
		break;

	case SENSOR_TRIG_DELTA:
		LOG_INF("ITDS wake-up (delta) event occurred");
		break;

	default:
		break;
	}
}

void main(void)
{
	const struct device *devItds = device_get_binding("ITDS");
	if (devItds == NULL) {
		LOG_ERR("Failed to get ITDS device.");
		return;
	}
	LOG_INF("ITDS device initialized.");

	if (IS_ENABLED(CONFIG_ITDS_TRIGGER)) {
		/*
		 * Don't enable data-ready trigger if embedded functions are activated,
		 * as these usally require a high output data rate, which would result
		 * in a lot of values being logged.
		 */
#if !defined(CONFIG_ITDS_TAP) && !defined(CONFIG_ITDS_FREEFALL) && !defined(CONFIG_ITDS_DELTA)
		struct sensor_trigger trig_data_ready = { .type = SENSOR_TRIG_DATA_READY,
							  .chan = SENSOR_CHAN_ALL };
		if (sensor_trigger_set(devItds, &trig_data_ready, itds_handler) < 0) {
			LOG_ERR("Failed to configure ITDS data-ready trigger.");
			return;
		}
		LOG_INF("Successfully configured ITDS data-ready trigger.");
#endif /* CONFIG_ITDS_TRIGGER */

#ifdef CONFIG_ITDS_TAP
		struct sensor_trigger trig_single_tap = { .type = SENSOR_TRIG_TAP,
							  .chan = SENSOR_CHAN_ALL };
		if (sensor_trigger_set(devItds, &trig_single_tap, itds_handler) < 0) {
			LOG_ERR("Failed to configure ITDS single tap trigger.");
			return;
		}
		LOG_INF("Successfully configured ITDS single tap trigger.");

		struct sensor_trigger trig_double_tap = { .type = SENSOR_TRIG_DOUBLE_TAP,
							  .chan = SENSOR_CHAN_ALL };
		if (sensor_trigger_set(devItds, &trig_double_tap, itds_handler) < 0) {
			LOG_ERR("Failed to configure ITDS double tap trigger.");
			return;
		}
		LOG_INF("Successfully configured ITDS double tap trigger.");
#endif /* CONFIG_ITDS_TAP */

#ifdef CONFIG_ITDS_FREEFALL
		struct sensor_trigger trig_freefall = { .type = SENSOR_TRIG_FREEFALL,
							.chan = SENSOR_CHAN_ALL };
		if (sensor_trigger_set(devItds, &trig_freefall, itds_handler) < 0) {
			LOG_ERR("Failed to configure ITDS free-fall trigger.");
			return;
		}
		LOG_INF("Successfully configured ITDS free-fall trigger.");
#endif /* CONFIG_ITDS_FREEFALL */

#ifdef CONFIG_ITDS_DELTA
		struct sensor_trigger trig_delta = { .type = SENSOR_TRIG_DELTA,
						     .chan = SENSOR_CHAN_ALL };
		if (sensor_trigger_set(devItds, &trig_delta, itds_handler) < 0) {
			LOG_ERR("Failed to configure ITDS delta (wake-up) trigger.");
			return;
		}
		LOG_INF("Successfully configured ITDS delta (wake-up) trigger.");
#endif /* CONFIG_ITDS_DELTA */
	}

	while (!IS_ENABLED(CONFIG_ITDS_TRIGGER)) {
		process_acceleration_sample(devItds);
		k_sleep(K_MSEC(2000));
	}

	k_sleep(K_FOREVER);
}
