/*
 * Copyright (c) 2025 Pivot International Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_RENESAS_RA_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_RENESAS_RA_H_

#include <zephyr/drivers/sensor.h>

enum sensor_attribute_qdec_renesas_ra {
	/* Number of counts per revolution */
	SENSOR_ATTR_QDEC_MOD_VAL = SENSOR_ATTR_PRIV_START,
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_QDEC_RENESAS_RA_H_ */
