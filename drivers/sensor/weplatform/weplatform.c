/*
 * Copyright (c) 2022 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "weplatform.h"

#ifdef CONFIG_I2C
#include <drivers/i2c.h>
#endif /* CONFIG_I2C */

#ifdef CONFIG_SPI
#include <drivers/spi.h>
#endif /* CONFIG_SPI */

/**
 * @brief Read data starting from the addressed register
 * @param[in] interface Sensor interface
 * @param[in] regAdr The register address to read from
 * @param[in] numBytesToRead Number of bytes to read
 * @param[out] data The read data will be stored here
 * @retval Error code
 */
inline int8_t WE_ReadReg(WE_sensorInterface_t *interface, uint8_t regAdr, uint16_t numBytesToRead,
			 uint8_t *data)
{
	int status = 0;

	switch (interface->interfaceType) {
	case WE_i2c:
#ifdef CONFIG_I2C
		if (numBytesToRead > 1 && interface->options.i2c.useRegAddrMsbForMultiBytesRead) {
			/*
			 * Register address most significant bit is used to
			 * enable multi bytes read
			 */
			regAdr |= 1 << 7;
		}
		if (interface->options.i2c.slaveTransmitterMode) {
			status = i2c_read_dt(interface->handle, data, numBytesToRead);
		} else {
			status = i2c_burst_read_dt(interface->handle, regAdr, data, numBytesToRead);
		}
#else
		status = -EIO;
#endif /* CONFIG_I2C */
		break;

	case WE_spi: {
#ifdef CONFIG_SPI
		uint8_t bytesStep = interface->options.spi.burstMode ? numBytesToRead : 1;

		for (uint8_t i = 0; i < numBytesToRead; i += bytesStep) {
			uint8_t buffer_tx[2] = { (regAdr + i) | (1 << 7), 0 };

			/*  Write 1 byte containing register address (MSB=1) + 1 dummy byte */
			const struct spi_buf tx_buf = { .buf = buffer_tx, .len = 2 };
			const struct spi_buf_set tx_buf_set = { .buffers = &tx_buf, .count = 1 };

			/* Skip first byte, then read "bytesStep" bytes of data */
			const struct spi_buf rx_buf[2] = { { .buf = NULL, .len = 1 },
							   { .buf = data + i, .len = bytesStep } };
			const struct spi_buf_set rx_buf_set = { .buffers = rx_buf, .count = 2 };

			status = spi_transceive_dt(interface->handle, &tx_buf_set, &rx_buf_set);
			if (status != 0) {
				/* Error, abort */
				break;
			}
		}
#else
		status = -EIO;
#endif /* CONFIG_SPI */
		break;
	}
	}

	return status == 0 ? WE_SUCCESS : WE_FAIL;
}

/**
 * @brief Write data starting from the addressed register
 * @param[in] interface Sensor interface
 * @param[in] regAdr Address of register to be written
 * @param[in] numBytesToWrite Number of bytes to write
 * @param[in] data Data to be written
 * @retval Error code
 */
inline int8_t WE_WriteReg(WE_sensorInterface_t *interface, uint8_t regAdr, uint16_t numBytesToWrite,
			  uint8_t *data)
{
	int status = 0;

	switch (interface->interfaceType) {
	case WE_i2c:
#ifdef CONFIG_I2C
		status = i2c_burst_write_dt(interface->handle, regAdr, data, numBytesToWrite);
#else
		status = -EIO;
#endif /* CONFIG_I2C */
		break;

	case WE_spi: {
#ifdef CONFIG_SPI
		uint8_t bytesStep = interface->options.spi.burstMode ? numBytesToWrite : 1;

		for (uint8_t i = 0; i < numBytesToWrite; i += bytesStep) {
			uint8_t buffer_tx[1] = { (regAdr + i) & ~(1 << 7) };

			/*
			 * First write 1 byte containing register address (MSB=0), then
			 * write "bytesStep" bytes of data
			 */
			const struct spi_buf tx_buf[2] = { { .buf = buffer_tx, .len = 1 },
							   { .buf = data + i, .len = bytesStep } };
			const struct spi_buf_set tx_buf_set = { .buffers = tx_buf, .count = 2 };

			status = spi_write_dt(interface->handle, &tx_buf_set);

			if (status != 0) {
				/* Error, abort */
				break;
			}
		}
#else
		status = -EIO;
#endif /* CONFIG_SPI */
		break;
	}
	}

	return status == 0 ? WE_SUCCESS : WE_FAIL;
}

/**
 * @brief Checks if the sensor interface is ready.
 * @param[in] interface Sensor interface
 * @return WE_SUCCESS if interface is ready, WE_FAIL if not.
 */
int8_t WE_isSensorInterfaceReady(WE_sensorInterface_t *interface)
{
	switch (interface->interfaceType) {
	case WE_i2c:
#ifdef CONFIG_I2C
		return device_is_ready(((const struct i2c_dt_spec *)interface->handle)->bus) ?
			       WE_SUCCESS :
			       WE_FAIL;
#else
		return WE_FAIL;
#endif

	case WE_spi:
#ifdef CONFIG_SPI
		return spi_is_ready(interface->handle) ? WE_SUCCESS : WE_FAIL;
#else
		return WE_FAIL;
#endif

	default:
		return WE_FAIL;
	}
}
