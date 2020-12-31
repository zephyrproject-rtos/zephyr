/*
 * Copyright (c) 2020 SER Consulting LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <stdio.h>
#include <devicetree.h>
#include <drivers/sensor.h>
#include <drivers/gpio.h>
#include <sys/util.h>
#include <sys/printk.h>
#include <inttypes.h>

#define LM73C DT_INST(0, ti_lm73c)

#if DT_NODE_HAS_STATUS(LM73C, okay)
#define LM73C_LABEL DT_LABEL(LM73C)
#else
#error Your devicetree has no enabled nodes with compatible "ti_lm73c"
#define LM73C_LABEL "<none>"
#endif

void main(void)
{
	const struct device *lm73c = device_get_binding(LM73C_LABEL);
	bool flm73c = false;

	if (lm73c == NULL) {
		printk("No device \"%s\" found; did initialization fail?\n",
		       LM73C_LABEL);
		return;
	}
	printk("Found device \"%s\"\n", LM73C_LABEL);
	flm73c = true;
	int ret;

	while (1) {
		if (flm73c) {
			ret = sensor_sample_fetch(lm73c);
			if (ret < 0) {
				printk("LM73C fetch error %d\n", ret);
			} else {
				struct sensor_value ttemp;

				sensor_channel_get(lm73c,
					SENSOR_CHAN_AMBIENT_TEMP, &ttemp);
#ifdef CONFIG_NEWLIB_LIBC_FLOAT_PRINTF
				printf("Temperature: %.5f C\n",
					sensor_value_to_double(&ttemp));
#else
				printk("Temperature: %d\n", ttemp.val1);
#endif
			}
		}
		k_sleep(K_MSEC(10000));
	}
}
