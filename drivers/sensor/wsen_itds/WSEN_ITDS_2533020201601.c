/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver file for the WSEN-ITDS sensor.
 */

#include "WSEN_ITDS_2533020201601.h"

#include <stdio.h>

#include <weplatform.h>

/**
 * @brief Default sensor interface configuration.
 */
static WE_sensorInterface_t itdsDefaultSensorInterface = {
	.sensorType = WE_ITDS,
	.interfaceType = WE_i2c,
	.options = { .i2c = { .address = ITDS_ADDRESS_I2C_1,
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
 * @brief Stores the current value of the full scale parameter.

 * The value is updated when calling ITDS_setFullScale() or
 * ITDS_getFullScale().
 */
static ITDS_fullScale_t currentFullScale = ITDS_twoG;

/**
 * @brief Read data from sensor.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] regAdr Address of register to read from
 * @param[in] numBytesToRead Number of bytes to be read
 * @param[out] data Target buffer
 * @return Error Code
 */
static inline int8_t ITDS_ReadReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
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
static inline int8_t ITDS_WriteReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
				   uint16_t numBytesToWrite, uint8_t *data)
{
	return WE_WriteReg(sensorInterface, regAdr, numBytesToWrite, data);
}

/**
 * @brief Returns the default sensor interface configuration.
 * @param[out] sensorInterface Sensor interface configuration (output parameter)
 * @return Error code
 */
int8_t ITDS_getDefaultInterface(WE_sensorInterface_t *sensorInterface)
{
	*sensorInterface = itdsDefaultSensorInterface;
	return WE_SUCCESS;
}

/**
 * @brief Checks if the sensor interface is ready.
 * @param[in] sensorInterface Pointer to sensor interface
 * @return WE_SUCCESS if interface is ready, WE_FAIL if not.
 */
int8_t ITDS_isInterfaceReady(WE_sensorInterface_t *sensorInterface)
{
	return WE_isSensorInterfaceReady(sensorInterface);
}

/**
 * @brief Read the device ID
 *
 * Expected value is ITDS_DEVICE_ID_VALUE.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] deviceID The returned device ID.
 * @retval Error code
 */
int8_t ITDS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID)
{
	return ITDS_ReadReg(sensorInterface, ITDS_DEVICE_ID_REG, 1, deviceID);
}

/* CTRL_1 */

/**
 * @brief Set the output data rate
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] odr Output data rate
 * @retval Error code
 */
int8_t ITDS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, ITDS_outputDataRate_t odr)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.outputDataRate = odr;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Read the output data rate
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] odr The returned output data rate.
 * @retval Error code
 */
int8_t ITDS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, ITDS_outputDataRate_t *odr)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*odr = (ITDS_outputDataRate_t)ctrl1.outputDataRate;

	return WE_SUCCESS;
}

/**
 * @brief Set the operating mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] opMode Operating mode
 * @retval Error code
 */
int8_t ITDS_setOperatingMode(WE_sensorInterface_t *sensorInterface, ITDS_operatingMode_t opMode)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.operatingMode = opMode;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Read the operating mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] opMode The returned operating mode.
 * @retval Error code
 */
int8_t ITDS_getOperatingMode(WE_sensorInterface_t *sensorInterface, ITDS_operatingMode_t *opMode)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*opMode = (ITDS_operatingMode_t)ctrl1.operatingMode;

	return WE_SUCCESS;
}

/**
 * @brief Set the power mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] powerMode Power mode
 * @retval Error code
 */
int8_t ITDS_setPowerMode(WE_sensorInterface_t *sensorInterface, ITDS_powerMode_t powerMode)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.powerMode = powerMode;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Read the power mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] powerMode The returned power mode.
 * @retval Error code
 */
int8_t ITDS_getPowerMode(WE_sensorInterface_t *sensorInterface, ITDS_powerMode_t *powerMode)
{
	ITDS_ctrl1_t ctrl1;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*powerMode = ctrl1.powerMode;

	return WE_SUCCESS;
}

/* CTRL REG 2 */

/**
 * @brief (Re)boot the device [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] reboot Reboot state
 * @retval Error code
 */
int8_t ITDS_reboot(WE_sensorInterface_t *sensorInterface, ITDS_state_t reboot)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.boot = reboot;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the reboot state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] rebooting The returned reboot state.
 * @retval Error code
 */
int8_t ITDS_isRebooting(WE_sensorInterface_t *sensorInterface, ITDS_state_t *rebooting)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*rebooting = (ITDS_state_t)ctrl2.boot;

	return WE_SUCCESS;
}

/**
 * @brief Set software reset [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] swReset Software reset state
 * @retval Error code
 */
int8_t ITDS_softReset(WE_sensorInterface_t *sensorInterface, ITDS_state_t swReset)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.softReset = swReset;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the software reset state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] swReset The returned software reset state.
 * @retval Error code
 */
int8_t ITDS_getSoftResetState(WE_sensorInterface_t *sensorInterface, ITDS_state_t *swReset)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*swReset = (ITDS_state_t)ctrl2.softReset;

	return WE_SUCCESS;
}

/**
 * @brief Disconnect CS pin pull up [pull up connected, pull up disconnected]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] disconnectPU CS pin pull up state
 * @retval Error code
 */
int8_t ITDS_setCSPullUpDisconnected(WE_sensorInterface_t *sensorInterface,
				    ITDS_state_t disconnectPU)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.disCSPullUp = disconnectPU;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the CS pin pull up state [pull up connected, pull up disconnected]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] puDisconnected The returned CS pin pull up state
 * @retval Error code
 */
int8_t ITDS_isCSPullUpDisconnected(WE_sensorInterface_t *sensorInterface,
				   ITDS_state_t *puDisconnected)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*puDisconnected = (ITDS_state_t)ctrl2.disCSPullUp;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable block data update mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] bdu Block data update state
 * @retval Error code
 */
int8_t ITDS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, ITDS_state_t bdu)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.blockDataUpdate = bdu;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the block data update state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] bdu The returned block data update state
 * @retval Error code
 */
int8_t ITDS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *bdu)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}
	*bdu = (ITDS_state_t)ctrl2.blockDataUpdate;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable auto increment mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] autoIncr Auto increment mode state
 * @retval Error code
 */
int8_t ITDS_enableAutoIncrement(WE_sensorInterface_t *sensorInterface, ITDS_state_t autoIncr)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.autoAddIncr = autoIncr;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the auto increment mode state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] autoIncr The returned auto increment mode state
 * @retval Error code
 */
int8_t ITDS_isAutoIncrementEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *autoIncr)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*autoIncr = (ITDS_state_t)ctrl2.autoAddIncr;

	return WE_SUCCESS;
}

/**
 * @brief Disable the I2C interface
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] i2cDisable I2C interface disable state (0: I2C enabled, 1: I2C disabled)
 * @retval Error code
 */
int8_t ITDS_disableI2CInterface(WE_sensorInterface_t *sensorInterface, ITDS_state_t i2cDisable)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.i2cDisable = i2cDisable;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the I2C interface disable state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] i2cDisabled The returned I2C interface disable state (0: I2C enabled, 1: I2C disabled)
 * @retval Error code
 */
int8_t ITDS_isI2CInterfaceDisabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *i2cDisabled)
{
	ITDS_ctrl2_t ctrl2;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*i2cDisabled = (ITDS_state_t)ctrl2.i2cDisable;

	return WE_SUCCESS;
}

/* CTRL REG 3 */

/**
 * @brief Set self test mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] selfTest Self test mode
 * @retval Error code
 */
int8_t ITDS_setSelfTestMode(WE_sensorInterface_t *sensorInterface, ITDS_selfTestConfig_t selfTest)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.selfTestMode = selfTest;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the self test mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] selfTest The returned self test mode
 * @retval Error code
 */
int8_t ITDS_getSelfTestMode(WE_sensorInterface_t *sensorInterface, ITDS_selfTestConfig_t *selfTest)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*selfTest = (ITDS_selfTestConfig_t)ctrl3.selfTestMode;

	return WE_SUCCESS;
}

/**
 * @brief Set the interrupt pin type [push-pull/open-drain]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] pinType Interrupt pin type
 * @retval Error code
 */
int8_t ITDS_setInterruptPinType(WE_sensorInterface_t *sensorInterface,
				ITDS_interruptPinConfig_t pinType)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.intPinConf = pinType;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the interrupt pin type [push-pull/open-drain]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pinType The returned interrupt pin type.
 * @retval Error code
 */
int8_t ITDS_getInterruptPinType(WE_sensorInterface_t *sensorInterface,
				ITDS_interruptPinConfig_t *pinType)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*pinType = (ITDS_interruptPinConfig_t)ctrl3.intPinConf;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable latched interrupts
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] lir Latched interrupts state
 * @retval Error code
 */
int8_t ITDS_enableLatchedInterrupt(WE_sensorInterface_t *sensorInterface, ITDS_state_t lir)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.enLatchedInterrupt = lir;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the latched interrupts state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lir The returned latched interrupts state.
 * @retval Error code
 */
int8_t ITDS_isLatchedInterruptEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *lir)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*lir = (ITDS_state_t)ctrl3.enLatchedInterrupt;

	return WE_SUCCESS;
}

/**
 * @brief Set the interrupt active level [active high/active low]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] level Interrupt active level
 * @retval Error code
 */
int8_t ITDS_setInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    ITDS_interruptActiveLevel_t level)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.intActiveLevel = level;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the interrupt active level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] level The returned interrupt active level
 * @retval Error code
 */
int8_t ITDS_getInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    ITDS_interruptActiveLevel_t *level)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*level = (ITDS_interruptActiveLevel_t)ctrl3.intActiveLevel;

	return WE_SUCCESS;
}

/**
 * @brief Request single data conversion
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] start Set to true to trigger single data conversion.
 * @retval Error code
 */
int8_t ITDS_startSingleDataConversion(WE_sensorInterface_t *sensorInterface, ITDS_state_t start)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.startSingleDataConv = start;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Returns true if single data conversion has been requested.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] start Is set to true if single data conversion has been requested.
 * @retval Error code
 */
int8_t ITDS_isSingleDataConversionStarted(WE_sensorInterface_t *sensorInterface,
					  ITDS_state_t *start)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*start = (ITDS_state_t)ctrl3.startSingleDataConv;

	return WE_SUCCESS;
}

/**
 * @brief Set the single data conversion (on-demand) trigger.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] conversionTrigger Single data conversion (on-demand) trigger
 * @retval Error code
 */
int8_t ITDS_setSingleDataConversionTrigger(WE_sensorInterface_t *sensorInterface,
					   ITDS_singleDataConversionTrigger_t conversionTrigger)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.singleConvTrigger = conversionTrigger;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the single data conversion (on-demand) trigger
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] conversionTrigger The returned single data conversion (on-demand) trigger.
 * @retval Error code
 */
int8_t ITDS_getSingleDataConversionTrigger(WE_sensorInterface_t *sensorInterface,
					   ITDS_singleDataConversionTrigger_t *conversionTrigger)
{
	ITDS_ctrl3_t ctrl3;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*conversionTrigger = (ITDS_singleDataConversionTrigger_t)ctrl3.singleConvTrigger;

	return WE_SUCCESS;
}

/* CTRL REG 4  */

/**
 * @brief Enable/disable the 6D orientation changed interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int06D The 6D orientation changed interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enable6DOnINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int06D)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.sixDINT0 = int06D;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the 6D interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int06D The returned 6D interrupt enable state
 * @retval Error code
 */
int8_t ITDS_is6DOnINT0Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int06D)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int06D = (ITDS_state_t)ctrl4.sixDINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the single-tap interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0SingleTap Single-tap interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableSingleTapINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0SingleTap)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.singleTapINT0 = int0SingleTap;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the single-tap interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0SingleTap The returned single-tap interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isSingleTapINT0Enabled(WE_sensorInterface_t *sensorInterface,
				   ITDS_state_t *int0SingleTap)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0SingleTap = (ITDS_state_t)ctrl4.singleTapINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the wake-up interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0WakeUp Wake-up interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableWakeUpOnINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0WakeUp)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.wakeUpINT0 = int0WakeUp;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the wake-up interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0WakeUp The returned wake-up interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isWakeUpOnINT0Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int0WakeUp)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0WakeUp = (ITDS_state_t)ctrl4.wakeUpINT0;
	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the free-fall interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0FreeFall Free-fall interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFreeFallINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0FreeFall)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.freeFallINT0 = int0FreeFall;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the free-fall interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0FreeFall The returned free-fall enable state
 * @retval Error code
 */
int8_t ITDS_isFreeFallINT0Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int0FreeFall)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0FreeFall = (ITDS_state_t)ctrl4.freeFallINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the double-tap interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0DoubleTap The double-tap interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableDoubleTapINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0DoubleTap)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.doubleTapINT0 = int0DoubleTap;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the double-tap interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0DoubleTap The returned double-tap interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isDoubleTapINT0Enabled(WE_sensorInterface_t *sensorInterface,
				   ITDS_state_t *int0DoubleTap)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0DoubleTap = (ITDS_state_t)ctrl4.doubleTapINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO full interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0FifoFull FIFO full interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFifoFullINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0FifoFull)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.fifoFullINT0 = int0FifoFull;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the FIFO full interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0FifoFull The returned FIFO full interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isFifoFullINT0Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int0FifoFull)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0FifoFull = (ITDS_state_t)ctrl4.fifoFullINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO threshold interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0FifoThreshold FIFO threshold interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFifoThresholdINT0(WE_sensorInterface_t *sensorInterface,
				    ITDS_state_t int0FifoThreshold)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.fifoThresholdINT0 = int0FifoThreshold;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the FIFO threshold interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0FifoThreshold The returned FIFO threshold interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isFifoThresholdINT0Enabled(WE_sensorInterface_t *sensorInterface,
				       ITDS_state_t *int0FifoThreshold)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0FifoThreshold = (ITDS_state_t)ctrl4.fifoThresholdINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the data-ready interrupt on INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int0DataReady Data-ready interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableDataReadyINT0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int0DataReady)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	ctrl4.dataReadyINT0 = int0DataReady;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4);
}

/**
 * @brief Check if the data-ready interrupt on INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int0DataReady The returned data-ready interrupt enable State
 * @retval Error code
 */
int8_t ITDS_isDataReadyINT0Enabled(WE_sensorInterface_t *sensorInterface,
				   ITDS_state_t *int0DataReady)
{
	ITDS_ctrl4_t ctrl4;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_4_REG, 1, (uint8_t *)&ctrl4)) {
		return WE_FAIL;
	}

	*int0DataReady = (ITDS_state_t)ctrl4.dataReadyINT0;

	return WE_SUCCESS;
}

/* CTRL REG 5 */

/**
 * @brief Enable/disable the sleep status interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1SleepStatus Sleep status interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableSleepStatusINT1(WE_sensorInterface_t *sensorInterface,
				  ITDS_state_t int1SleepStatus)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.sleepStateINT1 = int1SleepStatus;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the sleep status interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1SleepStatus The returned sleep status interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isSleepStatusINT1Enabled(WE_sensorInterface_t *sensorInterface,
				     ITDS_state_t *int1SleepStatus)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1SleepStatus = (ITDS_state_t)ctrl5.sleepStateINT1;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the sleep status change interrupt on INT_1
 * (signaling transition from active to inactive and vice versa)
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1SleepChange Sleep status change signal on INT_1
 * @retval Error code
 */
int8_t ITDS_enableSleepStatusChangeINT1(WE_sensorInterface_t *sensorInterface,
					ITDS_state_t int1SleepChange)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.sleepStatusChangeINT1 = int1SleepChange;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the sleep status change interrupt on INT_1 is enabled
 * (signaling transition from active to inactive and vice versa)
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1SleepChange The returned sleep status change interrupt state
 * @retval Error code
 */
int8_t ITDS_isSleepStatusChangeINT1Enabled(WE_sensorInterface_t *sensorInterface,
					   ITDS_state_t *int1SleepChange)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1SleepChange = (ITDS_state_t)ctrl5.sleepStatusChangeINT1;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the boot interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1Boot Boot interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableBootStatusINT1(WE_sensorInterface_t *sensorInterface, ITDS_state_t int1Boot)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.bootStatusINT1 = int1Boot;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the boot interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1Boot The returned boot interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isBootStatusINT1Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int1Boot)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1Boot = (ITDS_state_t)ctrl5.bootStatusINT1;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the temperature data-ready interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1TempDataReady The temperature data-ready interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableTempDataReadyINT1(WE_sensorInterface_t *sensorInterface,
				    ITDS_state_t int1TempDataReady)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.tempDataReadyINT1 = int1TempDataReady;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the temperature data-ready interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1TempDataReady The returned temperature data-ready interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isTempDataReadyINT1Enabled(WE_sensorInterface_t *sensorInterface,
				       ITDS_state_t *int1TempDataReady)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1TempDataReady = (ITDS_state_t)ctrl5.tempDataReadyINT1;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO overrun interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1FifoOverrun FIFO overrun interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFifoOverrunIntINT1(WE_sensorInterface_t *sensorInterface,
				     ITDS_state_t int1FifoOverrun)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.fifoOverrunINT1 = int1FifoOverrun;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the FIFO overrun interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1FifoOverrun The returned FIFO overrun interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isFifoOverrunIntINT1Enabled(WE_sensorInterface_t *sensorInterface,
					ITDS_state_t *int1FifoOverrun)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1FifoOverrun = (ITDS_state_t)ctrl5.fifoOverrunINT1;
	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO full interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1FifoFull FIFO full interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFifoFullINT1(WE_sensorInterface_t *sensorInterface, ITDS_state_t int1FifoFull)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.fifoFullINT1 = int1FifoFull;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the FIFO full interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1FifoFull The returned FIFO full interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isFifoFullINT1Enabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int1FifoFull)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1FifoFull = (ITDS_state_t)ctrl5.fifoFullINT1;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO threshold interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1FifoThresholdInt FIFO threshold interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableFifoThresholdINT1(WE_sensorInterface_t *sensorInterface,
				    ITDS_state_t int1FifoThresholdInt)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.fifoThresholdINT1 = int1FifoThresholdInt;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the FIFO threshold interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1FifoThresholdInt The returned FIFO threshold interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isFifoThresholdINT1Enabled(WE_sensorInterface_t *sensorInterface,
				       ITDS_state_t *int1FifoThresholdInt)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1FifoThresholdInt = (ITDS_state_t)ctrl5.fifoThresholdINT1;
	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the data-ready interrupt on INT_1
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1DataReadyInt Data-ready interrupt enable state
 * @retval Error code
 */
int8_t ITDS_enableDataReadyINT1(WE_sensorInterface_t *sensorInterface,
				ITDS_state_t int1DataReadyInt)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	ctrl5.dataReadyINT1 = int1DataReadyInt;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5);
}

/**
 * @brief Check if the data-ready interrupt on INT_1 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1DataReadyInt The returned data-ready interrupt enable state
 * @retval Error code
 */
int8_t ITDS_isDataReadyINT1Enabled(WE_sensorInterface_t *sensorInterface,
				   ITDS_state_t *int1DataReadyInt)
{
	ITDS_ctrl5_t ctrl5;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_5_REG, 1, (uint8_t *)&ctrl5)) {
		return WE_FAIL;
	}

	*int1DataReadyInt = (ITDS_state_t)ctrl5.dataReadyINT1;

	return WE_SUCCESS;
}

/* CTRL REG 6 */

/**
 * @brief Set the filtering cut-off
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] filteringCutoff Filtering cut-off
 * @retval Error code
 */
int8_t ITDS_setFilteringCutoff(WE_sensorInterface_t *sensorInterface,
			       ITDS_bandwidth_t filteringCutoff)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	ctrl6.filterBandwidth = filteringCutoff;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6);
}

/**
 * @brief Read filtering cut-off
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] filteringCutoff The returned filtering cut-off
 * @retval Error code
 */
int8_t ITDS_getFilteringCutoff(WE_sensorInterface_t *sensorInterface,
			       ITDS_bandwidth_t *filteringCutoff)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	*filteringCutoff = (ITDS_bandwidth_t)ctrl6.filterBandwidth;

	return WE_SUCCESS;
}

/**
 * @brief Set the full scale
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fullScale Full scale
 * @retval Error code
 */
int8_t ITDS_setFullScale(WE_sensorInterface_t *sensorInterface, ITDS_fullScale_t fullScale)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	ctrl6.fullScale = fullScale;

	int8_t errCode = ITDS_WriteReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6);

	/* Store current full scale value to allow convenient conversion of sensor readings */
	if (WE_SUCCESS == errCode) {
		currentFullScale = fullScale;
	}

	return errCode;
}

/**
 * @brief Read the full scale
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fullScale The returned full scale.
 * @retval Error code
 */
int8_t ITDS_getFullScale(WE_sensorInterface_t *sensorInterface, ITDS_fullScale_t *fullScale)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	*fullScale = (ITDS_fullScale_t)ctrl6.fullScale;

	/* Store current full scale value to allow convenient conversion of sensor readings */
	currentFullScale = *fullScale;

	return WE_SUCCESS;
}

/**
 * @brief Set the filter type [low-pass filter/high-pass filter]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] filterType Filter Type
 * @retval Error code
 */
int8_t ITDS_setFilterPath(WE_sensorInterface_t *sensorInterface, ITDS_filterType_t filterType)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	ctrl6.filterPath = filterType;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6);
}

/**
 * @brief Read the filter type
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] filterType The returned filter type
 * @retval Error code
 */
int8_t ITDS_getFilterPath(WE_sensorInterface_t *sensorInterface, ITDS_filterType_t *filterType)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	*filterType = (ITDS_filterType_t)ctrl6.filterPath;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the low noise configuration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] lowNoise Low noise configuration
 * @retval Error code
 */
int8_t ITDS_enableLowNoise(WE_sensorInterface_t *sensorInterface, ITDS_state_t lowNoise)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	ctrl6.enLowNoise = lowNoise;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6);
}

/**
 * @brief Read the low noise configuration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lowNoise The returned low noise configuration
 * @retval Error code
 */
int8_t ITDS_isLowNoiseEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *lowNoise)
{
	ITDS_ctrl6_t ctrl6;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_6_REG, 1, (uint8_t *)&ctrl6)) {
		return WE_FAIL;
	}

	*lowNoise = (ITDS_state_t)ctrl6.enLowNoise;

	return WE_SUCCESS;
}

/* STATUS REG 0x27 */
/* Note: The status register is partially duplicated to the STATUS_DETECT register. */

/**
 * @brief Get overall sensor event status
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] status The returned sensor event data
 * @retval Error code
 */
int8_t ITDS_getStatusRegister(WE_sensorInterface_t *sensorInterface, ITDS_status_t *status)
{
	return ITDS_ReadReg(sensorInterface, ITDS_STATUS_REG, 1, (uint8_t *)status);
}

/**
 * @brief Check if new acceleration samples are available.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] dataReady The returned data-ready state.
 * @retval Error code
 */
int8_t ITDS_isAccelerationDataReady(WE_sensorInterface_t *sensorInterface, ITDS_state_t *dataReady)
{
	ITDS_status_t statusRegister;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_STATUS_REG, 1, (uint8_t *)&statusRegister)) {
		return WE_FAIL;
	}

	*dataReady = (ITDS_state_t)statusRegister.dataReady;

	return WE_SUCCESS;
}

/**
 * @brief Read the single-tap event state [not detected/detected]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] singleTap The returned single-tap event state.
 * @retval Error code
 */
int8_t ITDS_getSingleTapState(WE_sensorInterface_t *sensorInterface, ITDS_state_t *singleTap)
{
	ITDS_status_t statusRegister;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_STATUS_REG, 1, (uint8_t *)&statusRegister)) {
		return WE_FAIL;
	}

	*singleTap = (ITDS_state_t)statusRegister.singleTap;

	return WE_SUCCESS;
}

/**
 * @brief Read the double-tap event state [not detected/detected]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] doubleTap The returned double-tap event state
 * @retval Error code
 */
int8_t ITDS_getDoubleTapState(WE_sensorInterface_t *sensorInterface, ITDS_state_t *doubleTap)
{
	ITDS_status_t statusRegister;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_STATUS_REG, 1, (uint8_t *)&statusRegister)) {
		return WE_FAIL;
	}

	*doubleTap = (ITDS_state_t)statusRegister.doubleTap;

	return WE_SUCCESS;
}

/**
 * @brief Read the sleep state [not sleeping/sleeping]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] sleepState The returned sleep state.
 * @retval Error code
 */
int8_t ITDS_getSleepState(WE_sensorInterface_t *sensorInterface, ITDS_state_t *sleepState)
{
	ITDS_status_t statusRegister;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_STATUS_REG, 1, (uint8_t *)&statusRegister)) {
		return WE_FAIL;
	}

	*sleepState = (ITDS_state_t)statusRegister.sleepState;

	return WE_SUCCESS;
}

/* X_OUT_L_REG */

/**
 * @brief Read the raw X-axis acceleration sensor output
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] xRawAcc The returned raw X-axis acceleration
 * @retval Error code
 */
int8_t ITDS_getRawAccelerationX(WE_sensorInterface_t *sensorInterface, int16_t *xRawAcc)
{
	int16_t xAxisAccelerationRaw = 0;
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_X_OUT_L_REG, 2, tmp)) {
		return WE_FAIL;
	}

	xAxisAccelerationRaw = (int16_t)(tmp[1] << 8);
	xAxisAccelerationRaw |= (int16_t)tmp[0];

	*xRawAcc = xAxisAccelerationRaw;
	return WE_SUCCESS;
}

/* Y_OUT_L_REG */

/**
 * @brief Read the raw Y-axis acceleration sensor output
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] yRawAcc The returned raw Y-axis acceleration
 * @retval Error code
 */
int8_t ITDS_getRawAccelerationY(WE_sensorInterface_t *sensorInterface, int16_t *yRawAcc)
{
	int16_t yAxisAccelerationRaw = 0;
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_Y_OUT_L_REG, 2, tmp)) {
		return WE_FAIL;
	}

	yAxisAccelerationRaw = (int16_t)(tmp[1] << 8);
	yAxisAccelerationRaw |= (int16_t)tmp[0];

	*yRawAcc = yAxisAccelerationRaw;
	return WE_SUCCESS;
}

/* Z_OUT_L_REG */

/**
 * @brief Read the raw Z-axis acceleration sensor output
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] zRawAcc The returned raw Z-axis acceleration
 * @retval Error code
 */
int8_t ITDS_getRawAccelerationZ(WE_sensorInterface_t *sensorInterface, int16_t *zRawAcc)
{
	int16_t zAxisAccelerationRaw = 0;
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_Z_OUT_L_REG, 2, tmp)) {
		return WE_FAIL;
	}

	zAxisAccelerationRaw = (int16_t)(tmp[1] << 8);
	zAxisAccelerationRaw |= (int16_t)tmp[0];

	*zRawAcc = zAxisAccelerationRaw;
	return WE_SUCCESS;
}

/**
 * @brief Returns one or more acceleration samples (raw) for all axes.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to be read (1-32)
 * @param[out] xRawAcc X-axis raw acceleration
 * @param[out] yRawAcc Y-axis raw acceleration
 * @param[out] zRawAcc Z-axis raw acceleration
 * @retval Error code
 */
int8_t ITDS_getRawAccelerations(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				int16_t *xRawAcc, int16_t *yRawAcc, int16_t *zRawAcc)
{
	/* Max. buffer size is 192 (32 slot, 16 bit values) */
	uint8_t buffer[192];

	if (numSamples > 32) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_X_OUT_L_REG, 3 * 2 * numSamples, buffer)) {
		return WE_FAIL;
	}

	uint16_t sample;
	uint8_t *bufferPtr = buffer;
	for (uint8_t i = 0; i < numSamples; i++) {
		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		xRawAcc[i] = sample;

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		yRawAcc[i] = sample;

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		zRawAcc[i] = sample;
	}

	return WE_SUCCESS;
}

/**
 * @brief Reads the X axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] xAcc X axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationX_float(WE_sensorInterface_t *sensorInterface, float *xAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationX(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*xAcc = ITDS_convertAcceleration_float(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Reads the Y axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] yAcc Y axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationY_float(WE_sensorInterface_t *sensorInterface, float *yAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationY(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*yAcc = ITDS_convertAcceleration_float(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Reads the Z axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] zAcc Z axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationZ_float(WE_sensorInterface_t *sensorInterface, float *zAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationZ(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*zAcc = ITDS_convertAcceleration_float(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Returns one or more acceleration samples in [mg] for all axes.
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to be read (1-32)
 * @param[out] xAcc X-axis acceleration in [mg]
 * @param[out] yAcc Y-axis acceleration in [mg]
 * @param[out] zAcc Z-axis acceleration in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerations_float(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				   float *xAcc, float *yAcc, float *zAcc)
{
	/* Max. buffer size is 192 (32 slot, 16 bit values) */
	uint8_t buffer[192];

	if (numSamples > 32) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_X_OUT_L_REG, 3 * 2 * numSamples, buffer)) {
		return WE_FAIL;
	}

	uint16_t sample;
	uint8_t *bufferPtr = buffer;
	for (uint8_t i = 0; i < numSamples; i++) {
		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		xAcc[i] = ITDS_convertAcceleration_float((int16_t)sample, currentFullScale);

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		yAcc[i] = ITDS_convertAcceleration_float((int16_t)sample, currentFullScale);

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		zAcc[i] = ITDS_convertAcceleration_float((int16_t)sample, currentFullScale);
	}

	return WE_SUCCESS;
}

/**
 * @brief Converts the supplied raw acceleration into [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @param[in] fullScale Accelerometer full scale
 * @retval The converted acceleration in [mg]
 */
float ITDS_convertAcceleration_float(int16_t acc, ITDS_fullScale_t fullScale)
{
	switch (fullScale) {
	case ITDS_twoG:
		return ITDS_convertAccelerationFs2g_float(acc);

	case ITDS_fourG:
		return ITDS_convertAccelerationFs4g_float(acc);

	case ITDS_eightG:
		return ITDS_convertAccelerationFs8g_float(acc);

	case ITDS_sixteenG:
		return ITDS_convertAccelerationFs16g_float(acc);

	default:
		break;
	}

	return 0;
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_twoG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
float ITDS_convertAccelerationFs2g_float(int16_t acc)
{
	return ((float)acc) * 0.061f;
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_fourG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
float ITDS_convertAccelerationFs4g_float(int16_t acc)
{
	return ((float)acc) * 0.122f;
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_eightG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
float ITDS_convertAccelerationFs8g_float(int16_t acc)
{
	return ((float)acc) * 0.244f;
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_sixteenG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
float ITDS_convertAccelerationFs16g_float(int16_t acc)
{
	return ((float)acc) * 0.488f;
}

/**
 * @brief Reads the X axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] xAcc X axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationX_int(WE_sensorInterface_t *sensorInterface, int16_t *xAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationX(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*xAcc = ITDS_convertAcceleration_int(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Reads the Y axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] yAcc Y axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationY_int(WE_sensorInterface_t *sensorInterface, int16_t *yAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationY(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*yAcc = ITDS_convertAcceleration_int(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Reads the Z axis acceleration in [mg].
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] zAcc Z axis acceleration value in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerationZ_int(WE_sensorInterface_t *sensorInterface, int16_t *zAcc)
{
	int16_t rawAcc;
	if (WE_FAIL == ITDS_getRawAccelerationZ(sensorInterface, &rawAcc)) {
		return WE_FAIL;
	}
	*zAcc = ITDS_convertAcceleration_int(rawAcc, currentFullScale);
	return WE_SUCCESS;
}

/**
 * @brief Returns one or more acceleration samples in [mg] for all axes.
 *
 * Note that this functions relies on the current full scale value.
 * Make sure that the current full scale value is known by calling
 * ITDS_setFullScale() or ITDS_getFullScale() at least once prior to
 * calling this function.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to be read (1-32)
 * @param[out] xAcc X-axis acceleration in [mg]
 * @param[out] yAcc Y-axis acceleration in [mg]
 * @param[out] zAcc Z-axis acceleration in [mg]
 * @retval Error code
 */
int8_t ITDS_getAccelerations_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				 int16_t *xAcc, int16_t *yAcc, int16_t *zAcc)
{
	/* Max. buffer size is 192 (32 slot, 16 bit values) */
	uint8_t buffer[192];

	if (numSamples > 32) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_X_OUT_L_REG, 3 * 2 * numSamples, buffer)) {
		return WE_FAIL;
	}

	uint16_t sample;
	uint8_t *bufferPtr = buffer;
	for (uint8_t i = 0; i < numSamples; i++) {
		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		xAcc[i] = ITDS_convertAcceleration_int((int16_t)sample, currentFullScale);

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		yAcc[i] = ITDS_convertAcceleration_int((int16_t)sample, currentFullScale);

		sample = ((int16_t)*bufferPtr);
		bufferPtr++;
		sample |= (int16_t)((*bufferPtr) << 8);
		bufferPtr++;
		zAcc[i] = ITDS_convertAcceleration_int((int16_t)sample, currentFullScale);
	}

	return WE_SUCCESS;
}

/**
 * @brief Converts the supplied raw acceleration into [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @param[in] fullScale Accelerometer full scale
 * @retval The converted acceleration in [mg]
 */
int16_t ITDS_convertAcceleration_int(int16_t acc, ITDS_fullScale_t fullScale)
{
	switch (fullScale) {
	case ITDS_twoG:
		return ITDS_convertAccelerationFs2g_int(acc);

	case ITDS_sixteenG:
		return ITDS_convertAccelerationFs16g_int(acc);

	case ITDS_fourG:
		return ITDS_convertAccelerationFs4g_int(acc);

	case ITDS_eightG:
		return ITDS_convertAccelerationFs8g_int(acc);

	default:
		return 0;
	}
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_twoG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
int16_t ITDS_convertAccelerationFs2g_int(int16_t acc)
{
	return (int16_t)((((int32_t)acc) * 61) / 1000);
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_fourG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
int16_t ITDS_convertAccelerationFs4g_int(int16_t acc)
{
	return (int16_t)((((int32_t)acc) * 122) / 1000);
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_eightG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
int16_t ITDS_convertAccelerationFs8g_int(int16_t acc)
{
	return (int16_t)((((int32_t)acc) * 244) / 1000);
}

/**
 * @brief Converts the supplied raw acceleration sampled using
 * ITDS_sixteenG to [mg]
 * @param[in] acc Raw acceleration value (accelerometer output)
 * @retval The converted acceleration in [mg]
 */
int16_t ITDS_convertAccelerationFs16g_int(int16_t acc)
{
	return (int16_t)((((int32_t)acc) * 488) / 1000);
}

/* ITDS_T_OUT_REG */

/**
 * @brief Read the 8 bit temperature
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] temp8bit The returned temperature
 * @retval Error code
 */

int8_t ITDS_getTemperature8bit(WE_sensorInterface_t *sensorInterface, uint8_t *temp8bit)
{
	uint8_t temperatureValue8bit;
	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_T_OUT_REG, 1, &temperatureValue8bit)) {
		return WE_FAIL;
	}

	*temp8bit = temperatureValue8bit;
	return WE_SUCCESS;
}

/**
 * @brief Read the 12 bit temperature
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] temp12bit The returned temperature
 * @retval Error code
 */
int8_t ITDS_getRawTemperature12bit(WE_sensorInterface_t *sensorInterface, int16_t *temp12bit)
{
	uint8_t temp[2] = { 0 };

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_T_OUT_L_REG, 2, temp)) {
		return WE_FAIL;
	}

	*temp12bit = (int16_t)(temp[1] << 8);
	*temp12bit |= (int16_t)temp[0];

	*temp12bit = (*temp12bit) >> 4;

	return WE_SUCCESS;
}

/**
 * @brief Read the 12 bit temperature in °C
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tempDegC The returned temperature
 * @retval Error code
 */
int8_t ITDS_getTemperature12bit(WE_sensorInterface_t *sensorInterface, float *tempDegC)
{
	int16_t rawTemp = 0;
	if (WE_SUCCESS == ITDS_getRawTemperature12bit(sensorInterface, &rawTemp)) {
		*tempDegC = (((float)rawTemp) / 16.0) + 25.0;
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/* FIFO_CTRL (0x2E) */

/**
 * @brief Set the FIFO threshold of the sensor
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fifoThreshold FIFO threshold (value between 0 and 31)
 * @retval Error code
 */
int8_t ITDS_setFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t fifoThreshold)
{
	ITDS_fifoCtrl_t fifoCtrl;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl)) {
		return WE_FAIL;
	}

	fifoCtrl.fifoThresholdLevel = fifoThreshold;

	return ITDS_WriteReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl);
}

/**
 * @brief Read the FIFO threshold
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoThreshold The returned FIFO threshold (value between 0 and 31)
 * @retval Error code
 */
int8_t ITDS_getFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t *fifoThreshold)
{
	ITDS_fifoCtrl_t fifoCtrl;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl)) {
		return WE_FAIL;
	}

	*fifoThreshold = fifoCtrl.fifoThresholdLevel;

	return WE_SUCCESS;
}

/**
 * @brief Set the FIFO mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fifoMode FIFO mode
 * @retval Error code
 */
int8_t ITDS_setFifoMode(WE_sensorInterface_t *sensorInterface, ITDS_FifoMode_t fifoMode)
{
	ITDS_fifoCtrl_t fifoCtrl;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl)) {
		return WE_FAIL;
	}

	fifoCtrl.fifoMode = fifoMode;

	return ITDS_WriteReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl);
}

/**
 * @brief Read the FIFO mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoMode The returned FIFO mode
 * @retval Error code
 */
int8_t ITDS_getFifoMode(WE_sensorInterface_t *sensorInterface, ITDS_FifoMode_t *fifoMode)
{
	ITDS_fifoCtrl_t fifoCtrl;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrl)) {
		return WE_FAIL;
	}

	*fifoMode = (ITDS_FifoMode_t)fifoCtrl.fifoMode;

	return WE_SUCCESS;
}

/* FIFO_SAMPLES (0x2F) */

/**
 * @brief Read the FIFO samples status
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoSamplesStatus The returned FIFO samples status
 * @retval Error code
 */
int8_t ITDS_getFifoSamplesRegister(WE_sensorInterface_t *sensorInterface,
				   ITDS_fifoSamples_t *fifoSamplesStatus)
{
	return ITDS_ReadReg(sensorInterface, ITDS_FIFO_SAMPLES_REG, 1,
			    (uint8_t *)fifoSamplesStatus);
}

/**
 * @brief Read the FIFO threshold state [FIFO filling is lower than threshold level /
 * FIFO filling is equal to or higher than the threshold level]
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoThr The returned FIFO threshold state
 * @retval Error code
 */
int8_t ITDS_isFifoThresholdReached(WE_sensorInterface_t *sensorInterface, ITDS_state_t *fifoThr)
{
	ITDS_fifoSamples_t fifoSamples;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_FIFO_SAMPLES_REG, 1, (uint8_t *)&fifoSamples)) {
		return WE_FAIL;
	}

	*fifoThr = (ITDS_state_t)fifoSamples.fifoThresholdState;

	return WE_SUCCESS;
}

/**
 * @brief Read the FIFO overrun state [FIFO is not completely filled /
 * FIFO completely filled and at least one sample has been overwritten]
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoOverrun The returned FIFO overrun state.
 * @retval Error code
 */
int8_t ITDS_getFifoOverrunState(WE_sensorInterface_t *sensorInterface, ITDS_state_t *fifoOverrun)
{
	ITDS_fifoSamples_t fifoSamples;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_FIFO_SAMPLES_REG, 1, (uint8_t *)&fifoSamples)) {
		return WE_FAIL;
	}

	*fifoOverrun = (ITDS_state_t)fifoSamples.fifoOverrunState;

	return WE_SUCCESS;
}

/**
 * @brief Read the FIFO fill level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoFill The returned FIFO fill level (0-32)
 * @retval Error code
 */
int8_t ITDS_getFifoFillLevel(WE_sensorInterface_t *sensorInterface, uint8_t *fifoFill)
{
	ITDS_fifoSamples_t fifoSamples;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_FIFO_SAMPLES_REG, 1, (uint8_t *)&fifoSamples)) {
		return WE_FAIL;
	}

	*fifoFill = fifoSamples.fifoFillLevel;

	return WE_SUCCESS;
}

/* TAP_X_TH (0x30) */

/**
 * @brief Enable/disable 4D orientation detection
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] detection4D The 4D orientation detection enable state
 * @retval Error code
 */
int8_t ITDS_enable4DDetection(WE_sensorInterface_t *sensorInterface, ITDS_state_t detection4D)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	tapXThresh.fourDDetectionEnabled = detection4D;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh);
}

/**
 * @brief Check if 4D orientation detection is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] detection4D The returned 4D orientation detection enable state.
 * @retval Error code
 */
int8_t ITDS_is4DDetectionEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *detection4D)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	*detection4D = (ITDS_state_t)tapXThresh.fourDDetectionEnabled;

	return WE_SUCCESS;
}

/**
 * @brief Set the tap threshold for axis X
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapThresholdX Tap threshold for axis X (5 bits)
 * @retval Error code
 */
int8_t ITDS_setTapThresholdX(WE_sensorInterface_t *sensorInterface, uint8_t tapThresholdX)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	tapXThresh.xAxisTapThreshold = tapThresholdX;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh);
}

/**
 * @brief Read the tap threshold for axis X
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapThresholdX The returned tap threshold for axis X
 * @retval Error code
 */
int8_t ITDS_getTapThresholdX(WE_sensorInterface_t *sensorInterface, uint8_t *tapThresholdX)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	*tapThresholdX = tapXThresh.xAxisTapThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Set the 6D orientation detection threshold (degrees)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] threshold6D 6D orientation detection threshold
 * @retval Error code
 */
int8_t ITDS_set6DThreshold(WE_sensorInterface_t *sensorInterface,
			   ITDS_thresholdDegree_t threshold6D)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	tapXThresh.sixDThreshold = threshold6D;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh);
}

/**
 * @brief Read the 6D orientation detection threshold (degrees)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] threshold6D The returned 6D orientation detection threshold
 * @retval Error code
 */
int8_t ITDS_get6DThreshold(WE_sensorInterface_t *sensorInterface,
			   ITDS_thresholdDegree_t *threshold6D)
{
	ITDS_tapXThreshold_t tapXThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_X_TH_REG, 1, (uint8_t *)&tapXThresh)) {
		return WE_FAIL;
	}

	*threshold6D = (ITDS_thresholdDegree_t)tapXThresh.sixDThreshold;

	return WE_SUCCESS;
}

/* TAP_Y_TH (0x31) */

/**
 * @brief Set the tap threshold for axis Y
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapThresholdY Tap threshold for axis Y (5 bits)
 * @retval Error code
 */
int8_t ITDS_setTapThresholdY(WE_sensorInterface_t *sensorInterface, uint8_t tapThresholdY)
{
	ITDS_tapYThreshold_t tapYThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh)) {
		return WE_FAIL;
	}

	tapYThresh.yAxisTapThreshold = tapThresholdY;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh);
}

/**
 * @brief Read the tap threshold for axis Y
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapThresholdY The returned tap threshold for axis Y.
 * @retval Error code
 */
int8_t ITDS_getTapThresholdY(WE_sensorInterface_t *sensorInterface, uint8_t *tapThresholdY)
{
	ITDS_tapYThreshold_t tapYThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh)) {
		return WE_FAIL;
	}

	*tapThresholdY = tapYThresh.yAxisTapThreshold;
	return WE_SUCCESS;
}

/**
 * @brief Set the axis tap detection priority
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] priority Axis tap detection priority
 * @retval Error code
 */
int8_t ITDS_setTapAxisPriority(WE_sensorInterface_t *sensorInterface,
			       ITDS_tapAxisPriority_t priority)
{
	ITDS_tapYThreshold_t tapYThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh)) {
		return WE_FAIL;
	}

	tapYThresh.tapAxisPriority = priority;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh);
}

/**
 * @brief Read the axis tap detection priority
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] priority The returned axis tap detection priority
 * @retval Error code
 */
int8_t ITDS_getTapAxisPriority(WE_sensorInterface_t *sensorInterface,
			       ITDS_tapAxisPriority_t *priority)
{
	ITDS_tapYThreshold_t tapYThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Y_TH_REG, 1, (uint8_t *)&tapYThresh)) {
		return WE_FAIL;
	}

	*priority = (ITDS_tapAxisPriority_t)tapYThresh.tapAxisPriority;

	return WE_SUCCESS;
}

/* TAP_Z_TH (0x32) */

/**
 * @brief Set the tap threshold for axis Z
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapThresholdZ Tap threshold for axis Z (5 bits)
 * @retval Error code
 */
int8_t ITDS_setTapThresholdZ(WE_sensorInterface_t *sensorInterface, uint8_t tapThresholdZ)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	tapZThresh.zAxisTapThreshold = tapThresholdZ;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh);
}

/**
 * @brief Read the tap threshold for axis Z
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapThresholdZ The returned tap threshold for axis Z.
 * @retval Error code
 */
int8_t ITDS_getTapThresholdZ(WE_sensorInterface_t *sensorInterface, uint8_t *tapThresholdZ)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	*tapThresholdZ = tapZThresh.zAxisTapThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable tap recognition in X direction
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapX Tap X direction state
 * @retval Error code
 */
int8_t ITDS_enableTapX(WE_sensorInterface_t *sensorInterface, ITDS_state_t tapX)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	tapZThresh.enTapX = tapX;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh);
}

/**
 * @brief Check if detection of tap events in X direction is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapX The returned tap X direction state
 * @retval Error code
 */
int8_t ITDS_isTapXEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapX)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	*tapX = (ITDS_state_t)tapZThresh.enTapX;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable tap recognition in Y direction
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapY Tap Y direction state
 * @retval Error code
 */
int8_t ITDS_enableTapY(WE_sensorInterface_t *sensorInterface, ITDS_state_t tapY)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	tapZThresh.enTapY = tapY;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh);
}

/**
 * @brief Check if detection of tap events in Y direction is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapY The returned tap Y direction state
 * @retval Error code
 */
int8_t ITDS_isTapYEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapY)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	*tapY = (ITDS_state_t)tapZThresh.enTapY;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable tap recognition in Z direction
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] tapZ Tap Z direction state
 * @retval Error code
 */
int8_t ITDS_enableTapZ(WE_sensorInterface_t *sensorInterface, ITDS_state_t tapZ)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	tapZThresh.enTapZ = tapZ;

	return ITDS_WriteReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh);
}

/**
 * @brief Check if detection of tap events in Z direction is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapZ The returned tap Z direction state
 * @retval Error code
 */
int8_t ITDS_isTapZEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapZ)
{
	ITDS_tapZThreshold_t tapZThresh;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_TAP_Z_TH_REG, 1, (uint8_t *)&tapZThresh)) {
		return WE_FAIL;
	}

	*tapZ = (ITDS_state_t)tapZThresh.enTapZ;

	return WE_SUCCESS;
}

/* INT_DUR (0x33) */

/**
 * @brief Set the maximum duration time gap for double-tap recognition (LATENCY)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] latencyTime Latency value (4 bits)
 * @retval Error code
 */
int8_t ITDS_setTapLatencyTime(WE_sensorInterface_t *sensorInterface, uint8_t latencyTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	intDuration.latency = latencyTime;

	return ITDS_WriteReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration);
}

/**
 * @brief Read the maximum duration time gap for double-tap recognition (LATENCY)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] latencyTime The returned latency time
 * @retval Error code
 */
int8_t ITDS_getTapLatencyTime(WE_sensorInterface_t *sensorInterface, uint8_t *latencyTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	*latencyTime = intDuration.latency;

	return WE_SUCCESS;
}

/**
 * @brief Set the expected quiet time after a tap detection (QUIET)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] quietTime Quiet time value (2 bits)
 * @retval Error code
 */
int8_t ITDS_setTapQuietTime(WE_sensorInterface_t *sensorInterface, uint8_t quietTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	intDuration.quiet = quietTime;

	return ITDS_WriteReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration);
}

/**
 * @brief Read the expected quiet time after a tap detection (QUIET)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] quietTime The returned quiet time
 * @retval Error code
 */
int8_t ITDS_getTapQuietTime(WE_sensorInterface_t *sensorInterface, uint8_t *quietTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	*quietTime = intDuration.quiet;

	return WE_SUCCESS;
}

/**
 * @brief Set the maximum duration of over-threshold events (SHOCK)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] shockTime Shock time value (2 bits)
 * @retval Error code
 */
int8_t ITDS_setTapShockTime(WE_sensorInterface_t *sensorInterface, uint8_t shockTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	intDuration.shock = shockTime;

	return ITDS_WriteReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration);
}

/**
 * @brief Read the maximum duration of over-threshold events (SHOCK)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] shockTime The returned shock time.
 * @retval Error code
 */
int8_t ITDS_getTapShockTime(WE_sensorInterface_t *sensorInterface, uint8_t *shockTime)
{
	ITDS_intDuration_t intDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_INT_DUR_REG, 1, (uint8_t *)&intDuration)) {
		return WE_FAIL;
	}

	*shockTime = intDuration.shock;

	return WE_SUCCESS;
}

/* WAKE_UP_TH */

/**
 * @brief Enable/disable the single and double-tap event OR only single-tap event
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] doubleTap Tap event state [0: only single, 1: single and double-tap]
 * @retval Error code
 */
int8_t ITDS_enableDoubleTapEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t doubleTap)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	wakeUpThreshReg.enDoubleTapEvent = doubleTap;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg);
}

/**
 * @brief Check if double-tap events are enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] doubleTap The returned tap event state [0: only single, 1: single and double-tap]
 * @retval Error code
 */
int8_t ITDS_isDoubleTapEventEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *doubleTap)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	*doubleTap = (ITDS_state_t)wakeUpThreshReg.enDoubleTapEvent;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable inactivity (sleep) detection
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] inactivity Sleep detection enable state
 * @retval Error code
 */
int8_t ITDS_enableInactivityDetection(WE_sensorInterface_t *sensorInterface,
				      ITDS_state_t inactivity)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	wakeUpThreshReg.enInactivityEvent = inactivity;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg);
}

/**
 * @brief Check if inactivity (sleep) detection is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] inactivity The returned inactivity (sleep) detection enable state.
 * @retval Error code
 */
int8_t ITDS_isInactivityDetectionEnabled(WE_sensorInterface_t *sensorInterface,
					 ITDS_state_t *inactivity)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	*inactivity = (ITDS_state_t)wakeUpThreshReg.enInactivityEvent;

	return WE_SUCCESS;
}

/**
 * @brief Set wake-up threshold
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] wakeUpThresh Wake-up threshold (six bits)
 * @retval Error code
 */
int8_t ITDS_setWakeUpThreshold(WE_sensorInterface_t *sensorInterface, uint8_t wakeUpThresh)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	wakeUpThreshReg.wakeUpThreshold = wakeUpThresh;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg);
}

/**
 * @brief Read the wake-up threshold
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] wakeUpThresh The returned wake-up threshold.
 * @retval Error code
 */
int8_t ITDS_getWakeUpThreshold(WE_sensorInterface_t *sensorInterface, uint8_t *wakeUpThresh)
{
	ITDS_wakeUpThreshold_t wakeUpThreshReg;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_TH_REG, 1, (uint8_t *)&wakeUpThreshReg)) {
		return WE_FAIL;
	}

	*wakeUpThresh = wakeUpThreshReg.wakeUpThreshold;

	return WE_SUCCESS;
}

/* WAKE_UP_DUR */

/**
 * @brief Set free-fall duration MSB
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] freeFallDurationMsb Free-fall duration MSB
 * @retval Error code
 */
int8_t ITDS_setFreeFallDurationMSB(WE_sensorInterface_t *sensorInterface,
				   uint8_t freeFallDurationMsb)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	wakeUpDuration.freeFallDurationMSB = freeFallDurationMsb;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration);
}

/**
 * @brief Read the free-fall duration MSB
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] freeFallDurationMsb The returned free-fall duration MSB
 * @retval Error code
 */
int8_t ITDS_getFreeFallDurationMSB(WE_sensorInterface_t *sensorInterface,
				   uint8_t *freeFallDurationMsb)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	*freeFallDurationMsb = wakeUpDuration.freeFallDurationMSB;
	return WE_SUCCESS;
}

/**
 * @brief Enable/disable stationary detection
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] stationary Stationary detection enable state
 * @retval Error code
 */
int8_t ITDS_enableStationaryDetection(WE_sensorInterface_t *sensorInterface,
				      ITDS_state_t stationary)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	wakeUpDuration.enStationary = stationary;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration);
}

/**
 * @brief Check if stationary detection is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] stationary The returned stationary detection enable state
 * @retval Error code
 */
int8_t ITDS_isStationaryDetectionEnabled(WE_sensorInterface_t *sensorInterface,
					 ITDS_state_t *stationary)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	*stationary = (ITDS_state_t)wakeUpDuration.enStationary;

	return WE_SUCCESS;
}

/**
 * @brief Set wake-up duration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] duration Wake-up duration (two bits)
 * @retval Error code
 */
int8_t ITDS_setWakeUpDuration(WE_sensorInterface_t *sensorInterface, uint8_t duration)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	wakeUpDuration.wakeUpDuration = duration;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration);
}

/**
 * @brief Read the wake-up duration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] duration The returned wake-up duration (two bits)
 * @retval Error code
 */
int8_t ITDS_getWakeUpDuration(WE_sensorInterface_t *sensorInterface, uint8_t *duration)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	*duration = wakeUpDuration.wakeUpDuration;

	return WE_SUCCESS;
}

/**
 * @brief Set the sleep mode duration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] duration Sleep mode duration (4 bits)
 * @retval Error code
 */
int8_t ITDS_setSleepDuration(WE_sensorInterface_t *sensorInterface, uint8_t duration)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	wakeUpDuration.sleepDuration = duration;

	return ITDS_WriteReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration);
}

/**
 * @brief Read the sleep mode duration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] duration The returned sleep mode duration
 * @retval Error code
 */
int8_t ITDS_getSleepDuration(WE_sensorInterface_t *sensorInterface, uint8_t *duration)
{
	ITDS_wakeUpDuration_t wakeUpDuration;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_DUR_REG, 1, (uint8_t *)&wakeUpDuration)) {
		return WE_FAIL;
	}

	*duration = wakeUpDuration.sleepDuration;

	return WE_SUCCESS;
}

/* FREE_FALL */

/**
 * @brief Set the free-fall duration (both LSB and MSB).
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] freeFallDuration Free-fall duration (6 bits)
 * @retval Error code
 */
int8_t ITDS_setFreeFallDuration(WE_sensorInterface_t *sensorInterface, uint8_t freeFallDuration)
{
	/* Set first 5 bits as LSB, 6th bit as MSB */
	if (WE_FAIL == ITDS_setFreeFallDurationLSB(sensorInterface, freeFallDuration & 0x1F)) {
		return WE_FAIL;
	}
	return ITDS_setFreeFallDurationMSB(sensorInterface, (freeFallDuration >> 5) & 0x1);
}

/**
 * @brief Read the free-fall duration (both LSB and MSB).
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] freeFallDuration The returned free-fall duration (6 bits)
 * @retval Error code
 */
int8_t ITDS_getFreeFallDuration(WE_sensorInterface_t *sensorInterface, uint8_t *freeFallDuration)
{
	uint8_t lsb;
	uint8_t msb;

	if (WE_FAIL == ITDS_getFreeFallDurationLSB(sensorInterface, &lsb)) {
		return WE_FAIL;
	}
	if (WE_FAIL == ITDS_getFreeFallDurationMSB(sensorInterface, &msb)) {
		return WE_FAIL;
	}

	*freeFallDuration = (lsb & 0x1F) | ((msb & 0x1) << 5);

	return WE_SUCCESS;
}

/**
 * @brief Set free-fall duration LSB
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] freeFallDurationLsb Free-fall duration LSB (5 bits)
 * @retval Error code
 */
int8_t ITDS_setFreeFallDurationLSB(WE_sensorInterface_t *sensorInterface,
				   uint8_t freeFallDurationLsb)
{
	ITDS_freeFall_t freeFall;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall)) {
		return WE_FAIL;
	}

	freeFall.freeFallDurationLSB = freeFallDurationLsb;

	return ITDS_WriteReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall);
}

/**
 * @brief Read the free-fall duration LSB
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] freeFallDurationLsb The returned free-fall duration LSB (5 bits)
 * @retval Error code
 */
int8_t ITDS_getFreeFallDurationLSB(WE_sensorInterface_t *sensorInterface,
				   uint8_t *freeFallDurationLsb)
{
	ITDS_freeFall_t freeFall;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall)) {
		return WE_FAIL;
	}

	*freeFallDurationLsb = freeFall.freeFallDurationLSB;
	return WE_SUCCESS;
}

/**
 * @brief Set free-fall threshold
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] threshold Encoded free-fall threshold value (3 bits)
 * @retval Error code
 */
int8_t ITDS_setFreeFallThreshold(WE_sensorInterface_t *sensorInterface,
				 ITDS_FreeFallThreshold_t threshold)
{
	ITDS_freeFall_t freeFall;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall)) {
		return WE_FAIL;
	}

	freeFall.freeFallThreshold = threshold;

	return ITDS_WriteReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall);
}

/**
 * @brief Read the free-fall threshold
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] threshold The returned encoded free-fall threshold value (3 bits)
 * @retval Error code
 */
int8_t ITDS_getFreeFallThreshold(WE_sensorInterface_t *sensorInterface,
				 ITDS_FreeFallThreshold_t *threshold)
{
	ITDS_freeFall_t freeFall;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_FREE_FALL_REG, 1, (uint8_t *)&freeFall)) {
		return WE_FAIL;
	}

	*threshold = (ITDS_FreeFallThreshold_t)freeFall.freeFallThreshold;
	return WE_SUCCESS;
}

/* STATUS_DETECT */
/* Note: Most of the status bits are already covered by the STATUS_REG register. */

/**
 * @brief Read the status detect register state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] statusDetect The returned status detect register state
 * @retval Error code
 */
int8_t ITDS_getStatusDetectRegister(WE_sensorInterface_t *sensorInterface,
				    ITDS_statusDetect_t *statusDetect)
{
	return ITDS_ReadReg(sensorInterface, ITDS_STATUS_DETECT_REG, 1, (uint8_t *)statusDetect);
}

/**
 * @brief Check if new temperature samples are available.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] dataReady The returned data-ready state
 * @retval Error code
 */
int8_t ITDS_isTemperatureDataReady(WE_sensorInterface_t *sensorInterface, ITDS_state_t *dataReady)
{
	ITDS_statusDetect_t statusDetect;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_STATUS_DETECT_REG, 1, (uint8_t *)&statusDetect)) {
		return WE_FAIL;
	}

	*dataReady = (ITDS_state_t)statusDetect.temperatureDataReady;
	return WE_SUCCESS;
}

/* WAKE_UP_EVENT */

/**
 * @brief Read the overall wake-up event status
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] status The returned wake-up event status
 * @retval Error code
 */
int8_t ITDS_getWakeUpEventRegister(WE_sensorInterface_t *sensorInterface,
				   ITDS_wakeUpEvent_t *status)
{
	return ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)status);
}

/**
 * @brief Read the wake-up event detection status on axis X
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] wakeUpX The returned wake-up event detection status on axis X.
 * @retval Error code
 */
int8_t ITDS_isWakeUpXEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *wakeUpX)
{
	ITDS_wakeUpEvent_t wakeUpEvent;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)&wakeUpEvent)) {
		return WE_FAIL;
	}

	*wakeUpX = (ITDS_state_t)wakeUpEvent.wakeUpX;

	return WE_SUCCESS;
}

/**
 * @brief Read the wake-up event detection status on axis Y
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] wakeUpY The returned wake-up event detection status on axis Y.
 * @retval Error code
 */
int8_t ITDS_isWakeUpYEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *wakeUpY)
{
	ITDS_wakeUpEvent_t wakeUpEvent;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)&wakeUpEvent)) {
		return WE_FAIL;
	}

	*wakeUpY = (ITDS_state_t)wakeUpEvent.wakeUpY;

	return WE_SUCCESS;
}

/**
 * @brief Read the wake-up event detection status on axis Z
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] wakeUpZ The returned wake-up event detection status on axis Z.
 * @retval Error code
 */
int8_t ITDS_isWakeUpZEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *wakeUpZ)
{
	ITDS_wakeUpEvent_t wakeUpEvent;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)&wakeUpEvent)) {
		return WE_FAIL;
	}

	*wakeUpZ = (ITDS_state_t)wakeUpEvent.wakeUpZ;

	return WE_SUCCESS;
}

/**
 * @brief Read the wake-up event detection status (wake-up event on any axis)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] wakeUpState The returned wake-up event detection state.
 * @retval Error code
 */
int8_t ITDS_isWakeUpEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *wakeUpState)
{
	ITDS_wakeUpEvent_t wakeUpEvent;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)&wakeUpEvent)) {
		return WE_FAIL;
	}

	*wakeUpState = (ITDS_state_t)wakeUpEvent.wakeUpState;

	return WE_SUCCESS;
}

/**
 * @brief Read the free-fall event state [not detected/detected]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] freeFall The returned free-fall event status.
 * @retval Error code
 */
int8_t ITDS_isFreeFallEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *freeFall)
{
	ITDS_wakeUpEvent_t wakeUpEvent;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_WAKE_UP_EVENT_REG, 1, (uint8_t *)&wakeUpEvent)) {
		return WE_FAIL;
	}

	*freeFall = (ITDS_state_t)wakeUpEvent.freeFallState;

	return WE_SUCCESS;
}

/* TAP EVENT 0x39 */

/**
 * @brief Read the overall tap event status
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] status The returned tap event status
 * @retval Error code
 */
int8_t ITDS_getTapEventRegister(WE_sensorInterface_t *sensorInterface, ITDS_tapEvent_t *status)
{
	return ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)status);
}

/**
 * @brief Read the tap event status (tap event on any axis)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapEventState The returned tap event state
 * @retval Error code
 */
int8_t ITDS_isTapEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapEventState)
{
	ITDS_tapEvent_t tapEvent;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)&tapEvent)) {
		return WE_FAIL;
	}

	*tapEventState = (ITDS_state_t)tapEvent.tapEventState;

	return WE_SUCCESS;
}

/**
 * @brief Read the tap event acceleration sign (direction of tap event)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapSign The returned tap event acceleration sign
 * @retval Error code
 */
int8_t ITDS_getTapSign(WE_sensorInterface_t *sensorInterface, ITDS_tapSign_t *tapSign)
{
	ITDS_tapEvent_t tapEvent;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)&tapEvent)) {
		return WE_FAIL;
	}

	*tapSign = (ITDS_tapSign_t)tapEvent.tapSign;

	return WE_SUCCESS;
}

/**
 * @brief Read the tap event status on axis X
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapXAxis The returned tap event status on axis X.
 * @retval Error code
 */
int8_t ITDS_isTapEventXAxis(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapXAxis)
{
	ITDS_tapEvent_t tapEvent;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)&tapEvent)) {
		return WE_FAIL;
	}

	*tapXAxis = (ITDS_state_t)tapEvent.tapXAxis;

	return WE_SUCCESS;
}

/**
 * @brief Read the tap event status on axis Y
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapYAxis The returned tap event status on axis Y.
 * @retval Error code
 */
int8_t ITDS_isTapEventYAxis(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapYAxis)
{
	ITDS_tapEvent_t tapEvent;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)&tapEvent)) {
		return WE_FAIL;
	}

	*tapYAxis = (ITDS_state_t)tapEvent.tapYAxis;

	return WE_SUCCESS;
}

/**
 * @brief Read the tap event status on axis Z
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tapZAxis The returned tap event status on axis Z.
 * @retval Error code
 */
int8_t ITDS_isTapEventZAxis(WE_sensorInterface_t *sensorInterface, ITDS_state_t *tapZAxis)
{
	ITDS_tapEvent_t tapEvent;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_TAP_EVENT_REG, 1, (uint8_t *)&tapEvent)) {
		return WE_FAIL;
	}

	*tapZAxis = (ITDS_state_t)tapEvent.tapZAxis;

	return WE_SUCCESS;
}

/* 6D_EVENT */

/**
 * @brief Read register containing info on 6D orientation change event.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] status The returned 6D event status.
 * @retval Error code
 */
int8_t ITDS_get6dEventRegister(WE_sensorInterface_t *sensorInterface, ITDS_6dEvent_t *status)
{
	return ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)status);
}

/**
 * @brief Check if 6D orientation change event has occurred.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] orientationChanged The returned 6D orientation change event status
 * @retval Error code
 */
int8_t ITDS_has6dOrientationChanged(WE_sensorInterface_t *sensorInterface,
				    ITDS_state_t *orientationChanged)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*orientationChanged = (ITDS_state_t)event6d.sixDChange;

	return WE_SUCCESS;
}

/**
 * @brief Read the XL over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] xlOverThreshold The returned XL over threshold state
 * @retval Error code
 */
int8_t ITDS_isXLOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *xlOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*xlOverThreshold = (ITDS_state_t)event6d.xlOverThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Read the XH over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] xhOverThreshold The returned XH over threshold state
 * @retval Error code
 */
int8_t ITDS_isXHOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *xhOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*xhOverThreshold = (ITDS_state_t)event6d.xhOverThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Read the YL over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] ylOverThreshold The returned YL over threshold state
 * @retval Error code
 */
int8_t ITDS_isYLOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *ylOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*ylOverThreshold = (ITDS_state_t)event6d.ylOverThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Read the YH over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] yhOverThreshold The returned YH over threshold state
 * @retval Error code
 */
int8_t ITDS_isYHOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *yhOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*yhOverThreshold = (ITDS_state_t)event6d.yhOverThreshold;
	return WE_SUCCESS;
}

/**
 * @brief Read the ZL over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] zlOverThreshold The returned ZL over threshold state
 * @retval Error code
 */
int8_t ITDS_isZLOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *zlOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*zlOverThreshold = (ITDS_state_t)event6d.zlOverThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Read the ZH over threshold state (6D orientation)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] zhOverThreshold The returned ZH over threshold state
 * @retval Error code
 */
int8_t ITDS_isZHOverThreshold(WE_sensorInterface_t *sensorInterface, ITDS_state_t *zhOverThreshold)
{
	ITDS_6dEvent_t event6d;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_6D_EVENT_REG, 1, (uint8_t *)&event6d)) {
		return WE_FAIL;
	}

	*zhOverThreshold = (ITDS_state_t)event6d.zhOverThreshold;

	return WE_SUCCESS;
}

/* ALL_INT_EVENT */

/**
 * @brief Read register containing info on all interrupt events
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] events The returned interrupt events status
 * @retval Error code
 */
int8_t ITDS_getAllInterruptEvents(WE_sensorInterface_t *sensorInterface,
				  ITDS_allInterruptEvents_t *events)
{
	return ITDS_ReadReg(sensorInterface, ITDS_ALL_INT_EVENT_REG, 1, (uint8_t *)events);
}

/**
 * @brief Read the sleep change interrupt event state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] sleep The returned sleep change interrupt event state
 * @retval Error code
 */
int8_t ITDS_isSleepChangeEvent(WE_sensorInterface_t *sensorInterface, ITDS_state_t *sleep)
{
	ITDS_allInterruptEvents_t allInterrupts;

	if (WE_FAIL ==
	    ITDS_ReadReg(sensorInterface, ITDS_ALL_INT_EVENT_REG, 1, (uint8_t *)&allInterrupts)) {
		return WE_FAIL;
	}

	*sleep = (ITDS_state_t)allInterrupts.sleepChangeState;

	return WE_SUCCESS;
}

/* X_Y_Z_OFS_USR */

/**
 * @brief Set the user offset for axis X (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offsetValueXAxis User offset for axis X
 * @retval Error code
 */
int8_t ITDS_setOffsetValueX(WE_sensorInterface_t *sensorInterface, int8_t offsetValueXAxis)
{
	return ITDS_WriteReg(sensorInterface, ITDS_X_OFS_USR_REG, 1, (uint8_t *)&offsetValueXAxis);
}

/**
 * @brief Read the user offset for axis X (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offsetvalueXAxis The returned user offset for axis X.
 * @retval Error code
 */
int8_t ITDS_getOffsetValueX(WE_sensorInterface_t *sensorInterface, int8_t *offsetvalueXAxis)
{
	return ITDS_ReadReg(sensorInterface, ITDS_X_OFS_USR_REG, 1, (uint8_t *)offsetvalueXAxis);
}

/**
 * @brief Set the user offset for axis Y (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offsetValueYAxis User offset for axis Y
 * @retval Error code
 */
int8_t ITDS_setOffsetValueY(WE_sensorInterface_t *sensorInterface, int8_t offsetValueYAxis)
{
	return ITDS_WriteReg(sensorInterface, ITDS_Y_OFS_USR_REG, 1, (uint8_t *)&offsetValueYAxis);
}

/**
 * @brief Read the user offset for axis Y (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offsetValueYAxis The returned user offset for axis Y.
 * @retval Error code
 */
int8_t ITDS_getOffsetValueY(WE_sensorInterface_t *sensorInterface, int8_t *offsetValueYAxis)
{
	return ITDS_ReadReg(sensorInterface, ITDS_Y_OFS_USR_REG, 1, (uint8_t *)offsetValueYAxis);
}

/**
 * @brief Set the user offset for axis Z (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offsetvalueZAxis The user offset for axis Z
 * @retval Error code
 */
int8_t ITDS_setOffsetValueZ(WE_sensorInterface_t *sensorInterface, int8_t offsetvalueZAxis)
{
	return ITDS_WriteReg(sensorInterface, ITDS_Z_OFS_USR_REG, 1, (uint8_t *)&offsetvalueZAxis);
}

/**
 * @brief Read the user offset for axis Z (for output data and/or wake-up)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offsetValueZAxis The returned user offset for axis Z.
 * @retval Error code
 */
int8_t ITDS_getOffsetValueZ(WE_sensorInterface_t *sensorInterface, int8_t *offsetValueZAxis)
{
	return ITDS_ReadReg(sensorInterface, ITDS_Z_OFS_USR_REG, 1, (uint8_t *)offsetValueZAxis);
}

/* CTRL_7 */

/**
 * @brief Select the data ready interrupt mode [latched mode / pulsed mode]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] drdyPulsed Data ready interrupt mode
 * @retval Error code
 */
int8_t ITDS_setDataReadyPulsed(WE_sensorInterface_t *sensorInterface, ITDS_drdyPulse_t drdyPulsed)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.drdyPulse = drdyPulsed;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Read the data ready interrupt mode [latched mode / pulsed mode]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] drdyPulsed The returned data ready interrupt mode
 * @retval Error code
 */
int8_t ITDS_isDataReadyPulsed(WE_sensorInterface_t *sensorInterface, ITDS_drdyPulse_t *drdyPulsed)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*drdyPulsed = (ITDS_drdyPulse_t)ctrl7.drdyPulse;

	return WE_SUCCESS;
}

/**
 * @brief Enable signal routing from INT_1 to INT_0
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] int1OnInt0 Signal routing INT_1 to INT_0 state
 * @retval Error code
 */
int8_t ITDS_setInt1OnInt0(WE_sensorInterface_t *sensorInterface, ITDS_state_t int1OnInt0)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.INT1toINT0 = int1OnInt0;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if signal routing from INT_1 to INT_0 is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] int1OnInt0 The returned routing enable state.
 * @retval Error code
 */
int8_t ITDS_getInt1OnInt0(WE_sensorInterface_t *sensorInterface, ITDS_state_t *int1OnInt0)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*int1OnInt0 = (ITDS_state_t)ctrl7.INT1toINT0;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable interrupts
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] interrupts Interrupts enable state
 * @retval Error code
 */
int8_t ITDS_enableInterrupts(WE_sensorInterface_t *sensorInterface, ITDS_state_t interrupts)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.enInterrupts = interrupts;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if interrupts are enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] interrupts The returned interrupts enable state.
 * @retval Error code
 */
int8_t ITDS_areInterruptsEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *interrupts)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*interrupts = (ITDS_state_t)ctrl7.enInterrupts;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the application of the user offset values to output data
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] applyOffset State
 * @retval Error code
 */
int8_t ITDS_enableApplyOffset(WE_sensorInterface_t *sensorInterface, ITDS_state_t applyOffset)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.applyOffset = applyOffset;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if application of user offset values to output data is enabled.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] applyOffset Returned enable state.
 * @retval Error code
 */
int8_t ITDS_isApplyOffsetEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *applyOffset)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*applyOffset = (ITDS_state_t)ctrl7.applyOffset;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the application of user offset values to data only for wake-up functions
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] applyOffset State
 * @retval Error code
 */
int8_t ITDS_enableApplyWakeUpOffset(WE_sensorInterface_t *sensorInterface, ITDS_state_t applyOffset)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.applyWakeUpOffset = applyOffset;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if application user offset values to data only for wake-up functions is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] applyOffset The returned enable state
 * @retval Error code
 */
int8_t ITDS_isApplyWakeUpOffsetEnabled(WE_sensorInterface_t *sensorInterface,
				       ITDS_state_t *applyOffset)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*applyOffset = (ITDS_state_t)ctrl7.applyWakeUpOffset;

	return WE_SUCCESS;
}

/**
 * @brief Set the weight of the user offset words
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offsetWeight Offset weight
 * @retval Error code
 */
int8_t ITDS_setOffsetWeight(WE_sensorInterface_t *sensorInterface, ITDS_state_t offsetWeight)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.userOffset = offsetWeight;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Read the weight of the user offset words
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offsetWeight The returned offset weight.
 * @retval Error code
 */
int8_t ITDS_getOffsetWeight(WE_sensorInterface_t *sensorInterface, ITDS_state_t *offsetWeight)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*offsetWeight = (ITDS_state_t)ctrl7.userOffset;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable high pass filter reference mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] refMode State
 * @retval Error code
 */
int8_t ITDS_enableHighPassRefMode(WE_sensorInterface_t *sensorInterface, ITDS_state_t refMode)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.highPassRefMode = refMode;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if high pass filter reference mode is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] refMode The returned reference mode state
 * @retval Error code
 */
int8_t ITDS_isHighPassRefModeEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *refMode)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*refMode = (ITDS_state_t)ctrl7.highPassRefMode;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the low pass filter for 6D orientation detection
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] lowPassOn6D Low pass filter enable state
 * @retval Error code
 */
int8_t ITDS_enableLowPassOn6D(WE_sensorInterface_t *sensorInterface, ITDS_state_t lowPassOn6D)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	ctrl7.lowPassOn6D = lowPassOn6D;

	return ITDS_WriteReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7);
}

/**
 * @brief Check if the low pass filter for 6D orientation detection is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lowPassOn6D The returned low pass filter enable state
 * @retval Error code
 */
int8_t ITDS_isLowPassOn6DEnabled(WE_sensorInterface_t *sensorInterface, ITDS_state_t *lowPassOn6D)
{
	ITDS_ctrl7_t ctrl7;

	if (WE_FAIL == ITDS_ReadReg(sensorInterface, ITDS_CTRL_7_REG, 1, (uint8_t *)&ctrl7)) {
		return WE_FAIL;
	}

	*lowPassOn6D = (ITDS_state_t)ctrl7.lowPassOn6D;

	return WE_SUCCESS;
}
