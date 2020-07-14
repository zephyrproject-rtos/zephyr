/* vl53l0x_platform.c - Zephyr customization of ST vl53l0x library.
 * (library is located in ext/hal/st/lib/sensor/vl53l0x/)
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vl53l0x_platform.h"

#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/i2c.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(VL53L0X, CONFIG_SENSOR_LOG_LEVEL);

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,
				 uint32_t count)
{

	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int = 0;
	uint8_t I2CBuffer[count+1];

	I2CBuffer[0] = index;
	memcpy(&I2CBuffer[1], pdata, count);

	status_int = i2c_write(Dev->i2c, I2CBuffer, count+1, Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to write");
	}

	return Status;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,
				uint32_t count)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;

	status_int = i2c_burst_read(Dev->i2c, Dev->I2cDevAddr, index, pdata,
				    count);
	if (status_int < 0) {
		LOG_ERR("Failed to read");
		return -EIO;
	}

	return Status;
}


VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;

	status_int = i2c_reg_write_byte(Dev->i2c, Dev->I2cDevAddr, index, data);

	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_write_byte failed (%d)", Status);
	}

	return Status;
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t I2CBuffer[3];

	I2CBuffer[0] = index;
	I2CBuffer[1] = data >> 8;
	I2CBuffer[2] = data & 0x00FF;

	status_int = i2c_write(Dev->i2c, I2CBuffer, 3, Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t I2CBuffer[5];

	I2CBuffer[0] = index;
	I2CBuffer[1] = (data >> 24) & 0xFF;
	I2CBuffer[2] = (data >> 16) & 0xFF;
	I2CBuffer[3] = (data >> 8)  & 0xFF;
	I2CBuffer[4] = (data >> 0) & 0xFF;

	status_int = i2c_write(Dev->i2c, I2CBuffer, 5, Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index,
				 uint8_t AndData, uint8_t OrData)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;
	uint8_t data;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_RdByte(Dev, index, &data);
	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("VL53L0X_RdByte failed (%d)", Status);
	}

	if (Status == VL53L0X_ERROR_NONE) {
		data = (data & AndData) | OrData;
		status_int = VL53L0X_WrByte(Dev, index, data);
		if (status_int != 0) {
			Status = VL53L0X_ERROR_CONTROL_INTERFACE;
			LOG_DBG("VL53L0X_WrByte failed.(%d)", Status);
		}
	}

	return Status;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;

	status_int = i2c_reg_read_byte(Dev->i2c, Dev->I2cDevAddr, index, data);
	if (status_int < 0) {
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_read_byte failed (%d)", Status);
	}

	return Status;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[2];

	status_int = i2c_burst_read(Dev->i2c, Dev->I2cDevAddr, index, buf, 2);
	if (status_int < 0) {
		LOG_ERR("i2c_burst_read failed");
		return -EIO;
	}
	*data = ((uint16_t)buf[0]<<8) + (uint16_t)buf[1];

	return Status;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[4];

	status_int = i2c_burst_read(Dev->i2c, Dev->I2cDevAddr, index, buf, 4);
	if (status_int < 0) {
		LOG_ERR("i2c_burst_read failed");
		return -EIO;
	}
	*data = ((uint32_t)buf[0]<<24) + ((uint32_t)buf[1]<<16) +
		((uint32_t)buf[2]<<8) + (uint32_t)buf[3];

	return Status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
	k_sleep(K_MSEC(2));
	return VL53L0X_ERROR_NONE;
}
