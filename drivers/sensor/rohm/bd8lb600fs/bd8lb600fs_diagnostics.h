/*
 * Copyright (c) 2024 SILA Embedded Solutions GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BD8LB600FS_BD8LB600FS_DIAGNOSTICS_H_
#define ZEPHYR_DRIVERS_SENSOR_BD8LB600FS_BD8LB600FS_DIAGNOSTICS_H_

#include <zephyr/device.h>

struct bd8lb600fs_diagnostics_data {
	/* open load detection */
	uint32_t old;
	/* over current protection or thermal shutdown*/
	uint32_t ocp_or_tsd;
};

struct bd8lb600fs_diagnostics_config {
	const struct device *parent_dev;
};

#endif /* ZEPHYR_DRIVERS_SENSOR_BD8LB600FS_BD8LB600FS_DIAGNOSTICS_H_ */
