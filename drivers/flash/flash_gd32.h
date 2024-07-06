/*
 * Copyright (c) 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>

#define SOC_NV_FLASH_NODE	DT_INST(0, soc_nv_flash)
#define SOC_NV_FLASH_SIZE	DT_REG_SIZE(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_ADDR	DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define SOC_NV_FLASH_PRG_SIZE	DT_PROP(SOC_NV_FLASH_NODE, write_block_size)

#if (4 == SOC_NV_FLASH_PRG_SIZE)
typedef uint32_t flash_prg_t;
#elif (2 == SOC_NV_FLASH_PRG_SIZE)
typedef uint16_t flash_prg_t;
#elif (1 == SOC_NV_FLASH_PRG_SIZE)
typedef uint8_t flash_prg_t;
#else
#error "Invalid write-block-size value in FMC DTS"
#endif

/* Helper for conditional compilation directives, KB cannot be used because it has type casting. */
#define PRE_KB(x) ((x) << 10)

bool flash_gd32_valid_range(k_off_t offset, uint32_t len, bool write);

int flash_gd32_write_range(k_off_t offset, const void *data, size_t len);

int flash_gd32_erase_block(k_off_t offset, size_t size);

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_gd32_pages_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);
#endif
#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_GD32_H_ */
