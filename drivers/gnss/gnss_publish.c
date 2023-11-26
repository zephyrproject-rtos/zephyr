/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss/gnss_publish.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/iterable_sections.h>

static struct k_spinlock lock;

void gnss_publish_data(const struct device *dev, const struct gnss_data *data)
{
	K_SPINLOCK(&lock) {
		STRUCT_SECTION_FOREACH(gnss_data_callback, callback) {
			if (callback->dev == NULL || callback->dev == dev) {
				callback->callback(dev, data);
			}
		}
	}
}

#if CONFIG_GNSS_SATELLITES
void gnss_publish_satellites(const struct device *dev, const struct gnss_satellite *satellites,
			     uint16_t size)
{
	K_SPINLOCK(&lock) {
		STRUCT_SECTION_FOREACH(gnss_satellites_callback, callback) {
			if (callback->dev == NULL || callback->dev == dev) {
				callback->callback(dev, satellites, size);
			}
		}
	}
}
#endif
