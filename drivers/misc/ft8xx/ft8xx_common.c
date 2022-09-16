/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>

#include <zephyr/sys/byteorder.h>
#include "ft8xx_drv.h"

void ft8xx_wr8(uint32_t address, uint8_t data)
{
	int err;

	err = ft8xx_drv_write(address, &data, sizeof(data));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wr16(uint32_t address, uint16_t data)
{
	int err;
	uint8_t buffer[2];

	*(uint16_t *)buffer = sys_cpu_to_le16(data);
	err = ft8xx_drv_write(address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wr32(uint32_t address, uint32_t data)
{
	int err;
	uint8_t buffer[4];

	*(uint32_t *)buffer = sys_cpu_to_le32(data);
	err = ft8xx_drv_write(address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

uint8_t ft8xx_rd8(uint32_t address)
{
	int err;
	uint8_t data = 0;

	err = ft8xx_drv_read(address, &data, sizeof(data));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return data;
}

uint16_t ft8xx_rd16(uint32_t address)
{
	int err;
	uint8_t buffer[2] = {0};

	err = ft8xx_drv_read(address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return sys_le16_to_cpu(*(const uint16_t *)buffer);
}

uint32_t ft8xx_rd32(uint32_t address)
{
	int err;
	uint8_t buffer[4] = {0};

	err = ft8xx_drv_read(address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return sys_le32_to_cpu(*(const uint32_t *)buffer);
}
