/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver file for the WSEN-TIDS sensor.
 */

#include "WSEN_TIDS_2521020222501.h"

#include <stdio.h>

#include <weplatform.h>

/**
 * @brief Default sensor interface configuration.
 */
static WE_sensorInterface_t tidsDefaultSensorInterface = {
	.sensorType = WE_TIDS,
	.interfaceType = WE_i2c,
	.options = { .i2c = { .address = TIDS_ADDRESS_I2C_1,
			      .burstMode = 0,
			      .slaveTransmitterMode = 0,
			      .useRegAddrMsbForMultiBytesRead = 0,
			      .reserved = 0 },
		     .spi = { .chipSelectPort = 0,
			      .chipSelectPin = 0,
			      .burstMode = 0,
			      .reserved = 0 },
		     .readTimeout = 1000,
		     .writeTimeout = 1000 },
	.handle = 0
};

/**
 * @brief Read data from sensor.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] regAdr Address of register to read from
 * @param[in] numBytesToRead Number of bytes to be read
 * @param[out] data Target buffer
 * @return Error Code
 */
static inline int8_t TIDS_ReadReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
				  uint16_t numBytesToRead, uint8_t *data)
{
	return WE_ReadReg(sensorInterface, regAdr, numBytesToRead, data);
}

/**
 * @brief Write data to sensor.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] regAdr Address of register to write to
 * @param[in] numBytesToWrite Number of bytes to be written
 * @param[in] data Source buffer
 * @return Error Code
 */
static inline int8_t TIDS_WriteReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
				   uint16_t numBytesToWrite, uint8_t *data)
{
	return WE_WriteReg(sensorInterface, regAdr, numBytesToWrite, data);
}

/**
 * @brief Returns the default sensor interface configuration.
 * @param[out] sensorInterface Sensor interface configuration (output parameter)
 * @return Error code
 */
int8_t TIDS_getDefaultInterface(WE_sensorInterface_t *sensorInterface)
{
	*sensorInterface = tidsDefaultSensorInterface;
	return WE_SUCCESS;
}

/**
 * @brief Checks if the sensor interface is ready.
 * @param[in] sensorInterface Pointer to sensor interface
 * @return WE_SUCCESS if interface is ready, WE_FAIL if not.
 */
int8_t TIDS_isInterfaceReady(WE_sensorInterface_t *sensorInterface)
{
	return WE_isSensorInterfaceReady(sensorInterface);
}

/**
 * @brief Read the device ID
 *
 * Expected value is TIDS_DEVICE_ID_VALUE.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] deviceID The returned device ID.
 * @retval Error code
 */
int8_t TIDS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID)
{
	return TIDS_ReadReg(sensorInterface, TIDS_DEVICE_ID_REG, 1, deviceID);
}

/**
 * @brief Set software reset [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] swReset Software reset state
 * @retval Error code
 */
int8_t TIDS_softReset(WE_sensorInterface_t *sensorInterface, TIDS_state_t swReset)
{
	TIDS_softReset_t swRstReg;

	if (WE_FAIL ==
	    TIDS_ReadReg(sensorInterface, TIDS_SOFT_RESET_REG, 1, (uint8_t *)&swRstReg)) {
		return WE_FAIL;
	}

	swRstReg.reset = swReset;

	return TIDS_WriteReg(sensorInterface, TIDS_SOFT_RESET_REG, 1, (uint8_t *)&swRstReg);
}

/**
 * @brief Read the software reset state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] swReset The returned software reset state.
 * @retval Error code
 */
int8_t TIDS_getSoftResetState(WE_sensorInterface_t *sensorInterface, TIDS_state_t *swReset)
{
	TIDS_softReset_t swRstReg;

	if (WE_FAIL ==
	    TIDS_ReadReg(sensorInterface, TIDS_SOFT_RESET_REG, 1, (uint8_t *)&swRstReg)) {
		return WE_FAIL;
	}

	*swReset = (TIDS_state_t)swRstReg.reset;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable continuous (free run) mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] mode Continuous mode state
 * @retval Error code
 */
int8_t TIDS_enableContinuousMode(WE_sensorInterface_t *sensorInterface, TIDS_state_t mode)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	ctrlReg.freeRunBit = mode;

	return TIDS_WriteReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg);
}

/**
 * @brief Check if continuous (free run) mode is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] mode The returned continuous mode enable state
 * @retval Error code
 */
int8_t TIDS_isContinuousModeEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *mode)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	*mode = (TIDS_state_t)ctrlReg.freeRunBit;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable block data update mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] bdu Block data update state
 * @retval Error code
 */
int8_t TIDS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, TIDS_state_t bdu)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	ctrlReg.blockDataUpdate = bdu;

	return TIDS_WriteReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg);
}

/**
 * @brief Read the block data update state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] bdu The returned block data update state
 * @retval Error code
 */
int8_t TIDS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *bdu)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	*bdu = (TIDS_state_t)ctrlReg.blockDataUpdate;

	return WE_SUCCESS;
}

/**
 * @brief Set the output data rate of the sensor
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] odr Output data rate
 * @return Error code
 */
int8_t TIDS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, TIDS_outputDataRate_t odr)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	ctrlReg.outputDataRate = odr;

	return TIDS_WriteReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg);
}

/**
 * @brief Read the output data rate of the sensor
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] odr The returned output data rate
 * @return Error code
 */
int8_t TIDS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, TIDS_outputDataRate_t *odr)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	*odr = (TIDS_outputDataRate_t)ctrlReg.outputDataRate;

	return WE_SUCCESS;
}

/**
 * @brief Trigger capturing of a new value in one-shot mode.
 * Note: One shot mode can be used for measurement frequencies up to 1 Hz.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] oneShot One shot bit state
 * @return Error code
 */
int8_t TIDS_enableOneShot(WE_sensorInterface_t *sensorInterface, TIDS_state_t oneShot)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	ctrlReg.oneShotBit = oneShot;

	return TIDS_WriteReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg);
}

/**
 * @brief Read the one shot bit state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] oneShot The returned one shot bit state
 * @retval Error code
 */
int8_t TIDS_isOneShotEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *oneShot)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	*oneShot = (TIDS_state_t)ctrlReg.oneShotBit;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable auto increment mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] autoIncr Auto increment mode state
 * @retval Error code
 */
int8_t TIDS_enableAutoIncrement(WE_sensorInterface_t *sensorInterface, TIDS_state_t autoIncr)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	ctrlReg.autoAddIncr = autoIncr;

	return TIDS_WriteReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg);
}

/**
 * @brief Read the auto increment mode state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] autoIncr The returned auto increment mode state
 * @retval Error code
 */
int8_t TIDS_isAutoIncrementEnabled(WE_sensorInterface_t *sensorInterface, TIDS_state_t *autoIncr)
{
	TIDS_ctrl_t ctrlReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_CTRL_REG, 1, (uint8_t *)&ctrlReg)) {
		return WE_FAIL;
	}

	*autoIncr = (TIDS_state_t)ctrlReg.autoAddIncr;

	return WE_SUCCESS;
}

/**
 * @brief Set upper temperature limit
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] hLimit Upper limit
 * @retval Error code
 */
int8_t TIDS_setTempHighLimit(WE_sensorInterface_t *sensorInterface, uint8_t hLimit)
{
	return TIDS_WriteReg(sensorInterface, TIDS_LIMIT_T_H_REG, 1, &hLimit);
}

/**
 * @brief Get upper temperature limit
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] hLimit The returned temperature high limit
 * @retval Error code
 */
int8_t TIDS_getTempHighLimit(WE_sensorInterface_t *sensorInterface, uint8_t *hLimit)
{
	return TIDS_ReadReg(sensorInterface, TIDS_LIMIT_T_H_REG, 1, hLimit);
}

/**
 * @brief Set lower temperature limit
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] lLimit Low limit
 * @retval Error code
 */
int8_t TIDS_setTempLowLimit(WE_sensorInterface_t *sensorInterface, uint8_t lLimit)
{
	return TIDS_WriteReg(sensorInterface, TIDS_LIMIT_T_L_REG, 1, &lLimit);
}

/**
 * @brief Get lower temperature limit
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lLimit The returned temperature low limit
 * @retval Error code
 */
int8_t TIDS_getTempLowLimit(WE_sensorInterface_t *sensorInterface, uint8_t *lLimit)
{
	return TIDS_ReadReg(sensorInterface, TIDS_LIMIT_T_L_REG, 1, lLimit);
}

/**
 * @brief Get overall sensor status
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] status The returned sensor status data
 * @retval Error code
 */
int8_t TIDS_getStatusRegister(WE_sensorInterface_t *sensorInterface, TIDS_status_t *status)
{
	return TIDS_ReadReg(sensorInterface, TIDS_STATUS_REG, 1, (uint8_t *)status);
}

/**
 * @brief Check if the sensor is busy
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] busy The returned busy state
 * @retval Error code
 */
int8_t TIDS_isBusy(WE_sensorInterface_t *sensorInterface, TIDS_state_t *busy)
{
	TIDS_status_t statusReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*busy = (TIDS_state_t)statusReg.busy;

	return WE_SUCCESS;
}

/**
 * @brief Check if upper limit has been exceeded
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned limit exceeded state
 * @retval Error code
 */
int8_t TIDS_isUpperLimitExceeded(WE_sensorInterface_t *sensorInterface, TIDS_state_t *state)
{
	TIDS_status_t statusReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*state = (TIDS_state_t)statusReg.upperLimitExceeded;

	return WE_SUCCESS;
}

/**
 * @brief Check if lower limit has been exceeded
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned limit exceeded state
 * @retval Error code
 */
int8_t TIDS_isLowerLimitExceeded(WE_sensorInterface_t *sensorInterface, TIDS_state_t *state)
{
	TIDS_status_t statusReg;

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*state = (TIDS_state_t)statusReg.lowerLimitExceeded;

	return WE_SUCCESS;
}

/**
 * @brief Read the raw measured temperature value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] rawTemp The returned temperature measurement
 * @retval Error code
 */
int8_t TIDS_getRawTemperature(WE_sensorInterface_t *sensorInterface, int16_t *rawTemp)
{
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_DATA_T_L_REG, 1, &tmp[0])) {
		return WE_FAIL;
	}

	if (WE_FAIL == TIDS_ReadReg(sensorInterface, TIDS_DATA_T_H_REG, 1, &tmp[1])) {
		return WE_FAIL;
	}

	*rawTemp = (int16_t)(tmp[1] << 8);
	*rawTemp |= (int16_t)tmp[0];
	return WE_SUCCESS;
}

/**
 * @brief Read the measured temperature value in °C
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tempDegC The returned temperature measurement
 * @retval Error code
 */
int8_t TIDS_getTemperature(WE_sensorInterface_t *sensorInterface, float *tempDegC)
{
	int16_t rawTemp = 0;
	if (WE_FAIL == TIDS_getRawTemperature(sensorInterface, &rawTemp)) {
		return WE_FAIL;
	}

	*tempDegC = (float)rawTemp;
	*tempDegC = *tempDegC / 100;
	return WE_SUCCESS;
}
