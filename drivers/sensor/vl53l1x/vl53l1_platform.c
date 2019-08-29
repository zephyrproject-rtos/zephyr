/*
 * Copyright (c) 2017, STMicroelectronics - All Rights Reserved
 * Copyright (c) 2019 Makaio GmbH
 *
 * This file is part of VL53L1 Core and is dual licensed,
 * either 'STMicroelectronics
 * Proprietary license'
 * or 'BSD 3-clause "New" or "Revised" License' , at your option.
 *
 ******************************************************************************
 *
 * 'STMicroelectronics Proprietary license'
 *
 ******************************************************************************
 *
 * License terms: STMicroelectronics Proprietary in accordance with licensing
 * terms at www.st.com/sla0081
 *
 * STMicroelectronics confidential
 * Reproduction and Communication of this document is strictly prohibited
 * unless specifically authorized in writing by STMicroelectronics.
 *
 *
 ******************************************************************************
 *
 * Alternatively, VL53L1 Core may be distributed under the terms of
 * 'BSD 3-clause "New" or "Revised" License', in which case the following
 * provisions apply instead of the ones mentioned above :
 *
 ******************************************************************************
 *
 * License terms: BSD 3-clause "New" or "Revised" License.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 ******************************************************************************
 *
 */


#include "vl53l1_platform.h"
#include "vl53l1_api.h"
#include <drivers/sensor.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/i2c.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_DECLARE(VL53L1X);

#include <string.h>

VL53L1_Error VL53L1_WriteMulti(VL53L1_DEV Dev, u16_t index,
				u8_t *pdata, us32_t count)
{
	LOG_DBG("Write multi (count: %u)", count);

	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;
	u8_t I2CBuffer[count + 2];

	u8_t reg_addr[] = { index >> 8, index & 0xff };

	memcpy(&I2CBuffer[2], pdata, count);

	status_int =  i2c_write(Dev->I2cHandle, I2CBuffer, count + 2,
				Dev->I2cDevAddr);


	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", status_int);
	}

	return Status;
}

VL53L1_Error VL53L1_ReadMulti(VL53L1_DEV Dev, u16_t index,
				u8_t *pdata, us32_t count)
{
	LOG_DBG("Read multi");

	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;

	u8_t reg_addr[] = { index >> 8, index & 0xff };

	status_int =  i2c_write_read(Dev->I2cHandle, Dev->I2cDevAddr,
				     reg_addr, ARRAY_SIZE(reg_addr),
				     pdata, count);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write_read failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_WrByte(VL53L1_DEV Dev, u16_t index, u8_t data)
{
	LOG_DBG("Write byte");
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;

	u8_t I2CBuffer[] = {
		(index >> 8),
		(index & 0xff),
		data
	};

	status_int = i2c_write(Dev->I2cHandle, I2CBuffer,
					ARRAY_SIZE(I2CBuffer), Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_WrWord(VL53L1_DEV Dev, u16_t index, u16_t data)
{
	LOG_DBG("Write word");

	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;
	u8_t I2CBuffer[] = {
		(index >> 8),
		(index & 0xff),
		data >> 8,
		data & 0x00FF
	};

	status_int = i2c_write(Dev->I2cHandle, I2CBuffer,
					ARRAY_SIZE(I2CBuffer), Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_WrDWord(VL53L1_DEV Dev, u16_t index, us32_t data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;
	u8_t I2CBuffer[] = {
		(index >> 8),
		(index & 0xff),
		(data >> 24) & 0xFF,
		(data >> 16) & 0xFF,
		(data >> 8)  & 0xFF,
		(data >> 0) & 0xFF
	};

	status_int = i2c_write(Dev->I2cHandle, I2CBuffer,
					ARRAY_SIZE(I2CBuffer), Dev->I2cDevAddr);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_UpdateByte(VL53L1_DEV Dev, u16_t index,
				u8_t AndData, u8_t OrData)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	u8_t data;

	s32_t status_int = VL53L1_RdByte(Dev, index, &data);

	if (status_int < 0) {
		return VL53L1_ERROR_CONTROL_INTERFACE;
	}

	data = (data & AndData) | OrData;
	status_int = VL53L1_WrByte(Dev, index, data);
	if (status_int != 0) {
		return VL53L1_ERROR_CONTROL_INTERFACE;
	}

	return Status;
}

VL53L1_Error VL53L1_RdByte(VL53L1_DEV Dev, u16_t index, u8_t *data)
{
	LOG_DBG("Read byte");

	VL53L1_Error Status = VL53L1_ERROR_NONE;
	s32_t status_int;

	u8_t reg_addr[] = { index >> 8, index & 0xff };

	status_int =  i2c_write_read(Dev->I2cHandle, Dev->I2cDevAddr,
				     reg_addr, ARRAY_SIZE(reg_addr),
				     data, 1);

	if (status_int < 0) {
		Status = VL53L1_ERROR_CONTROL_INTERFACE;
		LOG_ERR("i2c_write_read failed (%d)", Status);
	}

	return Status;
}

VL53L1_Error VL53L1_RdWord(VL53L1_DEV Dev, u16_t index, u16_t *data)
{
	LOG_DBG("Read word");

	VL53L1_Error Status = VL53L1_ERROR_NONE;

	s32_t status_int;

	u8_t reg_addr[] = { index >> 8, index & 0xff };
	u8_t buf[2];

	status_int =  i2c_write_read(Dev->I2cHandle, Dev->I2cDevAddr,
				     reg_addr, ARRAY_SIZE(reg_addr),
				     buf, ARRAY_SIZE(buf));

	if (status_int < 0) {
		LOG_ERR("i2c_write_read failed");
		return -EIO;
	}

	*data = ((u16_t)buf[0] << 8) + (u16_t)buf[1];

	return Status;
}

VL53L1_Error VL53L1_RdDWord(VL53L1_DEV Dev, u16_t index, us32_t *data)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;

	LOG_ERR("Implement %s", log_strdup(__func__));

	return Status;
}

VL53L1_Error VL53L1_GetTickCount(
	us32_t *ptick_count_ms)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	*ptick_count_ms = k_uptime_get_32();
	return status;
}

VL53L1_Error VL53L1_GetTimerFrequency(s32_t *ptimer_freq_hz)
{
	LOG_ERR("Not implemented");
	return VL53L1_ERROR_NOT_IMPLEMENTED;
}

VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, s32_t wait_ms)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	k_sleep(wait_ms);

	return status;
}

VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, s32_t wait_us)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	k_busy_wait(wait_us);

	return status;
}

VL53L1_Error VL53L1_WaitValueMaskEx(
	VL53L1_Dev_t *pdev,
	us32_t timeout_ms,
	u16_t index,
	u8_t value,
	u8_t mask,
	us32_t poll_delay_ms)
{
	u8_t register_value = 0;

	VL53L1_Error status = VL53L1_ERROR_NONE;

	s32_t attempts = timeout_ms / poll_delay_ms;

	for (s32_t x = 0; x < attempts; x++) {

		status = VL53L1_RdByte(
			pdev,
			index,
			&register_value);

		if (status == VL53L1_ERROR_NONE &&
				(register_value & mask) == value) {
			return VL53L1_ERROR_NONE;
		}
		k_sleep(poll_delay_ms);
	}

	return VL53L1_ERROR_TIME_OUT;
}




