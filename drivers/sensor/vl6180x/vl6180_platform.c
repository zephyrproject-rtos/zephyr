/* vl6180x_platform.c - Zephyr customization of ST vl6180x library.
 * (library is located in ext/hal/st/lib/sensor/vl6180x/)
 */

/*
 * Copyright (c) 2017 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vl6180_platform.h"
#include "vl6180_api.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(VL6180X, CONFIG_SENSOR_LOG_LEVEL);

#define VL6180X_ERROR_CONTROL_INTERFACE -5


int VL6180_RdMulti(VL6180Dev_t dev, uint16_t index, uint8_t *data, int nData)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t reg[2];

	reg[0] = (index >> 8)  & 0xFF;
	reg[1] = (index >> 0)  & 0xFF;
	status_int = i2c_write_read(dev->i2c, dev->I2cDevAddr, reg, sizeof(reg), data, nData);
	if (status_int < 0) {
		LOG_ERR("Failed to read");
		return -EIO;
	}

	return Status;
}

int VL6180_WrByte(VL6180Dev_t Dev, uint16_t index, uint8_t data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t I2CBuffer[3];

	I2CBuffer[0] = (index >> 8)  & 0xFF;
	I2CBuffer[1] = (index >> 0)  & 0xFF;
	I2CBuffer[2] = data;

	status_int = i2c_write(Dev->i2c, I2CBuffer, sizeof(I2CBuffer), Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL6180X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_write_byte failed (%d)", Status);
	}

	return Status;
}

int VL6180_WrWord(VL6180Dev_t Dev, uint16_t index, uint16_t data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t I2CBuffer[4];

	I2CBuffer[0] = (index >> 8)  & 0xFF;
	I2CBuffer[1] = (index >> 0)  & 0xFF;
	I2CBuffer[2] = data >> 8;
	I2CBuffer[3] = data & 0x00FF;

	status_int = i2c_write(Dev->i2c, I2CBuffer, sizeof(I2CBuffer), Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL6180X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

int VL6180_WrDWord(VL6180Dev_t Dev, uint16_t index, uint32_t data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t I2CBuffer[6];

	I2CBuffer[0] = (index >> 8)  & 0xFF;
	I2CBuffer[1] = (index >> 0)  & 0xFF;
	I2CBuffer[2] = (data >> 24) & 0xFF;
	I2CBuffer[3] = (data >> 16) & 0xFF;
	I2CBuffer[4] = (data >> 8)  & 0xFF;
	I2CBuffer[5] = (data >> 0) & 0xFF;

	status_int = i2c_write(Dev->i2c, I2CBuffer, sizeof(I2CBuffer), Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL6180X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

int  VL6180_UpdateByte(VL6180Dev_t Dev, uint16_t index,
				 uint8_t AndData, uint8_t OrData)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t deviceAddress;
	uint8_t data;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL6180_RdByte(Dev, index, &data);
	if (status_int < 0) {
		Status = VL6180X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("VL6180_RdByte failed (%d)", Status);
	}

	if (Status == NoError) {
		data = (data & AndData) | OrData;
		status_int = VL6180_WrByte(Dev, index, data);
		if (status_int != 0) {
			Status = VL6180X_ERROR_CONTROL_INTERFACE;
			LOG_DBG("VL6180_WrByte failed.(%d)", Status);
		}
	}

	return Status;
}

int VL6180_RdByte(VL6180Dev_t dev, uint16_t index, uint8_t *data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t reg[2];

	reg[0] = (index >> 8)  & 0xFF;
	reg[1] = (index >> 0)  & 0xFF;
	status_int = i2c_write_read(dev->i2c, dev->I2cDevAddr, reg, sizeof(reg), data, 1);
	if (status_int < 0) {
		Status = VL6180X_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_read_byte failed (%d)", Status);
	}

	return Status;
}

int VL6180_RdWord(VL6180Dev_t dev, uint16_t index, uint16_t *data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t reg[2];
	uint8_t buf[2];

	reg[0] = (index >> 8)  & 0xFF;
	reg[1] = (index >> 0)  & 0xFF;
	status_int = i2c_write_read(dev->i2c, dev->I2cDevAddr, reg, sizeof(reg), buf, sizeof(buf));
	if (status_int < 0) {
		LOG_ERR("i2c_burst_read failed");
		return -EIO;
	}
	*data = ((uint16_t)buf[0]<<8) + (uint16_t)buf[1];

	return Status;
}

int VL6180_RdDWord(VL6180Dev_t dev, uint16_t index, uint32_t *data)
{
	int Status = NoError;
	int32_t status_int;
	uint8_t reg[2];
	uint8_t buf[4];

	reg[0] = (index >> 8)  & 0xFF;
	reg[1] = (index >> 0)  & 0xFF;
	status_int = i2c_write_read(dev->i2c, dev->I2cDevAddr, reg, sizeof(reg), buf, sizeof(buf));
	if (status_int < 0) {
		LOG_ERR("i2c_burst_read failed");
		return -EIO;
	}
	*data = ((uint32_t)buf[0]<<24) + ((uint32_t)buf[1]<<16) +
		((uint32_t)buf[2]<<8) + (uint32_t)buf[3];

	return Status;
}

void VL6180_PollDelay(VL6180Dev_t Dev)
{
	k_sleep(K_MSEC(2));
}

