/*
 *Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_

#include "bmi323.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i3c.h>

#define BMI323_DEVICE_I3C_BUS(inst)                                                                \
	extern const struct bosch_bmi323_bus_api bosch_bmi323_i3c_bus_api;                         \
                                                                                                   \
	static const struct i3c_dt_spec i3c_spec##inst =                                           \
		I3C_DT_SPEC_INST_GET(inst);                                                        \
                                                                                                   \
	static const struct bosch_bmi323_bus bosch_bmi323_bus_api##inst = {                        \
		.context = &i3c_spec##inst, .api = &bosch_bmi323_i3c_bus_api}

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_ */
