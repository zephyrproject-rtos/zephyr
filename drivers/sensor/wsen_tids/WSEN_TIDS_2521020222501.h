/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the WSEN-TIDS sensor driver.
 *
 * #### INFORMATIVE ####
 * This sensor only has a I2C communication interface.
 */

#ifndef _WSEN_TIDS_H
#define _WSEN_TIDS_H

/*         Includes         */

#include <stdint.h>

#include <WeSensorsSDK.h>


/*         TIDS 2521020222501 DEVICE_ID         */

#define TIDS_DEVICE_ID_VALUE        0xA0     /**< Device ID of TIDS 2521020222501 Sensor */


/*         Available TIDS I2C slave addresses         */

#define TIDS_ADDRESS_I2C_0          0x3F     /**< When SAO of TIDS is connected to ground */
#define TIDS_ADDRESS_I2C_1          0x38     /**< When SAO of TIDS is connected to positive supply voltage */


/*         Register address definitions         */

#define TIDS_DEVICE_ID_REG          0x01     /**< Device ID register */
#define TIDS_LIMIT_T_H_REG          0x02     /**< Temperature high limit register */
#define TIDS_LIMIT_T_L_REG          0x03     /**< Temperature low limit register */
#define TIDS_CTRL_REG               0x04     /**< Control register */
#define TIDS_STATUS_REG             0x05     /**< Status register */
#define TIDS_DATA_T_L_REG           0x06     /**< Temperature output LSB value register */
#define TIDS_DATA_T_H_REG           0x07     /**< Temperature output MSB value register */
#define TIDS_SOFT_RESET_REG         0x0C     /**< Software reset register */


/*         Register type definitions         */

/**
 * @brief Control register
 * Address 0x04
 * Type  R/W
 * Default value: 0x00
 *
 *     AVG1  | AVG0  | Temperature output data-rate (Hz)
 *  ---------------- ----------------------------------------------------
 *      0    |  0    |      25
 *      0    |  1    |      50
 *      1    |  0    |      100
 *      1    |  1    |      200
 */
typedef struct {
	uint8_t oneShotBit      : 1;  /**< Trigger a single measurement by setting this bit to 1; Bit is automatically reset to 0 */
	uint8_t reserved01      : 1;  /**< Must be set to 0 */
	uint8_t freeRunBit      : 1;  /**< FREERUN: 1: Enable continuous mode, 0: Disable continuous mode */
	uint8_t autoAddIncr     : 1;  /**< IF_ADD_INC: Register address automatically incremented during a multiple byte access with I2C interface. Default value 1 (0: disable; 1: enable) */
	uint8_t outputDataRate  : 2;  /**< AVG[1:0]: Output data rate in continuous mode. Default '00' */
	uint8_t blockDataUpdate : 1;  /**< BDU: Block data update. 0: continuous update; 1: output registers are not updated until both MSB and LSB have been read */
	uint8_t reserved02      : 1;  /**< Must be set to 0 */
} TIDS_ctrl_t;

/**
 * @brief Status register
 * Address 0x05
 * Type  R
 * Default value: Output; 0x00
 */
typedef struct {
	uint8_t busy               : 1; /**< BUSY: Temperature conversion status (0: data conversion complete; 1: data conversion in progress) */
	uint8_t upperLimitExceeded : 1; /**< OVER_THL: Temperature upper limit status (0: temperature is below upper limit or disabled; 1: temperature exceeded high limit */
	uint8_t lowerLimitExceeded : 2; /**< UNDER_TLL: Temperature lower limit status (0: temperature is above lower limit or disabled; 1: temperature exceeded low limit */
	uint8_t reserved01         : 5; /**< Must be set to 0 */
} TIDS_status_t;

/**
 * @brief Software reset register
 * Address 0x0C
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t reserved01 : 1; /**< Must be set to 0 */
	uint8_t reset      : 1; /**< SOFT_RESET: Perform software reset by setting this bit to 1 */
	uint8_t reserved02 : 6; /**< Must be set to 0 */
} TIDS_softReset_t;


/*         Functional type definition         */

typedef enum {
	TIDS_disable = 0,
	TIDS_enable = 1
} TIDS_state_t;


typedef enum {
	TIDS_outputDataRate25Hz  = 0,  /**< 25 Hz */
	TIDS_outputDataRate50Hz  = 1,  /**< 50 Hz */
	TIDS_outputDataRate100Hz = 2,  /**< 100 Hz */
	TIDS_outputDataRate200Hz = 3,  /**< 200 Hz */
} TIDS_outputDataRate_t;

#ifdef __cplusplus
extern "C" {
#endif

/*         Function definitions         */

/* Sensor/interface initialization */
int8_t TIDS_getDefaultInterface(WE_sensorInterface_t *sensorInterface);

int8_t TIDS_isInterfaceReady(WE_sensorInterface_t *sensorInterface);

/* Device ID */
int8_t TIDS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID);

/* Software reset */
int8_t TIDS_softReset(WE_sensorInterface_t *sensorInterface, TIDS_state_t swReset);
int8_t TIDS_getSoftResetState(WE_sensorInterface_t *sensorInterface, TIDS_state_t *swReset);

/* Free run mode */
int8_t TIDS_enableContinuousMode(WE_sensorInterface_t *sensorInterface, TIDS_state_t mode);
int8_t TIDS_isContinuousModeEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *mode);

/* Block data update */
int8_t TIDS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, TIDS_state_t bdu);
int8_t TIDS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *bdu);

/* Output data rate */
int8_t TIDS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, TIDS_outputDataRate_t odr);
int8_t TIDS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, TIDS_outputDataRate_t *odr);

/* One shot mode */
int8_t TIDS_enableOneShot(WE_sensorInterface_t *sensorInterface, TIDS_state_t oneShot);
int8_t TIDS_isOneShotEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *oneShot);

/* Address auto increment */
int8_t TIDS_enableAutoIncrement(WE_sensorInterface_t *sensorInterface, TIDS_state_t autoIncr);
int8_t TIDS_isAutoIncrementEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *autoIncr);

/* Temperature limits */
int8_t TIDS_setTempHighLimit(WE_sensorInterface_t *sensorInterface, uint8_t hLimit);
int8_t TIDS_getTempHighLimit(WE_sensorInterface_t *sensorInterface, uint8_t *hLimit);

int8_t TIDS_setTempLowLimit(WE_sensorInterface_t *sensorInterface, uint8_t lLimit);
int8_t TIDS_getTempLowLimit(WE_sensorInterface_t *sensorInterface, uint8_t *lLimit);

/* Status */
int8_t TIDS_getStatusRegister(WE_sensorInterface_t *sensorInterface, TIDS_status_t *status);
int8_t TIDS_isBusy(WE_sensorInterface_t *sensorInterface, TIDS_state_t *busy);
int8_t TIDS_isUpperLimitExceeded(WE_sensorInterface_t *sensorInterface, TIDS_state_t *state);
int8_t TIDS_isLowerLimitExceeded(WE_sensorInterface_t *sensorInterface, TIDS_state_t *state);

/* Standard data out */
int8_t TIDS_getRawTemperature(WE_sensorInterface_t *sensorInterface, int16_t *rawTemp);
int8_t TIDS_getTemperature(WE_sensorInterface_t *sensorInterface, float *tempDegC);

#ifdef __cplusplus
}
#endif

#endif /* _WSEN_TIDS_H */
