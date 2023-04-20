/*
 * Copyright (c) 2018 Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app);

enum sample_stats_state {
	SAMPLE_STATS_STATE_UNINITIALIZED = 0,
	SAMPLE_STATS_STATE_ENABLED,
	SAMPLE_STATS_STATE_DISABLED,
};

struct sample_stats {
	int64_t accumulator;
	uint32_t count;
	uint64_t sample_window_start;
	enum sample_stats_state state;
};

static void data_ready_trigger_handler(const struct device *sensor,
				       const struct sensor_trigger *trigger)
{
	static struct sample_stats stats[SENSOR_CHAN_ALL];
	const int64_t now = k_uptime_get();
	struct sensor_value value;

	if (sensor_sample_fetch(sensor)) {
		LOG_ERR("Failed to fetch samples on data ready handler");
	}
	for (int i = 0; i < SENSOR_CHAN_ALL; ++i) {
		int rc;

		/* Skip disabled channels */
		if (stats[i].state == SAMPLE_STATS_STATE_DISABLED) {
			continue;
		}
		/* Skip 3 axis channels */
		if (i == SENSOR_CHAN_ACCEL_XYZ || i == SENSOR_CHAN_GYRO_XYZ ||
		    i == SENSOR_CHAN_MAGN_XYZ) {
			continue;
		}

		rc = sensor_channel_get(sensor, i, &value);
		if (rc == -ENOTSUP && stats[i].state == SAMPLE_STATS_STATE_UNINITIALIZED) {
			/* Stop reading this channel if the driver told us it's not supported. */
			stats[i].state = SAMPLE_STATS_STATE_DISABLED;
		}
		if (rc != 0) {
			/* Skip on any error. */
			continue;
		}
		/* Do something with the data */
		stats[i].accumulator += value.val1 * INT64_C(1000000) + value.val2;
		if (stats[i].count++ == 0) {
			stats[i].sample_window_start = now;
		} else if (now > stats[i].sample_window_start + CONFIG_SAMPLE_PRINT_TIMEOUT_MS) {
			int64_t micro_value = stats[i].accumulator / stats[i].count;

			value.val1 = micro_value / 1000000;
			value.val2 = (int32_t)llabs(micro_value - (value.val1 * 1000000));
			LOG_INF("chan=%d, num_samples=%u, data=%d.%06d", i, stats[i].count,
				value.val1, value.val2);

			stats[i].accumulator = 0;
			stats[i].count = 0;
		}
	}
}

int main(void)
{
	STRUCT_SECTION_FOREACH(sensor_info, sensor)
	{
		struct sensor_trigger trigger = {
			.chan = SENSOR_CHAN_ALL,
			.type = SENSOR_TRIG_DATA_READY,
		};
		sensor_trigger_set(sensor->dev, &trigger, data_ready_trigger_handler);
	}
	return 0;
}
