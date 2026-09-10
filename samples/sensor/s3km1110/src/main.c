/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/s3km1110.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#if DT_HAS_COMPAT_STATUS_OKAY(iclegend_s3km1110)
#define RADAR_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(iclegend_s3km1110)
#else
#error "No enabled iclegend,s3km1110 devicetree node found"
#endif

#define SAMPLE_PERIOD_MS 1000
#define RETRY_DELAY_MS   250

static const char *target_status_to_string(int status)
{
	switch (status) {
	case S3KM1110_TARGET_NO_TARGET:
		return "No target";
	case S3KM1110_TARGET_MOVING:
		return "Moving target";
	case S3KM1110_TARGET_STATIC:
		return "Static target";
	case S3KM1110_TARGET_BOTH:
		return "Moving + static";
	default:
		return "Error";
	}
}

static void print_channel(const struct device *dev, const char *label, enum sensor_channel chan)
{
	struct sensor_value val;
	int rc = sensor_channel_get(dev, chan, &val);

	printk("  %-19s ", label);

	if (rc == -ENODATA) {
		printk("n/a\n");
		return;
	} else if (rc != 0) {
		printk("error (%d)\n", rc);
		return;
	}

	switch ((int)chan) {
	case SENSOR_CHAN_PROX:
		printk("%s\n", val.val1 ? "occupied" : "clear");
		break;
	case SENSOR_CHAN_S3KM1110_TARGET_STATUS:
		printk("%s\n", target_status_to_string(val.val1));
		break;
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_S3KM1110_MOVING_DISTANCE:
	case SENSOR_CHAN_S3KM1110_STATIC_DISTANCE:
		printk("%.3f m\n", sensor_value_to_double(&val));
		break;
	case SENSOR_CHAN_S3KM1110_MOVING_ENERGY:
	case SENSOR_CHAN_S3KM1110_STATIC_ENERGY:
		printk("%d%%\n", val.val1);
		break;
	default:
		printk("unknown channel\n");
		break;
	}
}

int main(void)
{
	const struct device *radar = DEVICE_DT_GET(RADAR_NODE);
	int rc;

	if (!device_is_ready(radar)) {
		printk("Radar device %s is not ready\n", radar->name);
		return 0;
	}

	printk("Iclegend S3KM1110 sample started on %s\n", radar->name);

	while (1) {
		rc = sensor_sample_fetch(radar);
		if (rc != 0) {
			printk("sample_fetch failed: %d\n", rc);
			k_sleep(K_MSEC(RETRY_DELAY_MS));
			continue;
		}

		printk("mmWave Sensor Report\n");
		printk("====================\n\n");

		/* Presence & target status */
		print_channel(radar, "Presence", SENSOR_CHAN_PROX);
		print_channel(radar, "Target",
			      (enum sensor_channel)SENSOR_CHAN_S3KM1110_TARGET_STATUS);
		printk("\n");

		/* Distances */
		printk("  Distances\n");
		printk("  *********\n\n");
		print_channel(radar, "Detection", SENSOR_CHAN_DISTANCE);
		print_channel(radar, "Moving target",
			      (enum sensor_channel)SENSOR_CHAN_S3KM1110_MOVING_DISTANCE);
		print_channel(radar, "Static target",
			      (enum sensor_channel)SENSOR_CHAN_S3KM1110_STATIC_DISTANCE);

		/* Energies */
		printk("\n");
		printk("  Energies\n");
		printk("  ********\n\n");
		print_channel(radar, "Moving",
			      (enum sensor_channel)SENSOR_CHAN_S3KM1110_MOVING_ENERGY);
		print_channel(radar, "Static",
			      (enum sensor_channel)SENSOR_CHAN_S3KM1110_STATIC_ENERGY);
		printk("\n");

		k_sleep(K_MSEC(SAMPLE_PERIOD_MS));
	}
}
