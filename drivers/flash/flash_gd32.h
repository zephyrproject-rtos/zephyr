/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_

#include <devicetree.h>
#include <drivers/flash.h>

#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_ADDR DT_REG_ADDR(SOC_NV_FLASH_NODE)

#define SOC_NV_FLASH_ERASE_BLOCK_SIZE \
		DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE \
		DT_PROP(SOC_NV_FLASH_NODE, write_block_size)

#if (SOC_NV_FLASH_WRITE_BLOCK_SIZE == 4)
typedef uint32_t flash_prog_t;
#elif (SOC_NV_FLASH_WRITE_BLOCK_SIZE == 2)
typedef uint16_t flash_prog_t;
#elif (SOC_NV_FLASH_WRITE_BLOCK_SIZE == 1)
typedef uint8_t flash_prog_t;
#else
#error "Invalid write-block-size value in FMC DTS"
#endif

int flash_gd32_programming(off_t offset, const void *data, size_t len);

int flash_gd32_page_erase(uint32_t page);

void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_ */
