/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the WSEN-HIDS sensor driver.
 */

#ifndef _WSEN_HIDS_H
#define _WSEN_HIDS_H

/*         Includes         */

#include <stdint.h>

#include <WeSensorsSDK.h>


/*         HIDS 2511020213301 DEVICE_ID         */

#define HIDS_DEVICE_ID_VALUE    0xBC    /**< Device ID of HIDS Sensor */


/*         Available HIDS I2C slave addresses         */

#define HIDS_ADDRESS_I2C_0      0x5F    /**< HIDS I2C address */


/*         Register address definitions         */

#define HIDS_DEVICE_ID_REG      0x0F    /**< Device ID register */
#define HIDS_AVERAGE_REG        0x10    /**< Average configuration register */
#define HIDS_CTRL_REG_1         0x20    /**< Control register 1 */
#define HIDS_CTRL_REG_2         0x21    /**< Control register 2 */
#define HIDS_CTRL_REG_3         0x22    /**< Control register 3 */
#define HIDS_STATUS_REG         0x27    /**< Status register */
#define HIDS_H_OUT_L_REG        0x28    /**< Humidity output LSB value register */
#define HIDS_H_OUT_H_REG        0x29    /**< Humidity output MSB value register */
#define HIDS_T_OUT_L_REG        0x2A    /**< Temperature output LSB value register */
#define HIDS_T_OUT_H_REG        0x2B    /**< Temperature output MSB value register */
#define HIDS_H0_RH_X2           0x30    /**< H0_RH_X2 calibration register */
#define HIDS_H1_RH_X2           0x31    /**< H1_RH_X2 calibration register */
#define HIDS_T0_DEGC_X8         0x32    /**< T0_DEGC_X8 calibration register */
#define HIDS_T1_DEGC_X8         0x33    /**< T1_DEGC_X8 calibration register */
#define HIDS_T0_T1_DEGC_H2      0x35    /**< T0_T1_DEGC_H2 calibration register */
#define HIDS_H0_T0_OUT_L        0x36    /**< H0_T0_OUT_L LSB calibration register */
#define HIDS_H0_T0_OUT_H        0x37    /**< H0_T0_OUT_H MSB calibration register */
#define HIDS_H1_T0_OUT_L        0x3A    /**< H1_T0_OUT_L LSB calibration register */
#define HIDS_H1_T0_OUT_H        0x3B    /**< H1_T0_OUT_H MSB calibration register */
#define HIDS_T0_OUT_L           0x3C    /**< T0_OUT_L LSB calibration register */
#define HIDS_T0_OUT_H           0x3D    /**< T0_OUT_H MSB calibration register */
#define HIDS_T1_OUT_L           0x3E    /**< T1_OUT_L LSB calibration register */
#define HIDS_T1_OUT_H           0x3F    /**< T1_OUT_H MSB calibration register */


/*         Register type definitions         */

/**
 * @brief Humidity and temperature average configuration
 *
 * Address 0x10
 * Type  R/W
 * Default value: 0x1B
 *
 *   AVG 2:0   |  (AVGT) | (AVGH) |
 * ----------------------------------
 *       000   |  2      |  4     |
 *       001   |  4      |  8     |
 *       010   |  8      |  16    |
 *       011   |  16     |  32    |
 *       100   |  32     |  64    |
 *       101   |  64     |  128   |
 *       110   |  128    |  256   |
 *       111   |  256    |  512   |
 * ----------------------------------
 */
typedef struct {
	uint8_t avgHum : 3;    /**< AVG_H: Select the number of averaged humidity samples (4 - 512) */
	uint8_t avgTemp : 3;   /**< AVG_T: Select the number of averaged temperature samples (2 - 256) */
	uint8_t notUsed01 : 2; /**< This bit must be set to 0 for proper operation of the device */
} HIDS_averageConfig_t;

/**
 * @brief Control Register 1
 *
 * Address 0x20
 * Type  R/W
 * Default value: 0x00
 *
 *      ODR1  | ODR0   | Humidity/temperature output data rate (Hz)
 *   ---------------------------------------------------------------------
 *       0    |  0     |              One-shot mode
 *       0    |  1     |                 1
 *       1    |  0     |                 7
 *       1    |  1     |                 12.5
 */
typedef struct {
	uint8_t odr : 2;              /**< ODR: Output data rate selection */
	uint8_t bdu : 1;              /**< BDU: Block data update. 0 - continuous update; 1 - output registers are not updated until both MSB and LSB have been read */
	uint8_t notUsed01 : 4;        /**< This bit must be set to 0 for proper operation of the device */
	uint8_t powerControlMode : 1; /**< PD: (0: power-down mode; 1: active mode) */
} HIDS_ctrl1_t;

/**
 * @brief Control Register 2
 *
 * Address 0x21
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t oneShotBit : 1;         /**< One-shot enable (0: conversion done; 1: start new conversion) */
	uint8_t heater : 1;             /**< Heater (0: heater disabled; 1: heater enabled) */
	uint8_t notUsed01 : 5;          /**< This bit must be set to 0 for proper operation of the device */
	uint8_t rebootMemory : 1;       /**< BOOT (0: normal mode; 1: reboot) */
} HIDS_ctrl2_t;

/**
 * @brief Control Register 3
 *
 * Address 0x22
 * Type  R/W
 * Default value: 0x00
 */
typedef struct {
	uint8_t notUsed01 : 2;          /**< This bit must be set to 0 for proper operation of the device */
	uint8_t enDataReady : 1;        /**< DRDY_EN Data ready interrupt control (0: Data ready interrupt disabled - default; 1: Data ready signal available on pin 3) */
	uint8_t notUsed02 : 3;          /**< This bit must be set to 0 for proper operation of the device */
	uint8_t interruptPinConfig : 1; /**< PP_OD (0: push-pull - default; 1: open drain) */
	uint8_t drdyOutputLevel : 1;    /**< DRDY_H_L: Data ready interrupt level (0: active high - default; 1: active low) */
} HIDS_ctrl3_t;

/**
 * @brief Status register
 *
 * Address 0x27
 * Type  R
 * Default value: 0x00
 */
typedef struct {
	uint8_t humDataAvailable : 1;   /**< H_DA: Pressure data available (0: new humidity sample not yet available; 1: new humidity sample is available) */
	uint8_t tempDataAvailable : 1;  /**< T_DA: Temperature data available (0: new temperature sample not yet available; 1: new temperature sample is available) */
	uint8_t notUsed01 : 6;          /**< This bit must be set to 0 for proper operation of the device */
} HIDS_status_t;


/*         Functional type definitions         */

/**
 * @brief HIDS state type.
 */
typedef enum {
	HIDS_disable = 0,
	HIDS_enable = 1
} HIDS_state_t;


/**
 * @brief Power mode
 */
typedef enum {
	HIDS_powerDownMode = 0,
	HIDS_activeMode = 1
} HIDS_powerMode_t;

/**
 * @brief Output data rate
 */
typedef enum {
	HIDS_oneShot = 0,   /**< One-shot */
	HIDS_odr1Hz = 1,    /**< 1Hz */
	HIDS_odr7Hz = 2,    /**< 7Hz */
	HIDS_odr12_5Hz = 3, /**< 12.5Hz */
} HIDS_outputDataRate_t;

/**
 * @brief Humidity averaging configuration
 */
typedef enum {
	HIDS_humidityAvg4 = 0,
	HIDS_humidityAvg8 = 1,
	HIDS_humidityAvg16 = 2,
	HIDS_humidityAvg32 = 3,
	HIDS_humidityAvg64 = 4,
	HIDS_humidityAvg128 = 5,
	HIDS_humidityAvg256 = 6,
	HIDS_humidityAvg512 = 7
} HIDS_humidityAverageConfig_t;

/**
 * @brief Temperature averaging configuration
 */
typedef enum {
	HIDS_temperatureAvg2 = 0,
	HIDS_temperatureAvg4 = 1,
	HIDS_temperatureAvg8 = 2,
	HIDS_temperatureAvg16 = 3,
	HIDS_temperatureAvg32 = 4,
	HIDS_temperatureAvg64 = 5,
	HIDS_temperatureAvg128 = 6,
	HIDS_temperatureAvg256 = 7
} HIDS_temperatureAverageConfig_t;

/**
 * @brief Interrupt active level
 */
typedef enum {
	HIDS_activeHigh = 0,
	HIDS_activeLow = 1
} HIDS_interruptActiveLevel_t;

/**
 * @brief Interrupt pin configuration
 */
typedef enum {
	HIDS_pushPull = 0,
	HIDS_openDrain = 1
} HIDS_interruptPinConfig_t;

/*         Function definitions         */

#ifdef __cplusplus
extern "C" {
#endif

int8_t HIDS_getDefaultInterface(WE_sensorInterface_t *sensorInterface);

int8_t HIDS_isInterfaceReady(WE_sensorInterface_t *sensorInterface);

int8_t HIDS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID);

uint8_t HIDS_setHumidityAverageConfig(WE_sensorInterface_t *sensorInterface,
				      HIDS_humidityAverageConfig_t avgHum);
uint8_t HIDS_getHumidityAverageConfig(WE_sensorInterface_t *sensorInterface,
				      HIDS_humidityAverageConfig_t *avgHum);
uint8_t HIDS_setTemperatureAverageConfig(WE_sensorInterface_t *sensorInterface,
					 HIDS_temperatureAverageConfig_t avgTemp);
uint8_t HIDS_getTemperatureAverageConfig(WE_sensorInterface_t *sensorInterface,
					 HIDS_temperatureAverageConfig_t *avgTemp);

int8_t HIDS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, HIDS_outputDataRate_t odr);
int8_t HIDS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, HIDS_outputDataRate_t *odr);

int8_t HIDS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, HIDS_state_t bdu);
int8_t HIDS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, HIDS_state_t *bdu);

int8_t HIDS_setPowerMode(WE_sensorInterface_t *sensorInterface, HIDS_powerMode_t pd);
int8_t HIDS_getPowerMode(WE_sensorInterface_t *sensorInterface, HIDS_powerMode_t *pd);

int8_t HIDS_enableOneShot(WE_sensorInterface_t *sensorInterface, HIDS_state_t oneShot);
int8_t HIDS_isOneShotEnabled(WE_sensorInterface_t *sensorInterface, HIDS_state_t *oneShot);

int8_t HIDS_enableHeater(WE_sensorInterface_t *sensorInterface, HIDS_state_t heater);
int8_t HIDS_isHeaterEnabled(WE_sensorInterface_t *sensorInterface, HIDS_state_t *heater);

int8_t HIDS_reboot(WE_sensorInterface_t *sensorInterface, HIDS_state_t reboot);
int8_t HIDS_isRebooting(WE_sensorInterface_t *sensorInterface, HIDS_state_t *rebooting);

int8_t HIDS_enableDataReadyInterrupt(WE_sensorInterface_t *sensorInterface, HIDS_state_t drdy);
int8_t HIDS_isDataReadyInterruptEnabled(WE_sensorInterface_t *sensorInterface, HIDS_state_t *drdy);

int8_t HIDS_setInterruptPinType(WE_sensorInterface_t *sensorInterface,
				HIDS_interruptPinConfig_t pinType);
int8_t HIDS_getInterruptPinType(WE_sensorInterface_t *sensorInterface,
				HIDS_interruptPinConfig_t *pinType);

int8_t HIDS_setInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    HIDS_interruptActiveLevel_t level);
int8_t HIDS_getInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    HIDS_interruptActiveLevel_t *level);

int8_t HIDS_isHumidityDataAvailable(WE_sensorInterface_t *sensorInterface, HIDS_state_t *state);
int8_t HIDS_isTemperatureDataAvailable(WE_sensorInterface_t *sensorInterface, HIDS_state_t *state);

int8_t HIDS_getRawHumidity(WE_sensorInterface_t *sensorInterface, int16_t *rawHumidity);
int8_t HIDS_getRawTemperature(WE_sensorInterface_t *sensorInterface, int16_t *rawTemp);
int8_t HIDS_getRawValues(WE_sensorInterface_t *sensorInterface, int16_t *rawHumidity,
			 int16_t *rawTemp);

int8_t HIDS_getHumidity_float(WE_sensorInterface_t *sensorInterface, float *humidity);
int8_t HIDS_getTemperature_float(WE_sensorInterface_t *sensorInterface, float *tempDegC);

int8_t HIDS_convertHumidity_float(WE_sensorInterface_t *sensorInterface, int16_t rawHumidity,
				  float *humidity);
int8_t HIDS_convertTemperature_float(WE_sensorInterface_t *sensorInterface, int16_t rawTemp,
				     float *tempDegC);

int8_t HIDS_getHumidity_int8(WE_sensorInterface_t *sensorInterface, int8_t *humidity);
int8_t HIDS_getTemperature_int8(WE_sensorInterface_t *sensorInterface, int8_t *tempDegC);

int8_t HIDS_convertHumidity_int8(WE_sensorInterface_t *sensorInterface, int16_t rawHumidity,
				 int8_t *humidity);
int8_t HIDS_convertTemperature_int8(WE_sensorInterface_t *sensorInterface, int16_t rawTemp,
				    int8_t *tempDegC);

int8_t HIDS_getHumidity_uint16(WE_sensorInterface_t *sensorInterface, uint16_t *humidity);
int8_t HIDS_getTemperature_int16(WE_sensorInterface_t *sensorInterface, int16_t *temperature);

int8_t HIDS_convertHumidity_uint16(WE_sensorInterface_t *sensorInterface, int16_t rawHumidity,
				   uint16_t *humidity);
int8_t HIDS_convertTemperature_int16(WE_sensorInterface_t *sensorInterface, int16_t rawTemp,
				     int16_t *temperature);

int8_t HIDS_readCalibrationData(WE_sensorInterface_t *sensorInterface);

#ifdef __cplusplus
}
#endif

#endif /* _WSEN_HIDS_H */
