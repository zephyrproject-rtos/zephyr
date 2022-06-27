/*
 * Copyright (c) 2022 Wuerth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver file for the WSEN-PDUS sensor.
 */

#include "WSEN_PDUS_25131308XXX01.h"

#include <stdio.h>

#include <weplatform.h>

/**
 * @brief Default sensor interface configuration.
 */
static WE_sensorInterface_t pdusDefaultSensorInterface = {
	.sensorType = WE_PDUS,
	.interfaceType = WE_i2c,
	.options = { .i2c = { .address = PDUS_ADDRESS_I2C,
			      .burstMode = 0,
			      .slaveTransmitterMode = 1,
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
 * Note that this sensor doesn't have any registers to request - it
 * will simply send up to 4 bytes of data in response to any read
 * request.
 *
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] numBytesToRead Number of bytes to be read
 * @param[out] data Target buffer
 * @return Error Code
 */
static inline int8_t PDUS_ReadReg(WE_sensorInterface_t *sensorInterface, uint16_t numBytesToRead,
				  uint8_t *data)
{
	/*
	 * Caution: This sensor uses 5V Vcc and logic levels.
	 * Level conversion to 3.3V is required to talk with a STM32 or any
	 * other 3.3V MCU.
	 * This sensor only supports I2C read operation and returns either
	 * 2 or 4 bytes when the sensor address is written to the I2C bus.
	 * Sending a register address is not required.
	 * The first 2 bytes returned are the raw pressure value and the next 2
	 * bytes are the raw temperature values.
	 *
	 * See chapter "reading digital output data" of the PDUS user manual for
	 * the protocol.
	 *
	 * 1st I2C master sends sensor's I2C address (PDUS_ADDRESS_I2C) with read
	 * bit set and waits for ACK by sensor.
	 * 2nd I2C master read either 2 or 4 bytes back from the sensor; the slave
	 * (sensor) will send up to 4 bytes.
	 * Master has to ACK each byte and provide clock.
	 */

	return WE_ReadReg(sensorInterface, 0, numBytesToRead, data);
}

/**
 * @brief Returns the default sensor interface configuration.
 * @param[out] sensorInterface Sensor interface configuration (output parameter)
 * @return Error code
 */
int8_t PDUS_getDefaultInterface(WE_sensorInterface_t *sensorInterface)
{
	*sensorInterface = pdusDefaultSensorInterface;
	return WE_SUCCESS;
}

/**
 * @brief Checks if the sensor interface is ready.
 * @param[in] sensorInterface Pointer to sensor interface
 * @return WE_SUCCESS if interface is ready, WE_FAIL if not.
 */
int8_t PDUS_isInterfaceReady(WE_sensorInterface_t *sensorInterface)
{
	return WE_isSensorInterfaceReady(sensorInterface);
}

/**
 * @brief Read the raw pressure
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] data Pointer to raw pressure value (unconverted), 15 bits
 * @retval Error code
 */
int8_t PDUS_getRawPressure(WE_sensorInterface_t *sensorInterface, uint16_t *pressure)
{
	uint8_t tmp[2] = { 0 };

	if (WE_FAIL == PDUS_ReadReg(sensorInterface, 2, tmp)) {
		return WE_FAIL;
	}

	*pressure = (uint16_t)((tmp[0] & 0x7F) << 8);
	*pressure |= (uint16_t)tmp[1];

	return WE_SUCCESS;
}

/**
 * @brief Read the raw pressure and temperature values
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[out] pressure Pointer to raw pressure value (unconverted), 15 bits
 * @param[out] temperature Pointer to raw temperature value (unconverted), 15 bits
 * @retval Error code
 */
int8_t PDUS_getRawPressureAndTemperature(WE_sensorInterface_t *sensorInterface, uint16_t *pressure,
					 uint16_t *temperature)
{
	uint8_t tmp[4] = { 0 };

	if (WE_FAIL == PDUS_ReadReg(sensorInterface, 4, tmp)) {
		return WE_FAIL;
	}

	*pressure = (uint16_t)((tmp[0] & 0x7F) << 8);
	*pressure |= (uint16_t)tmp[1];

	*temperature = (uint16_t)((tmp[2] & 0x7F) << 8);
	*temperature |= (uint16_t)tmp[3];

	return WE_SUCCESS;
}

/**
 * @brief Read the pressure and temperature values
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] type PDUS sensor type (i.e. pressure measurement range) for internal conversion of pressure
 * @param[out] presskPa Pointer to pressure value
 * @retval Error code
 */
int8_t PDUS_getPressure_float(WE_sensorInterface_t *sensorInterface, PDUS_SensorType_t type,
			      float *presskPa)
{
	uint16_t rawPres = 0;

	if (WE_FAIL == PDUS_getRawPressure(sensorInterface, &rawPres)) {
		return WE_FAIL;
	}

	if (rawPres < P_MIN_VAL_PDUS) {
		rawPres = P_MIN_VAL_PDUS;
	}

	/* Perform conversion (depending on sensor sub-type) */
	return PDUS_convertPressureToFloat(type, rawPres, presskPa);
}

/**
 * @brief Read the pressure and temperature values
 * @param[in] sensorInterface Pointer to sensor interface
 * @param[in] type PDUS sensor type (i.e. pressure measurement range) for internal conversion of pressure
 * @param[out] presskPa Pointer to pressure value
 * @param[out] tempDegC Pointer to temperature value
 * @retval Error code
 */
int8_t PDUS_getPressureAndTemperature_float(WE_sensorInterface_t *sensorInterface,
					    PDUS_SensorType_t type, float *presskPa,
					    float *tempDegC)
{
	uint16_t rawPres = 0;
	uint16_t rawTemp = 0;

	if (WE_FAIL == PDUS_getRawPressureAndTemperature(sensorInterface, &rawPres, &rawTemp)) {
		return WE_FAIL;
	}

	if (rawPres < P_MIN_VAL_PDUS) {
		rawPres = P_MIN_VAL_PDUS;
	}

	if (rawTemp < T_MIN_VAL_PDUS) {
		rawTemp = T_MIN_VAL_PDUS;
	}

	/* Apply temperature offset to raw temperature and convert to °C (0-70°C) */
	*tempDegC = (((float)(rawTemp - T_MIN_VAL_PDUS) * 4.272) / 1000);

	/* Perform conversion regarding sensor sub-type */
	return PDUS_convertPressureToFloat(type, rawPres, presskPa);
}

/**
 * @brief Converts a raw pressure value to kPa, depending on the PDUS sensor type.
 * @param[in] type PDUS sensor type (i.e. pressure measurement range)
 * @param[in] rawPressure Raw pressure value as returned by the sensor
 * @param[out] presskPa Pointer to pressure value
 * @retval Error code
 */
int8_t PDUS_convertPressureToFloat(PDUS_SensorType_t type, uint16_t rawPressure, float *presskPa)
{
	float temp = (float)(rawPressure - P_MIN_VAL_PDUS);
	switch (type) {
	case PDUS_pdus0:
		*presskPa = ((temp * 7.63f) / 1000000) - 0.1f;
		break;

	case PDUS_pdus1:
		*presskPa = ((temp * 7.63f) / 100000) - 1.0f;
		break;

	case PDUS_pdus2:
		*presskPa = ((temp * 7.63f) / 10000) - 10.0f;
		break;

	case PDUS_pdus3:
		*presskPa = ((temp * 3.815f) / 1000);
		break;

	case PDUS_pdus4:
		*presskPa = ((temp * 4.196f) / 100) - 100.0f;
		break;

	default:
		return WE_FAIL;
	}
	return WE_SUCCESS;
}
