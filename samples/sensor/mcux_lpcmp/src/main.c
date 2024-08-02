/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 * Copyright NXP 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/mcux_lpcmp.h>
#include <zephyr/kernel.h>
#include <stdio.h>

struct lpcmp_attr {
	int16_t attr;
	int32_t val;
};

static const struct lpcmp_attr attrs[] = {
	{.attr = SENSOR_ATTR_MCUX_LPCMP_POSITIVE_MUX_INPUT, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_NEGATIVE_MUX_INPUT, .val = 7},

	{.attr = SENSOR_ATTR_MCUX_LPCMP_DAC_ENABLE, .val = 1},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_DAC_HIGH_POWER_MODE_ENABLE, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_DAC_REFERENCE_VOLTAGE_SOURCE, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_DAC_OUTPUT_VOLTAGE, .val = (0xFF >> 1)},

	{.attr = SENSOR_ATTR_MCUX_LPCMP_SAMPLE_ENABLE, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_FILTER_COUNT, .val = 7},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_FILTER_PERIOD, .val = (0xFF >> 1)},

	{.attr = SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_ENABLE, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_COUTA_WINDOW_SIGNAL_INVERT_ENABLE, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_COUTA_SIGNAL, .val = 0},
	{.attr = SENSOR_ATTR_MCUX_LPCMP_COUT_EVENT_TO_CLOSE_WINDOW, .val = 0},
};

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
static const int16_t triggers[] = {
	SENSOR_TRIG_MCUX_LPCMP_OUTPUT_RISING,
	SENSOR_TRIG_MCUX_LPCMP_OUTPUT_FALLING,
};
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */

static void lpcmp_input_handler(bool above_threshold)
{
	if (above_threshold) {
		printf("LPCMP input above threshold\n");
	} else {
		printf("LPCMP input below threshold\n");
	}
}

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
static void lpcmp_trigger_handler(const struct device *dev, const struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);

	lpcmp_input_handler((int16_t)trigger->type == SENSOR_TRIG_MCUX_LPCMP_OUTPUT_RISING);
}
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */

int main(void)
{
#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	struct sensor_trigger trigger[ARRAY_SIZE(triggers)] = {
		[0] = {
				.chan = SENSOR_CHAN_MCUX_LPCMP_OUTPUT,
				.type = triggers[0],
			},
		[1] = {
				.chan = SENSOR_CHAN_MCUX_LPCMP_OUTPUT,
				.type = triggers[1],
			}
		};
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */
	const struct device *const lpcmp = DEVICE_DT_GET(DT_NODELABEL(lpcmp0));
	struct sensor_value val;
	int err;

	if (!device_is_ready(lpcmp)) {
		printf("LPCMP device not ready\n");
		return 0;
	}

	/* Set LPCMP attributes */
	val.val2 = 0;
	for (uint8_t i = 0; i < ARRAY_SIZE(attrs); i++) {
		val.val1 = attrs[i].val;
		err = sensor_attr_set(lpcmp, SENSOR_CHAN_MCUX_LPCMP_OUTPUT, attrs[i].attr, &val);
		if (err) {
			printf("failed to set attribute %d (err %d)\n", i, err);
			return 0;
		}
	}

	/* Delay for analog components (DAC, CMP, ...) to settle */
	k_sleep(K_MSEC(1));

#ifdef CONFIG_MCUX_LPCMP_TRIGGER
	/* Set LPCMP triggers */
	for (uint8_t i = 0; i < ARRAY_SIZE(triggers); i++) {
		err = sensor_trigger_set(lpcmp, &trigger[i], lpcmp_trigger_handler);
		if (err) {
			printf("failed to set trigger %d (err %d)\n", i, err);
			return 0;
		}
	}

	/* Await trigger */
	while (true) {
		k_sleep(K_MSEC(1));
	}
#else  /* !CONFIG_MCUX_LPCMP_TRIGGER */
	err = sensor_sample_fetch(lpcmp);
	if (err) {
		printf("failed to fetch sample (err %d)\n", err);
		return 0;
	}

	err = sensor_channel_get(lpcmp, SENSOR_CHAN_MCUX_LPCMP_OUTPUT, &val);
	if (err) {
		printf("failed to get channel (err %d)\n", err);
		return 0;
	}

	lpcmp_input_handler(val.val1 == 1);
#endif /* CONFIG_MCUX_LPCMP_TRIGGER */

	return 0;
}
