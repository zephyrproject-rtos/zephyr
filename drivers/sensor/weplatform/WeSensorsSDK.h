/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief This is the main header file of the WE Sensors SDK.
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WESENSORSSDK_H_
#define ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WESENSORSSDK_H_

/**
 * @brief SDK major version number.
 */
#define WE_SENSOR_SDK_MAJOR_VERSION 2

/**
 * @brief SDK minor version number.
 */
#define WE_SENSOR_SDK_MINOR_VERSION 2

/**
 * @brief Return code for successful operations.
 */
#define WE_SUCCESS 0

/**
 * @brief Return code for failed operations.
 */
#define WE_FAIL -1

/**
 * @brief Types of sensors supported by the WE Sensors SDK.
 */
typedef enum {
	WE_HIDS, /* Humidity sensor */
	WE_ITDS, /* Acceleration sensor */
	WE_TIDS, /* Temperature sensor */
	WE_PADS, /* Absolute pressure sensor */
	WE_PDUS, /* Differential pressure sensor */
	WE_SENSOR_TYPE_MAX
} WE_sensorType_t;

/**
 * @brief Supported digital interfaces of the sensors.
 */
typedef enum { WE_i2c, WE_spi } WE_sensorInterfaceType_t;

/**
 * @brief Options for I2C interface.
 */
typedef struct {
	/**
	 * @brief The sensor's I2C address.
	 */
	uint8_t address;

	/**
	 * @brief Enables receiving of multiple bytes in a single read operation.
	 */
	uint8_t burstMode : 1;

	/**
	 * @brief Enables slave-transmitter mode (i.e. read-only, polling mode IO operation).
	 * In this mode, no register addresses are used.
	 * Data is polled from the sensor by sending the I2C address and read bit.
	 */
	uint8_t slaveTransmitterMode : 1;

	/**
	 * @brief Enables usage of most significant bit of I2C register address to enable
	 * multi byte read (required e.g. by HIDS humidity sensor).
	 */
	uint8_t useRegAddrMsbForMultiBytesRead : 1;

	/**
	 * @brief Currently unused.
	 */
	uint8_t reserved : 5;
} WE_i2cOptions_t;

/**
 * @brief Options for SPI interface.
 */
typedef struct {
	/**
	 * @brief HAL port of chip select pin. The type of the port depends on the platform.
	 */
	void *chipSelectPort;

	/**
	 * @brief Pin to use for chip select.
	 */
	uint16_t chipSelectPin;

	/**
	 * @brief Enables receiving of multiple bytes in a single read operation.
	 */
	uint8_t burstMode : 1;

	/**
	 * @brief Currently unused.
	 */
	uint8_t reserved : 7;
} WE_spiOptions_t;

/**
 * @brief Interface options.
 */
typedef struct {
	/**
	 * @brief I2C interface options.
	 */
	WE_i2cOptions_t i2c;

	/**
	 * @brief SPI interface options.
	 */
	WE_spiOptions_t spi;

	/**
	 * @brief Timeout (ms) for read operations.
	 */
	uint16_t readTimeout;

	/**
	 * @brief Timeout (ms) for write operations.
	 */
	uint16_t writeTimeout;
} WE_sensorInterfaceOptions_t;

/**
 * @brief Sensor interface configuration structure.
 */
typedef struct {
	/**
	 * @brief Sensor type specifier.
	 */
	WE_sensorType_t sensorType;

	/**
	 * @brief Specifies the interface to be used to communicate with the sensor.
	 */
	WE_sensorInterfaceType_t interfaceType;

	/**
	 * @brief Options of sensor interface.
	 */
	WE_sensorInterfaceOptions_t options;

	/**
	 * @brief HAL interface handle. The type of the handle depends on the interface used.
	 */
	void *handle;
} WE_sensorInterface_t;

#endif /* ZEPHYR_DRIVERS_SENSOR_WEPLATFORM_WESENSORSSDK_H_ */
