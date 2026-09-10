/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define RAM_SIZE2 (0x4000000)
#define SRAM2_ADDR (DT_CHOSEN_SRAM_ADDR + RAM_SIZE2)

MEMORY
{
    SRAM2      (wx) : ORIGIN = SRAM2_ADDR, LENGTH = RAM_SIZE2
}

PHDRS
{
    sram2_phdr PT_LOAD;
}

#define MPU_ALIGN(region_size) \
    . = ALIGN(4)

#include <xtensa-dc233c.ld>
