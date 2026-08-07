/*
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

ASSERT(__rom_region_start == DT_REG_ADDR(DT_CHOSEN(mcuboot_image_ram)),
	"image is not linked at the MCUboot RAM-load address")
ASSERT(__data_region_load_start >= DT_REG_ADDR(DT_CHOSEN(mcuboot_image_ram)),
	"initialized data is not loaded from the image region")
ASSERT(__data_region_load_start <
	DT_REG_ADDR(DT_CHOSEN(mcuboot_image_ram)) + DT_REG_SIZE(DT_CHOSEN(mcuboot_image_ram)),
	"initialized data load address exceeds the image region")
ASSERT(__data_region_start >= DT_REG_ADDR(DT_CHOSEN(zephyr_sram)),
	"initialized data is not placed in runtime SRAM")
ASSERT(__data_region_start != __data_region_load_start,
	"initialized data does not have split load and runtime addresses")
ASSERT(__bss_start >= DT_REG_ADDR(DT_CHOSEN(zephyr_sram)),
	"BSS is not placed in runtime SRAM")
