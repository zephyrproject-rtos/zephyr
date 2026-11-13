/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define TACH_SAMPLE_PERIOD K_MSEC(1000)

struct tach_info {
	const struct device *dev;
	/* TACH edges the hardware counts clocks over */
	uint8_t edges;
	/* TACH periods the fan produces per revolution */
	uint8_t pulses_per_round;
	/* Frequency in Hz of the clock the driver converts with */
	uint32_t clock_hz;
};

#define TACH_INFO_ENTRY(node_id)                                                                   \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.edges = DT_PROP(node_id, tach_edges),                                             \
		.pulses_per_round = DT_PROP(node_id, pulses_per_round),                            \
		.clock_hz = DT_PROP(node_id, clock_frequency),                                     \
	},

/* Every enabled microchip,xec-tach node, in devicetree order */
static const struct tach_info tachs[] = {
	DT_FOREACH_STATUS_OKAY(microchip_xec_tach, TACH_INFO_ENTRY)};

BUILD_ASSERT(ARRAY_SIZE(tachs) > 0,
	     "This sample needs at least one enabled microchip,xec-tach node");

static void tach_report(const struct tach_info *tach)
{
	struct sensor_value rpm;
	int ret;

	ret = sensor_sample_fetch_chan(tach->dev, SENSOR_CHAN_RPM);
	if (ret != 0) {
		/*
		 * -EIO means no count was latched within the time the block
		 * needs to saturate its counter, so it is not counting at all.
		 */
		printk("%s: sample fetch failed (%d)\n", tach->dev->name, ret);
		return;
	}

	ret = sensor_channel_get(tach->dev, SENSOR_CHAN_RPM, &rpm);
	if (ret != 0) {
		printk("%s: channel get failed (%d)\n", tach->dev->name, ret);
		return;
	}

	/*
	 * A saturated counter reads as zero RPM: the fan is stopped, jammed, or
	 * turning slower than the configured measurement window can resolve.
	 */
	if ((rpm.val1 == 0) && (rpm.val2 == 0)) {
		printk("%s: fan stopped\n", tach->dev->name);
		return;
	}

	printk("%s: %d.%06d RPM\n", tach->dev->name, rpm.val1, rpm.val2);
}

int main(void)
{
	printk("Microchip XEC tachometer sample\n");

	for (size_t i = 0; i < ARRAY_SIZE(tachs); i++) {
		const struct tach_info *tach = &tachs[i];

		if (!device_is_ready(tach->dev)) {
			printk("%s: device not ready\n", tach->dev->name);
			return 0;
		}

		printk("%s: %u TACH edges per measurement, %u TACH periods per "
		       "revolution, %u Hz counting clock\n",
		       tach->dev->name, tach->edges, tach->pulses_per_round, tach->clock_hz);
	}

	while (1) {
		for (size_t i = 0; i < ARRAY_SIZE(tachs); i++) {
			tach_report(&tachs[i]);
		}

		k_sleep(TACH_SAMPLE_PERIOD);
	}

	return 0;
}
