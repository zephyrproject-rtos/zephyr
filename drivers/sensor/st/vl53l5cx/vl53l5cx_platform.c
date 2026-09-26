/*
 * Copyright (c) 2024 Fabian Blatz <fabianblatz@gmail.com>
 * Copyright (c) 2026 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include "platform.h"
#include "vl53l5cx_api.h"

uint8_t VL53L5CX_RdByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_value)
{
	return VL53L5CX_RdMulti(p_platform, RegisterAdress, p_value, 1);
}

/* Keep a dedicated 1 byte write function to avoid generating a 2 messages transfer for 1 byte */
uint8_t VL53L5CX_WrByte(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t value)
{
	uint8_t buffer[3];	/* 2 bytes of address and 1 byte of data */
	int ret;

	/* Address is written in big-endian */
	buffer[0] = (uint8_t)((RegisterAdress & 0xff00) >> 8);
	buffer[1] = (uint8_t)(RegisterAdress & 0x00ff);
	buffer[2] = value;

	ret = i2c_write_dt(&p_platform->i2c, buffer, 3);
	if (ret < 0) {
		return VL53L5CX_STATUS_ERROR;
	}

	return VL53L5CX_STATUS_OK;
}

uint8_t VL53L5CX_WrMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
			 uint32_t size)
{
	uint16_t addr;
	struct i2c_msg msg[2] = {
		{.buf = (uint8_t *)&addr,
		 .len = sizeof(addr),
		 .flags = I2C_MSG_WRITE},
		{.flags = I2C_MSG_WRITE | I2C_MSG_STOP},
	};
	uint32_t sent = 0;

	do {
		addr = sys_cpu_to_be16(RegisterAdress + sent);
		msg[1].buf = p_values + sent;
		msg[1].len = MIN(size - sent, CONFIG_VL53L5CX_MAX_I2C_CHUNK_SIZE);

		if (i2c_transfer_dt(&p_platform->i2c, msg, 2) < 0) {
			return VL53L5CX_STATUS_ERROR;
		}
		sent += msg[1].len;
	} while (sent < size);

	return VL53L5CX_STATUS_OK;
}

uint8_t VL53L5CX_RdMulti(VL53L5CX_Platform *p_platform, uint16_t RegisterAdress, uint8_t *p_values,
			 uint32_t size)
{
	uint32_t read_size = 0;
	uint16_t addr;
	size_t len;

	do {
		addr = sys_cpu_to_be16(RegisterAdress + read_size);
		len = MIN(size - read_size, CONFIG_VL53L5CX_MAX_I2C_CHUNK_SIZE);

		if (i2c_write_read_dt(&p_platform->i2c, (uint8_t *)&addr, 2, p_values + read_size,
				      len) < 0) {
			return VL53L5CX_STATUS_ERROR;
		}

		read_size += len;
	} while (read_size < size);

	return VL53L5CX_STATUS_OK;
}

void VL53L5CX_SwapBuffer(uint8_t *buffer, uint16_t size)
{
	uint32_t i, tmp;

	/* Example of possible implementation using <string.h> */
	for (i = 0; i < size; i = i + 4) {
		tmp = (buffer[i] << 24) | (buffer[i + 1] << 16) | (buffer[i + 2] << 8) |
		      (buffer[i + 3]);

		memcpy(&buffer[i], &tmp, 4);
	}
}

uint8_t VL53L5CX_WaitMs(VL53L5CX_Platform *p_platform, uint32_t TimeMs)
{
	ARG_UNUSED(p_platform);

	k_sleep(K_MSEC(TimeMs));

	return VL53L5CX_STATUS_OK;
}
