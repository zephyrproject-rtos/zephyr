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

#define ACS71020 DT_INST(0, allegro_acs71020)

#if DT_NODE_HAS_STATUS(ACS71020, okay)
#define ACS71020_LABEL DT_LABEL(ACS71020)
#else
#error Your devicetree has no enabled nodes with compatible "allegroacs71020"
#define ACS71020_LABEL "<none>"
#endif

void main(void)
{
	// ACS71020 AC power monitor
	const struct device *acs = device_get_binding(ACS71020_LABEL);
	bool facs = false;

	if (acs == NULL) {
		printk("No device \"%s\"\n", ACS71020_LABEL);
		return;
	} else {
		printk("Found device \"%s\"\n", ACS71020_LABEL);
		facs = true;
	}

	int ret;

	while (1) {

		if (facs) {
			ret = sensor_sample_fetch(acs);
			if (ret < 0) {
				printk("ACS71020 fetch error %d\n", ret);
			} else {
				struct sensor_value volts, amps, watts;
				sensor_channel_get(acs, SENSOR_CHAN_VOLTAGE, &volts);
				sensor_channel_get(acs, SENSOR_CHAN_CURRENT, &amps);
				sensor_channel_get(acs, SENSOR_CHAN_POWER, &watts);
				printk("RMS Voltage: %d.%06d; RMS Current: %d.%06d; Power: %d.%06d\n",
				       volts.val1, volts.val2, amps.val1, amps.val2,
				       watts.val1, watts.val2);
			}
		}

		k_sleep(K_MSEC(10000));
	}
}
