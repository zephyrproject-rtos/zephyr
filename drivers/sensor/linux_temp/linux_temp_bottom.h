/*
 * Copyright 2026 Bayrem Gharsellaoui
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_LINUX_TEMP_LINUX_TEMP_BOTTOM_H_
#define ZEPHYR_DRIVERS_SENSOR_LINUX_TEMP_LINUX_TEMP_BOTTOM_H_

#include <stdint.h>

int linux_temp_read(const char *path, int32_t *temperature_mc);

#endif /* ZEPHYR_DRIVERS_SENSOR_LINUX_TEMP_LINUX_TEMP_BOTTOM_H_ */
