/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/misc/ft8xx/ft8xx_common.h>

#include <zephyr/sys/byteorder.h>
#include "ft8xx_drv.h"

void ft8xx_wr8(union ft8xx_bus bus, uint32_t address, uint8_t data)
{
	int err;

	err = ft8xx_drv_write(bus, address, &data, sizeof(data));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wr16(union ft8xx_bus bus, uint32_t address, uint16_t data)
{
	int err;
	uint8_t buffer[2];

	*(uint16_t *)buffer = sys_cpu_to_le16(data);
	err = ft8xx_drv_write(bus, address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wr32(union ft8xx_bus bus, uint32_t address, uint32_t data)
{
	int err;
	uint8_t buffer[4];

	*(uint32_t *)buffer = sys_cpu_to_le32(data);
	err = ft8xx_drv_write(bus, address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wrblock(union ft8xx_bus bus, uint32_t address, uint8_t *data, uint32_t data_length)
{
	int err;

	err = ft8xx_drv_write(bus, address, data, data_length);
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}

void ft8xx_wrblock_dual(union ft8xx_bus bus, uint32_t address, uint8_t *data, uint32_t data_length, 
				uint8_t *data2, 
                uint32_t data2_length,
				uint8_t padsize)

{
	int err;

	err = ft8xx_drv_write_dual(bus, data, data_length, data2, data2_length, padsize);
	__ASSERT(err == 0, "Writing FT8xx data at 0x%x failed", address);
}



int ft8xx_append_block_int16(uint8_t *block ,uint32_t size, uint32_t offset, int16_t value)
{
	if (offset+sizeof(value) < size)
	{
		*(int16_t *)(block_array+offset) = sys_cpu_to_le16(value);
		new_offset = offset + sizeof(value);
	}
}

int ft8xx_append_block_uint16(uint8_t *block, uint32_t size, uint32_t offset, uint16_t value)
{
	if (offset+sizeof(value) < size)
	{
		*(uint16_t *)(block_array+offset) = sys_cpu_to_le16(value);
		new_offset = offset + sizeof(value);
	}
}

int ft8xx_append_block_int32(uint8_t * block, uint32_t size, uint32_t offset, int32_t value)
{
	if (offset+sizeof(value) < size)
	{
		*(int32_t *)(block_array+offset) = sys_cpu_to_le32(value);
		new_offset = offset + sizeof(value);
	}
}

int ft8xx_append_block_uint32(uint8_t * block, uint32_t size, uint32_t offset, uint32_t value)
{
	if (offset+sizeof(value) < size)
	{
		*(uint32_t *)(block_array+offset) = sys_cpu_to_le32(value);
		new_offset = offset + sizeof(value);
	}
}

int ft8xx_append_block_data(uint8_t *block, uint32_t size, uint32_t offset, uint8_t *data, uint32_t data_size)
{
	if (offset+data_size < size)
	{
		memcpy(*(void *)(block_array+offset)),data,data_size) 
		new_offset = offset + data_size;
	}
}





uint8_t ft8xx_rd8(union ft8xx_bus bus, uint32_t address)
{
	int err;
	uint8_t data = 0;

	err = ft8xx_drv_read(bus, address, &data, sizeof(data));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return data;
}

uint16_t ft8xx_rd16(union ft8xx_bus bus, uint32_t address)
{
	int err;
	uint8_t buffer[2] = {0};

	err = ft8xx_drv_read(bus, address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return sys_le16_to_cpu(*(const uint16_t *)buffer);
}

uint32_t ft8xx_rd32(union ft8xx_bus bus, uint32_t address)
{
	int err;
	uint8_t buffer[4] = {0};

	err = ft8xx_drv_read(bus, address, buffer, sizeof(buffer));
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return sys_le32_to_cpu(*(const uint32_t *)buffer);
}

int ft8xx_rdblock(union ft8xx_bus bus, uint32_t address, uint8_t *data , data_length)
{
	int err;

	err = ft8xx_drv_read(bus, address, read_data, data_length);
	__ASSERT(err == 0, "Reading FT8xx data from 0x%x failed", address);

	return err;
}

