/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define SRAM2_ADDR  (CONFIG_SRAM_BASE_ADDRESS + RAM_SIZE2)
#define RAM_SIZE2 (0x4000000)

MEMORY
{
    SRAM2      (wx) : ORIGIN = (CONFIG_SRAM_BASE_ADDRESS + RAM_SIZE2), LENGTH = RAM_SIZE2
}

PHDRS
{
    sram2_phdr PT_LOAD;
}

#define MPU_ALIGN(region_size) \
    . = ALIGN(4)

#include <xtensa-sample-controller.ld>
