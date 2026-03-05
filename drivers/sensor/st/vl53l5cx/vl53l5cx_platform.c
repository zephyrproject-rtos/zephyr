/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include "vl53l5cx_platform.h"
#include "vl53l5cx_api.h"

uint8_t RdByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value)
{
	return RdMulti(p_platform, RegisterAdress, p_value, 1);
}

uint8_t WrByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value)
{
	return WrMulti(p_platform, RegisterAdress, &value, 1);
}

uint8_t WrMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
		uint32_t size)
{
	RegisterAdress = sys_cpu_to_be16(RegisterAdress);

	struct i2c_msg msg[2] = {
		{.buf = (uint8_t *)&RegisterAdress,
		 .len = sizeof(RegisterAdress),
		 .flags = I2C_MSG_WRITE},
		{.buf = p_values, .len = size, .flags = I2C_MSG_WRITE | I2C_MSG_STOP},
	};

	if (i2c_transfer_dt(&p_platform->i2c, msg, 2) < 0) {
		return VL53L5CX_STATUS_ERROR;
	}

	return VL53L5CX_STATUS_OK;
}

uint8_t RdMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
		uint32_t size)
{
	RegisterAdress = sys_cpu_to_be16(RegisterAdress);

	if (i2c_write_read_dt(&p_platform->i2c, (uint8_t *)&RegisterAdress, 2, p_values, size) <
	    0) {
		return VL53L5CX_STATUS_ERROR;
	}
	return VL53L5CX_STATUS_OK;
}

void SwapBuffer(uint8_t *buffer, uint16_t size)
{
	uint32_t i, tmp;

	/* Example of possible implementation using <string.h> */
	for (i = 0; i < size; i = i + 4) {
		tmp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) |
		      (buffer[i + 3]);

		memcpy(&buffer[i], &tmp, 4);
	}
}

uint8_t WaitMs(VL53L5CX_Platform *p_platform, uint32_t TimeMs)
{
	ARG_UNUSED(p_platform);
	k_sleep(K_MSEC(TimeMs));
	return VL53L5CX_STATUS_OK;
}
