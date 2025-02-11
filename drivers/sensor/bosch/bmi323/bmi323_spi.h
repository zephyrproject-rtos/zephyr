/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_SPI_H_
#define ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_SPI_H_

#include "bmi323.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>

#define BMI323_DEVICE_SPI_BUS(inst)                                                                \
	extern const struct bosch_bmi323_bus_api bosch_bmi323_spi_bus_api;                         \
                                                                                                   \
	static const struct spi_dt_spec spi_spec##inst =                                           \
		SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0);                                    \
                                                                                                   \
	static const struct bosch_bmi323_bus bosch_bmi323_bus_api##inst = {                        \
		.context = &spi_spec##inst, .api = &bosch_bmi323_spi_bus_api}

#endif /* ZEPHYR_DRIVERS_SENSOR_BMI323_BMI323_SPI_H_ */
