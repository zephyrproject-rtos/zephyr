/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SENSORY_H__
#define SENSORY_H__
#include <zephyr.h>
#define INVALID_SENSOR_VALUE	(s32_t)(-10000)

int sensors_init(void);
int sensors_get_temperature(void);
int sensors_get_humidity(void);
int sensors_get_temperature_external(void);
void sensors_set_temperature_external(s16_t tmp);

#endif

