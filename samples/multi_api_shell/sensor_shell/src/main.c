/*
 * Copyright (c) 2022 Trackunit A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

struct sensor_info {
	const struct device *dev;
	const char *vendor;
	const char *model;
	const char *friendly_name;
};



/*
 * Note that DEVICE_DT_PROPERTY and DT_PROP use the same node so they 
 * can be mixed
 */
#ifdef CONFIG_LEGACY_DEVICE_MODEL 

#define SENSOR_INFO_INITIALIZER(node_id)                                \
	{                                                                   \
		.dev = DEVICE_DT_GET(node_id),                                  \
		.vendor = DEVICE_DT_PROPERTY(node_id, vendor),                  \
		.model = DEVICE_DT_PROPERTY(node_id, model),                    \
		.friendly_name = DT_PROP_OR(node_id, label, "")                 \
	},

#else

#define SENSOR_INFO_INITIALIZER(node_id)                                \
	{                                                                   \
		.dev = DEVICE_DT_API_GET(node_id, sensor),                      \
		.vendor = DEVICE_DT_PROPERTY(node_id, vendor),                  \
		.model = DEVICE_DT_PROPERTY(node_id, model),                    \
		.friendly_name = DT_PROP_OR(node_id, label, "")                 \
	},

#endif /* CONFIG_LEGACY_DEVICE_MODEL */

/*
 * It is known at compile time if any sensor exists in the system, save
 * memory by warning user to exclude the shell using KConfig.
 */
#if (DEVICE_DT_API_SUPPORTED_ANY(sensor) == 0)
	#warning "Sensor shell is enabled without any sensor in system"
#endif

/* 
 * Every sensor with status OK in devicetree will be filled into this
 * list automatically during compile time
 */
static struct sensor_info sensors[] = 
{
	DEVICE_DT_API_FOREACH(SENSOR_INFO_INITIALIZER, sensor)
};

/*
 * Yes, this is not actually a shell, it is an example showing how to
 * identify and include all sensors in the system.
 */
void main(void)
{
	/* Print sensor info header */
	printk("Sensor info:\n");
	printk("Sensor count: %u\n\n", ARRAY_SIZE(sensors));

	/* Print sensor info for each sensor */
	for (int i = 0; i < ARRAY_SIZE(sensors); i++) {
		printk("device: %u, vendor: %s, model: %s, friendly_name: %s\n",
			(size_t)sensors[i].dev, sensors[i].vendor, sensors[i].model,
			sensors[i].friendly_name);
	}
}
