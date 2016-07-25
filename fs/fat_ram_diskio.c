/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <stdint.h>
#include <misc/__assert.h>
#include <diskio.h>
#include <ff.h>

static uint8_t file_buff[CONFIG_FS_VOLUME_SIZE];

static void *lba_to_address(uint32_t lba)
{
	__ASSERT(((lba * _MIN_SS) < CONFIG_FS_VOLUME_SIZE), "FS bound error");
	return &file_buff[(lba * _MIN_SS)];
}

DSTATUS fat_disk_status(void)
{
	return RES_OK;
}

DSTATUS fat_disk_initialize(void)
{
	return RES_OK;
}

DRESULT fat_disk_read(void *buff, uint32_t sector, uint32_t count)
{
	memcpy(buff, lba_to_address(sector), count * _MIN_SS);

	return RES_OK;
}

DRESULT fat_disk_write(void *buff, uint32_t sector, uint32_t count)
{
	memcpy(lba_to_address(sector), buff, count * _MIN_SS);

	return RES_OK;
}

DRESULT fat_disk_ioctl(uint8_t cmd, void *buff)
{
	switch (cmd) {
	case CTRL_SYNC:
		return RES_OK;
	case GET_SECTOR_COUNT:
		*(uint32_t *) buff = CONFIG_FS_VOLUME_SIZE / _MIN_SS;
		return RES_OK;
	case GET_BLOCK_SIZE:
		*(uint32_t *) buff = CONFIG_FS_BLOCK_SIZE / _MIN_SS;
		return RES_OK;
	case CTRL_TRIM:
		break;
	}

	return RES_PARERR;
}
