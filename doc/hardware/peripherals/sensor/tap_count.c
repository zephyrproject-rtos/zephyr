/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/sensor.h>


const struct device *const accel0 = DEVICE_DT_GET(DT_ALIAS(accel0));

static struct tap_count_state {
	struct sensor_trigger trig;
	uint32_t count;
} tap_count_state = {
	.trig = {
		.chan = SENSOR_CHAN_ACCEL_XYZ,
		.type = SENSOR_TRIG_TAP,
	},
	.count = 0,
};

void tap_handler(const struct device *dev, const struct sensor_trigger *trig)
{
	struct tap_count_state *state = CONTAINER_OF(trig, struct tap_count_state, trig);

	state->count++;

	printk("Tap! Total Taps: %u\n", state->count);
}

int main(void)
{
	int rc;

	printk("Tap Counter Example (%s)\n", CONFIG_ARCH);

	rc = sensor_trigger_set(accel0, &tap_count_state.trig, tap_handler);

	if (rc != 0) {
		printk("Failed to set trigger handler for taps, error %d\n", rc);
		return rc;
	}

	return rc;
}
