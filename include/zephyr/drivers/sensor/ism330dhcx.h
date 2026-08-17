/*
 * Copyright (c) 2026 Filics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_

#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sensor_attribute_ism330dhcx {
	/** Real output data rate in Hz, trimmed by INTERNAL_FREQ_FINE. Read-only. */
	SENSOR_ATTR_ISM330DHCX_REAL_ODR = SENSOR_ATTR_PRIV_START,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_ISM330DHCX_H_ */
