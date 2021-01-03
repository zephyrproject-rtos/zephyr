/*
 * Copyright (c) 2020 SER Consulting LLC / Steven Riedl
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

#define MCP9600 DT_INST(0, microchip_mcp9600)

#if DT_NODE_HAS_STATUS(MCP9600, okay)
#define MCP9600_LABEL DT_LABEL(MCP9600)
#else
#error Your devicetree has no enabled nodes with "microchip_mcp9600"
#define MCP9600_LABEL "<none>"
#endif

void main(void)
{
	bool fmcp = false;
	int ret;

	const struct device *mcp = device_get_binding(MCP9600_LABEL);

	if (mcp == NULL) {
		printk("No device \"%s\" found; did initialization fail?\n",
				MCP9600_LABEL);
		return;
	}
	printk("Found device \"%s\"\n", MCP9600_LABEL);
	fmcp = true;

	while (1) {
		if (fmcp) {
			ret = sensor_sample_fetch(mcp);
			if (ret < 0) {
				printk("\"%s\" fetch error %d\n",
					   MCP9600_LABEL, ret);
			} else {
				struct sensor_value ttemp;

				sensor_channel_get(mcp,
					SENSOR_CHAN_AMBIENT_TEMP, &ttemp);
#ifdef CONFIG_NEWLIB_LIBC_FLOAT_PRINTF
				printf("Temperature: %.4f C\n",
					sensor_value_to_double(&ttemp));
#else
				printk("Temperature: %d C\n", ttemp.val1);
#endif
			}
		}
		k_sleep(K_MSEC(10000));
	}
}
