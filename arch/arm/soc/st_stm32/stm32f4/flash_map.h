/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F4X_FLASH_MAP_H_
#define _STM32F4X_FLASH_MAP_H_

#include "sys/types.h"

struct stm32f4x_flash_sector {
	const off_t start;
	const off_t end;
};

#define STM32F4X_FLASH_SECTOR(offset, bytes)	\
	{					\
	.start	= offset,			\
	.end	= offset + bytes - 1,		\
	}

__unused
static struct stm32f4x_flash_sector stm32f4xx_sectors[] = {
	STM32F4X_FLASH_SECTOR(0x00000, KB(16)),
	STM32F4X_FLASH_SECTOR(0x04000, KB(16)),
	STM32F4X_FLASH_SECTOR(0x08000, KB(16)),
	STM32F4X_FLASH_SECTOR(0x0c000, KB(16)),
	STM32F4X_FLASH_SECTOR(0x10000, KB(64)),
	STM32F4X_FLASH_SECTOR(0x20000, KB(128)),
	STM32F4X_FLASH_SECTOR(0x40000, KB(128)),
#ifdef CONFIG_SOC_STM32F401XE
	STM32F4X_FLASH_SECTOR(0x60000, KB(128)),
#endif
};

#define STM32F4X_FLASH_TIMEOUT		((u32_t) 0x000B0000)
#define STM32F4X_SECTOR_MASK		((u32_t) 0xFFFFFF07)
#define STM32F4X_SECTORS		ARRAY_SIZE(stm32f4xx_sectors)

#define STM32F4X_FLASH_END	\
	(stm32f4xx_sectors[ARRAY_SIZE(stm32f4xx_sectors) - 1].end)

__unused
static int stm32f4x_get_sector(off_t offset)
{
	int i;

	for (i = 0; i < STM32F4X_SECTORS; i++) {
		if (offset > stm32f4xx_sectors[i].end) {
			continue;
		}
		break;
	}

	return i;
}

#endif	/* _STM32F4X_FLASHMAP_H_ */
