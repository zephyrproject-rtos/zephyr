/*
 * Copyright (c) 2026 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_

#include <zephyr/devicetree.h>
#include <zephyr/drivers/i3c.h>
#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#endif

#include "bmi323.h"

#define IMU_BOSCH_BMI323_I3C_DUMMY_OFFSET 2

struct bosch_bmi323_i3c {
	const struct device *bus;
	const struct i3c_device_id id;
	struct i3c_device_desc *desc;
};

#define BMI323_DEVICE_I3C_BUS(inst)							\
	extern const struct bosch_bmi323_bus_api bosch_bmi323_i3c_bus_api;		\
											\
	COND_CODE_1(CONFIG_I3C_RTIO, (BMI323_I3C_IODEV_DEFINE(inst)), ());		\
											\
	static struct bosch_bmi323_i3c i3c_ctx##inst = {				\
		.bus = DEVICE_DT_GET(DT_INST_BUS(inst)),				\
		.id = I3C_DEVICE_ID_DT_INST(inst),					\
	};										\
											\
	static const struct bosch_bmi323_bus bosch_bmi323_bus_api##inst = {		\
		.context = &i3c_ctx##inst, .api = &bosch_bmi323_i3c_bus_api}

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_I3C_H_ */
