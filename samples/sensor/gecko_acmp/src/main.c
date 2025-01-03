/*
 * Copyright Kickmaker 2025
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/gecko_acmp.h>
#include <zephyr/kernel.h>
#include <stdio.h>

#ifdef CONFIG_GECKO_ACMP_TRIGGER
static const int16_t triggers[] = {
	SENSOR_TRIG_GECKO_ACMP_OUTPUT_RISING,
	SENSOR_TRIG_GECKO_ACMP_OUTPUT_FALLING,
};
#endif /* CONFIG_GECKO_ACMP_TRIGGER */

static void acmp_input_handler(bool above_threshold)
{
	if (above_threshold) {
		printf("ACMP input above threshold\n");
	} else {
		printf("ACMP input below threshold\n");
	}
}

#ifdef CONFIG_GECKO_ACMP_EDGE_COUNTER
static void acmp_read_edge_counters(const struct device *dev, struct sensor_value *val)
{
	int err;

	if (!val) {
		return;
	}

	err = sensor_sample_fetch(dev);
	if (err) {
		printf("failed to fetch sample (err %d)\n", err);
		return;
	}

	err = sensor_channel_get(dev, SENSOR_CHAN_GECKO_ACMP_RISING_EDGE_COUNTER, val);
	if (err) {
		printf("failed to get channel (err %d)\n", err);
		return;
	}

	printf("Rising edge counter: %d\n", val.val1);

	err = sensor_channel_get(dev, SENSOR_CHAN_GECKO_ACMP_FALLING_EDGE_COUNTER, &val);
	if (err) {
		printf("failed to get channel (err %d)\n", err);
		return;
	}

	printf("Falling edge counter: %d\n", val.val1);
}
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */

#ifdef CONFIG_GECKO_ACMP_TRIGGER
static void acmp_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);

	acmp_input_handler((int16_t)trigger->type == SENSOR_TRIG_GECKO_ACMP_OUTPUT_RISING);
}
#endif /* CONFIG_GECKO_ACMP_TRIGGER */

int main(void)
{
#ifdef CONFIG_GECKO_ACMP_TRIGGER
	struct sensor_trigger trigger[ARRAY_SIZE(triggers)] = {
		[0] = {
				.chan = SENSOR_CHAN_GECKO_ACMP_OUTPUT,
				.type = triggers[0],
			},
		[1] = {
				.chan = SENSOR_CHAN_GECKO_ACMP_OUTPUT,
				.type = triggers[1],
			}
		};
#endif /* CONFIG_GECKO_ACMP_TRIGGER */
	const struct device *const acmp = DEVICE_DT_GET(DT_NODELABEL(acmp0));
	int err;

	if (!device_is_ready(acmp)) {
		printf("ACMP device not ready\n");
		return 0;
	}

#ifdef CONFIG_GECKO_ACMP_TRIGGER
	/* Set ACMP triggers */
	for (uint8_t i = 0; i < ARRAY_SIZE(triggers); i++) {
		err = sensor_trigger_set(acmp, &trigger[i], acmp_trigger_handler);
		if (err) {
			printf("failed to set trigger %d (err %d)\n", i, err);
			return 0;
		}
	}

	/* Await trigger */
	while (true) {
#ifdef CONFIG_GECKO_ACMP_EDGE_COUNTER
		struct sensor_value val;

		acmp_read_edge_counters(acmp, &val);
#endif /* CONFIG_GECKO_ACMP_EDGE_COUNTER */
		k_sleep(K_SECONDS(1));
	}
#else  /* !CONFIG_GECKO_ACMP_TRIGGER */
	while (true) {
		struct sensor_value val;

		err = sensor_sample_fetch(acmp);
		if (err) {
			printf("failed to fetch sample (err %d)\n", err);
			return 0;
		}

		err = sensor_channel_get(acmp, SENSOR_CHAN_GECKO_ACMP_OUTPUT, &val);
		if (err) {
			printf("failed to get channel (err %d)\n", err);
			return 0;
		}

		acmp_input_handler(val.val1 == 1);

		k_sleep(K_MSEC(1));
	}

#endif /* CONFIG_GECKO_ACMP_TRIGGER */

	return 0;
}
