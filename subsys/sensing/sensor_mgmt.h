/*
 * Copyright (c) 2023 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SENSOR_MGMT_H_
#define SENSOR_MGMT_H_

#include <zephyr/sensing/sensing_datatypes.h>
#include <zephyr/sensing/sensing_sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

struct sensing_mgmt_context {
	bool sensing_initialized;
	int sensor_num;
	struct sensing_sensor_info *info;
};
/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* SENSOR_MGMT_H_ */
