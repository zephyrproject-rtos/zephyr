/**
 * @file ipso_filling_sensor.h
 * @brief
 *
 * Copyright (c) 2021 Laird Connectivity
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __IPSO_FILLING_SENSOR__
#define __IPSO_FILLING_SENSOR__

#include <zephyr/net/lwm2m.h>

/* Resource IDs for filling sensor */
/* clang-format off */
#define CONTAINER_HEIGHT_FILLING_SENSOR_RID           1
#define ACTUAL_FILL_PERCENTAGE_FILLING_SENSOR_RID     2
#define ACTUAL_FILL_LEVEL_FILLING_SENSOR_RID          3
#define HIGH_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID  4
#define CONTAINER_FULL_FILLING_SENSOR_RID             5
#define LOW_THRESHOLD_PERCENTAGE_FILLING_SENSOR_RID   6
#define CONTAINER_EMPTY_FILLING_SENSOR_RID            7
#define AVERAGE_FILL_SPEED_FILLING_SENSOR_RID         8
#define RESET_AVERAGE_FILL_SPEED_FILLING_SENSOR_RID   9
#define FORECAST_FULL_DATE_FILLING_SENSOR_RID         10
#define FORECAST_EMPTY_DATE_FILLING_SENSOR_RID        11
#define CONTAINER_OUT_OF_LOCATION_FILLING_SENSOR_RID  12
#define CONTAINER_OUT_OF_POSITION_FILLING_SENSOR_RID  13
/* clang-format on */

#endif /* __IPSO_FILLING_SENSOR__ */
