/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
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
#error Your devicetree has no enabled nodes with compatible "microchip9600"
#define MCP9600_LABEL "<none>"
#endif

void main(void)
{
	// ACS71020 AC power monitor
	const struct device *mcp = device_get_binding(MCP9600_LABEL);
	bool fmcp = false;

	if (mcp == NULL) {
		printk("No device \"%s\" found; did initialization fail?\n",
		       MCP9600_LABEL);
		return;
	} else {
		printk("Found device \"%s\"\n", MCP9600_LABEL);
		fmcp = true;
	}

	int ret;

	while (1) {
		if (fmcp) {
			ret = sensor_sample_fetch(mcp);
			if (ret < 0) {
				printk("MCP9600 fetch error %d\n", ret);
			} else {
				struct sensor_value ttemp, dtemp, ctemp;
				sensor_channel_get(mcp, SENSOR_CHAN_THERMOCOUPLE_TEMP, &ttemp);
				sensor_channel_get(mcp, SENSOR_CHAN_DIFF_TEMP, &dtemp);
				sensor_channel_get(mcp, SENSOR_CHAN_COLD_TEMP, &ctemp);

				printk("thermocouple temp: %d.%06d; diff temp: %d.%06d; cold temp: %d.%06d\n",
				       ttemp.val1, ttemp.val2, dtemp.val1, dtemp.val2,
				       ctemp.val1, ctemp.val2);
			}
		}
		k_sleep(K_MSEC(10000));
	}
}
