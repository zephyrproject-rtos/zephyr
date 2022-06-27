/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the WSEN-PDUS sensor driver.
 *
 * #### INFORMATIVE ####
 * The PDUS sensor has no registers to request.
 * It will automatically send up to 4 bytes as reply to any read request to it's PDUS_ADDRESS_I2C.
 * This sensor does not support write requests.
 * This sensor only has a I2C communication interface alongside the analog interface.
 */

#ifndef _WSEN_PDUS_H
#define _WSEN_PDUS_H

/*         Includes         */

#include <stdint.h>

#include <WeSensorsSDK.h>

/*         Available PDUS I2C slave addresses         */

#define PDUS_ADDRESS_I2C    (uint8_t) 0x78

/*         Misc. defines         */

#define P_MIN_VAL_PDUS      (uint16_t) 3277     /**< Minimum raw value for pressure */
#define T_MIN_VAL_PDUS      (uint16_t) 8192     /**< Minimum raw value for temperature in degrees Celsius */

/*         Functional type definitions         */

/**
 * @brief PDUS sensor model
 */
typedef enum {
	PDUS_pdus0,           /**< order code 2513130810001, range = -0.1 to +0.1 kPa */
	PDUS_pdus1,           /**< order code 2513130810101, range = -1 to +1 kPa */
	PDUS_pdus2,           /**< order code 2513130810201, range = -10 to +10 kPa */
	PDUS_pdus3,           /**< order code 2513130810301, range =  0 to 100 kPa */
	PDUS_pdus4,           /**< order code 2513130810401, range = -100 to +1000 kPa */
} PDUS_SensorType_t;

#ifdef __cplusplus
extern "C" {
#endif

/*         Function definitions         */

int8_t PDUS_getDefaultInterface(WE_sensorInterface_t *sensorInterface);

int8_t PDUS_isInterfaceReady(WE_sensorInterface_t *sensorInterface);

int8_t PDUS_getRawPressure(WE_sensorInterface_t *sensorInterface, uint16_t *pressure);
int8_t PDUS_getRawPressureAndTemperature(WE_sensorInterface_t *sensorInterface, uint16_t *pressure,
					 uint16_t *temperature);

int8_t PDUS_getPressure_float(WE_sensorInterface_t *sensorInterface, PDUS_SensorType_t type,
			      float *presskPa);
int8_t PDUS_getPressureAndTemperature_float(WE_sensorInterface_t *sensorInterface,
					    PDUS_SensorType_t type, float *presskPa,
					    float *tempDegC);

int8_t PDUS_convertPressureToFloat(PDUS_SensorType_t type, uint16_t rawPressure, float *presskPa);

#ifdef __cplusplus
}
#endif

#endif /* _WSEN_PDUS_H */
