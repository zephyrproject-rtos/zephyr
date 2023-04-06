/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_THERMISTOR_H_
#define ZEPHYR_DRIVERS_SENSOR_THERMISTOR_H_

/* The thermistor is connected to Ground with the resister between it and Vin. */
#define THERMISTOR_WIRING_VIN_R_THERM_GND (0)

/* The thermistor is connected to Vin with the resister between it and ground. */
#define THERMISTOR_WIRING_VIN_THERM_R_GND (1)

#endif /* ZEPHYR_DRIVERS_SENSOR_THERMISTOR_H_ */
