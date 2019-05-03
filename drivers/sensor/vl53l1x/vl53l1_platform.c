/*
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include "vl53l1_platform.h"

#include <sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <i2c.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(VL53L1X);

VL53L1_Error VL53L1_WriteMulti(VL53L1_DEV Dev, uint16_t index, uint8_t *pdata,
			       uint32_t count)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int = 0;
	uint8_t buf[count + 2];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;
	memcpy(&buf[2], pdata, count);

	status_int = i2c_write(Dev->i2c, buf, count + 2, Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("Failed to write");
	}

	return Status;
}

VL53L1_Error VL53L1_ReadMulti(VL53L1_DEV Dev, uint16_t index, uint8_t *pdata,
			      uint32_t count)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	u8_t buf[2];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;

	status_int =
		i2c_write_read(Dev->i2c, Dev->I2cDevAddr, buf, 2, pdata, count);
	if (status_int < 0) {
		LOG_ERR("Failed to read");
		return -EIO;
	}

	return Status;
}

VL53L1_Error VL53L1_WrByte(VL53L1_DEV Dev, uint16_t index, uint8_t data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	u8_t buf[3];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;
	buf[2] = data;

	status_int = i2c_write(Dev->i2c, buf, 3, Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_write_byte failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_WrWord(VL53L1_DEV Dev, uint16_t index, uint16_t data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[4];

	buf[0] = index >> 8;
	buf[1] = index & 0x00FF;
	buf[2] = data >> 8;
	buf[3] = data & 0x00FF;

	status_int = i2c_write(Dev->i2c, buf, 4, Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_WrDWord(VL53L1_DEV Dev, uint16_t index, uint32_t data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[6];

	buf[0] = index >> 8;
	buf[1] = index & 0x00FF;
	buf[2] = (data >> 24) & 0xFF;
	buf[3] = (data >> 16) & 0xFF;
	buf[4] = (data >> 8) & 0xFF;
	buf[5] = (data >> 0) & 0xFF;

	status_int = i2c_write(Dev->i2c, buf, 6, Dev->I2cDevAddr);
	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_UpdateByte(VL53L1_DEV Dev, uint16_t index, uint8_t AndData,
			       uint8_t OrData)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;
	uint8_t data;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L1_RdByte(Dev, index, &data);
	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("VL53L1_RdByte failed (%d)", Status);
	}

	if (Status == VL53L1_ERROR_NONE) {
		data = (data & AndData) | OrData;
		status_int = VL53L1_WrByte(Dev, index, data);
		if (status_int != 0) {
			Status = VL53L1_ERROR_CONTROL_INTERFACE;
			LOG_DBG("VL53L1_WrByte failed.(%d)", Status);
		}
	}

	return Status;
}

VL53L1_Error VL53L1_RdByte(VL53L1_DEV Dev, uint16_t index, uint8_t *data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[2];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;

	status_int = i2c_write_read(Dev->i2c, Dev->I2cDevAddr, buf, 2, buf, 1);
	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_reg_read_byte failed (%d)", Status);
	}
	*data = buf[0];

	return Status;
}

VL53L1_Error VL53L1_RdWord(VL53L1_DEV Dev, uint16_t index, uint16_t *data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	uint8_t buf[2];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;

	status_int = i2c_write_read(Dev->i2c, Dev->I2cDevAddr, buf, 2, buf, 2);
	if (status_int < 0) {
		LOG_ERR("i2c_write_read failed");
		return -EIO;
	}
	*data = ((uint16_t)buf[0] << 8) + (uint16_t)buf[1];

	return Status;
}

VL53L1_Error VL53L1_RdDWord(VL53L1_DEV Dev, uint16_t index, uint32_t *data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	int32_t status_int;
	u8_t buf[4];

	buf[0] = index >> 8;
	buf[1] = index & 0xFF;

	status_int = i2c_write_read(Dev->i2c, Dev->I2cDevAddr, buf, 2, buf, 4);
	if (status_int < 0) {
		LOG_ERR("i2c_write_read failed");
		return -EIO;
	}
	*data = ((uint32_t)buf[0] << 24) + ((uint32_t)buf[1] << 16) +
		((uint32_t)buf[2] << 8) + (uint32_t)buf[3];

	return Status;
}

VL53L1_Error VL53L1_GetTickCount(uint32_t *ptick_count_ms)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	*ptick_count_ms = k_cycle_get_32();
	return status;
}

VL53L1_Error VL53L1_GetTimerFrequency(int32_t *ptimer_freq_hz)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	return status;
}

VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	k_sleep(wait_ms);
	return status;
}

VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	k_busy_wait(wait_us);
	return status;
}

VL53L1_Error VL53L1_WaitValueMaskEx(VL53L1_Dev_t *pdev, uint32_t timeout_ms,
				    uint16_t index, uint8_t value, uint8_t mask,
				    uint32_t poll_delay_ms)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	return status;
}
