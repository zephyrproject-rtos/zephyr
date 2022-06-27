/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver file for the WSEN-PADS sensor.
 */

#include "WSEN_PADS_2511020213301.h"

#include <stdio.h>

#include <weplatform.h>

/**
 * @brief Default sensor interface configuration.
 */
static WE_sensorInterface_t padsDefaultSensorInterface = {
	.sensorType = WE_PADS,
	.interfaceType = WE_i2c,
	.options = { .i2c = { .address = PADS_ADDRESS_I2C_1,
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

/* FIFO buffer stores pressure (3 bytes) and temperature (2 bytes) values. */
uint8_t fifoBuffer[PADS_FIFO_BUFFER_SIZE * 5] = { 0 };

/**
 * @brief Read data from sensor.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] regAdr Address of register to read from
 * @param[in] numBytesToRead Number of bytes to be read
 * @param[out] data Target buffer
 * @return Error Code
 */
static inline int8_t PADS_ReadReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
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
static inline int8_t PADS_WriteReg(WE_sensorInterface_t *sensorInterface, uint8_t regAdr,
				   uint16_t numBytesToWrite, uint8_t *data)
{
	return WE_WriteReg(sensorInterface, regAdr, numBytesToWrite, data);
}

/**
 * @brief Returns the default sensor interface configuration.
 * @param[out] sensorInterface Sensor interface configuration (output parameter)
 * @return Error code
 */
int8_t PADS_getDefaultInterface(WE_sensorInterface_t *sensorInterface)
{
	*sensorInterface = padsDefaultSensorInterface;
	return WE_SUCCESS;
}

/**
 * @brief Checks if the sensor interface is ready.
 * @param[in] sensorInterface Pointer to sensor interface
 * @return WE_SUCCESS if interface is ready, WE_FAIL if not.
 */
int8_t PADS_isInterfaceReady(WE_sensorInterface_t *sensorInterface)
{
	return WE_isSensorInterfaceReady(sensorInterface);
}

/**
 * @brief Read the device ID
 *
 * Expected value is PADS_DEVICE_ID_VALUE.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] deviceID The returned device ID.
 * @retval Error code
 */
int8_t PADS_getDeviceID(WE_sensorInterface_t *sensorInterface, uint8_t *deviceID)
{
	return PADS_ReadReg(sensorInterface, PADS_DEVICE_ID_REG, 1, deviceID);
}

/**
 * @brief Enable the AUTOREFP function
 *
 * Note that when enabling AUTOREFP using this function, the AUTOREFP bit
 * will stay high only until the first conversion is complete. The function will remain
 * turned on even if the bit is zero.
 *
 * The function can be turned off with PADS_resetAutoRefp().
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] autoRefp Turns AUTOREFP function on
 * @retval Error code
 */
int8_t PADS_enableAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t autoRefp)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.autoRefp = autoRefp;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if the AUTOREFP function is currently being enabled.
 *
 * Note that when enabling AUTOREFP using PADS_enableAutoRefp(WE_sensorInterface_t* sensorInterface, ), the AUTOREFP bit
 * will stay high only until the first conversion is complete. The function will remain
 * turned on even if the bit is zero.
 *
 * The function can be turned off with PADS_resetAutoRefp().
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] autoRefp The returned state
 * @retval Error code
 */
int8_t PADS_isEnablingAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t *autoRefp)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}
	*autoRefp = (PADS_state_t)interruptConfigurationReg.autoRefp;
	return WE_SUCCESS;
}

/**
 * @brief Turn off the AUTOREFP function
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] reset Reset state
 * @retval Error code
 */
int8_t PADS_resetAutoRefp(WE_sensorInterface_t *sensorInterface, PADS_state_t reset)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.resetAutoRefp = reset;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Enable the AUTOZERO function
 *
 * Note that when enabling AUTOZERO using this function, the AUTOZERO bit
 * will stay high only until the first conversion is complete. The function will remain
 * turned on even if the bit is zero.
 *
 * The function can be turned off with PADS_resetAutoZeroMode().
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] autoZero Turns AUTOZERO function on
 * @retval Error code
 */
int8_t PADS_enableAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t autoZero)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.autoZero = autoZero;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if the AUTOZERO function is currently being enabled.
 *
 * Note that when enabling AUTOZERO using PADS_enableAutoZeroMode(), the AUTOZERO bit
 * will stay high only until the first conversion is complete. The function will remain
 * turned on even if the bit is zero.
 *
 * The function can be turned off with PADS_resetAutoZeroMode().
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] autoZero The returned state
 * @retval Error code
 */
int8_t PADS_isEnablingAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t *autoZero)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	*autoZero = (PADS_state_t)interruptConfigurationReg.autoZero;

	return WE_SUCCESS;
}

/**
 * @brief Turn off the AUTOZERO function
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] reset Reset state
 * @retval Error code
 */
int8_t PADS_resetAutoZeroMode(WE_sensorInterface_t *sensorInterface, PADS_state_t reset)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.resetAutoZero = reset;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Enable/disable the differential pressure interrupt [enabled,disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] diffEn Differential pressure interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableDiffPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t diffEn)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.diffInt = diffEn;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if the differential pressure interrupt is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] diffIntState The returned differential interrupt enable state
 * @retval Error code
 */
int8_t PADS_isDiffPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *diffIntState)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	*diffIntState = (PADS_state_t)interruptConfigurationReg.diffInt;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable latched interrupt [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] state Latched interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableLatchedInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.latchedInt = state;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if latched interrupts are enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] latchInt The returned latched interrupts enable state
 * @retval Error code
 */
int8_t PADS_isLatchedInterruptEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *latchInt)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	*latchInt = (PADS_state_t)interruptConfigurationReg.latchedInt;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the low pressure interrupt [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] state Low pressure interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableLowPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.lowPresInt = state;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if the low pressure interrupt is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lpint The returned low pressure interrupt enable state
 * @retval Error code
 */
int8_t PADS_isLowPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					  PADS_state_t *lpint)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	*lpint = (PADS_state_t)interruptConfigurationReg.lowPresInt;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the high pressure interrupt [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] state High pressure interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableHighPressureInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t state)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	interruptConfigurationReg.highPresInt = state;

	return PADS_WriteReg(sensorInterface, PADS_INT_CFG_REG, 1,
			     (uint8_t *)&interruptConfigurationReg);
}

/**
 * @brief Check if the high pressure interrupt is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] hpint The returned high pressure interrupt enable state
 * @retval Error code
 */
int8_t PADS_isHighPressureInterruptEnabled(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *hpint)
{
	PADS_interruptConfiguration_t interruptConfigurationReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_INT_CFG_REG, 1,
				    (uint8_t *)&interruptConfigurationReg)) {
		return WE_FAIL;
	}

	*hpint = (PADS_state_t)interruptConfigurationReg.highPresInt;

	return WE_SUCCESS;
}

/**
 * @brief Read interrupt source register
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] intSource The returned interrupt source register state
 * @retval Error code
 */
int8_t PADS_getInterruptSource(WE_sensorInterface_t *sensorInterface, PADS_intSource_t *intSource)
{
	return PADS_ReadReg(sensorInterface, PADS_INT_SOURCE_REG, 1, (uint8_t *)intSource);
}

/**
 * @brief Read the state of the interrupts
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] intState Returned state of the interrupts
 * @retval Error code
 */
int8_t PADS_getInterruptStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *intState)
{
	PADS_intSource_t int_source;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INT_SOURCE_REG, 1, (uint8_t *)&int_source)) {
		return WE_FAIL;
	}

	*intState = (PADS_state_t)int_source.intStatus;

	return WE_SUCCESS;
}

/**
 * @brief Read the state of the differential low pressure interrupt [not active, active]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lpState The returned state of the differential low pressure interrupt
 * @retval Error code
 */
int8_t PADS_getLowPressureInterruptStatus(WE_sensorInterface_t *sensorInterface,
					  PADS_state_t *lpState)
{
	PADS_intSource_t int_source;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INT_SOURCE_REG, 1, (uint8_t *)&int_source)) {
		return WE_FAIL;
	}

	*lpState = (PADS_state_t)int_source.diffPresLowEvent;

	return WE_SUCCESS;
}

/**
 * @brief Read the state of the differential high pressure interrupt [not active, active]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] hpState The returned state of the differential high pressure interrupt
 * @retval No error
 */
int8_t PADS_getHighPressureInterruptStatus(WE_sensorInterface_t *sensorInterface,
					   PADS_state_t *hpState)
{
	PADS_intSource_t int_source;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INT_SOURCE_REG, 1, (uint8_t *)&int_source)) {
		return WE_FAIL;
	}

	*hpState = int_source.diffPresHighEvent;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the FIFO full interrupt
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fullState FIFO full interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableFifoFullInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t fullState)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.fifoFullInt = fullState;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Enable/disable the FIFO threshold interrupt
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] threshState FIFO threshold interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableFifoThresholdInterrupt(WE_sensorInterface_t *sensorInterface,
					 PADS_state_t threshState)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.fifoThresholdInt = threshState;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Enable/disable the FIFO overrun interrupt
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] ovrState FIFO overrun interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableFifoOverrunInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t ovrState)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.fifoOverrunInt = ovrState;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Check if FIFO is full [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoFull The returned FIFO full state
 * @retval Error code
 */
int8_t PADS_isFifoFull(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoFull)
{
	PADS_fifoStatus2_t fifo_status2;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_STATUS2_REG, 1, (uint8_t *)&fifo_status2)) {
		return WE_FAIL;
	}

	*fifoFull = (PADS_state_t)fifo_status2.fifoFull;

	return WE_SUCCESS;
}

/**
 * @brief Check if FIFO fill level has exceeded the user defined threshold [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoWtm The returned FIFO threshold reached state
 * @retval Error code
 */
int8_t PADS_isFifoThresholdReached(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoWtm)
{
	PADS_fifoStatus2_t fifo_status2;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_STATUS2_REG, 1, (uint8_t *)&fifo_status2)) {
		return WE_FAIL;
	}

	*fifoWtm = (PADS_state_t)fifo_status2.fifoWtm;

	return WE_SUCCESS;
}

/**
 * @brief Read the FIFO overrun state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoOvr The returned FIFO overrun state
 * @retval Error code
 */
int8_t PADS_getFifoOverrunState(WE_sensorInterface_t *sensorInterface, PADS_state_t *fifoOvr)
{
	PADS_fifoStatus2_t fifo_status2;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_STATUS2_REG, 1, (uint8_t *)&fifo_status2)) {
		return WE_FAIL;
	}

	*fifoOvr = (PADS_state_t)fifo_status2.fifoOverrun;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the data ready signal interrupt
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] drdy Data ready interrupt enable state
 * @retval Error code
 */
int8_t PADS_enableDataReadyInterrupt(WE_sensorInterface_t *sensorInterface, PADS_state_t drdy)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.dataReadyInt = drdy;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Check if the data ready signal interrupt is enabled [enabled,disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] drdy The returned data ready interrupt enable state
 * @retval Error code
 */
int8_t PADS_isDataReadyInterruptEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *drdy)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*drdy = (PADS_state_t)ctrl3.dataReadyInt;

	return WE_SUCCESS;
}

/**
 * @brief Configure interrupt events (interrupt event control)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] ctr Interrupt event configuration
 * @retval Error code
 */
int8_t PADS_setInterruptEventControl(WE_sensorInterface_t *sensorInterface,
				     PADS_interruptEventControl_t ctr)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	ctrl3.intEventCtrl = ctr;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3);
}

/**
 * @brief Read the interrupt event configuration (interrupt event control)
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] intEvent The returned interrupt event configuration
 * @retval Error code
 */
int8_t PADS_getInterruptEventControl(WE_sensorInterface_t *sensorInterface,
				     PADS_interruptEventControl_t *intEvent)
{
	PADS_ctrl3_t ctrl3;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_3_REG, 1, (uint8_t *)&ctrl3)) {
		return WE_FAIL;
	}

	*intEvent = (PADS_interruptEventControl_t)ctrl3.intEventCtrl;

	return WE_SUCCESS;
}

/**
 * @brief Set the pressure threshold (relative to reference pressure,
 * both in positive and negative direction).
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] thresholdPa Threshold in Pa. Resolution is 6.25 Pa.
 * @retval Error code
 */
int8_t PADS_setPressureThreshold(WE_sensorInterface_t *sensorInterface, uint32_t thresholdPa)
{
	uint32_t thresholdBits = (thresholdPa * 16) / 100;
	if (WE_FAIL ==
	    PADS_setPressureThresholdLSB(sensorInterface, (uint8_t)(thresholdBits & 0xFF))) {
		return WE_FAIL;
	}
	return PADS_setPressureThresholdMSB(sensorInterface,
					    (uint8_t)((thresholdBits >> 8) & 0xFF));
}

/**
 * @brief Read the pressure threshold (relative to reference pressure,
 * both in positive and negative direction).
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] thresholdPa The returned threshold in Pa
 * @retval Error code
 */
int8_t PADS_getPressureThreshold(WE_sensorInterface_t *sensorInterface, uint32_t *thresholdPa)
{
	uint8_t thrLSB, thrMSB;
	if (WE_FAIL == PADS_getPressureThresholdLSB(sensorInterface, &thrLSB)) {
		return WE_FAIL;
	}
	if (WE_FAIL == PADS_getPressureThresholdMSB(sensorInterface, &thrMSB)) {
		return WE_FAIL;
	}
	*thresholdPa = (thrLSB & 0xFF) | ((thrMSB & 0xFF) << 8);
	*thresholdPa = (*thresholdPa * 100) / 16;
	return WE_SUCCESS;
}

/**
 * @brief Set the LSB pressure threshold value
 *
 * @see PADS_setPressureThreshold()
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] thr Pressure threshold LSB
 * @retval Error code
 */
int8_t PADS_setPressureThresholdLSB(WE_sensorInterface_t *sensorInterface, uint8_t thr)
{
	return PADS_WriteReg(sensorInterface, PADS_THR_P_L_REG, 1, &thr);
}

/**
 * @brief Set the MSB pressure threshold value
 *
 * @see PADS_setPressureThreshold()
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] thr Pressure threshold MSB
 * @retval Error code
 */
int8_t PADS_setPressureThresholdMSB(WE_sensorInterface_t *sensorInterface, uint8_t thr)
{
	return PADS_WriteReg(sensorInterface, PADS_THR_P_H_REG, 1, &thr);
}

/**
 * @brief Read the LSB pressure threshold value
 *
 * @see PADS_getPressureThreshold()
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] thrLSB The returned pressure threshold LSB value
 * @retval Error code
 */
int8_t PADS_getPressureThresholdLSB(WE_sensorInterface_t *sensorInterface, uint8_t *thrLSB)
{
	return PADS_ReadReg(sensorInterface, PADS_THR_P_L_REG, 1, thrLSB);
}

/**
 * @brief Read the MSB pressure threshold value
 *
 * @see PADS_getPressureThreshold()
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] thrMSB The returned pressure threshold MSB value
 * @retval Error code
 */
int8_t PADS_getPressureThresholdMSB(WE_sensorInterface_t *sensorInterface, uint8_t *thrMSB)
{
	return PADS_ReadReg(sensorInterface, PADS_THR_P_H_REG, 1, thrMSB);
}

/**
 * @brief Disable the I2C interface
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] i2cDisable I2C interface disable state (0: I2C enabled, 1: I2C disabled)
 * @retval Error code
 */
int8_t PADS_disableI2CInterface(WE_sensorInterface_t *sensorInterface, PADS_state_t i2cDisable)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	interfaceCtrl.disableI2C = i2cDisable;

	return PADS_WriteReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1,
			     (uint8_t *)&interfaceCtrl);
}

/**
 * @brief Read the I2C interface disable state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] i2cDisabled The returned I2C interface disable state (0: I2C enabled, 1: I2C disabled)
 * @retval Error code
 */
int8_t PADS_isI2CInterfaceDisabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *i2cDisabled)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	*i2cDisabled = (PADS_state_t)interfaceCtrl.disableI2C;

	return WE_SUCCESS;
}

/**
 * @brief Disable/enable the internal pull-down on interrupt pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] pullDownState Disable pull-down state (0: PD connected; 1: PD disconnected)
 * @retval Error code
 */
int8_t PADS_disablePullDownIntPin(WE_sensorInterface_t *sensorInterface, PADS_state_t pullDownState)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	interfaceCtrl.disPullDownOnIntPin = pullDownState;

	return PADS_WriteReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1,
			     (uint8_t *)&interfaceCtrl);
}

/**
 * @brief Read the state of the pull down on the interrupt pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pinState The returned pull-down state (0: PD connected; 1: PD disconnected)
 * @retval Error code
 */
int8_t PADS_isPullDownIntDisabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *pinState)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	*pinState = (PADS_state_t)interfaceCtrl.disPullDownOnIntPin;

	return WE_SUCCESS;
}

/**
 * @brief Set internal pull-up on the SAO pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] saoStatus SAO pull-up state
 * @retval Error code
 */
int8_t PADS_setSAOPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t saoStatus)
{
	PADS_interfaceCtrl_t interfaceCtrl;
	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	interfaceCtrl.pullUpOnSAOPin = saoStatus;

	return PADS_WriteReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1,
			     (uint8_t *)&interfaceCtrl);
}

/**
 * @brief Read the state of the pull-up on the SAO pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] saoPinState The returned SAO pull-up state
 * @retval Error code
 */
int8_t PADS_isSAOPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t *saoPinState)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	*saoPinState = (PADS_state_t)interfaceCtrl.pullUpOnSAOPin;

	return WE_SUCCESS;
}

/**
 * @brief Set internal pull-up on the SDA pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] sdaStatus SDA pull-up state
 * @retval Error code
 */
int8_t PADS_setSDAPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t sdaStatus)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	interfaceCtrl.pullUpOnSDAPin = sdaStatus;

	return PADS_WriteReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1,
			     (uint8_t *)&interfaceCtrl);
}

/**
 * @brief Read the state of the pull-up on the SDA pin
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] sdaPinState The returned SDA pull-up state
 * @retval Error code
 */
int8_t PADS_isSDAPullUp(WE_sensorInterface_t *sensorInterface, PADS_state_t *sdaPinState)
{
	PADS_interfaceCtrl_t interfaceCtrl;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INTERFACE_CTRL_REG, 1, (uint8_t *)&interfaceCtrl)) {
		return WE_FAIL;
	}

	*sdaPinState = (PADS_state_t)interfaceCtrl.pullUpOnSDAPin;

	return WE_SUCCESS;
}

/**
 * @brief Set the output data rate of the sensor
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] odr output data rate
 * @retval Error code
 */
int8_t PADS_setOutputDataRate(WE_sensorInterface_t *sensorInterface, PADS_outputDataRate_t odr)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.outputDataRate = odr;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Read the output data rate of the sensor
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] odr The returned output data rate
 * @retval Error code
 */
int8_t PADS_getOutputDataRate(WE_sensorInterface_t *sensorInterface, PADS_outputDataRate_t *odr)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*odr = (PADS_outputDataRate_t)ctrl1.outputDataRate;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the low pass filter
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] filterEnabled Filter state (0: disabled, 1: enabled)
 * @retval Error code
 */
int8_t PADS_enableLowPassFilter(WE_sensorInterface_t *sensorInterface, PADS_state_t filterEnabled)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.enLowPassFilter = filterEnabled;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Check if the low pass filter is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] filterEnabled The returned low pass filter enable state
 * @retval Error code
 */
int8_t PADS_isLowPassFilterEnabled(WE_sensorInterface_t *sensorInterface,
				   PADS_state_t *filterEnabled)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*filterEnabled = (PADS_state_t)ctrl1.enLowPassFilter;

	return WE_SUCCESS;
}

/**
 * @brief Set the low pass filter configuration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] conf Low pass filter configuration
 * @retval Error code
 */
int8_t PADS_setLowPassFilterConfig(WE_sensorInterface_t *sensorInterface, PADS_filterConf_t conf)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.lowPassFilterConfig = conf;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Read the low pass filter configuration
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] conf The returned low pass filter configuration
 * @retval Error code
 */
int8_t PADS_getLowPassFilterConfig(WE_sensorInterface_t *sensorInterface, PADS_filterConf_t *conf)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*conf = (PADS_filterConf_t)ctrl1.lowPassFilterConfig;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable block data update
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] bdu Block data update state
 * @retval Error code
 */
int8_t PADS_enableBlockDataUpdate(WE_sensorInterface_t *sensorInterface, PADS_state_t bdu)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	ctrl1.blockDataUpdate = bdu;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1);
}

/**
 * @brief Check if block data update is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] bdu The returned block data update enable state
 * @retval Error code
 */
int8_t PADS_isBlockDataUpdateEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *bdu)
{
	PADS_ctrl1_t ctrl1;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_1_REG, 1, (uint8_t *)&ctrl1)) {
		return WE_FAIL;
	}

	*bdu = (PADS_state_t)ctrl1.blockDataUpdate;

	return WE_SUCCESS;
}

/**
 * @brief (Re)boot the device [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] reboot Reboot state
 * @retval Error code
 */
int8_t PADS_reboot(WE_sensorInterface_t *sensorInterface, PADS_state_t reboot)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.boot = reboot;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the reboot state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] rebooting The returned reboot state.
 * @retval Error code
 */
int8_t PADS_isRebooting(WE_sensorInterface_t *sensorInterface, PADS_state_t *reboot)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}
	*reboot = (PADS_state_t)ctrl2.boot;

	return WE_SUCCESS;
}

/**
 * @brief Read the boot state
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] boot The returned Boot state
 * @retval Error code
 */
int8_t PADS_getBootStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *boot)
{
	PADS_intSource_t int_source_reg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_INT_SOURCE_REG, 1, (uint8_t *)&int_source_reg)) {
		return WE_FAIL;
	}

	*boot = (PADS_state_t)int_source_reg.bootOn;

	return WE_SUCCESS;
}

/**
 * @brief Set the interrupt active level [active high/active low]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] level Interrupt active level
 * @retval Error code
 */
int8_t PADS_setInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    PADS_interruptActiveLevel_t level)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.intActiveLevel = level;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the interrupt active level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] level The returned interrupt active level
 * @retval Error code
 */
int8_t PADS_getInterruptActiveLevel(WE_sensorInterface_t *sensorInterface,
				    PADS_interruptActiveLevel_t *level)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*level = (PADS_interruptActiveLevel_t)ctrl2.intActiveLevel;

	return WE_SUCCESS;
}

/**
 * @brief Set the interrupt pin type [push-pull/open-drain]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] pinType Interrupt pin type
 * @retval Error code
 */
int8_t PADS_setInterruptPinType(WE_sensorInterface_t *sensorInterface,
				PADS_interruptPinConfig_t pinType)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.openDrainOnINTPin = pinType;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the interrupt pin type [push-pull/open-drain]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pinType The returned interrupt pin type.
 * @retval Error code
 */
int8_t PADS_getInterruptPinType(WE_sensorInterface_t *sensorInterface,
				PADS_interruptPinConfig_t *pinType)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*pinType = (PADS_interruptPinConfig_t)ctrl2.openDrainOnINTPin;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the auto address increment feature
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] autoInc Auto address increment feature enable state
 * @retval Error code
 */
int8_t PADS_enableAutoIncrement(WE_sensorInterface_t *sensorInterface, PADS_state_t autoInc)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.autoAddIncr = autoInc;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Check if the auto address increment feature is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] inc The returned auto address increment feature enable state
 * @retval Error code
 */
int8_t PADS_isAutoIncrementEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *inc)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*inc = (PADS_state_t)ctrl2.autoAddIncr;

	return WE_SUCCESS;
}

/**
 * @brief Set software reset [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] swReset Software reset state
 * @retval Error code
 */
int8_t PADS_softReset(WE_sensorInterface_t *sensorInterface, PADS_state_t swReset)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.softwareReset = swReset;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the software reset state [enabled, disabled]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] swReset The returned software reset state.
 * @retval Error code
 */
int8_t PADS_getSoftResetState(WE_sensorInterface_t *sensorInterface, PADS_state_t *swReset)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}
	*swReset = (PADS_state_t)ctrl2.softwareReset;

	return WE_SUCCESS;
}

/**
 * @brief Set the power mode of the sensor [low noise, low current]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] mode Power mode
 * @retval Error code
 */
int8_t PADS_setPowerMode(WE_sensorInterface_t *sensorInterface, PADS_powerMode_t mode)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.lowNoiseMode = mode;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Read the power mode [low noise, low current]
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] mode The returned power mode
 * @retval Error code
 */
int8_t PADS_getPowerMode(WE_sensorInterface_t *sensorInterface, PADS_powerMode_t *mode)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}
	*mode = (PADS_powerMode_t)ctrl2.lowNoiseMode;

	return WE_SUCCESS;
}

/**
 * @brief Enable/disable the one shot mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] oneShot One shot bit state
 * @retval Error code
 */
int8_t PADS_enableOneShot(WE_sensorInterface_t *sensorInterface, PADS_state_t oneShot)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	ctrl2.oneShotBit = oneShot;

	return PADS_WriteReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2);
}

/**
 * @brief Check if one shot mode is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] oneShot The returned one shot bit state
 * @retval Error code
 */
int8_t PADS_isOneShotEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *oneShot)
{
	PADS_ctrl2_t ctrl2;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_CTRL_2_REG, 1, (uint8_t *)&ctrl2)) {
		return WE_FAIL;
	}

	*oneShot = (PADS_state_t)ctrl2.oneShotBit;

	return WE_SUCCESS;
}

/**
 * @brief Set LSB part of the pressure offset value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offset LSB part of the pressure offset value
 * @retval Error code
 */
int8_t PADS_setPressureOffsetLSB(WE_sensorInterface_t *sensorInterface, uint8_t offset)
{
	return PADS_WriteReg(sensorInterface, PADS_OPC_P_L_REG, 1, &offset);
}

/**
 * @brief Read the LSB part of the pressure offset value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offset The returned LSB part of the pressure offset value
 * @retval Error code
 */
int8_t PADS_getPressureOffsetLSB(WE_sensorInterface_t *sensorInterface, uint8_t *offset)
{
	return PADS_ReadReg(sensorInterface, PADS_OPC_P_L_REG, 1, offset);
}

/**
 * @brief Set MSB part of the pressure offset value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] offset MSB part of the pressure offset value
 * @retval Error code
 */
int8_t PADS_setPressureOffsetMSB(WE_sensorInterface_t *sensorInterface, uint8_t offset)
{
	return PADS_WriteReg(sensorInterface, PADS_OPC_P_H_REG, 1, &offset);
}

/**
 * @brief Read the MSB part of the pressure offset value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] offset The returned MSB part of the pressure offset value
 * @retval Error code
 */
int8_t PADS_getPressureOffsetMSB(WE_sensorInterface_t *sensorInterface, uint8_t *offset)
{
	return PADS_ReadReg(sensorInterface, PADS_OPC_P_H_REG, 1, offset);
}

/**
 * @brief Set the FIFO mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fifoMode FIFO mode to be set
 * @retval Error code
 */
int8_t PADS_setFifoMode(WE_sensorInterface_t *sensorInterface, PADS_fifoMode_t fifoMode)
{
	PADS_fifoCtrl_t fifoCtrlReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg)) {
		return WE_FAIL;
	}

	fifoCtrlReg.fifoMode = fifoMode;

	return PADS_WriteReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg);
}

/**
 * @brief Read the FIFO mode
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoMode The returned FIFO mode
 * @retval Error code
 */
int8_t PADS_getFifoMode(WE_sensorInterface_t *sensorInterface, PADS_fifoMode_t *fifoMode)
{
	PADS_fifoCtrl_t fifoCtrlReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg)) {
		return WE_FAIL;
	}

	*fifoMode = (PADS_fifoMode_t)fifoCtrlReg.fifoMode;

	return WE_SUCCESS;
}

/**
 * @brief Set stop on user-defined FIFO threshold level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] state Stop on FIFO threshold state
 * @retval Error code
 */
int8_t PADS_enableStopOnThreshold(WE_sensorInterface_t *sensorInterface, PADS_state_t state)
{
	PADS_fifoCtrl_t fifoCtrlReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg)) {
		return WE_FAIL;
	}

	fifoCtrlReg.stopOnThreshold = state;

	return PADS_WriteReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg);
}

/**
 * @brief Check if stopping on user-defined threshold level is enabled
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state Stop on FIFO threshold enable state
 * @retval Error code
 */
int8_t PADS_isStopOnThresholdEnabled(WE_sensorInterface_t *sensorInterface, PADS_state_t *state)
{
	PADS_fifoCtrl_t fifoCtrlReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_CTRL_REG, 1, (uint8_t *)&fifoCtrlReg)) {
		return WE_FAIL;
	}

	*state = (PADS_state_t)fifoCtrlReg.stopOnThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Set the FIFO threshold level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] fifoThr FIFO threshold level (value between 0 and 127)
 * @retval Error code
 */
int8_t PADS_setFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t fifoThr)
{
	PADS_fifoThreshold_t fifoThresholdReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_WTM_REG, 1, (uint8_t *)&fifoThresholdReg)) {
		return WE_FAIL;
	}

	fifoThresholdReg.fifoThreshold = fifoThr;

	return PADS_WriteReg(sensorInterface, PADS_FIFO_WTM_REG, 1, (uint8_t *)&fifoThresholdReg);
}

/**
 * @brief Read the FIFO threshold level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoThr The returned FIFO threshold level
 * @retval Error code
 */
int8_t PADS_getFifoThreshold(WE_sensorInterface_t *sensorInterface, uint8_t *fifoThr)
{
	PADS_fifoThreshold_t fifoThresholdReg;

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_WTM_REG, 1, (uint8_t *)&fifoThresholdReg)) {
		return WE_FAIL;
	}

	*fifoThr = (uint8_t)fifoThresholdReg.fifoThreshold;

	return WE_SUCCESS;
}

/**
 * @brief Read the current FIFO fill level
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] fifoLevel The returned FIFO fill level
 * @retval Error code
 */
int8_t PADS_getFifoFillLevel(WE_sensorInterface_t *sensorInterface, uint8_t *fifoLevel)
{
	return PADS_ReadReg(sensorInterface, PADS_FIFO_STATUS1_REG, 1, fifoLevel);
}

/**
 * @brief Read the reference pressure
 *
 * Note: The reference pressure is set automatically when enabling AUTOZERO or AUTOREFP.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] referencePressurePa The returned reference pressure in Pa
 * @retval Error code
 */
int8_t PADS_getReferencePressure(WE_sensorInterface_t *sensorInterface,
				 uint32_t *referencePressurePa)
{
	if (WE_FAIL == PADS_getRawReferencePressure(sensorInterface, referencePressurePa)) {
		return WE_FAIL;
	}
	*referencePressurePa = PADS_convertPressure_int(*referencePressurePa);
	return WE_SUCCESS;
}

/**
 * @brief Read the raw reference pressure
 *
 * Note: The reference pressure is set automatically when enabling AUTOZERO or AUTOREFP.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] referencePressure The returned raw reference pressure.
 * @retval Error code
 */
int8_t PADS_getRawReferencePressure(WE_sensorInterface_t *sensorInterface,
				    uint32_t *referencePressure)
{
	uint8_t low, high;
	if (WE_FAIL == PADS_getReferencePressureLSB(sensorInterface, &low)) {
		return WE_FAIL;
	}
	if (WE_FAIL == PADS_getReferencePressureMSB(sensorInterface, &high)) {
		return WE_FAIL;
	}
	*referencePressure = (((uint32_t)high) << 16) | (((uint32_t)low) << 8);
	return WE_SUCCESS;
}

/**
 * @brief Read the LSB of the reference pressure
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] lowReferenceValue The returned reference pressure LSB
 * @retval Error code
 */
int8_t PADS_getReferencePressureLSB(WE_sensorInterface_t *sensorInterface,
				    uint8_t *lowReferenceValue)
{
	return PADS_ReadReg(sensorInterface, PADS_REF_P_L_REG, 1, lowReferenceValue);
}

/**
 * @brief Read the MSB of the reference pressure
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] highReferenceValue The returned reference pressure MSB
 * @retval Error code
 */
int8_t PADS_getReferencePressureMSB(WE_sensorInterface_t *sensorInterface,
				    uint8_t *highReferenceValue)
{
	return PADS_ReadReg(sensorInterface, PADS_REF_P_H_REG, 1, highReferenceValue);
}

/**
 * @brief Check if the temperature data register has been overwritten
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned temperature data overwritten state
 * @retval Error code
 */
int8_t PADS_getTemperatureOverrunStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *state)
{
	PADS_status_t statusReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*state = (PADS_state_t)statusReg.tempDataOverrun;

	return WE_SUCCESS;
}

/**
 * @brief Check if the pressure data register has been overwritten
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned pressure data overwritten state
 * @retval Error code
 */
int8_t PADS_getPressureOverrunStatus(WE_sensorInterface_t *sensorInterface, PADS_state_t *state)
{
	PADS_status_t statusReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*state = (PADS_state_t)statusReg.presDataOverrun;

	return WE_SUCCESS;
}

/**
 * @brief Check if new pressure data is available
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned pressure data availability state
 * @retval Error code
 */
int8_t PADS_isPressureDataAvailable(WE_sensorInterface_t *sensorInterface, PADS_state_t *state)
{
	PADS_status_t statusReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}
	*state = (PADS_state_t)statusReg.presDataAvailable;

	return WE_SUCCESS;
}

/**
 * @brief Check if new temperature data is available
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] state The returned temperature data availability state
 * @retval Error code
 */
int8_t PADS_isTemperatureDataAvailable(WE_sensorInterface_t *sensorInterface, PADS_state_t *state)
{
	PADS_status_t statusReg;

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_STATUS_REG, 1, (uint8_t *)&statusReg)) {
		return WE_FAIL;
	}

	*state = (PADS_state_t)statusReg.tempDataAvailable;

	return WE_SUCCESS;
}

/**
 * @brief Read the raw measured pressure value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] rawPres The returned raw pressure
 * @retval Error code
 */
int8_t PADS_getRawPressure(WE_sensorInterface_t *sensorInterface, int32_t *rawPres)
{
	uint8_t tmp[3] = { 0 };

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_DATA_P_XL_REG, 3, tmp)) {
		return WE_FAIL;
	}

	*rawPres = (int32_t)(tmp[2] << 24);
	*rawPres |= (int32_t)(tmp[1] << 16);
	*rawPres |= (int32_t)(tmp[0] << 8);
	*rawPres /= 256;

	return WE_SUCCESS;
}

/**
 * @brief Read the raw measured temperature value
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] rawTemp The returned raw temperature
 * @retval Error code
 */
int8_t PADS_getRawTemperature(WE_sensorInterface_t *sensorInterface, int16_t *rawTemp)
{
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == PADS_ReadReg(sensorInterface, PADS_DATA_T_L_REG, 2, tmp)) {
		return WE_FAIL;
	}

	*rawTemp = (int16_t)(tmp[1] << 8);
	*rawTemp |= (int16_t)tmp[0];

	return WE_SUCCESS;
}

/**
 * @brief Read one or more raw pressure values from FIFO
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] rawPres The returned FIFO pressure measurement(s)
 * @retval Error code
 */
int8_t PADS_getFifoRawPressure(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			       int32_t *rawPres)
{
	if (numSamples > PADS_FIFO_BUFFER_SIZE) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_DATA_P_XL_REG, 5 * numSamples, fifoBuffer)) {
		return WE_FAIL;
	}

	uint8_t *bufferPtr = fifoBuffer;
	for (uint8_t i = 0; i < numSamples; i++, bufferPtr += 5, rawPres++) {
		*rawPres = (int32_t)(bufferPtr[2] << 24);
		*rawPres |= (int32_t)(bufferPtr[1] << 16);
		*rawPres |= (int32_t)(bufferPtr[0] << 8);
		*rawPres /= 256;
	}

	return WE_SUCCESS;
}

/**
 * @brief Reads one or more raw temperature values from FIFO
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] rawTemp The returned FIFO temperature measurement(s)
 * @retval Error code
 */
int8_t PADS_getFifoRawTemperature(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				  int16_t *rawTemp)
{
	if (numSamples > PADS_FIFO_BUFFER_SIZE) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_DATA_P_XL_REG, 5 * numSamples, fifoBuffer)) {
		return WE_FAIL;
	}

	uint8_t *bufferPtr = fifoBuffer;
	for (uint8_t i = 0; i < numSamples; i++, bufferPtr += 5, rawTemp++) {
		*rawTemp = (int16_t)(bufferPtr[4] << 8);
		*rawTemp |= (int16_t)bufferPtr[3];
	}

	return WE_SUCCESS;
}

/**
 * @brief Reads one or more raw pressure and temperature values from FIFO
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] rawPres The returned FIFO pressure measurement(s)
 * @param[out] rawTemp The returned FIFO temperature measurement(s)
 * @retval Error code
 */
int8_t PADS_getFifoRawValues(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			     int32_t *rawPres, int16_t *rawTemp)
{
	if (numSamples > PADS_FIFO_BUFFER_SIZE) {
		return WE_FAIL;
	}

	if (WE_FAIL ==
	    PADS_ReadReg(sensorInterface, PADS_FIFO_DATA_P_XL_REG, 5 * numSamples, fifoBuffer)) {
		return WE_FAIL;
	}

	uint8_t *bufferPtr = fifoBuffer;
	for (uint8_t i = 0; i < numSamples; i++, bufferPtr += 5, rawPres++, rawTemp++) {
		*rawPres = (int32_t)(bufferPtr[2] << 24);
		*rawPres |= (int32_t)(bufferPtr[1] << 16);
		*rawPres |= (int32_t)(bufferPtr[0] << 8);
		*rawPres /= 256;

		*rawTemp = (int16_t)(bufferPtr[4] << 8);
		*rawTemp |= (int16_t)bufferPtr[3];
	}

	return WE_SUCCESS;
}

/**
 * @brief Read the measured pressure value in Pa
 *
 * Note that, depending on the mode of operation, the sensor's output register
 * might contain differential pressure values (e.g. if AUTOZERO is enabled).
 * In that case, the function PADS_getDifferentialPressure_int() should be used.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pressPa The returned pressure measurement
 * @retval Error code
 */
int8_t PADS_getPressure_int(WE_sensorInterface_t *sensorInterface, int32_t *pressPa)
{
	int32_t rawPressure = 0;
	if (PADS_getRawPressure(sensorInterface, &rawPressure) == WE_SUCCESS) {
		*pressPa = PADS_convertPressure_int(rawPressure);
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/**
 * @brief Read the measured differential pressure value in Pa
 *
 * Use this function if the sensor is configured to write differential pressure
 * values to the output register (e.g. if AUTOZERO is enabled).
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pressPa The returned differential pressure measurement
 * @retval Error code
 */
int8_t PADS_getDifferentialPressure_int(WE_sensorInterface_t *sensorInterface, int32_t *pressPa)
{
	int32_t rawPressure = 0;
	if (PADS_getRawPressure(sensorInterface, &rawPressure) == WE_SUCCESS) {
		*pressPa = PADS_convertDifferentialPressure_int(rawPressure);
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/**
 * @brief Read the measured temperature value in 0.01 °C
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] temperature The returned temperature measurement
 * @retval Error code
 */
int8_t PADS_getTemperature_int(WE_sensorInterface_t *sensorInterface, int16_t *temperature)
{
	return PADS_getRawTemperature(sensorInterface, temperature);
}

/**
 * @brief Read one or more pressure values from FIFO.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] pressPa The returned FIFO pressure measurement(s) in Pa
 * @retval Error code
 */
int8_t PADS_getFifoPressure_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				int32_t *pressPa)
{
	if (WE_FAIL == PADS_getFifoRawPressure(sensorInterface, numSamples, pressPa)) {
		return WE_FAIL;
	}

	for (uint8_t i = 0; i < numSamples; i++) {
		pressPa[i] = PADS_convertPressure_int(pressPa[i]);
	}

	return WE_SUCCESS;
}

/**
 * @brief Read one or more temperature values from FIFO.
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] temperature The returned FIFO temperature measurement(s) in 0.01 °C
 * @retval Error code
 */
int8_t PADS_getFifoTemperature_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
				   int16_t *temperature)
{
	return PADS_getFifoRawTemperature(sensorInterface, numSamples, temperature);
}

/**
 * @brief Reads one or more pressure and temperature values from FIFO
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numSamples Number of samples to read
 * @param[out] pressPa The returned FIFO pressure measurement(s) in Pa
 * @param[out] temperature The returned FIFO temperature measurement(s) in 0.01 °C
 * @retval Error code
 */
int8_t PADS_getFifoValues_int(WE_sensorInterface_t *sensorInterface, uint8_t numSamples,
			      int32_t *pressPa, int16_t *temperature)
{
	if (WE_FAIL == PADS_getFifoRawValues(sensorInterface, numSamples, pressPa, temperature)) {
		return WE_FAIL;
	}

	for (uint8_t i = 0; i < numSamples; i++) {
		pressPa[i] = PADS_convertPressure_int(pressPa[i]);
	}

	return WE_SUCCESS;
}

/**
 * @brief Converts the supplied raw pressure to [Pa]
 *
 * Note that, depending on the mode of operation, the sensor's output register
 * might contain differential pressure values (e.g. if AUTOZERO is enabled).
 * In that case, the function PADS_convertDifferentialPressure_int() should be used.
 *
 * @retval Pressure in [Pa]
 */
int32_t PADS_convertPressure_int(int32_t rawPres)
{
	return (rawPres * 100) / 4096;
}

/**
 * @brief Converts the supplied raw differential pressure to [Pa]
 *
 * Use this function if the sensor is configured to write differential pressure
 * values to the output register (e.g. if AUTOZERO is enabled).
 *
 * @retval Differential pressure in [Pa]
 */
int32_t PADS_convertDifferentialPressure_int(int32_t rawPres)
{
	return (rawPres * 25600) / 4096;
}

/**
 * @brief Read the measured pressure value in kPa
 *
 * Note that, depending on the mode of operation, the sensor's output register
 * might contain differential pressure values (e.g. if AUTOZERO is enabled).
 * In that case, the function PADS_getDifferentialPressure_float() should be used.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] presskPa The returned pressure measurement
 * @retval Error code
 */
int8_t PADS_getPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa)
{
	int32_t rawPressure = 0;
	if (PADS_getRawPressure(sensorInterface, &rawPressure) == WE_SUCCESS) {
		*presskPa = PADS_convertPressure_float(rawPressure);
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/**
 * @brief Read the measured differential pressure value in kPa
 *
 * Use this function if the sensor is configured to write differential pressure
 * values to the output register (e.g. if AUTOZERO is enabled).
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] presskPa The returned pressure measurement
 * @retval Error code
 */
int8_t PADS_getDifferentialPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa)
{
	int32_t rawPressure = 0;
	if (PADS_getRawPressure(sensorInterface, &rawPressure) == WE_SUCCESS) {
		*presskPa = PADS_convertDifferentialPressure_float(rawPressure);
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/**
 * @brief Read the measured temperature value in °C
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tempDegC The returned temperature measurement
 * @retval Error code
 */
int8_t PADS_getTemperature_float(WE_sensorInterface_t *sensorInterface, float *tempDegC)
{
	int16_t rawTemp = 0;
	if (PADS_getRawTemperature(sensorInterface, &rawTemp) == WE_SUCCESS) {
		*tempDegC = (float)rawTemp;
		*tempDegC = *tempDegC / 100;
	} else {
		return WE_FAIL;
	}

	return WE_SUCCESS;
}

/**
 * @brief Read the pressure value from FIFO in kPa
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] presskPa The returned FIFO pressure measurement
 * @retval Error code
 */
int8_t PADS_getFifoPressure_float(WE_sensorInterface_t *sensorInterface, float *presskPa)
{
	int32_t rawPressure = 0;
	if (PADS_getFifoRawPressure(sensorInterface, 1, &rawPressure) == WE_SUCCESS) {
		*presskPa = PADS_convertPressure_float(rawPressure);
	} else {
		return WE_FAIL;
	}
	return WE_SUCCESS;
}

/**
 * @brief Read the temperature value from FIFO in °C
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] tempDegC The returned FIFO temperature measurement
 * @retval Error code
 */
int8_t PADS_getFifoTemperature_float(WE_sensorInterface_t *sensorInterface, float *tempDegC)
{
	int16_t rawTemp = 0;
	if (PADS_getFifoRawTemperature(sensorInterface, 1, &rawTemp) == WE_SUCCESS) {
		*tempDegC = (float)rawTemp;
		*tempDegC = *tempDegC / 100;
	} else {
		return WE_FAIL;
	}

	return WE_SUCCESS;
}

/**
 * @brief Converts the supplied raw pressure to [kPa]
 *
 * Note that, depending on the mode of operation, the sensor's output register
 * might contain differential pressure values (e.g. if AUTOZERO is enabled).
 * In that case, the function PADS_convertDifferentialPressure_float() should be used.
 *
 * @retval Pressure in [kPa]
 */
float PADS_convertPressure_float(int32_t rawPres)
{
	return ((float)rawPres) / 40960;
}

/**
 * @brief Converts the supplied raw differential pressure to [kPa]
 *
 * Use this function if the sensor is configured to write differential pressure
 * values to the output register (e.g. if AUTOZERO is enabled).
 *
 * @retval Differential pressure in [kPa]
 */
float PADS_convertDifferentialPressure_float(int32_t rawPres)
{
	return ((float)rawPres) * 0.00625;
}
