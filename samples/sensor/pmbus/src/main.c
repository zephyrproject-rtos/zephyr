/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static const struct {
	const char *name;
	enum sensor_channel chan;
	const char *unit;
} channels[] = {
	{"VOUT", SENSOR_CHAN_VOLTAGE, "V"},
	{"IOUT", SENSOR_CHAN_CURRENT, "A"},
	{"POUT", SENSOR_CHAN_POWER, "W"},
	{"TEMP", SENSOR_CHAN_AMBIENT_TEMP, "C"},
};

int main(void)
{
	const struct device *const dev = DEVICE_DT_GET_ONE(zephyr_pmbus);
	struct sensor_value val;
	const char *sep;
	int64_t milli;

	if (!device_is_ready(dev)) {
		printk("%s is not ready\n", dev->name);
		return 0;
	}

	while (true) {
		if (sensor_sample_fetch(dev) < 0) {
			printk("fetch failed\n");
			k_sleep(K_SECONDS(1));
			continue;
		}

		sep = "";
		for (int i = 0; i < ARRAY_SIZE(channels); i++) {
			if (sensor_channel_get(dev, channels[i].chan, &val) < 0) {
				continue;
			}

			milli = sensor_value_to_milli(&val);
			printk("%s%s %s%lld.%03lld %s", sep, channels[i].name, milli < 0 ? "-" : "",
			       llabs(milli) / 1000, llabs(milli) % 1000, channels[i].unit);
			sep = ", ";
		}

		printk("\n");
		k_sleep(K_SECONDS(1));
	}

	return 0;
}
