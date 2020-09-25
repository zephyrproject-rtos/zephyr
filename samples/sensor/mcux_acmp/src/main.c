/*
 * Copyright (c) 2020 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/sensor/mcux_acmp.h>
#include <zephyr.h>

#include <stdio.h>

#ifdef CONFIG_BOARD_TWR_KE18F
#define ACMP_NODE  DT_NODELABEL(cmp2)
#define ACMP_LABEL DT_LABEL(ACMP_NODE)
#define ACMP_POSITIVE 5
#define ACMP_NEGATIVE 5
#define ACMP_DAC_VREF 0
#else
#error Unsupported board
#endif

#define ACMP_DAC_VALUE 128

struct acmp_attr {
	int16_t attr;
	int32_t val;
};

static const struct acmp_attr attrs[] = {
	/* Positive input port set to MUX */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_POSITIVE_PORT_INPUT, .val = 1 },
	/* Positive input channel */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_POSITIVE_MUX_INPUT,
	  .val = ACMP_POSITIVE },
	/* Negative input port set to DAC */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_NEGATIVE_PORT_INPUT, .val = 0 },
	/* Negative input channel */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_NEGATIVE_MUX_INPUT,
	  .val = ACMP_NEGATIVE },
	/* DAC voltage reference */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_DAC_VOLTAGE_REFERENCE,
	  .val = ACMP_DAC_VREF },
	/* DAC value */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_DAC_VALUE, .val = ACMP_DAC_VALUE },
	/* Hysteresis level */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_HYSTERESIS_LEVEL, .val = 3 },
	/* Offset level */
	{ .attr = SENSOR_ATTR_MCUX_ACMP_OFFSET_LEVEL, .val = 0 },
};

static const int16_t triggers[] = {
	SENSOR_TRIG_MCUX_ACMP_OUTPUT_RISING,
	SENSOR_TRIG_MCUX_ACMP_OUTPUT_FALLING,
};

static void acmp_input_handler(bool above_threshold)
{
	if (above_threshold) {
		printf("ACMP input above threshold\n");
	} else {
		printf("ACMP input below threshold\n");
	}
}

static void acmp_trigger_handler(const struct device *dev,
				 struct sensor_trigger *trigger)
{
	ARG_UNUSED(dev);

	acmp_input_handler((int16_t)trigger->type ==
			   SENSOR_TRIG_MCUX_ACMP_OUTPUT_RISING);
}

void main(void)
{
	struct sensor_trigger trigger;
	const struct device *acmp;
	struct sensor_value val;
	int err;
	int i;

	acmp = device_get_binding(ACMP_LABEL);
	if (!acmp) {
		printf("failed to get ACMP device instance");
		return;
	}

	/* Set ACMP attributes */
	val.val2 = 0;
	for (i = 0; i < ARRAY_SIZE(attrs); i++) {
		val.val1 = attrs[i].val;
		err = sensor_attr_set(acmp, SENSOR_CHAN_MCUX_ACMP_OUTPUT,
				      attrs[i].attr, &val);
		if (err) {
			printf("failed to set attribute %d (err %d)", i, err);
			return;
		}
	}

	/* Delay for analog components (DAC, CMP, ...) to settle */
	k_sleep(K_MSEC(1));

	/* Set ACMP triggers */
	trigger.chan = SENSOR_CHAN_MCUX_ACMP_OUTPUT;
	for (i = 0; i < ARRAY_SIZE(triggers); i++) {
		trigger.type = triggers[i];
		err = sensor_trigger_set(acmp, &trigger, acmp_trigger_handler);
		if (err) {
			printf("failed to set trigger %d (err %d)", i, err);
			return;
		}
	}

	printf("Adjust ACMP input voltage by turning the potentiometer\n");

	/* Read initial state */
	err = sensor_sample_fetch(acmp);
	if (err) {
		printf("failed to fetch sample (err %d)", err);
		return;
	}

	err = sensor_channel_get(acmp, SENSOR_CHAN_MCUX_ACMP_OUTPUT, &val);
	if (err) {
		printf("failed to get channel (err %d)", err);
		return;
	}

	acmp_input_handler(val.val1 == 1);

	/* Await trigger */
	while (true) {
		k_sleep(K_MSEC(1));
	}
}
