/*
 * SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __SENSORS_H__
#define __SENSORS_H__

#include <zephyr/drivers/sensor.h>

#ifdef CONFIG_BMC_APP_SENSORS
int read_die_temperature(struct sensor_value *val);
#else
static inline int read_die_temperature(struct sensor_value *val)
{
	return -ENODEV;
}
#endif

#endif
